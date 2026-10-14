#include <catch2/catch_test_macros.hpp>
#include "torrent/streamgate.h"

// The answer the UI process gives itself from its copy of a streamed file's
// pieces, instead of a round trip to the engine for every chunk.
TEST_CASE("StreamGate: the run on disk from a byte, whole pieces only", "[unit][stream]")
{
    const qint64 P = 1024;            // piece size
    QBitArray have(8);                // pieces 0..7 of the torrent
    have.setBit(1); have.setBit(2); have.setBit(3); have.setBit(5);

    // a file that starts half way into piece 1 and ends in piece 6
    const qint64 off = P + 512, size = 5 * P;

    CHECK(StreamGate::contiguousRun(have, P, off, size, 0, 1 << 20) == 512 + 2 * P);   // to the end of piece 3
    CHECK(StreamGate::contiguousRun(have, P, off, size, 0, 100) == 100);               // capped
    CHECK(StreamGate::contiguousRun(have, P, off, size, 3 * P, 1 << 20) == 0);         // piece 4 missing
    CHECK(StreamGate::contiguousRun(have, P, off, size, 3 * P + 512, 1 << 20) == P);   // piece 5, then a gap
    CHECK(StreamGate::contiguousRun(have, P, off, size, size, 10) == 0);               // past the end
    CHECK(StreamGate::contiguousRun(have, P, off, size, -1, 10) == 0);
    CHECK(StreamGate::contiguousRun(have, 0, off, size, 0, 10) == 0);

    // a finished piece extends the run, as a pieceFinished event does
    have.setBit(4);
    CHECK(StreamGate::contiguousRun(have, P, off, size, 0, 1 << 20) == 512 + 4 * P);
    // never past the file's own end
    have.setBit(6); have.setBit(7);
    CHECK(StreamGate::contiguousRun(have, P, off, size, 0, 1 << 20) == size);
}
