// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// A whole tunnel on loopback: a second userspace WireGuard device plays the
// VPN server, with TCP and UDP echo services on its tunnel address. Nothing
// leaves 127.0.0.1 and nothing on the machine is changed.

import (
	"bytes"
	"crypto/rand"
	"encoding/base64"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"io"
	"net"
	"net/netip"
	"strings"
	"testing"
	"time"

	"golang.org/x/crypto/curve25519"
	"golang.zx2c4.com/wireguard/device"
	"golang.zx2c4.com/wireguard/tun/netstack"
)

func keypair(t *testing.T) (priv, pub []byte) {
	priv = make([]byte, 32)
	rand.Read(priv)
	priv[0] &= 248
	priv[31] = (priv[31] & 127) | 64
	pub, err := curve25519.X25519(priv, curve25519.Basepoint)
	if err != nil {
		t.Fatal(err)
	}
	return
}

func freeUDPPort(t *testing.T) int {
	c, err := net.ListenUDP("udp4", &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1)})
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	return c.LocalAddr().(*net.UDPAddr).Port
}

type fakeServer struct {
	dev  *device.Device
	tnet *netstack.Net
	port int
	pub  []byte
}

// The "VPN server": answers on 10.9.0.1, TCP echo on 7000, UDP echo on 7001.
func startServer(t *testing.T, clientPub []byte) *fakeServer {
	priv, pub := keypair(t)
	tdev, tnet, err := netstack.CreateNetTUN([]netip.Addr{netip.MustParseAddr("10.9.0.1")}, nil, 1420)
	if err != nil {
		t.Fatal(err)
	}
	dev := device.NewDevice(tdev, &udpBind{host: "127.0.0.1"}, device.NewLogger(device.LogLevelSilent, ""))
	port := freeUDPPort(t)
	conf := fmt.Sprintf("private_key=%s\nlisten_port=%d\npublic_key=%s\nallowed_ip=10.9.0.2/32\n",
		hex.EncodeToString(priv), port, hex.EncodeToString(clientPub))
	if err := dev.IpcSet(conf); err != nil {
		t.Fatal(err)
	}
	if err := dev.Up(); err != nil {
		t.Fatal(err)
	}
	ln, err := tnet.ListenTCP(&net.TCPAddr{IP: net.IPv4(10, 9, 0, 1), Port: 7000})
	if err != nil {
		t.Fatal(err)
	}
	go func() {
		for {
			c, err := ln.Accept()
			if err != nil {
				return
			}
			go func() { io.Copy(c, c); c.Close() }()
		}
	}()
	uc, err := tnet.ListenUDP(&net.UDPAddr{IP: net.IPv4(10, 9, 0, 1), Port: 7001})
	if err != nil {
		t.Fatal(err)
	}
	go func() {
		buf := make([]byte, 2048)
		for {
			n, from, err := uc.ReadFrom(buf)
			if err != nil {
				return
			}
			uc.WriteTo(append([]byte("echo:"), buf[:n]...), from)
		}
	}()
	t.Cleanup(func() { ln.Close(); uc.Close(); dev.Close() })
	return &fakeServer{dev: dev, tnet: tnet, port: port, pub: pub}
}

func clientConf(priv []byte, s *fakeServer) string {
	return fmt.Sprintf(`[Interface]
PrivateKey = %s
Address = 10.9.0.2/32
DNS = 10.9.0.1

[Peer]
PublicKey = %s
AllowedIPs = 0.0.0.0/0
Endpoint = 127.0.0.1:%d
`, base64.StdEncoding.EncodeToString(priv), base64.StdEncoding.EncodeToString(s.pub), s.port)
}

