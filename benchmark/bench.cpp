// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
//
// Controlled A/B download benchmark. A single capped seeder feeds a fresh leecher
// over loopback; we run the leecher twice and report time-to-complete. This
// isolates the engine/config delta the BATorrent-vs-qBittorrent benchmark must
// prove (qBit is also libtorrent), with no network, no infra, and no GUI.
//
//   --ab config : stock libtorrent defaults  vs  BATorrent tuned settings_pack
//   --ab ramp   : same tuned config, piece_request_fast_ramp OFF vs ON — isolates
//                 the fork's slow-start patch alone (off == stock behavior).
//
// Latency is the variable that exercises the request pipeline. --rtt adds it with
// an in-process TCP delay relay between leecher and seeder (no sudo, deterministic)
// so the high-BDP path is reproducible. --files raises disk pressure.
//
// Build:  cmake -B build -DBAT_BENCH=ON && cmake --build build --target bat_bench
// Run:    ./bat_bench --ab ramp --rtt 100 --size 64 --seed-rate 8000 --trials 5

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
#include <libtorrent/peer_info.hpp>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace lt = libtorrent;
using clk = std::chrono::steady_clock;

// High ports so the bench never collides with a real torrent client on 6881.
static constexpr int kSeedPort  = 16881;
static constexpr int kRelayPort = 16890;

struct Profile {
    std::string name;
    bool tuned;       // apply BATorrent's settings_pack tuning
    bool fastRamp;    // fork: piece_request_fast_ramp
};

struct Args {
    int sizeMB = 256;
    int files = 1;
    int pieceKB = 0;      // 0 = libtorrent auto
    int seedRateKBs = 8000;
    int trials = 3;
    int rttMs = 0;        // round-trip latency injected by the relay (0 = direct)
    std::string ab = "ramp";
    std::string seeders;    // comma list of per-seeder caps KB/s (empty = one --seed-rate seeder)
    std::string seederRtt;  // comma list of per-seeder RTT ms (empty = all --rtt)
};

static std::vector<int> parseInts(const std::string &csv)
{
    std::vector<int> out;
    std::size_t i = 0;
    while (i < csv.size()) {
        std::size_t j = csv.find(',', i);
        if (j == std::string::npos) j = csv.size();
        out.push_back(std::atoi(csv.substr(i, j - i).c_str()));
        i = j + 1;
    }
    return out;
}

// ---- in-process TCP delay relay: models RTT without sudo/dummynet ----------
namespace relay {

struct Chunk { clk::time_point release; std::vector<char> data; bool eof = false; };

class DelayPipe {
    std::mutex m_;
    std::condition_variable cv_;
    std::deque<Chunk> q_;
public:
    void push(Chunk c) {
        { std::lock_guard<std::mutex> l(m_); q_.push_back(std::move(c)); }
        cv_.notify_one();
    }
    Chunk pop() {
        std::unique_lock<std::mutex> l(m_);
        cv_.wait(l, [&] { return !q_.empty(); });
        const auto rel = q_.front().release;
        l.unlock();
        std::this_thread::sleep_until(rel);   // the latency lives here
        l.lock();
        Chunk c = std::move(q_.front());
        q_.pop_front();
        return c;
    }
};

static void reader(int src, DelayPipe *pipe, std::chrono::milliseconds delay)
{
    std::vector<char> buf(64 * 1024);
    for (;;) {
        const ssize_t n = ::read(src, buf.data(), buf.size());
        if (n <= 0) { pipe->push({clk::now() + delay, {}, true}); return; }
        pipe->push({clk::now() + delay,
                    std::vector<char>(buf.data(), buf.data() + n), false});
    }
}

static void writer(int dst, DelayPipe *pipe)
{
    for (;;) {
        Chunk c = pipe->pop();
        if (c.eof) { ::shutdown(dst, SHUT_WR); return; }
        const char *p = c.data.data();
        std::size_t left = c.data.size();
        while (left) {
            const ssize_t n = ::write(dst, p, left);
            if (n <= 0) return;
            p += n; left -= std::size_t(n);
        }
    }
}

static int dialLoopback(int port)
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(std::uint16_t(port));
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&a), sizeof(a)) < 0) {
        ::close(fd); return -1;
    }
    return fd;
}

