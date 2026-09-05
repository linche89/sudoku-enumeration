// Bounded, read-only LDPCAN01/v2 support audit. This never evaluates F or N.
// The stored stabilizers are checked as divisors, not recomputed on every key.
// Build (Windows): g++ -O3 -std=c++20 -Wall -Wextra layer_support_probe.cpp
//                  -o layer_support_probe.exe -lbcrypt -lpsapi
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <psapi.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
constexpr u64 MAGIC = 0x314b434c4a464453ULL;
constexpr u64 SEED = 0x5344464a434b3031ULL;
struct Header {
    u64 magic;
    u32 version, c, layer, parent;
    u64 config, generation, entries, holes, cursor, chunks, chunkParents;
    u64 emissions, cacheHits, parentHash, wide, payloadHash, headerHash;
};
struct State {
    std::array<u16, 12> m{};
    bool operator==(const State&) const = default;
};
struct StateHash {
    size_t operator()(const State& s) const {
        u64 h = SEED;
        for (int i = 0; i < 3; ++i) {
            u64 word; std::memcpy(&word, s.m.data()+4*i, 8);
            h ^= word; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
        }
        return size_t(h);
    }
};
static_assert(sizeof(Header) == 128 && sizeof(State) == 24);

struct File {
    FILE* f;
    explicit File(const std::string& p) : f(std::fopen(p.c_str(), "rb")) {
        if (!f) throw std::runtime_error("cannot open input read-only: " + p);
    }
    ~File() { if (f) std::fclose(f); }
};

struct Hash64 {
    u64 h, tail = 0;
    unsigned tailBytes = 0;
    Hash64(u64 length, u64 seed) : h(seed ^ (0x9e3779b97f4a7c15ULL + length)) {}
    void add(const unsigned char* p, size_t n) {
        while (n) {
            if (!tailBytes && n >= 8) {
                u64 w; std::memcpy(&w, p, 8);
                h ^= w; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
                p += 8; n -= 8;
            } else {
                tail |= u64(*p++) << (8 * tailBytes++); --n;
                if (tailBytes == 8) {
                    h ^= tail; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
                    tail = 0; tailBytes = 0;
                }
            }
        }
    }
    u64 finish() {
        h ^= tail; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33;
        return h;
    }
};

struct Sha256 {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::vector<unsigned char> object;
    Sha256() {
        DWORD bytes = 0, size = 0;
        if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
            BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
                              reinterpret_cast<PUCHAR>(&size), sizeof(size), &bytes, 0) < 0)
            throw std::runtime_error("cannot initialize Windows SHA-256");
        object.resize(size);
        if (BCryptCreateHash(alg, &hash, object.data(), size, nullptr, 0, 0) < 0)
            throw std::runtime_error("cannot create SHA-256 hash");
    }
    ~Sha256() {
        if (hash) BCryptDestroyHash(hash);
        if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    }
    void add(const unsigned char* p, size_t n) {
        if (n > ULONG_MAX || BCryptHashData(hash, const_cast<PUCHAR>(p), ULONG(n), 0) < 0)
            throw std::runtime_error("SHA-256 update failed");
    }
    std::string finish() {
        unsigned char d[32];
        if (BCryptFinishHash(hash, d, sizeof(d), 0) < 0)
            throw std::runtime_error("SHA-256 finalization failed");
        const char* hex = "0123456789ABCDEF";
        std::string out;
        for (auto v : d) { out += hex[v >> 4]; out += hex[v & 15]; }
        return out;
    }
};

static void seek(FILE* f, u64 position) {
    if (_fseeki64(f, static_cast<__int64>(position), SEEK_SET))
        throw std::runtime_error("input seek failed");
}
static void read(FILE* f, void* p, size_t n) {
    if (std::fread(p, 1, n, f) != n) throw std::runtime_error("truncated input/read error");
}
static void sort_small(std::array<u16, 12>& a, unsigned n) {
    for (unsigned i = 1; i < n; ++i) {
        const u16 value = a[i]; unsigned j = i;
        while (j && value < a[j-1]) { a[j] = a[j-1]; --j; }
        a[j] = value;
    }
}
static std::array<u16, 12> slots(const State& s, int c, int layer) {
    std::array<u16, 12> out{};
    int degrees[12]{};
    for (int i = 0; i < 2*c; ++i) {
        if (i && s.m[i] < s.m[i-1]) throw std::runtime_error("sample key is not sorted");
        if (s.m[i] >> (2*c)) throw std::runtime_error("sample key exceeds C");
        int used = 0;
        for (int b = 0; b < c; ++b) {
            const int v = (s.m[i] >> (2*b)) & 3;
            if (v == 1) throw std::runtime_error("invalid native field 1");
            if (v) { ++used; const int bit = 2*b + (v & 1);
                out[i] |= u16(1u << bit); ++degrees[bit]; }
        }
        if (used != layer) throw std::runtime_error("sample mask has wrong degree");
    }
    for (int i = 0; i < 2*c; ++i)
        if (degrees[i] != layer) throw std::runtime_error("sample slot has wrong degree");
    for (int i = 2*c; i < 12; ++i)
        if (s.m[i]) throw std::runtime_error("nonzero unused key padding");
    sort_small(out, 2*c);
    return out;
}
static u64 group_order(int c) {
    u64 result = 1ULL << c;
    for (int i = 2; i <= c; ++i) result *= i;
    return result;
}

