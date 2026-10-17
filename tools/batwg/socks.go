// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// A SOCKS5 server on 127.0.0.1 whose every connection leaves through the
// tunnel: CONNECT for TCP (peers, HTTP trackers, web seeds) and UDP ASSOCIATE
// for UDP (uTP, DHT, UDP trackers). Names are looked up through the tunnel's
// DNS, never the machine's. It asks for a username and password made up per
// run, so other programs on the machine cannot borrow it.

import (
	"context"
	"crypto/subtle"
	"encoding/binary"
	"errors"
	"io"
	"net"
	"net/netip"
	"strconv"
	"sync"
	"sync/atomic"
	"time"
)

const (
	socksVer       = 5
	cmdConnect     = 1
	cmdUDP         = 3
	atypV4         = 1
	atypDomain     = 3
	atypV6         = 4
	repOK          = 0
	repFail        = 1
	repNetUnreach  = 3
	repHostUnreach = 4
	repCmdNotSup   = 7
	repAtypNotSup  = 8
)

// What the SOCKS server needs from the tunnel. The tunnel's netstack.Net
// fills it; tests can too.
type tunnelNet interface {
	DialContext(ctx context.Context, network, address string) (net.Conn, error)
	LookupContextHost(ctx context.Context, host string) ([]string, error)
	listenUDP(v6 bool, port uint16) (net.PacketConn, error)
	hasV4() bool
	hasV6() bool
}

type socksServer struct {
	ln   net.Listener
	tn   tunnelNet
	user []byte
	pass []byte
	wg   sync.WaitGroup
	logf func(string, ...any)
	fwd  atomic.Uint32 // the port the VPN forwards to us, 0 when none
}

func newSocksServer(addr string, tn tunnelNet, user, pass string, logf func(string, ...any)) (*socksServer, error) {
	ln, err := net.Listen("tcp", addr)
	if err != nil {
		return nil, err
	}
	return &socksServer{ln: ln, tn: tn, user: []byte(user), pass: []byte(pass), logf: logf}, nil
}

func (s *socksServer) port() int { return s.ln.Addr().(*net.TCPAddr).Port }

func (s *socksServer) serve() {
	for {
		c, err := s.ln.Accept()
		if err != nil {
			return
		}
		s.wg.Add(1)
		go func() {
			defer s.wg.Done()
			s.handle(c)
		}()
	}
}

func (s *socksServer) close() { s.ln.Close() }

func (s *socksServer) handle(c net.Conn) {
	defer c.Close()
	c.SetDeadline(time.Now().Add(20 * time.Second))
	if !s.greet(c) {
		return
	}
	var hdr [3]byte
	if _, err := io.ReadFull(c, hdr[:]); err != nil || hdr[0] != socksVer {
		return
	}
	host, port, atyp, err := readAddr(c)
	if err != nil {
		reply(c, repAtypNotSup, nil)
		return
	}
	switch hdr[1] {
	case cmdConnect:
		s.connect(c, host, port, atyp)
	case cmdUDP:
		s.associate(c)
	default:
		reply(c, repCmdNotSup, nil)
	}
}

// Method negotiation and the username/password check (RFC 1929).
func (s *socksServer) greet(c net.Conn) bool {
	var h [2]byte
	if _, err := io.ReadFull(c, h[:]); err != nil || h[0] != socksVer {
		return false
	}
	methods := make([]byte, h[1])
	if _, err := io.ReadFull(c, methods); err != nil {
		return false
	}
	offered := false
	for _, m := range methods {
		if m == 2 {
			offered = true
		}
	}
	if !offered {
		c.Write([]byte{socksVer, 0xff})
		return false
	}
	c.Write([]byte{socksVer, 2})
	var v [2]byte
	if _, err := io.ReadFull(c, v[:]); err != nil || v[0] != 1 {
		return false
	}
	user := make([]byte, v[1])
	if _, err := io.ReadFull(c, user); err != nil {
		return false
	}
	var pl [1]byte
	if _, err := io.ReadFull(c, pl[:]); err != nil {
		return false
	}
	pass := make([]byte, pl[0])
	if _, err := io.ReadFull(c, pass); err != nil {
		return false
	}
	ok := subtle.ConstantTimeCompare(user, s.user)&subtle.ConstantTimeCompare(pass, s.pass) == 1
	if !ok {
		c.Write([]byte{1, 1})
		return false
	}
	c.Write([]byte{1, 0})
	return true
}

func readAddr(r io.Reader) (host string, port uint16, atyp byte, err error) {
	var t [1]byte
	if _, err = io.ReadFull(r, t[:]); err != nil {
		return
	}
	atyp = t[0]
	switch atyp {
	case atypV4, atypV6:
		b := make([]byte, 4)
		if atyp == atypV6 {
			b = make([]byte, 16)
		}
		if _, err = io.ReadFull(r, b); err != nil {
			return
		}
		a, _ := netip.AddrFromSlice(b)
		host = a.String()
	case atypDomain:
		var l [1]byte
		if _, err = io.ReadFull(r, l[:]); err != nil {
			return
		}
		b := make([]byte, l[0])
		if _, err = io.ReadFull(r, b); err != nil {
			return
		}
		host = string(b)
	default:
		err = errors.New("address type")
		return
	}
	var p [2]byte
	if _, err = io.ReadFull(r, p[:]); err != nil {
		return
	}
	port = binary.BigEndian.Uint16(p[:])
	return
}