static void run(int listenPort, int upstreamPort, int rttMs)
{
    const int ls = ::socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    ::setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(std::uint16_t(listenPort));
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(ls, reinterpret_cast<sockaddr *>(&a), sizeof(a)) < 0) {
        std::perror("relay bind"); return;
    }
    ::listen(ls, 8);
    const std::chrono::milliseconds delay(rttMs / 2);   // half each way
    for (;;) {
        const int cs = ::accept(ls, nullptr, nullptr);
        if (cs < 0) continue;
        const int us = dialLoopback(upstreamPort);
        if (us < 0) { ::close(cs); continue; }
        auto *up = new DelayPipe();    // leaked per-connection; fine for a bench
        auto *down = new DelayPipe();
        std::thread(reader, cs, up, delay).detach();
        std::thread(writer, us, up).detach();
        std::thread(reader, us, down, delay).detach();
        std::thread(writer, cs, down).detach();
    }
}

} // namespace relay

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

static void applyProfile(lt::settings_pack &p, const Profile &prof)
{
    if (prof.tuned) {
        p.set_int(lt::settings_pack::file_pool_size, 40);
        p.set_int(lt::settings_pack::max_queued_disk_bytes, 6 * 1024 * 1024);
        p.set_int(lt::settings_pack::max_out_request_queue, 1500);
        p.set_int(lt::settings_pack::connection_speed, 50);
        p.set_int(lt::settings_pack::aio_threads, 10);
    }
    p.set_bool(lt::settings_pack::piece_request_fast_ramp, prof.fastRamp);
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
static double runLeech(const std::shared_ptr<lt::torrent_info> &ti,
                       const std::vector<int> &peerPorts,
                       const Profile &prof, const fs::path &saveDir)
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
    // All seeders live on 127.0.0.1 (distinct ports); without this libtorrent keeps
    // only ONE connection per IP and silently drops the rest — a harness artifact,
    // not the swarm behavior we want to measure.
    p.set_bool(lt::settings_pack::allow_multiple_connections_per_ip, true);
    applyProfile(p, prof);
    lt::session s(p);

    lt::add_torrent_params atp;
    atp.ti = ti;
    atp.save_path = saveDir.string();
    lt::torrent_handle h = s.add_torrent(atp);
    for (int port : peerPorts)
        h.connect_peer(lt::tcp::endpoint(lt::make_address("127.0.0.1"),
                                         std::uint16_t(port)));

    const auto start = clk::now();
    const auto deadline = start + std::chrono::minutes(10);
    std::map<int, std::int64_t> peerBytes;   // peer port -> payload bytes pulled
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::vector<lt::peer_info> pi;
        h.get_peer_info(pi);
        for (const auto &p : pi)
            peerBytes[p.ip.port()] = std::max(peerBytes[p.ip.port()], p.total_download);
        const lt::torrent_status st = h.status();
        if (st.is_seeding || st.progress >= 1.0f) {
            std::fprintf(stderr, "    peer split:");
            for (const auto &kv : peerBytes)
                std::fprintf(stderr, " :%d=%.1fMB", kv.first, kv.second / 1048576.0);
            std::fprintf(stderr, "\n");
            return std::chrono::duration<double>(clk::now() - start).count();
        }
        if (clk::now() > deadline)
            return -1.0;
    }
}

