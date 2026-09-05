// Immutable CLOSED F5 values, not F4 aliases and not partial native weights.
// Include after the native engine and layer_shared_chunks.h (atomic I/O only).
#ifndef FJ_LAYER_REVERSE_F5_CHUNKS_H
#define FJ_LAYER_REVERSE_F5_CHUNKS_H

namespace reverse_f5_chunks {
namespace fs = std::filesystem;
inline constexpr u64 MAGIC = 0x31304b4335465652ULL; // RVF5CK01
inline constexpr u64 META = 0x3130544d35465652ULL;  // RVF5MT01
inline constexpr u64 SEMANTICS = 0x313044544f4f5235ULL; // 5ROOTD01
struct Header {
    u64 magic = MAGIC;
    u32 version = 1, c = 0;
    char l4Sha256[64]{}, supportSha256[64]{};
    u64 semantics = SEMANTICS, l4HeaderHash = 0, supportHeaderHash = 0;
    u64 supportRepairHash = 0;
    u64 totalEntries = 0, chunkSize = 0, begin = 0, count = 0;
    u64 payloadHash = 0, headerHash = 0, liveEntries = 0, valueChecksum = 0;
    u64 reserved[2]{};
};
static_assert(sizeof(Header) == 256, "F5 chunk has an explicit256byte header");

inline bool sha_shape(const char* value) {
    for (unsigned i=0; i<64; ++i)
        if (!((value[i]>='0' && value[i]<='9') || (value[i]>='A' && value[i]<='F')))
            return false;
    return true;
}
inline void seal(Header& header) {
    header.headerHash = 0;
    header.headerHash = ck_hash64(&header, sizeof(header), CK_SEED);
}
inline void check_header(Header header) {
    const u64 wanted = header.headerHash; header.headerHash = 0;
    if ((header.magic != MAGIC && header.magic != META) || header.version != 1 ||
        header.semantics != SEMANTICS || (header.c != 5 && header.c != 6) ||
        !sha_shape(header.l4Sha256) || !sha_shape(header.supportSha256) ||
        !header.totalEntries || header.totalEntries >= UINT32_MAX-64ULL ||
        !header.chunkSize || header.chunkSize > 1000000 ||
        header.begin > header.totalEntries || header.count > header.totalEntries-header.begin ||
        ck_hash64(&header, sizeof(header), CK_SEED) != wanted)
        throw std::runtime_error("reverse F5 chunk header/domain/range checksum mismatch");
    for (u64 value : header.reserved)
        if (value) throw std::runtime_error("reverse F5 chunk reserved field is nonzero");
}
inline void check_lineage(const Header& found, const Header& expected) {
    if (found.c != expected.c || found.semantics != expected.semantics ||
        std::memcmp(found.l4Sha256, expected.l4Sha256, 64) ||
        std::memcmp(found.supportSha256, expected.supportSha256, 64) ||
        found.l4HeaderHash != expected.l4HeaderHash ||
        found.supportHeaderHash != expected.supportHeaderHash ||
        found.supportRepairHash != expected.supportRepairHash ||
        found.totalEntries != expected.totalEntries || found.chunkSize != expected.chunkSize)
        throw std::runtime_error("reverse F5 chunk belongs to different inputs/repair/semantics/geometry");
}
inline void validate_values(const Layer& support, Header& header, const u64* values) {
    if (header.totalEntries != support.size() || header.begin > support.size() ||
        header.count > support.size()-header.begin || (header.count && !values))
        throw std::runtime_error("reverse F5 value range is outside the support catalogue");
    u64 live = 0, checksum = 0;
    for (u64 j=0; j<header.count; ++j) {
        const bool present = support.stab[size_t(header.begin+j)] != 0;
        if (present != (values[j] != 0))
            throw std::runtime_error("reverse F5 requires positive CLOSED live values and zero holes");
        if (present && values[j]%120)
            throw std::runtime_error("closed F5 must be divisible by all five color permutations (120)");
        if (present) { ++live; checksum += values[j]; }
    }
    header.liveEntries = live; header.valueChecksum = checksum; // Mod2^64 diagnostic.
}
inline fs::path filename(const fs::path& directory, u64 begin) {
    return directory / ("f5-chunk-" + shared_chunks::hex(begin) + ".bin");
}
inline void commit(const fs::path& target, Header header, const u64* values = nullptr) {
    if (header.count && !values) throw std::runtime_error("missing closed F5 commit payload");
    if (fs::exists(target)) throw std::runtime_error("refusing to overwrite an immutable F5 chunk/manifest");
    seal(header); check_header(header);
    const fs::path temporary = shared_chunks::temporary_for(target);
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot create exclusive F5 temporary");
    try {
        shared_chunks::write_all(file, &header, sizeof(header));
        if (header.count) shared_chunks::write_all(file, values, size_t(header.count)*sizeof(u64));
        if (!FlushFileBuffers(file)) throw std::runtime_error("closed F5 flush failed; temporary retained");
        CloseHandle(file); file = INVALID_HANDLE_VALUE;
        if (!MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("nonreplacing F5 commit failed; temporary retained");
    } catch (...) {
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        throw;
    }
}
inline Header read_header(const fs::path& path) {
    std::ifstream input(path, std::ios::binary); Header header{};
    if (!input.read(reinterpret_cast<char*>(&header), sizeof(header)))
        throw std::runtime_error("truncated reverse F5 header");
    check_header(header); return header;
}
inline void open_namespace(const fs::path& directory, Header expected) {
    expected.magic = META; expected.begin = expected.count = 0;
    expected.payloadHash = expected.liveEntries = expected.valueChecksum = 0;
    const auto manifest = directory / "manifest.bin";
    if (!fs::exists(directory)) fs::create_directories(directory);
    if (!fs::exists(manifest)) {
        if (!fs::is_empty(directory))
            throw std::runtime_error("new F5 namespace must be empty; missing manifest is not inferred");
        commit(manifest, expected);
    }
    const auto found = read_header(manifest); check_lineage(found, expected);
    if (found.magic != META || found.begin || found.count || found.payloadHash ||
        found.liveEntries || found.valueChecksum || fs::file_size(manifest) != sizeof(Header))
        throw std::runtime_error("invalid reverse F5 namespace manifest");
}
inline Header read_chunk(const fs::path& path, const Header& expected,
                         const Layer& support, u64 begin, std::vector<u64>& values) {
    if (!expected.chunkSize || begin >= expected.totalEntries || begin%expected.chunkSize ||
        expected.totalEntries != support.size())
        throw std::runtime_error("invalid requested reverse F5 chunk range");
    const Header header = read_header(path); check_lineage(header, expected);
    const u64 count = std::min(expected.chunkSize, expected.totalEntries-begin);
    if (header.magic != MAGIC || header.begin != begin || header.count != count ||
        fs::file_size(path) != sizeof(Header)+count*sizeof(u64))
        throw std::runtime_error("reverse F5 chunk range/length mismatch");
    values.resize(size_t(count)); std::ifstream input(path, std::ios::binary);
    input.seekg(sizeof(Header));
    if (!input.read(reinterpret_cast<char*>(values.data()), std::streamsize(count*sizeof(u64))))
        throw std::runtime_error("truncated reverse F5 payload");
    if (ck_hash64(values.data(), size_t(count)*sizeof(u64), CK_SEED) != header.payloadHash)
        throw std::runtime_error("reverse F5 payload checksum mismatch");
    Header measured = header; validate_values(support, measured, values.data());
    if (measured.liveEntries != header.liveEntries || measured.valueChecksum != header.valueChecksum)
        throw std::runtime_error("reverse F5 chunk diagnostic totals mismatch");
    return header;
}
} // namespace reverse_f5_chunks
#endif
