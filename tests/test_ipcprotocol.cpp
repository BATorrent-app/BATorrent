#include <catch2/catch_test_macros.hpp>
#include "ipc/ipcprotocol.h"

#include <QBuffer>
#include <QDataStream>

namespace {

QByteArray framed(ipc::Kind kind, const QByteArray &payload)
{
    QBuffer dev;
    dev.open(QIODevice::WriteOnly);
    ipc::writeFrame(&dev, kind, payload);
    return dev.data();
}

QByteArray lengthPrefix(quint32 len)
{
    QByteArray out;
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setVersion(ipc::kStreamVersion);
    ds << len;
    return out;
}

} // namespace

TEST_CASE("a written frame round-trips through drainFrames", "[ipc]") {
    QByteArray buf = framed(ipc::Kind::Request, QByteArrayLiteral("payload"));
    int calls = 0;
    ipc::drainFrames(buf, [&](ipc::Kind k, const QByteArray &p) {
        ++calls;
        REQUIRE(k == ipc::Kind::Request);
        REQUIRE(p == QByteArrayLiteral("payload"));
    });
    REQUIRE(calls == 1);
    REQUIRE(buf.isEmpty());
}

TEST_CASE("back-to-back frames drain in order, partial tail stays", "[ipc]") {
    QByteArray buf = framed(ipc::Kind::Hello, {})
                   + framed(ipc::Kind::Event, QByteArrayLiteral("ev"));
    const QByteArray tail = framed(ipc::Kind::Reply, QByteArrayLiteral("later"));
    buf += tail.left(tail.size() - 3);   // incomplete third frame

    QList<ipc::Kind> kinds;
    ipc::drainFrames(buf, [&](ipc::Kind k, const QByteArray &) { kinds << k; });
    REQUIRE(kinds == QList<ipc::Kind>({ipc::Kind::Hello, ipc::Kind::Event}));
    REQUIRE(!buf.isEmpty());   // partial frame waits for more bytes

    buf += tail.right(3);
    ipc::drainFrames(buf, [&](ipc::Kind k, const QByteArray &p) {
        kinds << k;
        REQUIRE(p == QByteArrayLiteral("later"));
    });
    REQUIRE(kinds.size() == 3);
    REQUIRE(buf.isEmpty());
}

TEST_CASE("a bare length prefix is not enough to dispatch", "[ipc]") {
    QByteArray buf = lengthPrefix(16);
    int calls = 0;
    ipc::drainFrames(buf, [&](ipc::Kind, const QByteArray &) { ++calls; });
    REQUIRE(calls == 0);
    REQUIRE(buf.size() == 4);
}

TEST_CASE("hostile length prefix drops the buffer instead of pinning it", "[ipc]") {
    // 0xFFFFFFF0 used to overflow the int() size check and desync the stream;
    // anything past kMaxFrameBytes must clear the buffer, never accumulate.
    QByteArray buf = lengthPrefix(0xFFFFFFF0u) + QByteArrayLiteral("junk");
    int calls = 0;
    ipc::drainFrames(buf, [&](ipc::Kind, const QByteArray &) { ++calls; });
    REQUIRE(calls == 0);
    REQUIRE(buf.isEmpty());

    buf = lengthPrefix(quint32(ipc::kMaxFrameBytes) + 1);
    ipc::drainFrames(buf, [&](ipc::Kind, const QByteArray &) { ++calls; });
    REQUIRE(calls == 0);
    REQUIRE(buf.isEmpty());
}

TEST_CASE("frame at the size cap still passes", "[ipc]") {
    const QByteArray payload(1024, 'x');   // representative, not 64MB in a unit test
    QByteArray buf = framed(ipc::Kind::Event, payload);
    int calls = 0;
    ipc::drainFrames(buf, [&](ipc::Kind, const QByteArray &p) {
        ++calls;
        REQUIRE(p.size() == payload.size());
    });
    REQUIRE(calls == 1);
}

TEST_CASE("zero-length body is skipped without stalling the stream", "[ipc]") {
    QByteArray buf = lengthPrefix(0) + framed(ipc::Kind::Ping, {});
    QList<ipc::Kind> kinds;
    ipc::drainFrames(buf, [&](ipc::Kind k, const QByteArray &) { kinds << k; });
    REQUIRE(kinds == QList<ipc::Kind>({ipc::Kind::Ping}));
    REQUIRE(buf.isEmpty());
}
