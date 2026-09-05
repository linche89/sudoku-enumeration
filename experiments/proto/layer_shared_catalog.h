// Read-only support catalogue for the shared closed-F4 route.
// Include AFTER layer_dp_gate.cpp. Link Windows builds with -lbcrypt -lpsapi.
// The source T payload participates in BOTH checksums, then is ZEROED in full.
// It is never exposed as a usable factorization value by this API.
#ifndef LAYER_SHARED_CATALOG_H
#define LAYER_SHARED_CATALOG_H

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <bcrypt.h>
#include <psapi.h>

namespace shared_catalog {

inline constexpr const char* semanticVersion = "native-support-catalog-v1";
inline constexpr const char* c6SourceSha256 =
    "ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844";
inline constexpr const char* c6RepairVersion = "L4-stab12-order3-witness-v1";
using Clock = std::chrono::steady_clock;
inline double seconds(Clock::time_point from) {
    return std::chrono::duration<double>(Clock::now() - from).count();
}

struct Options {
    std::string path;
    int threads = 1;
    int expectedLayer = 4;
    bool checkpointReadonly = false; // Caller must opt in explicitly.
    bool repairC6 = false;           // Required for a complete C6 L4 support.
    u64 maxEntries = 0;              // Positive explicit capacity bound.
    u64 maxResidentBytes = 0;        // Positive, at most 55 GiB during loading.
    double maxSeconds = 0;          // Positive, at most 600 s during loading.
};

struct Result {
    Layer layer;                    // T is entirely zero when load returns.
    CkptHeader source{};             // Verbatim source provenance, NOT weights.
    std::string sourcePath, sourceSha256, repairVersion = "none";
    std::string domain = "CATALOGUE"; // SAMPLE is never global support/export.
    u64 sourcePayloadHash = 0, sourceBytes = 0;
    std::filesystem::file_time_type sourceModified{};
    u64 storedMass = 0, repairedMass = 0, repairedId = UINT32_MAX;
    u64 estimatedResidentBytes = 0, peakResidentBytes = 0;
    double readSeconds = 0, sha256Seconds = 0, zeroSeconds = 0;
    double indexSeconds = 0, repairSeconds = 0, wallSeconds = 0;
};

// No mutable file handle is opened anywhere in this header.
inline void verify_source_unchanged(const Result& result) {
    if (std::filesystem::file_size(result.sourcePath) != result.sourceBytes ||
        std::filesystem::last_write_time(result.sourcePath) != result.sourceModified)
        throw std::runtime_error("support catalogue changed since readonly load");
}

namespace detail {

class LoadBudget {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::thread worker;
public:
    std::atomic<u64> peak{0};
    explicit LoadBudget(const Options& options) {
        worker = std::thread([this, options] {
            const auto began = Clock::now();
            std::unique_lock<std::mutex> held(mutex);
            while (!cv.wait_for(held, std::chrono::milliseconds(100),
                                [this] { return done; })) {
                PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
                if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)) ||
                    seconds(began) > options.maxSeconds ||
                    pm.WorkingSetSize > options.maxResidentBytes) {
                    std::fprintf(stderr,
                        "BOUND support catalogue load time/RSS; no values accepted\n");
                    std::fflush(nullptr);
                    std::_Exit(124);
                }
                peak.store(std::max(peak.load(), u64(pm.WorkingSetSize)));
            }
        });
    }
    ~LoadBudget() {
        { std::lock_guard<std::mutex> held(mutex); done = true; }
        cv.notify_all(); worker.join();
    }
    LoadBudget(const LoadBudget&) = delete;
    LoadBudget& operator=(const LoadBudget&) = delete;
};