func reply(c net.Conn, code byte, bound *netip.AddrPort) {
	b := []byte{socksVer, code, 0}
	if bound != nil && bound.Addr().Is4() {
		a := bound.Addr().As4()
		b = append(b, atypV4)
		b = append(b, a[:]...)
		b = binary.BigEndian.AppendUint16(b, bound.Port())
	} else {
		b = append(b, atypV4, 0, 0, 0, 0, 0, 0)
	}
	c.Write(b)
}

func (s *socksServer) connect(c net.Conn, host string, port uint16, atyp byte) {
	if atyp == atypV6 && !s.tn.hasV6() || atyp == atypV4 && !s.tn.hasV4() {
		reply(c, repNetUnreach, nil)
		return
	}
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()
	up, err := s.tn.DialContext(ctx, "tcp", net.JoinHostPort(host, strconv.Itoa(int(port))))
	if err != nil {
		reply(c, repHostUnreach, nil)
		return
	}
	defer up.Close()
	reply(c, repOK, nil)
	c.SetDeadline(time.Time{})
	done := make(chan struct{}, 2)
	pipe := func(dst, src net.Conn) {
		io.Copy(dst, src)
		// half-close so the other side sees the end, then let it finish
		if tc, ok := dst.(interface{ CloseWrite() error }); ok {
			tc.CloseWrite()
		} else {
			dst.Close()
		}
		done <- struct{}{}
	}
	go pipe(up, c)
	go pipe(c, up)
	<-done
	// the other direction gets a moment to drain, then both close
	select {
	case <-done:
	case <-time.After(30 * time.Second):
	}
}

// The tunnel side of a UDP association for one address family. Its socket
// can be swapped for one on another port (when the forwarded port arrives or
// changes) without ending the association.
type udpOut struct {
	mu   sync.Mutex
	pc   net.PacketConn
	port uint16 // the local port asked for (0: any)
}

func (u *udpOut) get() net.PacketConn {
	if u == nil {
		return nil
	}
	u.mu.Lock()
	defer u.mu.Unlock()
	return u.pc
}

// UDP ASSOCIATE: a relay socket on 127.0.0.1 for the client, tunnel sockets
// for the far side, alive as long as the client keeps the control
// connection open. The IPv4 tunnel socket sits on the forwarded port when
// there is one, so the DHT and uTP go out from it and peers can reach in.
func (s *socksServer) associate(c net.Conn) {
	relay, err := net.ListenUDP("udp4", &net.UDPAddr{IP: net.IPv4(127, 0, 0, 1)})
	if err != nil {
		reply(c, repFail, nil)
		return
	}
	defer relay.Close()
	done := make(chan struct{})
	defer close(done)

	var client atomic.Pointer[netip.AddrPort]
	// tunnel -> client, for as long as that socket is the current one
	reader := func(o net.PacketConn) {
		buf := make([]byte, 65536)
		const room = 22 // the largest header, for an IPv6 source
		for {
			n, from, err := o.ReadFrom(buf[room:])
			if err != nil {
				return
			}
			to := client.Load()
			ua, ok := from.(*net.UDPAddr)
			if to == nil || !ok {
				continue
			}
			src := ua.AddrPort()
			h := udpHeader(netip.AddrPortFrom(src.Addr().Unmap(), src.Port()))
			start := room - len(h)
			copy(buf[start:], h)
			relay.WriteToUDPAddrPort(buf[start:room+n], *to)
		}
	}
	open := func(v6 bool, port uint16) *udpOut {
		pc, err := s.tn.listenUDP(v6, port)
		if err != nil && port != 0 {
			port = 0 // taken (a second association): any port will do
			pc, err = s.tn.listenUDP(v6, 0)
		}
		if err != nil {
			return nil
		}
		go reader(pc)
		return &udpOut{pc: pc, port: port}
	}
	var outs [2]*udpOut // [0] IPv4, [1] IPv6
	if s.tn.hasV4() {
		if outs[0] = open(false, uint16(s.fwd.Load())); outs[0] == nil {
			reply(c, repFail, nil)
			return
		}
	}
	if s.tn.hasV6() {
		outs[1] = open(true, 0)
	}
	defer func() {
		for _, o := range outs {
			if pc := o.get(); pc != nil {
				pc.Close()
			}
		}
	}()
	bound := relay.LocalAddr().(*net.UDPAddr).AddrPort()
	bound = netip.AddrPortFrom(bound.Addr().Unmap(), bound.Port())
	reply(c, repOK, &bound)
	c.SetDeadline(time.Time{})

	// the forwarded port arrived or changed: move the IPv4 socket onto it
	if o4 := outs[0]; o4 != nil {
		go func() {
			tick := time.NewTicker(2 * time.Second)
			defer tick.Stop()
			for {
				select {
				case <-done:
					return
				case <-tick.C:
				}
				want := uint16(s.fwd.Load())
				o4.mu.Lock()
				cur := o4.port
				o4.mu.Unlock()
				if want == 0 || want == cur {
					continue
				}
				pc, err := s.tn.listenUDP(false, want)
				if err != nil {
					continue
				}
				go reader(pc)
				o4.mu.Lock()
				old := o4.pc
				o4.pc, o4.port = pc, want
				o4.mu.Unlock()
				old.Close()
			}
		}()
	}

	names := newNameCache(s.tn)
	// client -> tunnel
	go func() {
		buf := make([]byte, 65536)
		for {
			n, from, err := relay.ReadFromUDPAddrPort(buf)
			if err != nil {
				return
			}
			if !from.Addr().Unmap().IsLoopback() {
				continue
			}
			f := netip.AddrPortFrom(from.Addr().Unmap(), from.Port())
			client.Store(&f)
			dst, name, payload, ok := parseUDPHeader(buf[:n])
			if !ok {
				continue
			}
			if name != "" {
				// a name (a UDP tracker) is looked up off this loop, so a
				// slow answer does not hold up the DHT
				p := append([]byte(nil), payload...)
				go func() {
					if a, ok := names.lookup(name); ok {
						send(outs, netip.AddrPortFrom(a, dst.Port()), p)
					}
				}()
				continue
			}
			send(outs, dst, payload)
		}
	}()
	// the association lives as long as the control connection
	io.Copy(io.Discard, c)
}

