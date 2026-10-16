// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// The WireGuard device, running entirely inside this process: its network
// stack is gVisor's, in memory, so there is no adapter, no route and no DNS
// change on the machine. The only thing it does on the real network is send
// and receive encrypted UDP to the VPN server, like any other program.

import (
	"bufio"
	"context"
	"errors"
	"net"
	"net/netip"
	"strconv"
	"strings"
	"sync"
	"time"

	"golang.zx2c4.com/wireguard/conn"
	"golang.zx2c4.com/wireguard/device"
	"golang.zx2c4.com/wireguard/tun/netstack"
)

// A plain UDP socket for the encrypted side. host "" listens on every
// address, the way a client must to hear the server back; tests pin it to
// 127.0.0.1.
type udpBind struct {
	host string
	mu   sync.Mutex
	c    *net.UDPConn
}

func (b *udpBind) Open(port uint16) ([]conn.ReceiveFunc, uint16, error) {
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.c != nil {
		return nil, 0, conn.ErrBindAlreadyOpen
	}
	network, ip := "udp", net.IP(nil)
	if b.host != "" {
		ip = net.ParseIP(b.host)
		if ip.To4() != nil {
			network = "udp4"
		}
	}
	c, err := net.ListenUDP(network, &net.UDPAddr{IP: ip, Port: int(port)})
	if err != nil {
		return nil, 0, err
	}
	b.c = c
	recv := func(packets [][]byte, sizes []int, eps []conn.Endpoint) (int, error) {
		n, ap, err := c.ReadFromUDPAddrPort(packets[0])
		if err != nil {
			return 0, err
		}
		sizes[0] = n
		eps[0] = &conn.StdNetEndpoint{AddrPort: netip.AddrPortFrom(ap.Addr().Unmap(), ap.Port())}
		return 1, nil
	}
	return []conn.ReceiveFunc{recv}, uint16(c.LocalAddr().(*net.UDPAddr).Port), nil
}

func (b *udpBind) Close() error {
	b.mu.Lock()
	defer b.mu.Unlock()
	if b.c == nil {
		return nil
	}
	err := b.c.Close()
	b.c = nil
	return err
}

func (b *udpBind) SetMark(uint32) error { return nil }
func (b *udpBind) BatchSize() int       { return 1 }

func (b *udpBind) Send(bufs [][]byte, ep conn.Endpoint) error {
	e, ok := ep.(*conn.StdNetEndpoint)
	if !ok {
		return conn.ErrWrongEndpointType
	}
	b.mu.Lock()
	c := b.c
	b.mu.Unlock()
	if c == nil {
		return net.ErrClosed
	}
	for _, buf := range bufs {
		if _, err := c.WriteToUDPAddrPort(buf, e.AddrPort); err != nil {
			return err
		}
	}
	return nil
}

func (b *udpBind) ParseEndpoint(s string) (conn.Endpoint, error) {
	ap, err := netip.ParseAddrPort(s)
	if err != nil {
		return nil, err
	}
	return &conn.StdNetEndpoint{AddrPort: ap}, nil
}

type tunnel struct {
	dev  *device.Device
	tnet *netstack.Net
	v4   netip.Addr // the tunnel's own addresses, invalid when absent
	v6   netip.Addr
}

var defaultDNS = []netip.Addr{netip.MustParseAddr("1.1.1.1"), netip.MustParseAddr("9.9.9.9")}

// The server's address, looked up on the real network (it has to be: it is
// how the tunnel is reached). IPv4 first, as most home networks have it.
func resolveEndpoint(hp string) (string, error) {
	host, port, err := net.SplitHostPort(hp)
	if err != nil {
		return "", err
	}
	if a, err := netip.ParseAddr(host); err == nil {
		return netip.AddrPortFrom(a.Unmap(), mustPort(port)).String(), nil
	}
	ctx, cancel := context.WithTimeout(context.Background(), 8*time.Second)
	defer cancel()
	ips, err := net.DefaultResolver.LookupNetIP(ctx, "ip", host)
	if err != nil || len(ips) == 0 {
		return "", errors.New("cannot find the server " + host)
	}
	pick := ips[0]
	for _, ip := range ips {
		if ip.Unmap().Is4() {
			pick = ip
			break
		}
	}
	return netip.AddrPortFrom(pick.Unmap(), mustPort(port)).String(), nil
}

func mustPort(s string) uint16 {
	p, _ := strconv.Atoi(s)
	return uint16(p)
}

func startTunnel(c *wgConf, bind conn.Bind, resolve func(string) (string, error), logf func(string, ...any)) (*tunnel, error) {
	dns := c.dns
	if len(dns) == 0 {
		dns = defaultDNS
	}
	mtu := c.mtu
	if mtu == 0 {
		mtu = 1420
	}
	tdev, tnet, err := netstack.CreateNetTUN(c.addresses, dns, mtu)
	if err != nil {
		return nil, err
	}
	logger := &device.Logger{Verbosef: device.DiscardLogf, Errorf: logf}
	dev := device.NewDevice(tdev, bind, logger)
	conf, err := c.uapi(resolve)
	if err != nil {
		dev.Close()
		return nil, err
	}
	if err := dev.IpcSet(conf); err != nil {
		dev.Close()
		return nil, err
	}
	if err := dev.Up(); err != nil {
		dev.Close()
		return nil, err
	}
	t := &tunnel{dev: dev, tnet: tnet}
	for _, a := range c.addresses {
		if a.Is4() && !t.v4.IsValid() {
			t.v4 = a
		} else if a.Is6() && !t.v6.IsValid() {
			t.v6 = a
		}
	}
	return t, nil
}

type peerStats struct {
	handshake time.Time // newest across peers; zero before the first
	rx, tx    uint64
}

func (t *tunnel) stats() peerStats {
	var st peerStats
	s, err := t.dev.IpcGet()
	if err != nil {
		return st
	}
	sc := bufio.NewScanner(strings.NewReader(s))
	for sc.Scan() {
		k, v, ok := strings.Cut(sc.Text(), "=")
		if !ok {
			continue
		}
		n, _ := strconv.ParseInt(v, 10, 64)
		switch k {
		case "last_handshake_time_sec":
			if n > 0 {
				if h := time.Unix(n, 0); h.After(st.handshake) {
					st.handshake = h
				}
			}
		case "rx_bytes":
			st.rx += uint64(n)
		case "tx_bytes":
			st.tx += uint64(n)
		}
	}
	return st
}

// Wait for the first handshake: proof that the keys are right and the server
// answers.
func (t *tunnel) waitHandshake(timeout time.Duration) bool {
	deadline := time.Now().Add(timeout)
	for time.Now().Before(deadline) {
		if !t.stats().handshake.IsZero() {
			return true
		}
		time.Sleep(100 * time.Millisecond)
	}
	return false
}

func (t *tunnel) close() { t.dev.Close() }