class Sha256 {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::vector<unsigned char> object;
    void release() noexcept {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        hash = nullptr; algorithm = nullptr;
    }
public:
    Sha256() {
        DWORD size = 0, returned = 0;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, 0) < 0 ||
            BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&size), sizeof(size), &returned, 0) < 0) {
            release(); throw std::runtime_error("cannot initialize SHA-256");
        }
        try { object.resize(size); }
        catch (...) { release(); throw; }
        if (BCryptCreateHash(algorithm, &hash, object.data(), size,
                             nullptr, 0, 0) < 0) {
            release(); throw std::runtime_error("cannot create SHA-256 state");
        }
    }
    ~Sha256() { release(); }
    Sha256(const Sha256&) = delete;
    Sha256& operator=(const Sha256&) = delete;
    void add(const void* data, size_t size) {
        const auto* cursor = static_cast<const unsigned char*>(data);
        constexpr size_t block = 8u << 20;
        while (size) {
            const ULONG take = ULONG(std::min(size, block));
            if (BCryptHashData(hash, const_cast<PUCHAR>(cursor), take, 0) < 0)
                throw std::runtime_error("SHA-256 update failed");
            cursor += take; size -= take;
        }
    }
    std::string finish() {
        unsigned char digest[32];
        if (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0)
            throw std::runtime_error("SHA-256 finalization failed");
        const char* hex = "0123456789ABCDEF";
        std::string out; out.reserve(64);
        for (unsigned char byte : digest) {
            out += hex[byte >> 4]; out += hex[byte & 15];
        }
        return out;
    }
};

inline std::string image_sha256(const CkptImage& image) {
    Sha256 hash;
    hash.add(&image.h, sizeof(image.h));
    hash.add(image.keys.data(), image.keys.size() * sizeof(State));
    hash.add(image.T.data(), image.T.size() * sizeof(u64));
    hash.add(image.stab.data(), image.stab.size() * sizeof(u32));
    hash.add(image.wide.data(), image.wide.size() * sizeof(u128));
    return hash.finish();
}

inline State missing_l4_key(u64& stab) {
    static constexpr u16 slotMasks[12] = {
        294,294,554,554,1161,1161,1360,1360,2181,2181,2640,2640
    };
    State raw{};
    for (int i = 0; i < 12; ++i)
        for (int b = 0; b < 6; ++b) {
            const unsigned pair = (slotMasks[i] >> (2*b)) & 3;
            if (pair == 3) throw std::runtime_error("invalid built-in repair mask");
            if (pair) raw.m[i] |= u16((pair == 1 ? 2 : 3) << (2*b));
        }
    std::sort(raw.m.begin(), raw.m.end());
    return canonize(raw, &stab);
}

} // namespace detail

