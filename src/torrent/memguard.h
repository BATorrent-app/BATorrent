#pragma once

// Decision logic for the memory safety-valve, split out as a pure function so it
// can be unit-tested without a live session. The guard exists to catch a runaway
// allocation bug — NOT normal operation — so its input must be the app's PRIVATE
// memory (see currentRssBytes), never the OS working/resident set: libtorrent 2.x
// maps the download files, so the working set balloons with reclaimable file-cache
// pages that are not a leak (this is what false-paused real users' downloads).
namespace bat {

enum class MemGuardAction { None, Pause, Panic };

// rssMB: the app's private memory in MB (-1 if unknown). capMB: configured limit
// (<= 0 disables). Pause at the cap (recoverable), panic at 2× (genuine runaway).
inline MemGuardAction memGuardEvaluate(long long rssMB, int capMB)
{
    if (capMB <= 0 || rssMB < 0) return MemGuardAction::None;
    if (rssMB >= static_cast<long long>(capMB) * 2) return MemGuardAction::Panic;
    if (rssMB >= capMB) return MemGuardAction::Pause;
    return MemGuardAction::None;
}

} // namespace bat