int main(int argc, char** argv) try {
    std::string input, expected, output, targetFile;
    u64 sampleCount = 0, seed = 20260905;
    double maxSeconds = 0;
    u64 maxRssMiB = 0;
    bool readonly = false, expectedValues = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i == argc) throw std::runtime_error("missing value after " + arg);
            return argv[i];
        };
        if (arg == "--input") input = value();
        else if (arg == "--expected-sha256") expected = value();
        else if (arg == "--sample-output") output = value();
        else if (arg == "--target-file") targetFile = value();
        else if (arg == "--samples") sampleCount = std::stoull(value());
        else if (arg == "--seed") seed = std::stoull(value());
        else if (arg == "--max-seconds") maxSeconds = std::stod(value());
        else if (arg == "--max-rss-mib") maxRssMiB = std::stoull(value());
        else if (arg == "--checkpointreadonly") readonly = true;
        else if (arg == "--closed-small-expected-values") expectedValues = true;
        else throw std::runtime_error("unknown option " + arg);
    }
    if (!readonly || input.empty() || maxSeconds <= 0 || maxSeconds > 300 ||
        maxRssMiB == 0 || maxRssMiB > 2048 || sampleCount > 10000 ||
        bool(sampleCount) != !output.empty())
        throw std::runtime_error("require --input, --checkpointreadonly, bounds <=300s/2048MiB; samples<=10000 needs new output");
    if (!expected.empty()) {
        std::transform(expected.begin(), expected.end(), expected.begin(),
                       [](unsigned char x) { return char(std::toupper(x)); });
        if (expected.size() != 64 || expected.find_first_not_of("0123456789ABCDEF") != std::string::npos)
            throw std::runtime_error("expected SHA-256 must contain 64 hex digits");
    }
    if (!output.empty() && std::filesystem::exists(output))
        throw std::runtime_error("sample output must not exist");
    const auto started = std::chrono::steady_clock::now();
    u64 peakRss = 0;
    auto guard = [&]() {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
        PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)))
            throw std::runtime_error("cannot enforce RSS guard");
        peakRss = std::max(peakRss, u64(pm.WorkingSetSize));
        if (seconds > maxSeconds || peakRss > maxRssMiB * (1ULL << 20))
            throw std::runtime_error("time or RSS guard exceeded; no result accepted");
    };
    const auto initialSize = std::filesystem::file_size(input);
    const auto initialTime = std::filesystem::last_write_time(input);
    File file(input);
    Header h{}; read(file.f, &h, sizeof(h));
    Header zero = h; zero.headerHash = 0;
    Hash64 hh(sizeof(h), SEED); hh.add(reinterpret_cast<unsigned char*>(&zero), sizeof(zero));
    if (hh.finish() != h.headerHash || h.magic != MAGIC || h.version != 2 ||
        h.c < 2 || h.c > 6 || h.layer < 1 || h.layer > h.c ||
        h.holes > h.entries || h.wide > h.entries || h.entries > 2000000000ULL ||
        initialSize != sizeof(h) + h.entries * 36 + h.wide * 16)
        throw std::runtime_error("header checksum, geometry, or exact length failed");
    if (h.parent ? (h.layer != h.parent+1 || !h.chunkParents || !h.chunks || h.cursor > h.chunks)
                 : bool(h.cursor || h.chunks || h.chunkParents || h.parentHash))
        throw std::runtime_error("invalid transition/snapshot header");
    if (expectedValues && (h.c > 5 || (h.parent && h.cursor != h.chunks)))
        throw std::runtime_error("expected values allowed only for closed C<=5 files");
    if (sampleCount && h.entries == h.holes) throw std::runtime_error("cannot sample empty support");
    const u64 group = group_order(h.c), stabOffset = sizeof(h) + h.entries * 32;
    std::unordered_map<State, size_t, StateHash> targetImages;
    std::vector<u64> targetCounts;
    if (!targetFile.empty()) {
        File targets(targetFile);
        char line[4096];
        while (std::fgets(line, sizeof(line), targets.f)) {
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
            if (targetCounts.size() == 128) throw std::runtime_error("at most 128 target states");
            std::istringstream row(line); State original{};
            for (unsigned i = 0; i < 2*h.c; ++i) {
                unsigned mask;
                if (!(row >> mask) || mask >= (1u << (2*h.c)))
                    throw std::runtime_error("target requires 2C true slot masks");
                for (unsigned b = 0; b < h.c; ++b) {
                    const unsigned v = (mask >> (2*b)) & 3;
                    if (v == 3) throw std::runtime_error("target uses both sides of box");
                    if (v) original.m[i] |= u16((v == 1 ? 2 : 3) << (2*b));
                }
            }
            sort_small(original.m, 2*h.c);
            slots(original, h.c, h.layer);
            std::array<unsigned, 6> perm{};
            for (unsigned b = 0; b < h.c; ++b) perm[b] = b;
            const size_t id = targetCounts.size();
            size_t images = 0;
            do {
                guard();
                for (unsigned flip = 0; flip < (1u << h.c); ++flip) {
                    State image{};
                    for (unsigned i = 0; i < 2*h.c; ++i)
                        for (unsigned b = 0; b < h.c; ++b) {
                            const unsigned v = (original.m[i] >> (2*b)) & 3;
                            if (v) image.m[i] |= u16((v ^ ((flip >> b)&1)) << (2*perm[b]));
                        }
                    sort_small(image.m, 2*h.c);
                    const auto [pos, added] = targetImages.emplace(image, id);
                    if (pos->second != id) throw std::runtime_error("target states share an orbit");
                    images += added;
                }
            } while (std::next_permutation(perm.begin(), perm.begin()+h.c));
            targetCounts.push_back(0);
            std::cout << "target=" << id << " coordinate_images=" << images << '\n';
        }
        if (std::ferror(targets.f) || targetCounts.empty()) throw std::runtime_error("empty or unreadable targets");
    }
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<u64> draw(0, h.entries ? h.entries-1 : 0);
    std::vector<State> samples(sampleCount);
    std::vector<u32> sampleStabs(sampleCount);
    std::vector<u64> sampleT(sampleCount);
    std::map<u64, std::vector<size_t>> selected;
    for (size_t k = 0; k < sampleCount;) {
        guard(); const u64 index = draw(rng); u32 stab;
        seek(file.f, stabOffset + index*4); read(file.f, &stab, sizeof(stab));
        if (!stab) continue;
        if (group % stab) throw std::runtime_error("sample stabilizer is not a group divisor");
        selected[index].push_back(k); sampleStabs[k++] = stab;
    }
    seek(file.f, sizeof(h));
    Sha256 sha; sha.add(reinterpret_cast<unsigned char*>(&h), sizeof(h));
    std::vector<unsigned char> buffer(24 * 262144);
    u64 chained = SEED, real = 0, holes = 0, mass = 0, candidateCount = 0;
    std::vector<u64> candidateIndices;
    std::vector<std::pair<u64, size_t>> targetHits;
    std::map<u32, u64> stabHistogram;
    for (int section = 0; section < (h.wide ? 4 : 3); ++section) {
        const u64 width = section == 0 ? 24 : section == 1 ? 8 : section == 2 ? 4 : 16;
        const u64 count = section == 3 ? h.wide : h.entries;
        Hash64 hash(count * width, chained);
        auto chosen = selected.begin();
        size_t candidatePos = 0;
        for (u64 offset = 0; offset < count;) {
            guard();
            const size_t records = size_t(std::min<u64>(count-offset, buffer.size()/width));
            const size_t bytes = records * size_t(width);
            read(file.f, buffer.data(), bytes);
            sha.add(buffer.data(), bytes); hash.add(buffer.data(), bytes);
            if (section == 0) {
                const auto* keys = reinterpret_cast<const State*>(buffer.data());
                for (size_t j = 0; j < records; ++j) {
                    const auto& s = keys[j];
                    if (!targetImages.empty()) {
                        const auto found = targetImages.find(s);
                        if (found != targetImages.end()) {
                            targetHits.emplace_back(offset+j, found->second);
                            if (targetHits.size() > 10000) throw std::runtime_error("too many target hits");
                        }
                    }
                    if (h.c == 6 && h.layer == 4 && s.m[0] == s.m[3] && s.m[4] == s.m[7] &&
                        s.m[8] == s.m[11] && s.m[0] < s.m[4] && s.m[4] < s.m[8]) {
                        slots(s, h.c, h.layer);
                        candidateIndices.push_back(offset+j);
                        if (candidateIndices.size() > 10000) throw std::runtime_error("too many candidate keys");
                    }
                }
                while (chosen != selected.end() && chosen->first < offset+records) {
                    for (auto k : chosen->second) samples[k] = keys[chosen->first-offset];
                    ++chosen;
                }
            } else if (section == 1) {
                while (chosen != selected.end() && chosen->first < offset+records) {
                    u64 value; std::memcpy(&value, buffer.data()+(chosen->first-offset)*8, 8);
                    for (auto k : chosen->second) sampleT[k] = value;
                    ++chosen;
                }
            } else if (section == 2) {
                const auto* stabs = reinterpret_cast<const u32*>(buffer.data());
                for (size_t j = 0; j < records; ++j) {
                    const u32 s = stabs[j];
                    if (!s) { ++holes; continue; }
                    if (group % s) throw std::runtime_error("stored stabilizer is not a group divisor");
                    ++real; mass += group/s; ++stabHistogram[s];
                }
                while (candidatePos < candidateIndices.size() && candidateIndices[candidatePos] < offset+records) {
                    const u64 index = candidateIndices[candidatePos++];
                    if (stabs[index-offset]) {
                        ++candidateCount;
                        std::cout << "candidate_444_index=" << index << " stab=" << stabs[index-offset] << '\n';
                    }
                }
            }
            offset += records;
        }
        chained = hash.finish();
        std::cout << "section=" << section << " bytes=" << count*width << " complete\n" << std::flush;
    }
    if (chained != h.payloadHash || holes != h.holes || real != h.entries-h.holes)
        throw std::runtime_error("payload hash or stored hole count mismatch");
    for (const auto& [index, target] : targetHits) {
        u32 stab; seek(file.f, stabOffset+index*4); read(file.f, &stab, 4);
        if (stab) {
            ++targetCounts[target];
            std::cout << "target=" << target << " stored_index=" << index << " stab=" << stab << '\n';
        }
    }
    const auto digest = sha.finish();
    if (!expected.empty() && digest != expected) throw std::runtime_error("full SHA-256 mismatch");
    if (std::filesystem::file_size(input) != initialSize ||
        std::filesystem::last_write_time(input) != initialTime)
        throw std::runtime_error("input changed during read-only audit");
    guard();
    std::vector<std::array<u16, 12>> decoded;
    for (const auto& s : samples) decoded.push_back(slots(s, h.c, h.layer));
    if (!output.empty()) {
        const int fd = _open(output.c_str(), _O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY,
                             _S_IREAD|_S_IWRITE);
        if (fd < 0) throw std::runtime_error("cannot exclusively create sample file");
        FILE* out = _fdopen(fd, "wb");
        if (!out) { _close(fd); throw std::runtime_error("sample stream failed"); }
        bool ok = std::fprintf(out, "# C=%u L=%u\n# seed=%llu sampling=uniform-record-with-replacement-reject-holes\n# source_sha256=%s\n",
                               h.c, h.layer, (unsigned long long)seed, digest.c_str()) >= 0;
        for (size_t k = 0; k < decoded.size(); ++k) {
            for (unsigned i = 0; i < 2*h.c; ++i)
                ok = ok && std::fprintf(out, "%s%u", i ? " " : "", decoded[k][i]) >= 0;
            if (expectedValues) {
                const u64 m = group/sampleStabs[k];
                if (sampleT[k] % m) { std::fclose(out); throw std::runtime_error("closed sample T/m is not integral"); }
                ok = ok && std::fprintf(out, " expected=%llu", (unsigned long long)(sampleT[k]/m)) >= 0;
            }
            ok = ok && std::fputc('\n', out) != EOF;
        }
        ok = std::fclose(out) == 0 && ok;
        if (!ok) throw std::runtime_error("sample write failed; output is unaccepted");
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    std::cout << "C=" << h.c << " layer=" << h.layer << " generation=" << h.generation
              << " entries=" << h.entries << " holes=" << holes << " real=" << real << '\n'
              << "stored_stabilizer_orbit_mass=" << mass << '\n'
              << "candidate_444_count=" << candidateCount << '\n'
              << "sha256=" << digest << '\n'
              << "payload_hash_verified=yes header_hash_verified=yes readonly=yes\n"
              << "semantic_full_key_audit=no stabilizers_recomputed=no weights_closed=not_claimed\n"
              << "sample_count=" << sampleCount << " seed=" << seed << '\n';
    for (const auto& [stab, count] : stabHistogram)
        std::cout << "stab=" << stab << " count=" << count << '\n';
    for (size_t i = 0; i < targetCounts.size(); ++i)
        std::cout << "target=" << i << " real_matches=" << targetCounts[i] << '\n';
    std::cout << "elapsed_seconds=" << elapsed << " peak_rss_bytes=" << peakRss << " [OK]\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
}
