// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BANDWIDTHSCHEDULE_H
#define BANDWIDTHSCHEDULE_H

// Pure decision for the bandwidth scheduler: does a given moment fall inside the
// user's scheduled alternate-speed window? Extracted from SessionManager so the
// day-mask + midnight-wrap logic can be unit-tested without a live session.
namespace bat {

// weekday: Mon=0 .. Sun=6. hour: 0..23. daysMask: bit i set ⇒ weekday i enabled.
// The window is [fromHour, toHour); when fromHour > toHour it wraps past midnight
// (e.g. 22 → 6). A day must be enabled in daysMask for the window to apply.
inline bool inBandwidthSchedule(int weekday, int hour, int daysMask,
                                int fromHour, int toHour)
{
    if (weekday < 0 || weekday > 6)
        return false;
    if ((daysMask & (1 << weekday)) == 0)
        return false;
    if (fromHour <= toHour)
        return hour >= fromHour && hour < toHour;
    // wraps midnight
    return hour >= fromHour || hour < toHour;
}

}

#endif // BANDWIDTHSCHEDULE_H
