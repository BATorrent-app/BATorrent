// SPDX-License-Identifier: MIT
// Fork patch 0004 (sequential_piece_order): the order a sequential torrent whose
// pieces are all top priority (how a streamed file is set up) comes down in.
// A real seeder and leecher over loopback, so this is the engine's own picker.

#include <catch2/catch_test_macros.hpp>

#ifdef BAT_LIBTORRENT_FORK

#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/peer_class.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_info.hpp>

#include <QTemporaryDir>

#include <chrono>
#include <fstream>
#include <random>
#include <vector>

namespace lt = libtorrent;

namespace {

constexpr int kPieces = 64;
constexpr int kPieceLen = 32 * 1024;

std::shared_ptr<lt::torrent_info> makeFile(const std::string &dir)
{
    std::vector<char> data(std::size_t(kPieces) * kPieceLen);
    std::mt19937 rng(7);
    for (char &c : data) c = char(rng());
    std::ofstream(dir + "/movie.mkv", std::ios::binary).write(data.data(), std::streamsize(data.size()));

    lt::file_storage fs;
    lt::add_files(fs, dir + "/movie.mkv");
    lt::create_torrent ct(fs, kPieceLen, lt::create_torrent::v1_only);
    lt::set_piece_hashes(ct, dir);
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    return std::make_shared<lt::torrent_info>(buf, lt::from_span);
}

lt::settings_pack loopback()
{
    lt::settings_pack p;
    p.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:0");
    p.set_bool(lt::settings_pack::enable_dht, false);
    p.set_bool(lt::settings_pack::enable_lsd, false);
    p.set_bool(lt::settings_pack::enable_upnp, false);
    p.set_bool(lt::settings_pack::enable_natpmp, false);
    return p;
}

// Piece indices in the order the leecher finished them.
std::vector<int> finishOrder(bool inOrder)
{
    QTemporaryDir seedDir, leechDir;
    const auto ti = makeFile(seedDir.path().toStdString());

    lt::session seeder(loopback());
    // loopback peers sit in the local class, which ignores the session limit;
    // a cap keeps the transfer request-paced instead of one instant burst
    lt::peer_class_info lpc = seeder.get_peer_class(lt::session_handle::local_peer_class_id);
    lpc.upload_limit = 1024 * 1024;
    seeder.set_peer_class(lt::session_handle::local_peer_class_id, lpc);
    lt::add_torrent_params seed;
    seed.ti = ti;
    seed.save_path = seedDir.path().toStdString();
    seed.flags |= lt::torrent_flags::seed_mode;
    seeder.add_torrent(seed);

    lt::settings_pack lp = loopback();
    lp.set_int(lt::settings_pack::alert_mask, lt::alert_category::piece_progress);
    lp.set_int(lt::settings_pack::hashing_threads, 1);   // keep completion in request order
    lp.set_bool(lt::settings_pack::sequential_piece_order, inOrder);
    lt::session leecher(lp);
    lt::add_torrent_params atp;
    atp.ti = ti;
    atp.save_path = leechDir.path().toStdString();
    atp.flags |= lt::torrent_flags::sequential_download;
    atp.file_priorities = {lt::top_priority};
    lt::torrent_handle h = leecher.add_torrent(atp);
    h.connect_peer(lt::tcp::endpoint(lt::make_address("127.0.0.1"), std::uint16_t(seeder.listen_port())));

    std::vector<int> order;
    const auto giveUp = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (int(order.size()) < kPieces && std::chrono::steady_clock::now() < giveUp) {
        leecher.wait_for_alert(std::chrono::milliseconds(200));
        std::vector<lt::alert *> alerts;
        leecher.pop_alerts(&alerts);
        for (lt::alert *a : alerts)
            if (auto *pf = lt::alert_cast<lt::piece_finished_alert>(a))
                order.push_back(static_cast<int>(pf->piece_index));
    }
    return order;
}

// How far ahead of its turn the most out-of-place piece arrived.
int maxJump(const std::vector<int> &order)
{
    int worst = 0;
    for (int pos = 0; pos < int(order.size()); ++pos)
        worst = std::max(worst, order[pos] - pos);
    return worst;
}

} // namespace

TEST_CASE("stock libtorrent fetches a top-priority sequential file out of order", "[streamorder]")
{
    const auto order = finishOrder(false);
    REQUIRE(order.size() == std::size_t(kPieces));
    // the top-priority bucket is shuffled: some piece lands far ahead of its turn
    CHECK(maxJump(order) > 16);
}

TEST_CASE("sequential_piece_order fetches it in index order", "[streamorder]")
{
    const auto order = finishOrder(true);
    REQUIRE(order.size() == std::size_t(kPieces));
    // blocks of neighbouring pieces overlap in flight, so allow a short reorder
    CHECK(maxJump(order) <= 4);
    CHECK(order.front() == 0);
}

#else

TEST_CASE("sequential_piece_order needs the libtorrent fork", "[streamorder]")
{
    SKIP("built against stock libtorrent");
}

#endif
