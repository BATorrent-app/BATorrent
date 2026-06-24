// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
//
// Controlled A/B download benchmark. A single capped seeder feeds a fresh leecher
// over loopback; we run the leecher twice — once with libtorrent's stock defaults
// (qBittorrent's baseline, since qBit is also libtorrent) and once with
// BATorrent's tuned settings — and report time-to-complete. This isolates the
// engine/config delta the BATorrent-vs-qBittorrent benchmark must prove, with no
// network, no infra, and no GUI. Add latency/loss with dummynet (`dnctl`) around
// the loopback to exercise the high-BDP path; raise --files to exercise the disk
// path (file_pool_size / max_queued_disk_bytes).
//
// Build:  cmake -B build -DBAT_BENCH=ON && cmake --build build --target bat_bench
// Run:    ./bat_bench --size 256 --files 1 --seed-rate 8000 --trials 3

#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_class.hpp>
#include <libtorrent/peer_class_type_filter.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace lt = libtorrent;
using clk = std::chrono::steady_clock;

struct Args {
    int sizeMB = 256;
    int files = 1;
    int pieceKB = 0;      // 0 = libtorrent auto
    int seedRateKBs = 8000;
    int trials = 3;
};

static void writeRandom(const fs::path &p, std::size_t bytes)
{
    std::ofstream f(p, std::ios::binary);
    std::mt19937_64 rng(0xBA70BEEF);   // fixed seed → identical data every run
    std::vector<char> buf(1 << 20);
    std::size_t left = bytes;
    while (left) {
        const std::size_t n = std::min(buf.size(), left);
        for (std::size_t i = 0; i < n; i += 8) {
            const std::uint64_t r = rng();
            std::memcpy(buf.data() + i, &r, std::min<std::size_t>(8, n - i));
        }
        f.write(buf.data(), std::streamsize(n));
        left -= n;
    }
}

// The two profiles under test. "stock" leaves libtorrent at its defaults (the
// qBittorrent baseline); "batorrent" applies our tuning from sessionmanager.cpp.
static void applyProfile(lt::settings_pack &p, const std::string &profile)
{
    if (profile == "batorrent") {
        p.set_int(lt::settings_pack::file_pool_size, 40);
        p.set_int(lt::settings_pack::max_queued_disk_bytes, 6 * 1024 * 1024);
        p.set_int(lt::settings_pack::max_out_request_queue, 1500);
        p.set_int(lt::settings_pack::connection_speed, 50);
        p.set_int(lt::settings_pack::aio_threads, 10);
    }
}

static lt::add_torrent_params makeTorrent(const fs::path &dir, const Args &a)
{
    lt::file_storage fst;
    lt::add_files(fst, dir.string());
    lt::create_torrent ct(fst, a.pieceKB > 0 ? a.pieceKB * 1024 : 0);
    lt::set_piece_hashes(ct, dir.parent_path().string());
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    lt::add_torrent_params atp;
    atp.ti = std::make_shared<lt::torrent_info>(buf, lt::from_span);
    return atp;
}

// Returns seconds to reach 100% (or -1 on timeout).
static double runLeech(const std::shared_ptr<lt::torrent_info> &ti, int seedPort,
                       const std::string &profile, const fs::path &saveDir)
{
    fs::remove_all(saveDir);
    fs::create_directories(saveDir);

    lt::settings_pack p;
    p.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:0");
    p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status);
    p.set_bool(lt::settings_pack::enable_dht, false);
    p.set_bool(lt::settings_pack::enable_lsd, false);
    p.set_bool(lt::settings_pack::enable_upnp, false);
    p.set_bool(lt::settings_pack::enable_natpmp, false);
    applyProfile(p, profile);
    lt::session s(p);

    lt::add_torrent_params atp;
    atp.ti = ti;
    atp.save_path = saveDir.string();
    lt::torrent_handle h = s.add_torrent(atp);
    h.connect_peer(lt::tcp::endpoint(lt::make_address("127.0.0.1"),
                                     std::uint16_t(seedPort)));

    const auto start = clk::now();
    const auto deadline = start + std::chrono::minutes(10);
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const lt::torrent_status st = h.status();
        if (st.is_seeding || st.progress >= 1.0f)
            return std::chrono::duration<double>(clk::now() - start).count();
        if (clk::now() > deadline)
            return -1.0;
    }
}

