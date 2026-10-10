// SPDX-License-Identifier: MIT
// The media child's sandbox: the profile text can't be talked into granting
// more, and a confined process really is refused what the grant leaves out.

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "ipc/mediasandbox.h"

#ifdef Q_OS_MACOS
#  include <arpa/inet.h>
#  include <cerrno>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

using namespace MediaSandbox;

TEST_CASE("the profile denies by default and grants only what it is given", "[unit][sandbox]")
{
    Grant g;
    g.readTrees = { "/Applications/BATorrent.app" };
    g.readFile = "/Users/x/Downloads/Movie.mkv";
    g.localPort = 54321;
    const QString p = profile(g);
    CHECK(p.startsWith("(version 1)\n(deny default)\n"));
    CHECK(p.contains("(subpath \"/Applications/BATorrent.app\")"));
    CHECK(p.contains("(literal \"/Users/x/Downloads/Movie.mkv\")"));
    CHECK(p.contains("(remote ip \"localhost:54321\")"));
    CHECK_FALSE(p.contains("(allow default"));
    CHECK_FALSE(p.contains("network-outbound (remote ip \"*"));
    CHECK_FALSE(p.contains("AGX"));

    SECTION("no port, no network") {
        g.localPort = 0;
        CHECK_FALSE(profile(g).contains("network-outbound"));
        g.localPort = 70000;
        CHECK_FALSE(profile(g).contains("network-outbound"));
    }
}

TEST_CASE("a file name cannot rewrite the sandbox", "[unit][sandbox][security]")
{
    Grant g;
    // A torrent picks its own file names: this one tries to close the string
    // and open everything.
    g.readFile = "/tmp/a\") (allow default) (literal \"b.mkv";
    const QString p = profile(g);
    REQUIRE_FALSE(p.isEmpty());
    CHECK(p.contains("(literal \"/tmp/a\\\") (allow default) (literal \\\"b.mkv\")"));

    g.readFile = "/tmp/back\\slash.mkv";
    CHECK(profile(g).contains("\"/tmp/back\\\\slash.mkv\""));

    for (const char *bad : { "/tmp/new\nline.mkv", "/tmp/nul\x01.mkv", "/tmp/del\x7f.mkv" }) {
        g.readFile = QString::fromLatin1(bad);
        CHECK(profile(g).isEmpty());
    }
    g.readFile.clear();
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
    GrantedFileRefused = 1 << 0,
    OtherFileRead      = 1 << 1,
    WriteAllowed       = 1 << 2,
    GrantedPortRefused = 1 << 3,
    OtherPortReached   = 1 << 4,
    InternetReached    = 1 << 5,
    HomeRead           = 1 << 6,
    EnterFailed        = 1 << 7,
};

} // namespace

// Forked before any Qt thread exists, so the child can safely do nothing but
// syscalls once confined. Each failed expectation sets a bit in the exit code.
TEST_CASE("a confined process is refused everything its grant leaves out",
          "[integration][sandbox][security]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    // The granted name is itself an injection attempt: if escaping failed, the
    // profile would grant everything and the secret below would open.
    const QString granted = dir.filePath("movie\") (allow default) (literal \"x.mkv");
    const QString secret = dir.filePath("secret.txt");
    for (const QString &f : { granted, secret }) {
        QFile out(f);
        REQUIRE(out.open(QIODevice::WriteOnly));
        out.write("x");
    }
    const QString home = QDir::homePath() + "/Library/Preferences/.GlobalPreferences.plist";
    int grantedPort = 0, otherPort = 0;
    const int a = listenOn(&grantedPort);
    const int b = listenOn(&otherPort);

    Grant g;
    g.readFile = QFileInfo(granted).canonicalFilePath();   // the sandbox matches the real path, not /var's symlink
    g.localPort = grantedPort;

    const pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
        int bits = 0;
        if (!enter(g, nullptr)) _exit(EnterFailed);
        if (openErrno(granted, O_RDONLY) != 0) bits |= GrantedFileRefused;
        if (openErrno(secret, O_RDONLY) != EPERM) bits |= OtherFileRead;
        if (openErrno(home, O_RDONLY) != EPERM) bits |= HomeRead;
        if (openErrno(dir.filePath("dropped.bin"), O_CREAT | O_WRONLY) != EPERM) bits |= WriteAllowed;
        if (connectErrno("127.0.0.1", grantedPort) != 0) bits |= GrantedPortRefused;
        if (connectErrno("127.0.0.1", otherPort) != EPERM) bits |= OtherPortReached;
        if (connectErrno("1.1.1.1", 443) != EPERM) bits |= InternetReached;
        _exit(bits);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    close(a);
    close(b);
    REQUIRE(WIFEXITED(status));
    const int bits = WEXITSTATUS(status);
    CHECK_FALSE(bits & EnterFailed);
    CHECK_FALSE(bits & GrantedFileRefused);
    CHECK_FALSE(bits & OtherFileRead);
    CHECK_FALSE(bits & HomeRead);
    CHECK_FALSE(bits & WriteAllowed);
    CHECK_FALSE(bits & GrantedPortRefused);
    CHECK_FALSE(bits & OtherPortReached);
    CHECK_FALSE(bits & InternetReached);
    CHECK_FALSE(QFile::exists(dir.filePath("dropped.bin")));
}

#endif