int main(int argc, char **argv)
{
    Args a;
    for (int i = 1; i < argc; ++i) {
        auto num = [&]() { return i + 1 < argc ? std::atoi(argv[++i]) : 0; };
        auto str = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (!std::strcmp(argv[i], "--size")) a.sizeMB = num();
        else if (!std::strcmp(argv[i], "--files")) a.files = num();
        else if (!std::strcmp(argv[i], "--piece")) a.pieceKB = num();
        else if (!std::strcmp(argv[i], "--seed-rate")) a.seedRateKBs = num();
        else if (!std::strcmp(argv[i], "--trials")) a.trials = num();
        else if (!std::strcmp(argv[i], "--rtt")) a.rttMs = num();
        else if (!std::strcmp(argv[i], "--ab")) a.ab = str();
        else if (!std::strcmp(argv[i], "--seeders")) a.seeders = str();
        else if (!std::strcmp(argv[i], "--seeder-rtt")) a.seederRtt = str();
    }

    Profile A, B;
    if (a.ab == "config") {
        A = {"stock", false, false};
        B = {"batorrent", true, true};
    } else {
        A = {"ramp-off", true, false};
        B = {"ramp-on", true, true};
    }

    const fs::path root = fs::temp_directory_path() / "bat-bench";
    const fs::path dataDir = root / "data";
    fs::remove_all(root);
    fs::create_directories(dataDir);

    std::printf("Generating %d MB across %d file(s)...\n", a.sizeMB, a.files);
    const std::size_t per = (std::size_t(a.sizeMB) * 1024 * 1024) / std::size_t(a.files);
    for (int i = 0; i < a.files; ++i)
        writeRandom(dataDir / ("file" + std::to_string(i) + ".bin"), per);

    const lt::add_torrent_params seedAtp = makeTorrent(dataDir, a);
    auto ti = seedAtp.ti;

    // One capped seeder per --seeders entry (default: a single --seed-rate seeder).
    // A heterogeneous mix (one fast + several slow) exercises peer selection and the
    // slow-peer tail; each seeder optionally gets its own RTT via the relay.
    std::vector<int> rates = a.seeders.empty() ? std::vector<int>{a.seedRateKBs}
                                               : parseInts(a.seeders);
    const std::vector<int> rtts = parseInts(a.seederRtt);
    std::vector<std::unique_ptr<lt::session>> seeders;
    std::vector<int> peerPorts;
    bool needRelay = false;
    for (std::size_t i = 0; i < rates.size(); ++i) {
        const int port = kSeedPort + int(i);
        const int rtt = i < rtts.size() ? rtts[i] : a.rttMs;

        lt::settings_pack sp;
        sp.set_str(lt::settings_pack::listen_interfaces,
                   std::string("127.0.0.1:") + std::to_string(port));
        sp.set_bool(lt::settings_pack::enable_dht, false);
        sp.set_bool(lt::settings_pack::enable_lsd, false);
        sp.set_int(lt::settings_pack::upload_rate_limit, rates[i] * 1024);
        auto s = std::make_unique<lt::session>(sp);
        // localhost peers land in local_peer_class, exempt from the session rate
        // limit — cap that class directly or the bottleneck won't bind.
        lt::peer_class_info lpc = s->get_peer_class(lt::session_handle::local_peer_class_id);
        lpc.upload_limit = rates[i] * 1024;
        s->set_peer_class(lt::session_handle::local_peer_class_id, lpc);

        lt::add_torrent_params atp = seedAtp;
        atp.save_path = dataDir.parent_path().string();
        atp.flags |= lt::torrent_flags::seed_mode;
        s->add_torrent(atp);
        seeders.push_back(std::move(s));

        if (rtt > 0) {
            const int relayPort = kRelayPort + int(i);
            std::thread(relay::run, relayPort, port, rtt).detach();
            peerPorts.push_back(relayPort);
            needRelay = true;
        } else {
            peerPorts.push_back(port);
        }
    }
    if (needRelay)
        std::this_thread::sleep_for(std::chrono::milliseconds(150));  // let relays bind

    std::printf("Seeders: ");
    for (std::size_t i = 0; i < rates.size(); ++i)
        std::printf("%d KB/s%s ", rates[i],
                    (i < rtts.size() ? rtts[i] : a.rttMs) > 0 ? "(+rtt)" : "");
    std::printf("\nA/B: %s vs %s | default RTT %dms | %d trials\n",
                A.name.c_str(), B.name.c_str(), a.rttMs, a.trials);
    std::printf("\n%-12s %8s %8s %12s\n", "profile", "trial", "secs", "MB/s");
    std::printf("------------------------------------------------\n");

    double sumA = 0, sumB = 0;
    int okA = 0, okB = 0;
    for (const Profile *prof : {&A, &B}) {
        for (int t = 1; t <= a.trials; ++t) {
            const double secs = runLeech(ti, peerPorts, *prof, root / ("leech-" + prof->name));
            const double mbps = secs > 0 ? double(a.sizeMB) / secs : 0;
            std::printf("%-12s %8d %8.2f %12.2f\n", prof->name.c_str(), t, secs, mbps);
            if (secs > 0) {
                if (prof == &A) { sumA += secs; ++okA; }
                else { sumB += secs; ++okB; }
            }
        }
    }

    std::printf("------------------------------------------------\n");
    if (okA && okB) {
        const double avgA = sumA / okA, avgB = sumB / okB;
        const double delta = (avgA - avgB) / avgA * 100.0;
        std::printf("avg %-10s: %.2fs\n", A.name.c_str(), avgA);
        std::printf("avg %-10s: %.2fs\n", B.name.c_str(), avgB);
        std::printf("delta         : %+.1f%% %s\n", delta,
                    delta > 0 ? ("(" + B.name + " faster)").c_str()
                              : "(no win — tune the scenario)");
    }
    fs::remove_all(root);
    return 0;
}
