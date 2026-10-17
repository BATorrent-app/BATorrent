// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// Port forwarding through the VPN, by NAT-PMP (RFC 6886) to the VPN's
// gateway inside the tunnel, the way Proton VPN offers it. The server hands
// out a public port and forwards what arrives on it to the same port on the
// tunnel address; the mapping lasts 60 s and is asked for again every 45 s.
// Servers without it simply do not answer, and the tunnel works as before.

import (
	"encoding/binary"
	"errors"
	"net"
	"net/netip"
	"time"
)

const (
	natpmpPort     = 5351
	natpmpLifetime = 60
	natpmpRenew    = 45 * time.Second
)

// The gateway to ask: a DNS server inside the tunnel's own network (Proton
// lists 10.2.0.1 for both), else the first address of the tunnel's /24.
func natpmpGateway(c *wgConf, own netip.Addr) netip.Addr {
	if !own.Is4() {
		return netip.Addr{}
	}
	o := own.As4()
	for _, d := range c.dns {
		if d.Is4() {
			a := d.As4()
			if a[0] == o[0] && a[1] == o[1] && a[2] == o[2] {
				return d
			}
		}
	}
	return netip.AddrFrom4([4]byte{o[0], o[1], o[2], 1})
}

// One mapping request; returns the public port. op 1 is UDP, 2 is TCP. The
// internal port 1 / external 0 is what Proton expects: it picks the port.
func natpmpMap(conn net.Conn, op byte) (uint16, error) {
	req := make([]byte, 12)
	req[0] = 0 // version
	req[1] = op
	binary.BigEndian.PutUint16(req[4:], 1)
	binary.BigEndian.PutUint16(req[6:], 0)
	binary.BigEndian.PutUint32(req[8:], natpmpLifetime)
	resp := make([]byte, 16)
	// RFC 6886 retries: 250 ms, doubling
	wait := 250 * time.Millisecond
	for try := 0; try < 3; try++ {
		if _, err := conn.Write(req); err != nil {
			return 0, err
		}
		conn.SetReadDeadline(time.Now().Add(wait))
		n, err := conn.Read(resp)
		if err == nil && n >= 16 && resp[1] == 128+op {
			if code := binary.BigEndian.Uint16(resp[2:]); code != 0 {
				return 0, errors.New("the server refused the mapping")
			}
			return binary.BigEndian.Uint16(resp[10:]), nil
		}
		wait *= 2
	}
	return 0, errors.New("no answer")
}

type portForward struct {
	conn net.Conn
}

// Ask for a port; nil when the server does not do port forwarding.
func startPortForward(t *tunnel, c *wgConf) (*portForward, uint16) {
	gw := natpmpGateway(c, t.v4)
	if !gw.IsValid() {
		return nil, 0
	}
	conn, err := t.tnet.DialUDPAddrPort(netip.AddrPort{}, netip.AddrPortFrom(gw, natpmpPort))
	if err != nil {
		return nil, 0
	}
	port, err := natpmpMap(conn, 1)
	if err != nil || port == 0 {
		conn.Close()
		return nil, 0
	}
	natpmpMap(conn, 2) // TCP too, so the mapping is whole
	return &portForward{conn: conn}, port
}

// Keep asking before the mapping lapses; report the port each time (0 when
// it is lost) through onPort, only when it changes.
func (pf *portForward) keep(port uint16, onPort func(uint16), stop <-chan struct{}) {
	tick := time.NewTicker(natpmpRenew)
	defer tick.Stop()
	fails := 0
	for {
		select {
		case <-stop:
			pf.conn.Close()
			return
		case <-tick.C:
			p, err := natpmpMap(pf.conn, 1)
			natpmpMap(pf.conn, 2)
			if err != nil {
				// one miss is within the lifetime; two and it is gone
				if fails++; fails >= 2 && port != 0 {
					port = 0
					onPort(0)
				}
				continue
			}
			fails = 0
			if p != port {
				port = p
				onPort(p)
			}
		}
	}
}