func send(outs [2]*udpOut, dst netip.AddrPort, p []byte) {
	o := outs[0]
	if dst.Addr().Is6() {
		o = outs[1]
	}
	if pc := o.get(); pc != nil {
		pc.WriteTo(p, net.UDPAddrFromAddrPort(dst))
	}
}

// RSV(2) FRAG(1) ATYP ADDR PORT DATA. Fragments are dropped, as most servers
// do; libtorrent never sends them.
func parseUDPHeader(b []byte) (dst netip.AddrPort, name string, payload []byte, ok bool) {
	if len(b) < 4 || b[2] != 0 {
		return
	}
	switch b[3] {
	case atypV4:
		if len(b) < 10 {
			return
		}
		a, _ := netip.AddrFromSlice(b[4:8])
		return netip.AddrPortFrom(a, binary.BigEndian.Uint16(b[8:10])), "", b[10:], true
	case atypV6:
		if len(b) < 22 {
			return
		}
		a, _ := netip.AddrFromSlice(b[4:20])
		return netip.AddrPortFrom(a.Unmap(), binary.BigEndian.Uint16(b[20:22])), "", b[22:], true
	case atypDomain:
		if len(b) < 5 {
			return
		}
		l := int(b[4])
		if len(b) < 5+l+2 {
			return
		}
		port := binary.BigEndian.Uint16(b[5+l : 7+l])
		return netip.AddrPortFrom(netip.Addr{}, port), string(b[5 : 5+l]), b[7+l:], true
	}
	return
}

func udpHeader(src netip.AddrPort) []byte {
	h := []byte{0, 0, 0}
	if src.Addr().Is4() {
		a := src.Addr().As4()
		h = append(h, atypV4)
		h = append(h, a[:]...)
	} else {
		a := src.Addr().As16()
		h = append(h, atypV6)
		h = append(h, a[:]...)
	}
	return binary.BigEndian.AppendUint16(h, src.Port())
}

// Names looked up through the tunnel, kept for a few minutes: a UDP
// tracker is announced to again and again.
type nameCache struct {
	tn tunnelNet
	mu sync.Mutex
	m  map[string]nameEntry
}

type nameEntry struct {
	a  netip.Addr
	at time.Time
}

func newNameCache(tn tunnelNet) *nameCache { return &nameCache{tn: tn, m: map[string]nameEntry{}} }

func (n *nameCache) lookup(name string) (netip.Addr, bool) {
	n.mu.Lock()
	e, ok := n.m[name]
	n.mu.Unlock()
	if ok && time.Since(e.at) < 5*time.Minute {
		return e.a, true
	}
	ctx, cancel := context.WithTimeout(context.Background(), 8*time.Second)
	defer cancel()
	addrs, err := n.tn.LookupContextHost(ctx, name)
	if err != nil {
		return netip.Addr{}, false
	}
	var pick netip.Addr
	for _, s := range addrs {
		a, err := netip.ParseAddr(s)
		if err != nil {
			continue
		}
		a = a.Unmap()
		if a.Is4() && n.tn.hasV4() || a.Is6() && n.tn.hasV6() {
			pick = a
			if a.Is4() {
				break
			}
		}
	}
	if !pick.IsValid() {
		return pick, false
	}
	n.mu.Lock()
	n.m[name] = nameEntry{a: pick, at: time.Now()}
	n.mu.Unlock()
	return pick, true
}
