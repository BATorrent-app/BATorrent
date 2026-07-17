#include <catch2/catch_test_macros.hpp>
#include "services/metadata/mkvchapters.h"

#include <QByteArray>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>

// ---- minimal EBML/Matroska encoders (test-only) -------------------------

static void putId(QByteArray &b, quint32 id) {
    int len = id <= 0xFF ? 1 : id <= 0xFFFF ? 2 : id <= 0xFFFFFF ? 3 : 4;
    for (int i = len - 1; i >= 0; --i) b.append(char((id >> (8 * i)) & 0xFF));
}
static void putSize(QByteArray &b, quint64 size) {
    // 8-byte vint form: 0x01 marker + 7 big-endian bytes
    b.append(char(0x01));
    for (int i = 6; i >= 0; --i) b.append(char((size >> (8 * i)) & 0xFF));
}
static QByteArray elem(quint32 id, const QByteArray &content) {
    QByteArray b; putId(b, id); putSize(b, quint64(content.size())); b += content; return b;
}
static QByteArray uintContent(quint64 v) {
    QByteArray b;
    for (int i = 7; i >= 0; --i) b.append(char((v >> (8 * i)) & 0xFF));
    return b;
}
static QByteArray chapterAtom(quint64 startNs, quint64 endNs, const QByteArray &name) {
    QByteArray c;
    c += elem(0x91, uintContent(startNs));            // ChapterTimeStart
    c += elem(0x92, uintContent(endNs));              // ChapterTimeEnd
    c += elem(0x80, elem(0x85, name));                // ChapterDisplay → ChapString
    return elem(0xB6, c);                             // ChapterAtom
}

static QByteArray buildMkv() {
    QByteArray edition;
    edition += chapterAtom(0, 90000000000ull, "Intro");                 // 0 .. 90s
    edition += chapterAtom(2700000000000ull, 2760000000000ull, "Credits"); // 45m .. 46m
    QByteArray chapters = elem(0x1043A770, elem(0x45B9, edition));

    QByteArray segChildren;
    segChildren += elem(0xEC, QByteArray(16, '\0'));   // a Void element to skip past
    segChildren += chapters;
    QByteArray segment = elem(0x18538067, segChildren);

    QByteArray ebml = elem(0x1A45DFA3, QByteArray(8, '\1'));  // header body (skipped by size)
    return ebml + segment;
}

static QString writeTemp(const QByteArray &bytes) {
    QTemporaryFile f(QDir::tempPath() + "/mkvchapXXXXXX.mkv");
    f.setAutoRemove(false);
    REQUIRE(f.open());
    f.write(bytes);
    f.close();
    return f.fileName();
}

TEST_CASE("mkvchapters: parses two chapters with times and names") {
    const QString path = writeTemp(buildMkv());
    auto ch = readMkvChapters(path);
    QFile::remove(path);

    REQUIRE(ch.size() == 2);
    REQUIRE(ch[0].name == "Intro");
    REQUIRE(ch[0].startMs == 0);
    REQUIRE(ch[0].endMs == 90000);
    REQUIRE(ch[0].kind == "intro");
    REQUIRE(ch[1].name == "Credits");
    REQUIRE(ch[1].startMs == 2700000);
    REQUIRE(ch[1].endMs == 2760000);
    REQUIRE(ch[1].kind == "credits");
}

TEST_CASE("mkvchapters: classification maps common labels") {
    for (const char *n : {"Opening", "Recap", "Previously On", "Abertura"}) {
        QByteArray ed = chapterAtom(0, 1000000000ull, n);
        QByteArray mkv = elem(0x1A45DFA3, QByteArray(8, '\1'))
                       + elem(0x18538067, elem(0x1043A770, elem(0x45B9, ed)));
        const QString p = writeTemp(mkv);
        auto ch = readMkvChapters(p); QFile::remove(p);
        REQUIRE(ch.size() == 1);
        REQUIRE(ch[0].kind == "intro");
    }
}

TEST_CASE("mkvchapters: hostile / non-mkv input never crashes, returns empty") {
    REQUIRE(readMkvChapters("/no/such/file.mkv").isEmpty());

    // random bytes
    const QString p1 = writeTemp(QByteArray(4096, '\xA5'));
    REQUIRE(readMkvChapters(p1).isEmpty());
    QFile::remove(p1);

    // valid header, truncated mid-chapters
    QByteArray full = buildMkv();
    const QString p2 = writeTemp(full.left(full.size() / 2));
    auto ch = readMkvChapters(p2);   // may be empty or partial — must not crash
    REQUIRE(ch.size() <= 2);
    QFile::remove(p2);

    // a size field claiming far more than the file holds
    QByteArray bad;
    putId(bad, 0x1A45DFA3); putSize(bad, 1ull << 40); bad += "xx";
    const QString p3 = writeTemp(bad);
    REQUIRE(readMkvChapters(p3).isEmpty());
    QFile::remove(p3);
}