func startClient(t *testing.T) (*tunnel, *socksServer) {
	cpriv, cpub := keypair(t)
	srv := startServer(t, cpub)
	c, err := parseConf(clientConf(cpriv, srv))
	if err != nil {
		t.Fatal(err)
	}
	tun, err := startTunnel(c, &udpBind{host: "127.0.0.1"}, resolveEndpoint, t.Logf)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(tun.close)
	if !tun.waitHandshake(5 * time.Second) {
		t.Fatal("no handshake")
	}
	ss, err := newSocksServer("127.0.0.1:0", tunnelAdapter{tun}, "u", "p", t.Logf)
	if err != nil {
		t.Fatal(err)
	}
	go ss.serve()
	t.Cleanup(ss.close)
	return tun, ss
}

// SOCKS5 greeting + password; returns the open control connection.
func socksLogin(t *testing.T, port int, user, pass string) (net.Conn, bool) {
	c, err := net.Dial("tcp", fmt.Sprintf("127.0.0.1:%d", port))
	if err != nil {
		t.Fatal(err)
	}
	c.SetDeadline(time.Now().Add(10 * time.Second))
	c.Write([]byte{5, 2, 0, 2})
	var m [2]byte
	if _, err := io.ReadFull(c, m[:]); err != nil || m[1] != 2 {
		t.Fatalf("method %v %v", m, err)
	}
	b := []byte{1, byte(len(user))}
	b = append(b, user...)
	b = append(b, byte(len(pass)))
	b = append(b, pass...)
	c.Write(b)
	var r [2]byte
	if _, err := io.ReadFull(c, r[:]); err != nil {
		t.Fatal(err)
	}
	return c, r[1] == 0
}

func socksRequest(t *testing.T, c net.Conn, cmd byte, dst netip.AddrPort) (byte, netip.AddrPort) {
	a := dst.Addr().As4()
	b := []byte{5, cmd, 0, 1}
	b = append(b, a[:]...)
	b = binary.BigEndian.AppendUint16(b, dst.Port())
	c.Write(b)
	var rep [10]byte
	if _, err := io.ReadFull(c, rep[:]); err != nil {
		t.Fatal(err)
	}
	bound := netip.AddrPortFrom(netip.AddrFrom4([4]byte(rep[4:8])), binary.BigEndian.Uint16(rep[8:10]))
	return rep[1], bound
}

func TestTunnelCarriesTCP(t *testing.T) {
	_, ss := startClient(t)
	c, ok := socksLogin(t, ss.port(), "u", "p")
	if !ok {
		t.Fatal("login refused")
	}
	defer c.Close()
	if rep, _ := socksRequest(t, c, cmdConnect, netip.MustParseAddrPort("10.9.0.1:7000")); rep != 0 {
		t.Fatalf("connect reply %d", rep)
	}
	msg := bytes.Repeat([]byte("piece"), 20000) // 100 KB, many packets
	go c.Write(msg)
	got := make([]byte, len(msg))
	if _, err := io.ReadFull(c, got); err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(got, msg) {
		t.Fatal("echo differs")
	}
}

func TestTunnelCarriesUDP(t *testing.T) {
	_, ss := startClient(t)
	c, ok := socksLogin(t, ss.port(), "u", "p")
	if !ok {
		t.Fatal("login refused")
	}
	defer c.Close()
	rep, relay := socksRequest(t, c, cmdUDP, netip.MustParseAddrPort("0.0.0.0:0"))
	if rep != 0 {
		t.Fatalf("associate reply %d", rep)
	}
	u, err := net.ListenUDP("udp4", &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1)})
	if err != nil {
		t.Fatal(err)
	}
	defer u.Close()
	pkt := append(udpHeader(netip.MustParseAddrPort("10.9.0.1:7001")), "dht ping"...)
	if _, err := u.WriteToUDPAddrPort(pkt, relay); err != nil {
		t.Fatal(err)
	}
	u.SetReadDeadline(time.Now().Add(5 * time.Second))
	buf := make([]byte, 2048)
	n, err := u.Read(buf)
	if err != nil {
		t.Fatal(err)
	}
	src, _, payload, ok := parseUDPHeader(buf[:n])
	if !ok || src.String() != "10.9.0.1:7001" || string(payload) != "echo:dht ping" {
		t.Fatalf("got %v %q", src, payload)
	}
}

