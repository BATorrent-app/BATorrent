// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What makes a release healthy, in one place for the release lists and the
// player's wait.
.pragma library

// Under this many seeds a release says "few seeds", and is no one's pick.
var FEW_SEEDS = 30

function fewSeeds(n) { return !(n >= FEW_SEEDS) }
