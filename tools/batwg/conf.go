// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// The wg-quick .conf a VPN provider hands out, read into what the userspace
// device needs. Only the keys that matter to a tunnel with no interface of its
// own: PostUp/PreDown/Table/SaveConfig would act on the system, and nothing
// here ever touches the system, so they are read past.

import (
	"bufio"
	"encoding/base64"
	"encoding/hex"
	"errors"
	"fmt"
	"net/netip"
	"strconv"
	"strings"
)

type peerConf struct {
	publicKey    []byte
	presharedKey []byte
	endpoint     string // host:port, resolved when the tunnel starts
	allowedIPs   []netip.Prefix
	keepalive    int
}

type wgConf struct {
	privateKey []byte
	addresses  []netip.Addr
	dns        []netip.Addr
	mtu        int
	peers      []peerConf
}

func decodeKey(s string) ([]byte, error) {
	k, err := base64.StdEncoding.DecodeString(strings.TrimSpace(s))
	if err != nil || len(k) != 32 {
		return nil, errors.New("not a WireGuard key")
	}
	return k, nil
}

func splitList(v string) []string {
	var out []string
	for _, p := range strings.Split(v, ",") {
		if p = strings.TrimSpace(p); p != "" {
			out = append(out, p)
		}
	}
	return out
}

func parseConf(text string) (*wgConf, error) {
	c := &wgConf{}
	section := ""
	var peer *peerConf
	sc := bufio.NewScanner(strings.NewReader(text))
	line := 0
	for sc.Scan() {
		line++
		s := sc.Text()
		if i := strings.IndexAny(s, "#;"); i >= 0 {
			s = s[:i]
		}
		s = strings.TrimSpace(s)
		if s == "" {
			continue
		}
		if strings.HasPrefix(s, "[") && strings.HasSuffix(s, "]") {
			section = strings.ToLower(strings.TrimSpace(s[1 : len(s)-1]))
			if section == "peer" {
				c.peers = append(c.peers, peerConf{})
				peer = &c.peers[len(c.peers)-1]
			} else if section != "interface" {
				return nil, fmt.Errorf("line %d: unknown section [%s]", line, section)
			}
			continue
		}
		eq := strings.IndexByte(s, '=')
		if eq < 0 {
			return nil, fmt.Errorf("line %d: expected key = value", line)
		}
		key := strings.ToLower(strings.TrimSpace(s[:eq]))
		val := strings.TrimSpace(s[eq+1:])
		var err error
		switch section {
		case "interface":
			switch key {
			case "privatekey":
				c.privateKey, err = decodeKey(val)
			case "address":
				for _, a := range splitList(val) {
					var p netip.Prefix
					if p, err = netip.ParsePrefix(a); err != nil {
						var ad netip.Addr
						if ad, err = netip.ParseAddr(a); err != nil {
							break
						}
						p = netip.PrefixFrom(ad, ad.BitLen())
					}
					c.addresses = append(c.addresses, p.Addr())
				}
			case "dns":
				// search domains share the line with the servers; only the
				// servers are wanted
				for _, a := range splitList(val) {
					if ad, e := netip.ParseAddr(a); e == nil {
						c.dns = append(c.dns, ad)
					}
				}
			case "mtu":
				c.mtu, err = strconv.Atoi(val)
				if err == nil && (c.mtu < 576 || c.mtu > 65535) {
					err = errors.New("out of range")
				}
			}
		case "peer":
			switch key {
			case "publickey":
				peer.publicKey, err = decodeKey(val)
			case "presharedkey":
				peer.presharedKey, err = decodeKey(val)
			case "endpoint":
				peer.endpoint = val
			case "allowedips":
				for _, a := range splitList(val) {
					var p netip.Prefix
					if p, err = netip.ParsePrefix(a); err != nil {
						break
					}
					peer.allowedIPs = append(peer.allowedIPs, p.Masked())
				}
			case "persistentkeepalive":
				if strings.EqualFold(val, "off") {
					peer.keepalive = 0
				} else {
					peer.keepalive, err = strconv.Atoi(val)
				}
			}
		default:
			return nil, fmt.Errorf("line %d: %s outside a section", line, key)
		}
		if err != nil {
			return nil, fmt.Errorf("line %d: %s: %v", line, key, err)
		}
	}
	if c.privateKey == nil {
		return nil, errors.New("no PrivateKey in [Interface]")
	}
	if len(c.addresses) == 0 {
		return nil, errors.New("no Address in [Interface]")
	}
	if len(c.peers) == 0 {
		return nil, errors.New("no [Peer]")
	}
	for i, p := range c.peers {
		if p.publicKey == nil {
			return nil, fmt.Errorf("peer %d has no PublicKey", i+1)
		}
		if p.endpoint == "" {
			return nil, fmt.Errorf("peer %d has no Endpoint", i+1)
		}
		if len(p.allowedIPs) == 0 {
			return nil, fmt.Errorf("peer %d has no AllowedIPs", i+1)
		}
	}
	return c, nil
}

// The UAPI text the device is configured with. Endpoints are given already
// resolved: resolve maps "host:port" to "ip:port".
func (c *wgConf) uapi(resolve func(string) (string, error)) (string, error) {
	var b strings.Builder
	fmt.Fprintf(&b, "private_key=%s\nreplace_peers=true\n", hex.EncodeToString(c.privateKey))
	for _, p := range c.peers {
		ep, err := resolve(p.endpoint)
		if err != nil {
			return "", err
		}
		fmt.Fprintf(&b, "public_key=%s\n", hex.EncodeToString(p.publicKey))
		if p.presharedKey != nil {
			fmt.Fprintf(&b, "preshared_key=%s\n", hex.EncodeToString(p.presharedKey))
		}
		fmt.Fprintf(&b, "endpoint=%s\n", ep)
		// A keepalive makes the device reach the server as soon as it is up
		// (so a bad key shows at once) and keeps NAT mappings from timing out
		// while the torrents are quiet.
		ka := p.keepalive
		if ka == 0 {
			ka = 25
		}
		fmt.Fprintf(&b, "persistent_keepalive_interval=%d\nreplace_allowed_ips=true\n", ka)
		for _, a := range p.allowedIPs {
			fmt.Fprintf(&b, "allowed_ip=%s\n", a)
		}
	}
	return b.String(), nil
}
