// Closed, immutable chunks for the shared-F4 experiment. Include after the
// native layer engine. No native production checkpoint is ever overwritten.
#pragma once
#include <filesystem>
#include <cwctype>
#include <iomanip>
#include <sstream>

namespace shared_chunks {
namespace fs = std::filesystem;
constexpr u64 MAGIC = 0x31304b4334524653ULL; // SFR4CK01
constexpr u64 META = 0x3130544d34524653ULL;  // SFR4MT01
constexpr u64 SEMANTICS = 0x31304e494d344653ULL; // SF4MIN01
constexpr u32 NONE = UINT32_MAX;
struct Header {
    u64 magic = MAGIC;
    u32 version = 1, c = 0;
    char sourceSha256[64]{};
    u64 semantics = SEMANTICS, sourceHeaderHash = 0, supportRepairHash = 0;
    u64 totalEntries = 0, chunkSize = 0, begin = 0, count = 0;
    u64 payloadHash = 0, headerHash = 0;
    u64 closedRepresentatives = 0, liveEntries = 0, valueChecksum = 0;
    u64 reserved[10]{};
};
static_assert(sizeof(Header) == 256, "explicit 256-byte little-endian header");

inline std::string hex(u64 value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}
inline void check_header(Header h) {
    const u64 stored = h.headerHash;
    h.headerHash = 0;
    if (h.version != 1 || h.semantics != SEMANTICS ||
        ck_hash64(&h, sizeof(h), CK_SEED) != stored)
        throw std::runtime_error("shared chunk header/version checksum mismatch");
    for (u64 word : h.reserved)
        if (word) throw std::runtime_error("shared chunk reserved field is nonzero");
}
inline void seal(Header& h) {
    h.headerHash = 0;
    h.headerHash = ck_hash64(&h, sizeof(h), CK_SEED);
}
inline void check_lineage(const Header& h, const Header& expected) {
    if (h.c != expected.c || h.semantics != expected.semantics ||
        std::memcmp(h.sourceSha256, expected.sourceSha256, 64) ||
        h.sourceHeaderHash != expected.sourceHeaderHash ||
        h.supportRepairHash != expected.supportRepairHash ||
        h.totalEntries != expected.totalEntries || h.chunkSize != expected.chunkSize)
        throw std::runtime_error("shared chunk belongs to different source/algorithm/chunk geometry");
}

// One process owns a namespace. The OS releases this mutex after a crash;
// there is no stale lock file to remove or override.
struct Lock {
    HANDLE handle = nullptr;
    explicit Lock(const fs::path& dir) {
        std::wstring path = fs::weakly_canonical(fs::absolute(dir)).wstring();
        for (wchar_t& ch : path) ch = wchar_t(std::towlower(ch));
        const u64 id = ck_hash64(path.data(), path.size() * sizeof(wchar_t), CK_SEED);
        const std::string name = "Local\\sudoku-shared-f4-" + hex(id);
        handle = CreateMutexA(nullptr, TRUE, name.c_str());
        if (!handle) throw std::runtime_error("cannot create shared namespace mutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(handle); handle = nullptr;
            throw std::runtime_error("another process owns this shared chunk namespace");
        }
    }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    ~Lock() { if (handle) { ReleaseMutex(handle); CloseHandle(handle); } }
};

inline void write_all(HANDLE file, const void* data, size_t bytes) {
    const char* at = static_cast<const char*>(data);
    while (bytes) {
        const DWORD requested = DWORD(std::min<size_t>(bytes, 1u << 26));
        DWORD written = 0;
        if (!WriteFile(file, at, requested, &written, nullptr) || written != requested)
            throw std::runtime_error("shared chunk WriteFile failed; uncommitted temporary retained");
        at += written; bytes -= written;
    }
}
inline fs::path temporary_for(const fs::path& target) {
    return fs::path(target.wstring() + L".tmp-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
}
inline void commit(const fs::path& target, Header h, const u32* aliases = nullptr,
                   const u64* values = nullptr) {
    if (fs::exists(target)) throw std::runtime_error("refusing to overwrite a committed chunk/manifest");
    seal(h);
    const fs::path temp = temporary_for(target);
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot create exclusive chunk temporary");
    try {
        write_all(file, &h, sizeof(h));
        if (h.count) {
            write_all(file, aliases, size_t(h.count) * sizeof(u32));
            write_all(file, values, size_t(h.count) * sizeof(u64));
        }
        if (!FlushFileBuffers(file)) throw std::runtime_error("chunk flush failed; temporary retained");
        CloseHandle(file); file = INVALID_HANDLE_VALUE;
        // MOVEFILE_REPLACE_EXISTING is deliberately NOT used.
        if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("exclusive chunk commit failed; temporary retained");
    } catch (...) {
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        throw;
    }
}
inline Header read_header(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    Header h{};
    if (!in.read(reinterpret_cast<char*>(&h), sizeof(h)))
        throw std::runtime_error("truncated shared chunk header");
    check_header(h);
    return h;
}
inline void open_namespace(const fs::path& dir, Header expected) {
    expected.magic = META; expected.count = 0; expected.begin = 0;
    const fs::path manifest = dir / "manifest.bin";
    if (!fs::exists(dir)) fs::create_directories(dir);
    if (!fs::exists(manifest)) {
        if (!fs::is_empty(dir))
            throw std::runtime_error("new namespace must be empty; missing manifest is not inferred");
        commit(manifest, expected);
    }
    const Header found = read_header(manifest);
    check_lineage(found, expected);
    if (found.magic != META || found.begin || found.count || found.payloadHash ||
        found.closedRepresentatives || found.liveEntries || found.valueChecksum ||
        fs::file_size(manifest) != sizeof(Header))
        throw std::runtime_error("invalid shared namespace manifest");
}
inline fs::path filename(const fs::path& dir, u64 begin) {
    return dir / ("chunk-" + hex(begin) + ".bin");
}
inline u64 payload_hash(const u32* aliases, const u64* values, u64 count) {
    u64 hash = ck_hash64(aliases, size_t(count) * sizeof(u32), CK_SEED);
    return ck_hash64(values, size_t(count) * sizeof(u64), hash);
}
// Validate the meaning of every record, not merely its byte checksum.
inline void validate_records(const Layer& layer, Header& h, const u32* aliases,
                             const u64* values) {
    if (h.totalEntries != layer.size() || h.begin > layer.size() ||
        h.count > layer.size()-h.begin || (h.count && (!aliases || !values)))
        throw std::runtime_error("shared record range is outside the source catalogue");
    u64 live = 0, representatives = 0, checksum = 0;
    for (u64 j = 0; j < h.count; ++j) {
        const u64 id = h.begin + j;
        const u32 alias = aliases[j]; const u64 value = values[j];
        if (!layer.stab[size_t(id)]) {
            if (alias != NONE || value) throw std::runtime_error("hole has a shared value or alias");
            continue;
        }
        ++live;
        if (alias >= layer.size() || !layer.stab[alias])
            throw std::runtime_error("shared alias does not point to a live catalogue ID");
        if (alias == id) {
            if (!value) throw std::runtime_error("representative has no closed exact F4 value");
            ++representatives; checksum += value; // documented modulo-2^64 diagnostic
        } else if (value) throw std::runtime_error("nonrepresentative stores an unexpected F4 value");
    }
    h.liveEntries = live; h.closedRepresentatives = representatives; h.valueChecksum = checksum;
}
inline Header read_chunk(const fs::path& path, const Header& expected, const Layer& layer,
                         u64 begin, std::vector<u32>& aliases, std::vector<u64>& values) {
    if (!expected.chunkSize || begin >= expected.totalEntries ||
        expected.totalEntries != layer.size() || begin % expected.chunkSize)
        throw std::runtime_error("invalid requested shared chunk range");
    Header h = read_header(path);
    check_lineage(h, expected);
    const u64 count = std::min(expected.chunkSize, expected.totalEntries - begin);
    if (h.magic != MAGIC || h.begin != begin || h.count != count ||
        fs::file_size(path) != sizeof(Header) + count * 12)
        throw std::runtime_error("shared chunk range/length mismatch");
    aliases.resize(size_t(count)); values.resize(size_t(count));
    std::ifstream in(path, std::ios::binary);
    in.seekg(sizeof(Header));
    if (!in.read(reinterpret_cast<char*>(aliases.data()), std::streamsize(count * 4)) ||
        !in.read(reinterpret_cast<char*>(values.data()), std::streamsize(count * 8)))
        throw std::runtime_error("truncated shared chunk payload");
    if (payload_hash(aliases.data(), values.data(), count) != h.payloadHash)
        throw std::runtime_error("shared chunk payload checksum mismatch");
    Header measured = h;
    validate_records(layer, measured, aliases.data(), values.data());
    if (measured.liveEntries != h.liveEntries ||
        measured.closedRepresentatives != h.closedRepresentatives || measured.valueChecksum != h.valueChecksum)
        throw std::runtime_error("shared chunk diagnostic totals mismatch");
    return h;
}
} // namespace shared_chunks