func TestSocksRefusesWrongPassword(t *testing.T) {
	_, ss := startClient(t)
	c, ok := socksLogin(t, ss.port(), "u", "wrong")
	defer c.Close()
	if ok {
		t.Fatal("a wrong password was accepted")
	}
}

func TestSocksRefusesNoAuth(t *testing.T) {
	_, ss := startClient(t)
	c, err := net.Dial("tcp", fmt.Sprintf("127.0.0.1:%d", ss.port()))
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	c.SetDeadline(time.Now().Add(5 * time.Second))
	c.Write([]byte{5, 1, 0})
	var m [2]byte
	io.ReadFull(c, m[:])
	if m[1] != 0xff {
		t.Fatalf("no-auth accepted: %v", m)
	}
}

// A wrong server key: no handshake, so the tunnel is never reported up.
func TestNoHandshakeWithWrongServerKey(t *testing.T) {
	cpriv, cpub := keypair(t)
	srv := startServer(t, cpub)
	_, otherPub := keypair(t)
	srv.pub = otherPub
	c, err := parseConf(clientConf(cpriv, srv))
	if err != nil {
		t.Fatal(err)
	}
	tun, err := startTunnel(c, &udpBind{host: "127.0.0.1"}, resolveEndpoint, t.Logf)
	if err != nil {
		t.Fatal(err)
	}
	defer tun.close()
	if tun.waitHandshake(3 * time.Second) {
		t.Fatal("handshake with the wrong key")
	}
}

func TestParseConf(t *testing.T) {
	_, pub := keypair(t)
	priv, _ := keypair(t)
	k := base64.StdEncoding.EncodeToString
	good := fmt.Sprintf(`# from a provider
[Interface]
PrivateKey = %s
Address = 10.2.0.2/32, fd00::2/128
DNS = 10.2.0.1, example.lan
MTU = 1380
PostUp = route add something   ; never run
Table = off

[Peer]
PublicKey = %s
AllowedIPs = 0.0.0.0/0, ::/0
Endpoint = vpn.example.com:51820
PersistentKeepalive = 15
`, k(priv), k(pub))
	c, err := parseConf(good)
	if err != nil {
		t.Fatal(err)
	}
	if len(c.addresses) != 2 || len(c.dns) != 1 || c.mtu != 1380 || c.peers[0].keepalive != 15 {
		t.Fatalf("%+v", c)
	}
	u, err := c.uapi(func(s string) (string, error) { return "1.2.3.4:51820", nil })
	if err != nil || !strings.Contains(u, "endpoint=1.2.3.4:51820") || !strings.Contains(u, "allowed_ip=::/0") ||
		!strings.Contains(u, "persistent_keepalive_interval=15") {
		t.Fatalf("uapi:\n%s", u)
	}
	for name, bad := range map[string]string{
		"no key":      "[Interface]\nAddress = 10.0.0.2/32\n[Peer]\nPublicKey = " + k(pub) + "\nEndpoint = a:1\nAllowedIPs = 0.0.0.0/0\n",
		"short key":   "[Interface]\nPrivateKey = abc\nAddress = 10.0.0.2/32\n",
		"no peer":     "[Interface]\nPrivateKey = " + k(priv) + "\nAddress = 10.0.0.2/32\n",
		"no endpoint": "[Interface]\nPrivateKey = " + k(priv) + "\nAddress = 10.0.0.2/32\n[Peer]\nPublicKey = " + k(pub) + "\nAllowedIPs = 0.0.0.0/0\n",
		"bad section": "[Wat]\n",
	} {
		if _, err := parseConf(bad); err == nil {
			t.Errorf("%s: accepted", name)
		}
	}
}
