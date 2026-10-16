// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

package main

// The stdin/stdout contract with the app, run as a real process (this test
// binary re-executed as batwg).

import (
	"encoding/json"
	"os"
	"os/exec"
	"strings"
	"testing"
	"time"
)

func TestMain(m *testing.M) {
	if os.Getenv("BATWG_AS_MAIN") == "1" {
		main()
		return
	}
	os.Exit(m.Run())
}

func runMain(t *testing.T, stdin string) (string, int) {
	cmd := exec.Command(os.Args[0])
	cmd.Env = append(os.Environ(), "BATWG_AS_MAIN=1")
	cmd.Stdin = strings.NewReader(stdin)
	done := make(chan struct{})
	var out []byte
	var err error
	go func() { out, err = cmd.Output(); close(done) }()
	select {
	case <-done:
	case <-time.After(20 * time.Second):
		cmd.Process.Kill()
		t.Fatal("batwg did not exit")
	}
	code := 0
	if ee, ok := err.(*exec.ExitError); ok {
		code = ee.ExitCode()
	}
	return string(out), code
}

// The app closing stdin before the config is complete: exit quietly.
func TestExitsWhenStdinClosesEarly(t *testing.T) {
	out, code := runMain(t, "[Interface]\n")
	if code != 0 || out != "" {
		t.Fatalf("code %d out %q", code, out)
	}
}

func TestBadConfIsReported(t *testing.T) {
	out, code := runMain(t, "[Interface]\nPrivateKey = nope\n.\n")
	if code != 1 {
		t.Fatalf("code %d", code)
	}
	var ev map[string]any
	if err := json.Unmarshal([]byte(strings.TrimSpace(out)), &ev); err != nil {
		t.Fatalf("%q: %v", out, err)
	}
	if ev["ev"] != "error" || ev["code"] != "conf" {
		t.Fatalf("%v", ev)
	}
}
