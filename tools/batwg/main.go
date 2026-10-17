// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// batwg: BATorrent's VPN. A WireGuard tunnel that lives inside this process
// and is reached through a SOCKS5 server on 127.0.0.1, so only the traffic
// handed to it goes through the VPN. It needs no administrator rights,
// creates no network adapter, adds no route and changes no DNS: the rest of
// the machine never notices it, and when it exits nothing is left behind.
//
// The app writes the .conf on stdin, ending it with a line holding a single
// ".", and keeps stdin open: when it closes (the app quit or crashed), batwg
// exits. Events go to stdout, one JSON object per line:
//
//	{"ev":"ready","port":N,"user":"…","pass":"…","ip":"10.2.0.2"}
//	{"ev":"fwd","port":N}   the port the VPN forwards to us (0: lost)
//	{"ev":"stats","rx":N,"tx":N,"handshake":unix}
//	{"ev":"stale"} / {"ev":"alive"}   the server stopped / started answering
//	{"ev":"error","code":"conf|server|handshake|traffic|socks","msg":"…"}
//
// An error is the last line before exiting with status 1.
package main

import (
	"bufio"
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"net"
	"net/netip"
	"os"
	"strings"
	"sync"
	"time"
)

var outMu sync.Mutex

func emit(v map[string]any) {
	b, _ := json.Marshal(v)
	outMu.Lock()
	os.Stdout.Write(append(b, '\n'))
	outMu.Unlock()
}

func fail(code, msg string) {
	emit(map[string]any{"ev": "error", "code": code, "msg": msg})
	os.Exit(1)
}

func logf(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "batwg: "+format+"\n", args...)
}

func randomHex(n int) string {
	b := make([]byte, n)
	rand.Read(b)
	return hex.EncodeToString(b)
}

// The tunnel as the SOCKS server sees it.
type tunnelAdapter struct{ t *tunnel }

func (a tunnelAdapter) DialContext(ctx context.Context, network, address string) (net.Conn, error) {
	return a.t.tnet.DialContext(ctx, network, address)
}
func (a tunnelAdapter) LookupContextHost(ctx context.Context, host string) ([]string, error) {
	return a.t.tnet.LookupContextHost(ctx, host)
}
func (a tunnelAdapter) listenUDP(v6 bool, port uint16) (net.PacketConn, error) {
	addr := a.t.v4
	if v6 {
		addr = a.t.v6
	}
	return a.t.tnet.ListenUDPAddrPort(netip.AddrPortFrom(addr, port))
}
func (a tunnelAdapter) hasV4() bool { return a.t.v4.IsValid() }
func (a tunnelAdapter) hasV6() bool { return a.t.v6.IsValid() }

// Proof that traffic flows, not just that the server shook hands: a name
// looked up through the tunnel's DNS, or failing that a TCP connection to a
// well-known address.
func probe(t *tunnel, timeout time.Duration) bool {
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	ok := make(chan bool, 2)
	go func() {
		_, err := t.tnet.LookupContextHost(ctx, "cloudflare.com")
		ok <- err == nil
	}()
	go func() {
		if !t.v4.IsValid() {
			ok <- false
			return
		}
		c, err := t.tnet.DialContext(ctx, "tcp", "1.1.1.1:443")
		if err == nil {
			c.Close()
		}
		ok <- err == nil
	}()
	for i := 0; i < 2; i++ {
		if <-ok {
			return true
		}
	}
	return false
}

// How long the server can go without a handshake before the tunnel counts as
// dropped. WireGuard renews it every two minutes while packets flow, and the
// keepalive keeps them flowing.
const staleAfter = 3 * time.Minute

func main() {
	in := bufio.NewScanner(os.Stdin)
	in.Buffer(make([]byte, 64*1024), 64*1024)
	var conf strings.Builder
	ended := false
	for in.Scan() {
		if strings.TrimSpace(in.Text()) == "." {
			ended = true
			break
		}
		conf.WriteString(in.Text())
		conf.WriteByte('\n')
	}
	if !ended {
		os.Exit(0) // the app went away before sending anything
	}
	c, err := parseConf(conf.String())
	if err != nil {
		fail("conf", err.Error())
	}

	t, err := startTunnel(c, &udpBind{}, resolveEndpoint, logf)
	if err != nil {
		code := "server"
		if strings.Contains(err.Error(), "IpcSet") || strings.Contains(err.Error(), "uapi") {
			code = "conf"
		}
		fail(code, err.Error())
	}
	if !t.waitHandshake(12 * time.Second) {
		t.close()
		fail("handshake", "the server did not answer")
	}
	if !probe(t, 10*time.Second) {
		t.close()
		fail("traffic", "the server answered but nothing passes through it")
	}

	user, pass := randomHex(8), randomHex(16)
	srv, err := newSocksServer("127.0.0.1:0", tunnelAdapter{t}, user, pass, logf)
	if err != nil {
		t.close()
		fail("socks", err.Error())
	}
	go srv.serve()
	ip := t.v4
	if !ip.IsValid() {
		ip = t.v6
	}
	emit(map[string]any{"ev": "ready", "port": srv.port(), "user": user, "pass": pass, "ip": ip.String()})

	// stdin closing is the app saying goodbye, however it went
	gone := make(chan struct{})
	// A forwarded port, when the server offers one (Proton's paid servers
	// do; others do not answer and nothing changes). Asked after "ready" so a
	// server without it does not slow the connection down; the UDP side moves
	// onto the port when it arrives.
	go func() {
		pf, fwd := startPortForward(t, c)
		if pf == nil {
			return
		}
		srv.fwd.Store(uint32(fwd))
		emit(map[string]any{"ev": "fwd", "port": fwd})
		pf.keep(fwd, func(p uint16) {
			srv.fwd.Store(uint32(p))
			emit(map[string]any{"ev": "fwd", "port": p})
		}, gone)
	}()
	go func() {
		for in.Scan() {
		}
		close(gone)
	}()

	tick := time.NewTicker(2 * time.Second)
	defer tick.Stop()
	stale := false
	for {
		select {
		case <-gone:
			srv.close()
			t.close()
			os.Exit(0)
		case <-tick.C:
			st := t.stats()
			emit(map[string]any{"ev": "stats", "rx": st.rx, "tx": st.tx, "handshake": st.handshake.Unix()})
			old := time.Since(st.handshake) > staleAfter
			if old != stale {
				stale = old
				if stale {
					emit(map[string]any{"ev": "stale"})
				} else {
					emit(map[string]any{"ev": "alive"})
				}
			}
		}
	}
}