int main(int argc, char **argv)
{
    Args a;
    for (int i = 1; i < argc; ++i) {
        auto next = [&]() { return i + 1 < argc ? std::atoi(argv[++i]) : 0; };
        if (!std::strcmp(argv[i], "--size")) a.sizeMB = next();
        else if (!std::strcmp(argv[i], "--files")) a.files = next();
        else if (!std::strcmp(argv[i], "--piece")) a.pieceKB = next();
        else if (!std::strcmp(argv[i], "--seed-rate")) a.seedRateKBs = next();
        else if (!std::strcmp(argv[i], "--trials")) a.trials = next();
    }

    const fs::path root = fs::temp_directory_path() / "bat-bench";
    const fs::path dataDir = root / "data";
    fs::remove_all(root);
    fs::create_directories(dataDir);

    std::printf("Generating %d MB across %d file(s)...\n", a.sizeMB, a.files);
    const std::size_t per = (std::size_t(a.sizeMB) * 1024 * 1024) / std::size_t(a.files);
    for (int i = 0; i < a.files; ++i)
        writeRandom(dataDir / ("file" + std::to_string(i) + ".bin"), per);

    lt::add_torrent_params seedAtp = makeTorrent(dataDir, a);
    auto ti = seedAtp.ti;

    // Capped seeder: the deterministic bottleneck both profiles compete against.
    lt::settings_pack sp;
    sp.set_str(lt::settings_pack::listen_interfaces, "127.0.0.1:6881");
    sp.set_bool(lt::settings_pack::enable_dht, false);
    sp.set_bool(lt::settings_pack::enable_lsd, false);
    sp.set_int(lt::settings_pack::upload_rate_limit, a.seedRateKBs * 1024);
    lt::session seeder(sp);
    // localhost peers land in local_peer_class, which is exempt from the session
    // rate limit — so cap that class directly, otherwise the bottleneck won't bind.
    {
        lt::peer_class_info lpc = seeder.get_peer_class(lt::session_handle::local_peer_class_id);
        lpc.upload_limit = a.seedRateKBs * 1024;
        seeder.set_peer_class(lt::session_handle::local_peer_class_id, lpc);
    }
    seedAtp.save_path = dataDir.parent_path().string();
    seedAtp.flags |= lt::torrent_flags::seed_mode;
    seeder.add_torrent(seedAtp);

    std::printf("Seeder capped at %d KB/s on 127.0.0.1:6881\n", a.seedRateKBs);
    std::printf("\n%-12s %8s %8s %12s\n", "profile", "trial", "secs", "MB/s");
    std::printf("------------------------------------------------\n");

    double sumStock = 0, sumBat = 0;
    int okStock = 0, okBat = 0;
    for (const std::string profile : {std::string("stock"), std::string("batorrent")}) {
        for (int t = 1; t <= a.trials; ++t) {
            const double secs = runLeech(ti, 6881, profile, root / ("leech-" + profile));
            const double mbps = secs > 0 ? double(a.sizeMB) / secs : 0;
            std::printf("%-12s %8d %8.2f %12.2f\n", profile.c_str(), t, secs, mbps);
            if (secs > 0) {
                if (profile == "stock") { sumStock += secs; ++okStock; }
                else { sumBat += secs; ++okBat; }
            }
        }
    }

    std::printf("------------------------------------------------\n");
    if (okStock && okBat) {
        const double avgStock = sumStock / okStock, avgBat = sumBat / okBat;
        const double delta = (avgStock - avgBat) / avgStock * 100.0;
        std::printf("avg stock     : %.2fs\n", avgStock);
        std::printf("avg batorrent : %.2fs\n", avgBat);
        std::printf("delta         : %+.1f%% %s\n", delta,
                    delta > 0 ? "(BATorrent faster)" : "(no win — tune the scenario)");
    }
    fs::remove_all(root);
    return 0;
}
