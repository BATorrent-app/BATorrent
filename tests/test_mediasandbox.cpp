// SPDX-License-Identifier: MIT
// The media child's sandbox: the profile text can't be talked into granting
// more, and a confined process really is refused files, writes and every
// network, while still mapping the frame ring the UI shares with it.

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "ipc/mediasandbox.h"
#include "ipc/sharedsegment.h"

#ifdef Q_OS_MACOS
#  include <arpa/inet.h>
#  include <cerrno>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <sys/mman.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

using namespace MediaSandbox;

TEST_CASE("the profile denies by default and grants only code", "[unit][sandbox]")
{
    Grant g;
    g.readTrees = { "/Applications/BATorrent.app" };
    const QString p = profile(g);
    CHECK(p.startsWith("(version 1)\n(deny default)\n"));
    CHECK(p.contains("(subpath \"/Applications/BATorrent.app\")"));
    CHECK_FALSE(p.contains("network-outbound"));
    CHECK_FALSE(p.contains("network-inbound"));
    CHECK_FALSE(p.contains("file-write"));
    CHECK_FALSE(p.contains("ipc-posix-shm-write-create"));
    CHECK_FALSE(p.contains("AGX"));
}

TEST_CASE("an install path cannot rewrite the sandbox", "[unit][sandbox][security]")
{
    Grant g;
    g.readTrees = { "/Apps/a\") (allow default) (subpath \"b" };
    const QString p = profile(g);
    REQUIRE_FALSE(p.isEmpty());
    CHECK(p.contains("(subpath \"/Apps/a\\\") (allow default) (subpath \\\"b\")"));

    g.readTrees = { "/Apps/back\\slash" };
    CHECK(profile(g).contains("\"/Apps/back\\\\slash\""));

    for (const char *bad : { "/Apps/new\nline", "/Apps/nul\x01", "/Apps/del\x7f" }) {
        g.readTrees = { QString::fromLatin1(bad) };
        CHECK(profile(g).isEmpty());
    }
    g.readTrees = { "/ok", "/bad\r" };
    CHECK(profile(g).isEmpty());
}

#ifdef Q_OS_MACOS

namespace {

int listenOn(int *port)
{
    const int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(bind(s, reinterpret_cast<sockaddr *>(&a), sizeof a) == 0);
    REQUIRE(listen(s, 4) == 0);
    socklen_t len = sizeof a;
    getsockname(s, reinterpret_cast<sockaddr *>(&a), &len);
    *port = ntohs(a.sin_port);
    return s;
}

int connectErrno(const char *ip, int port)
{
    const int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(uint16_t(port));
    inet_pton(AF_INET, ip, &a.sin_addr);
    const int rc = connect(s, reinterpret_cast<sockaddr *>(&a), sizeof a);
    const int err = rc == 0 ? 0 : errno;
    close(s);
    return err;
}

int openErrno(const QString &path, int flags)
{
    const int fd = open(QFile::encodeName(path).constData(), flags, 0600);
    const int err = fd >= 0 ? 0 : errno;
    if (fd >= 0) close(fd);
    return err;
}

enum Probe {
    OwnTreeRefused     = 1 << 0,
    OtherFileRead      = 1 << 1,
    WriteAllowed       = 1 << 2,
    LocalhostReached   = 1 << 3,
    InternetReached    = 1 << 4,
    HomeRead           = 1 << 5,
    RingRefused        = 1 << 6,
    ShmCreateAllowed   = 1 << 7,
};

} // namespace

// Forked before any Qt thread exists, so the child can safely do nothing but
// syscalls once confined. Each failed expectation sets a bit in the exit code.
TEST_CASE("a confined process is refused everything but its code and its ring",
          "[integration][sandbox][security]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString root = QFileInfo(dir.path()).canonicalFilePath();   // the sandbox matches real paths
    // The code tree's own name is an injection attempt: if escaping failed the
    // profile would grant everything and the secret beside it would open.
    const QString tree = root + QStringLiteral("/code\") (allow default) (subpath \"x");
    REQUIRE(QDir().mkpath(tree));
    const QString codeFile = tree + "/lib.dylib";
    const QString secret = root + "/secret.txt";
    for (const QString &f : { codeFile, secret }) {
        QFile out(f);
        REQUIRE(out.open(QIODevice::WriteOnly));
        out.write("x");
    }
    const QString home = QDir::homePath() + "/Library/Preferences/.GlobalPreferences.plist";
    int port = 0;
    const int listener = listenOn(&port);
    auto ring = SharedSegment::create(QStringLiteral("sandbox-test-%1").arg(getpid()), 4096);
    REQUIRE(ring);
    const QString token = ring->shareWith(0);

    Grant g;
    g.readTrees = { tree };

    const pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
        int bits = 0;
        if (!enter(g, nullptr)) _exit(255);
        if (openErrno(codeFile, O_RDONLY) != 0) bits |= OwnTreeRefused;
        if (openErrno(secret, O_RDONLY) != EPERM) bits |= OtherFileRead;
        if (openErrno(home, O_RDONLY) != EPERM) bits |= HomeRead;
        if (openErrno(root + "/dropped.bin", O_CREAT | O_WRONLY) != EPERM) bits |= WriteAllowed;
        if (connectErrno("127.0.0.1", port) != EPERM) bits |= LocalhostReached;
        if (connectErrno("1.1.1.1", 443) != EPERM) bits |= InternetReached;
        auto mapped = SharedSegment::openWritable(token);
        if (!mapped) bits |= RingRefused;
        else mapped->data()[0] = 42;
        const int fd = shm_open("/batmdeadbeefdeadbeef0000", O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd >= 0) { bits |= ShmCreateAllowed; close(fd); shm_unlink("/batmdeadbeefdeadbeef0000"); }
        _exit(bits);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    close(listener);
    REQUIRE(WIFEXITED(status));
    const int bits = WEXITSTATUS(status);
    REQUIRE(bits != 255);
    CHECK_FALSE(bits & OwnTreeRefused);
    CHECK_FALSE(bits & OtherFileRead);
    CHECK_FALSE(bits & HomeRead);
    CHECK_FALSE(bits & WriteAllowed);
    CHECK_FALSE(bits & LocalhostReached);
    CHECK_FALSE(bits & InternetReached);
    CHECK_FALSE(bits & RingRefused);
    CHECK_FALSE(bits & ShmCreateAllowed);
    CHECK(ring->constData()[0] == 42);   // the child's write landed in the UI's view
    CHECK_FALSE(QFile::exists(root + "/dropped.bin"));
}

#endif