// Preconditions: engine C/N2C and canonicalization globals are already set.
// All live keys remain at their original IDs, including the source holes.
// The only insertion is the certified C6 missing orbit at source.nEntries.
// No old weight can escape: all original T entries are overwritten before
// the Layer is installed or returned, even for closed small-C sources.
inline Result load(const Options& options) {
    if (C < 2 || C > 6 || N2C != 2*C || g_wlseed || g_forceWide ||
        g_rehearsalDenom || options.path.empty() || !options.checkpointReadonly ||
        options.threads < 1 || options.threads > 32 ||
        options.expectedLayer < 1 || options.expectedLayer > C ||
        !options.maxEntries || options.maxEntries >= UINT32_MAX - 64ULL ||
        !options.maxResidentBytes || options.maxResidentBytes > (55ULL << 30) ||
        !std::isfinite(options.maxSeconds) || options.maxSeconds <= 0 || options.maxSeconds > 600 ||
        (options.repairC6 && (C != 6 || options.expectedLayer != 4)))
        throw std::runtime_error("invalid bounded readonly support-loader options/configuration");
    if (C == 6 && (options.expectedLayer != 4 || !options.repairC6))
        throw std::runtime_error("C6 shared catalogue requires the certified L4 support repair");

    const auto began = Clock::now();
    detail::LoadBudget budget(options);
    Result result;
    result.sourcePath = std::filesystem::absolute(options.path).lexically_normal().string();
    result.sourceBytes = std::filesystem::file_size(result.sourcePath);
    result.sourceModified = std::filesystem::last_write_time(result.sourcePath);
    if (!ck_read_header(result.sourcePath, result.source))
        throw std::runtime_error("support header/configuration hash validation failed");
    const auto& header = result.source;
    if (header.layerIdx != u32(options.expectedLayer) || header.nWide)
        throw std::runtime_error("support catalogue layer/narrow-payload mismatch");
    if (C < 6 && header.parentLayer && header.cursorChunk != header.nChunks)
        throw std::runtime_error("small-C support catalogue transition is not closed");
    if (C == 6 && (header.gen != 37 || header.parentLayer != 3 ||
        header.nEntries != 903398620 || header.holes != 18 ||
        result.sourceBytes != 32522350448ULL))
        throw std::runtime_error("C6 support source is not the pinned generation37 catalogue");
    const u64 capacity = header.nEntries + u64(options.repairC6);
    if (!capacity || capacity > options.maxEntries || capacity >= UINT32_MAX - 64ULL)
        throw std::runtime_error("support catalogue exceeds the explicit capacity bound");
    u64 slots = 64;
    while (slots < 2*capacity) {
        if (slots > (UINT64_MAX >> 1)) throw std::runtime_error("index capacity overflow");
        slots <<= 1;
    }
    result.estimatedResidentBytes = capacity *
        (sizeof(State) + sizeof(u64) + sizeof(u32)) + slots * sizeof(u32);
    if (result.estimatedResidentBytes > options.maxResidentBytes)
        throw std::runtime_error("support catalogue allocation exceeds RSS budget before loading");
    MEMORYSTATUSEX memory{}; memory.dwLength = sizeof(memory);
    if (!GlobalMemoryStatusEx(&memory)) throw std::runtime_error("cannot inspect free RAM");
    const u64 headroom = C == 6 ? (8ULL << 30) : (16ULL << 20);
    const u64 requiredFree = C == 6 ? std::max(65ULL << 30,
        result.estimatedResidentBytes + headroom) : result.estimatedResidentBytes + headroom;
    if (memory.ullAvailPhys < requiredFree)
        throw std::runtime_error("insufficient currently available RAM for support catalogue");

    auto stamp = Clock::now();
    CkptImage image;
    // ck_read_file reserves the requested final capacity first, then reads
    // exact bytes and validates the old native payload INCLUDING old T.
    if (!ck_read_file(result.sourcePath, image, size_t(capacity)) ||
        std::memcmp(&image.h, &result.source, sizeof(CkptHeader)) != 0)
        throw std::runtime_error("support payload validation failed/source changed while reading");
    result.readSeconds = seconds(stamp);
    stamp = Clock::now();
    result.sourceSha256 = detail::image_sha256(image);
    result.sha256Seconds = seconds(stamp);
    result.sourcePayloadHash = header.payloadHash;
    if (C == 6 && result.sourceSha256 != c6SourceSha256)
        throw std::runtime_error("C6 support full SHA-256 does not match the protected source");
    verify_source_unchanged(result);

    stamp = Clock::now();
#pragma omp parallel for schedule(static) num_threads(options.threads)
    for (int64_t i = 0; i < int64_t(image.T.size()); ++i) image.T[size_t(i)] = 0;
    result.zeroSeconds = seconds(stamp);
    // No copy: ck_read_file already reserved capacity; resize only appends
    // the optional one zero-filled witness slot. All historical T is gone.
    Layer& layer = result.layer;
    layer.keys = std::move(image.keys);
    layer.T = std::move(image.T);
    layer.stab = std::move(image.stab);
    layer.keys.resize(size_t(capacity));
    layer.T.resize(size_t(capacity), 0);
    layer.stab.resize(size_t(capacity), 0);
    layer.fixedCap = true;
    layer.claimed = u32(header.nEntries);
    layer.holes = u32(header.holes);

    stamp = Clock::now();
    layer.table.assign(size_t(slots), 0); layer.mask = slots - 1;
    u64 group = 1ULL << C;
    for (int i = 2; i <= C; ++i) group *= unsigned(i);
    u64 duplicates = 0, invalidStabs = 0, countedHoles = 0, exhausted = 0, mass = 0;
#pragma omp parallel for schedule(static) num_threads(options.threads) \
    reduction(+:duplicates,invalidStabs,countedHoles,exhausted,mass)
    for (int64_t pos = 0; pos < int64_t(header.nEntries); ++pos) {
        const u32 stab = layer.stab[size_t(pos)];
        if (!stab) { ++countedHoles; continue; }
        if (group % stab) { ++invalidStabs; continue; }
        mass += group / stab;
        const State& key = layer.keys[size_t(pos)];
        u64 at = state_hash(key) & layer.mask;
        bool placed = false;
        for (u64 tries = 0; tries < slots; ++tries) {
            u32 wanted = 0;
            // Keys are immutable and fully populated before this region;
            // relaxed CAS safely publishes only their already-existing IDs.
            if (__atomic_compare_exchange_n(&layer.table[at], &wanted,
                    u32(pos)+1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                placed = true; break;
            }
            if (layer.keys[wanted-1] == key) { ++duplicates; placed = true; break; }
            at = (at + 1) & layer.mask;
        }
        if (!placed) ++exhausted;
    }
    if (duplicates || invalidStabs || exhausted || countedHoles != header.holes)
        throw std::runtime_error("support index uniqueness/stabilizer/hole/capacity check failed");
    result.indexSeconds = seconds(stamp);
    result.storedMass = mass;
    result.repairedMass = mass;
    stamp = Clock::now();
    if (options.repairC6) {
        if (mass != 41602261532320ULL)
            throw std::runtime_error("C6 source stabilizer mass mismatch");
        u64 stab = 0;
        const State missing = detail::missing_l4_key(stab);
        if (stab != 12 || layer.find(missing) != UINT32_MAX)
            throw std::runtime_error("C6 support repair witness precondition failed");
        const u32 id = u32(header.nEntries);
        layer.keys[id] = missing; layer.stab[id] = u32(stab); layer.T[id] = 0;
        u64 at = state_hash(missing) & layer.mask;
        u64 tries = 0;
        while (layer.table[at] && tries < slots) { at = (at+1)&layer.mask; ++tries; }
        if (tries == slots) throw std::runtime_error("support repair index exhausted");
        layer.table[at] = id + 1; layer.claimed = id + 1;
        result.repairedId = id; result.repairVersion = c6RepairVersion;
        result.repairedMass += group / stab;
        if (layer.real_size() != 903398603 || result.repairedMass != 41602261536160ULL)
            throw std::runtime_error("repaired C6 support count/mass mismatch");
    }
    result.repairSeconds = seconds(stamp);
    if (C == 5 && options.expectedLayer == 4 && layer.real_size() != 17120)
        throw std::runtime_error("shared C5 L4 catalogue must have all17120 native states");
    if (C == 4 && options.expectedLayer == 4 && layer.real_size() != 26)
        throw std::runtime_error("shared C4 L4 catalogue must have all26 native states");
    verify_source_unchanged(result);
    PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)))
        throw std::runtime_error("cannot inspect support-loader RSS");
    result.peakResidentBytes = std::max(budget.peak.load(), u64(pm.WorkingSetSize));
    result.wallSeconds = seconds(began);
    if (result.peakResidentBytes > options.maxResidentBytes || result.wallSeconds > options.maxSeconds)
        throw std::runtime_error("support loader final time/RSS bound exceeded");
    return result;
}

