// SPDX-License-Identifier: MIT
// WebUI pure-helper tests. The WebUI ships as one self-contained index.html, so
// rather than split the JS out we extract the pure helper block (fmtBytes …
// progressClass) straight from the file and exercise it with Node's built-in
// test runner — no npm deps, no DOM. Run: `node --test tests/webui/`.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const html = readFileSync(
  join(dirname(fileURLToPath(import.meta.url)), '../../src/webui/index.html'),
  'utf8',
);

// The pure helpers are defined contiguously; slice on stable anchors (not line
// numbers) so the test survives edits elsewhere in the file.
const start = html.indexOf('function fmtBytes');
const end = html.indexOf('/* ── Filtering ── */');
assert.ok(start >= 0 && end > start, 'WebUI helper block not found in index.html');

const { fmtBytes, fmtSpeed, classifyState, stateLabel, progressClass } = new Function(
  html.slice(start, end) +
    '\nreturn { fmtBytes, fmtSpeed, classifyState, stateLabel, progressClass };',
)();

test('fmtBytes scales and rounds by unit', () => {
  assert.equal(fmtBytes(0), '0 B');
  assert.equal(fmtBytes(null), '0 B');
  assert.equal(fmtBytes(512), '512 B');
  assert.equal(fmtBytes(1024), '1 KB');          // B/KB round to integer
  assert.equal(fmtBytes(1048576), '1.0 MB');     // MB+ keeps one decimal (1024^2)
  assert.equal(fmtBytes(1572864), '1.5 MB');
  assert.equal(fmtBytes(1073741824), '1.0 GB');  // 1024^3
});

test('fmtSpeed appends /s', () => {
  assert.equal(fmtSpeed(1024), '1 KB/s');
  assert.equal(fmtSpeed(0), '0 B/s');
});

test('classifyState maps engine states (EN + PT) to CSS classes', () => {
  assert.equal(classifyState('Downloading', false), 'downloading');
  assert.equal(classifyState('Seeding', false), 'seeding');
  assert.equal(classifyState('Checking', false), 'checking');
  assert.equal(classifyState('Paused', false), 'paused');
  assert.equal(classifyState('Finished', false), 'other');
  assert.equal(classifyState('Baixando', false), 'downloading');   // pt-BR
  assert.equal(classifyState('Semeando', false), 'seeding');       // pt-BR
});

test('classifyState: paused flag overrides any state', () => {
  assert.equal(classifyState('Downloading', true), 'paused');
  assert.equal(classifyState('Seeding', true), 'paused');
});

test('stateLabel shows Paused when paused, else the raw state', () => {
  assert.equal(stateLabel('Downloading', false), 'Downloading');
  assert.equal(stateLabel('Downloading', true), 'Paused');
});

test('progressClass: complete at 100%, paused overrides, else by state', () => {
  assert.equal(progressClass('Downloading', false, 1), 'complete');
  assert.equal(progressClass('Downloading', false, 0.5), 'downloading');
  assert.equal(progressClass('Seeding', false, 0.5), 'seeding');
  assert.equal(progressClass('Downloading', true, 0.5), 'paused');
});