// Explicitly bounded text/sample domain. This never claims that its support
// is globally complete, even if the supplied rows happen to be exhaustive.
// The caller must select this API via an explicit sampletext option and must
// forbid global catalogue/value exports whenever result.domain == "SAMPLE".
// Duplicate coordinate orbits are coalesced, preserving first-appearance IDs.
// Optional expected=F tokens are syntax-checked and deliberately ignored.
inline Result load_text(const Options& options) {
    if (C < 4 || C > 6 || N2C != 2*C || g_wlseed || g_forceWide ||
        g_rehearsalDenom || options.expectedLayer != 4 || options.repairC6 ||
        !options.checkpointReadonly || options.path.empty() ||
        !options.maxEntries || options.maxEntries > 200000 ||
        options.threads < 1 || options.threads > 32 ||
        !options.maxResidentBytes || options.maxResidentBytes > (2ULL << 30) ||
        !std::isfinite(options.maxSeconds) || options.maxSeconds <= 0 || options.maxSeconds > 120)
        throw std::runtime_error("invalid explicit bounded SAMPLE-domain loader options");
    const auto began = Clock::now(); detail::LoadBudget budget(options);
    Result result; result.domain = "SAMPLE";
    result.sourcePath = std::filesystem::absolute(options.path).lexically_normal().string();
    result.sourceBytes = std::filesystem::file_size(result.sourcePath);
    result.sourceModified = std::filesystem::last_write_time(result.sourcePath);
    if (!result.sourceBytes || result.sourceBytes > (64ULL << 20))
        throw std::runtime_error("sample text source must have between1byte and64MiB");
    u64 slots = 64; while (slots < 2*options.maxEntries) slots <<= 1;
    result.estimatedResidentBytes = options.maxEntries *
        (sizeof(State)+sizeof(u64)+sizeof(u32)) + slots*sizeof(u32) + result.sourceBytes*2;
    if (result.estimatedResidentBytes > options.maxResidentBytes)
        throw std::runtime_error("sample allocation exceeds explicit RSS budget");
    auto stamp = Clock::now();
    std::ifstream input(result.sourcePath, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open sample source readonly");
    std::string text(size_t(result.sourceBytes), '\0');
    input.read(text.data(), std::streamsize(text.size()));
    if (input.gcount() != std::streamsize(text.size()) || input.peek() != EOF)
        throw std::runtime_error("sample source changed/truncated during read");
    result.readSeconds = seconds(stamp);
    stamp = Clock::now(); detail::Sha256 sha; sha.add(text.data(), text.size());
    result.sourceSha256 = sha.finish(); result.sha256Seconds = seconds(stamp);
    Layer& layer = result.layer; layer.init_fixed(size_t(options.maxEntries));
    u64 group = 1ULL << C; for (int b = 2; b <= C; ++b) group *= unsigned(b);
    stamp = Clock::now();
    std::istringstream rows(text); std::string line; u64 lineCount = 0;
    while (std::getline(rows, line)) {
        if (line.size() > 8192) throw std::runtime_error("sample source line too long");
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        if (++lineCount > options.maxEntries)
            throw std::runtime_error("sample input row count exceeds explicit limit");
        std::istringstream fields(line); State raw{}; int columnDegrees[12]{};
        for (int i = 0; i < N2C; ++i) {
            u32 slotMask = 0;
            if (!(fields >> slotMask) || (slotMask >> N2C) ||
                __builtin_popcount(slotMask) != 4)
                throw std::runtime_error("sample row has invalid/missing slot mask");
            for (int b = 0; b < C; ++b) {
                const unsigned pair = (slotMask >> (2*b)) & 3;
                if (pair == 3) throw std::runtime_error("sample symbol occupies both box sides");
                if (pair) {
                    raw.m[i] |= u16((pair == 1 ? 2 : 3) << (2*b));
                    ++columnDegrees[2*b + (pair == 2)];
                }
            }
        }
        for (int slot = 0; slot < N2C; ++slot)
            if (columnDegrees[slot] != 4) throw std::runtime_error("unbalanced sample source");
        std::string extra;
        if (fields >> extra) {
            if (extra.compare(0, 9, "expected=") || extra.size() == 9 ||
                extra.find_first_not_of("0123456789", 9) != std::string::npos)
                throw std::runtime_error("unexpected sample row suffix");
            if (fields >> extra) throw std::runtime_error("extra sample row fields");
        }
        sort_masks(raw.m.data(), N2C);
        u64 stab = 0; const State key = canonize(raw, &stab);
        if (!stab || group % stab) throw std::runtime_error("invalid sample stabilizer");
        const u32 old = layer.find(key);
        if (old != UINT32_MAX) {
            if (layer.stab[old] != stab) throw std::runtime_error("sample stabilizer inconsistency");
            continue;
        }
        // Single-threaded canonical sample insertion has no lost-race holes.
        layer.find_or_add_mt(key, u32(stab)); result.storedMass += group/stab;
    }
    if (!layer.real_size()) throw std::runtime_error("empty sample support");
    result.indexSeconds = seconds(stamp); result.repairedMass = result.storedMass;
    // Keep allocations fixed and zero, but synthetic metadata counts actual
    // IDs, not reserved capacity. The magic deliberately differs from CK_MAGIC.
    CkptHeader& header = result.source;
    header.magic = 0x3154504D4153444CULL; // "LDSAMPT1", NOT LDPCAN01.
    header.version = 1; header.cVal = unsigned(C); header.layerIdx = 4;
    header.configHash = ck_hash64(semanticVersion, std::strlen(semanticVersion),
                                 ck_config_hash(4));
    header.nEntries = layer.size();
    u64 payload = CK_SEED;
    payload = ck_hash64(layer.keys.data(), layer.size()*sizeof(State), payload);
    payload = ck_hash64(layer.T.data(), layer.size()*sizeof(u64), payload);
    payload = ck_hash64(layer.stab.data(), layer.size()*sizeof(u32), payload);
    header.payloadHash = payload; result.sourcePayloadHash = payload;
    header.headerHash = ck_hash64(&header, sizeof(header), CK_SEED);
    verify_source_unchanged(result);
    PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)))
        throw std::runtime_error("cannot inspect sample-loader RSS");
    result.peakResidentBytes = std::max(budget.peak.load(), u64(pm.WorkingSetSize));
    result.wallSeconds = seconds(began);
    if (result.peakResidentBytes > options.maxResidentBytes || result.wallSeconds > options.maxSeconds)
        throw std::runtime_error("sample loader final time/RSS bound exceeded");
    return result;
}

} // namespace shared_catalog
#endif
