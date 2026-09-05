// Shared exact F4 values, with retained native responses and immutable chunks.
// Active production inputs are opened read-only; old partial T is discarded.
#define main layer_shared_native_original_main
#include "layer_dp_gate.cpp"
#undef main
#include "layer_shared_catalog.h"
#include "layer_shared_f4_core.h"
#include "layer_shared_chunks.h"

#include <atomic>
#include <condition_variable>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>

namespace shared_run {
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}
std::atomic<bool> stopRequested{false};
BOOL WINAPI console_stop(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT) {
        stopRequested.store(true); return TRUE;
    }
    return FALSE;
}
struct Options {
    std::string catalog, output, exportPath, verify, sampleText;
    u64 limit = 0, chunkSize = 0;
    int threads = 0, maxRssGiB = 0;
    double maxSeconds = 0, workSeconds = 0;
    bool readonly = false, repair = false;
};
struct Guard {
    std::mutex mutex;
    std::condition_variable condition;
    bool done = false;
    std::atomic<u64> peak{0};
    std::thread thread;
    Guard(double bound, u64 rss) : thread([this, bound, rss] {
        const auto start = Clock::now();
        std::unique_lock<std::mutex> lock(mutex);
        while (!condition.wait_for(lock, std::chrono::milliseconds(100), [this]{return done;})) {
            PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
            if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)) ||
                pm.WorkingSetSize > rss || seconds(start) > bound) {
                std::fprintf(stderr, "BOUND time/RSS; only earlier committed chunks are accepted\n");
                std::fflush(nullptr); std::_Exit(124);
            }
            peak.store(std::max(peak.load(), u64(pm.PeakWorkingSetSize)));
        }
    }) {}
    ~Guard() {
        { std::lock_guard<std::mutex> lock(mutex); done = true; }
        condition.notify_one(); thread.join();
    }
};
Options parse(int argc, char** argv) {
    if (argc < 2) throw std::runtime_error("usage: C catalog=PATH output=NEW_NAMESPACE chunk=... limit=... threads=... workseconds=... maxseconds=... maxrssgib=... checkpointreadonly [repair-l4-support] [verify=TEXT] [export=NEW_PATH]");
    C = std::stoi(argv[1]); N2C = 2*C;
    Options o;
    for (int i=2; i<argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "checkpointreadonly") { o.readonly = true; continue; }
        if (arg == "repair-l4-support") { o.repair = true; continue; }
        const auto at = arg.find('=');
        if (at == std::string::npos) throw std::runtime_error("expected option=value");
        const std::string k = arg.substr(0,at), v = arg.substr(at+1);
        if (k=="catalog") o.catalog=v;
        else if (k=="sampletext") o.sampleText=v;
        else if (k=="output") o.output=v;
        else if (k=="export") o.exportPath=v;
        else if (k=="verify") o.verify=v;
        else if (k=="limit") o.limit=std::stoull(v);
        else if (k=="chunk") o.chunkSize=std::stoull(v);
        else if (k=="threads") o.threads=std::stoi(v);
        else if (k=="maxrssgib") o.maxRssGiB=std::stoi(v);
        else if (k=="maxseconds") o.maxSeconds=std::stod(v);
        else if (k=="workseconds") o.workSeconds=std::stod(v);
        else throw std::runtime_error("unknown option " + k);
    }
    if (C<4 || C>6 || !o.readonly || o.output.empty() ||
        (o.catalog.empty() == o.sampleText.empty()) || !o.limit ||
        !o.chunkSize || o.chunkSize>1000000 || o.threads<1 || o.threads>24 ||
        o.maxRssGiB<1 || o.maxRssGiB>55 || !std::isfinite(o.maxSeconds) ||
        !std::isfinite(o.workSeconds) || o.workSeconds<=0 ||
        o.maxSeconds<o.workSeconds+5 || o.maxSeconds>28800 ||
        (o.repair && (C!=6 || !o.sampleText.empty())) ||
        (C==6 && o.sampleText.empty() && !o.repair) ||
        (!o.sampleText.empty() && !o.exportPath.empty()))
        throw std::runtime_error("require C4..6, readonly input, one catalogue source, explicit positive work/time/RSS/record bounds; sample export is forbidden");
    return o;
}

std::wstring normalized_path(const fs::path& path) {
    std::wstring result=fs::weakly_canonical(fs::absolute(path)).wstring();
    for (wchar_t& ch:result) ch=wchar_t(std::towlower(ch));
    return result;
}
void check_paths(const Options& o) {
    const fs::path dir=fs::weakly_canonical(fs::absolute(o.output));
    if (dir==dir.root_path() || normalized_path(dir)==normalized_path(fs::current_path()))
        throw std::runtime_error("shared namespace must be a dedicated subdirectory");
    const auto parent=normalized_path(dir);
    const auto source=normalized_path(o.sampleText.empty()?o.catalog:o.sampleText);
    if (source==parent || source.starts_with(parent+L"\\") || source.starts_with(parent+L"/"))
        throw std::runtime_error("readonly source cannot reside in its writable chunk namespace");
    if (!o.exportPath.empty() && normalized_path(o.exportPath)==source)
        throw std::runtime_error("export cannot replace the readonly source");
}

void verify_values(const std::string& path, const Layer& layer, const std::vector<u32>& aliases) {
    if (path.empty()) return;
    if (layer.real_size()>200000) throw std::runtime_error("text exact verification is limited to small disposable catalogues");
    std::ifstream in(path);
    if (!in) throw std::runtime_error("verification text unavailable");
    pairing_fiber::Input row;
    std::vector<bool> checked(layer.size(), false);
    u64 count=0;
    while (pairing_fiber::nextInput(in,4,row)) {
        if (!row.hasExpected || row.expected>UINT64_MAX) throw std::runtime_error("exact expected=F4 is required");
        const State key=canonize(pairing_fiber::encodeOriginal(row.graph));
        const u32 id=layer.find(key);
        if (id==UINT32_MAX || checked[id] || aliases[id]>=layer.size() ||
            layer.T[aliases[id]]!=u64(row.expected))
            throw std::runtime_error("shared F4 differs from complete independent reference");
        checked[id]=true; ++count;
    }
    if (count!=layer.real_size()) throw std::runtime_error("reference did not check every native value");
    std::cout << "exact_verified_native_values=" << count << '\n';
}

// Export only after EVERY alias and representative is closed. Use a unique
// private intermediate name, then an atomic rename without replacement.
void export_layer(const Options& o, Layer& layer, std::vector<u32>& aliases) {
    if (o.exportPath.empty()) return;
    const fs::path target=fs::absolute(o.exportPath);
    if (fs::exists(target)) throw std::runtime_error("refusing to overwrite exported native snapshot");
    const fs::path parent=target.parent_path();
    if (!fs::is_directory(parent)) throw std::runtime_error("export parent must already exist");
    const u64 bytes=128+u64(layer.size())*36;
    if (fs::space(parent).available < bytes+(1ULL<<30)) throw std::runtime_error("insufficient export disk headroom");
    std::vector<u32>().swap(layer.table); layer.mask=0;
    std::vector<u64> weighted(layer.size(),0);
    u64 overflows=0;
#pragma omp parallel for schedule(static) reduction(+:overflows)
    for (int64_t i=0; i<int64_t(layer.size()); ++i) {
        if (!layer.stab[size_t(i)]) continue;
        const u64 group=(1ULL<<C)*FACT[C];
        const u128 value=u128(group/layer.stab[size_t(i)])*layer.T[aliases[size_t(i)]];
        if (value>UINT64_MAX) ++overflows;
        else weighted[size_t(i)]=u64(value);
    }
    if (overflows) throw std::runtime_error("native T4 overflows u64; export refused");
    layer.T=std::move(weighted); std::vector<u32>().swap(aliases);
    const fs::path privatePath=shared_chunks::temporary_for(target);
    if (fs::exists(privatePath) || fs::exists(fs::path(privatePath.string()+".tmp")))
        throw std::runtime_error("private export path collision");
    if (!layer_save_file(privatePath.string(),layer,4,nullptr))
        throw std::runtime_error("closed native export failed");
    if (!MoveFileExW(privatePath.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("exclusive native export commit failed; private file retained");
    std::cout << "EXPORTED_CLOSED_NATIVE_L4 " << target.string()
              << " independent_readback_required=yes\n";
}

int run(int argc,char** argv) {
    const Options o=parse(argc,argv);
    check_paths(o);
    const auto started=Clock::now();
    shared_chunks::Lock lock(o.output);
    Guard guard(o.maxSeconds,u64(o.maxRssGiB)<<30);
    SetConsoleCtrlHandler(console_stop,TRUE);
    g_wlseed=false; g_forceWide=false; g_rehearsalDenom=0;
    FACT[0]=1; for (int i=1;i<=12;++i) FACT[i]=FACT[i-1]*u64(i);
    omp_set_num_threads(o.threads);
    shared_f4_bridge::initialize(C);
    shared_catalog::Options loading;
    loading.path=o.sampleText.empty()?o.catalog:o.sampleText;
    loading.threads=o.threads; loading.repairC6=o.repair;
    loading.checkpointReadonly=true; loading.expectedLayer=4;
    loading.maxEntries=(C==6 && o.sampleText.empty())?903398621ULL:200000ULL;
    loading.maxResidentBytes=std::min(u64(o.maxRssGiB)<<30,
        o.sampleText.empty()?(55ULL<<30):(2ULL<<30));
    loading.maxSeconds=std::min(o.maxSeconds,o.sampleText.empty()?600.0:120.0);
    auto catalog = o.sampleText.empty() ? shared_catalog::load(loading)
                                      : shared_catalog::load_text(loading);
    Layer& layer=catalog.layer;
    const u64 n=layer.size();
    std::cout << std::fixed << std::setprecision(6)
              << "CATALOGUE read_payload_hash_s=" << catalog.readSeconds
              << " sha256_s=" << catalog.sha256Seconds << " clear_old_weights_s=" << catalog.zeroSeconds
              << " index_s=" << catalog.indexSeconds << " repair_s=" << catalog.repairSeconds
              << " wall_s=" << catalog.wallSeconds << " peak_rss_bytes=" << catalog.peakResidentBytes << '\n' << std::flush;
    if (!n || std::min(o.chunkSize,n)>o.limit) throw std::runtime_error("limit must permit at least one whole chunk");
    shared_chunks::Header lineage;
    lineage.c=u32(C); lineage.sourceHeaderHash=catalog.source.headerHash;
    if (o.repair) {
        const u32 id=u32(catalog.repairedId);
        if (id>=layer.size()) throw std::runtime_error("missing explicit support-repair provenance");
        u64 hash=ck_hash64(catalog.repairVersion.data(),catalog.repairVersion.size(),CK_SEED);
        hash=ck_hash64(&layer.keys[id],sizeof(State),hash);
        lineage.supportRepairHash=ck_hash64(&layer.stab[id],sizeof(u32),hash);
    }
    lineage.totalEntries=n; lineage.chunkSize=o.chunkSize;
    if (catalog.sourceSha256.size()!=64) throw std::runtime_error("missing exact source SHA-256");
    std::memcpy(lineage.sourceSha256,catalog.sourceSha256.data(),64);
    shared_chunks::open_namespace(o.output,lineage);
    std::cout << "DOMAIN=" << (o.sampleText.empty()?"COMPLETE_NATIVE_L4":"SAMPLE_FIBER_CATALOGUE")
              << " entries=" << n << " live=" << layer.real_size()
              << " source_sha256=" << catalog.sourceSha256
              << " source_T=discarded source_mode=checkpointreadonly threads=" << o.threads << '\n' << std::flush;

    std::vector<u32> aliases(size_t(n),shared_chunks::NONE), localAliases;
    std::vector<u64> localValues;
    u64 prefix=0,totalReps=0,totalLive=0,totalChecksum=0,resumedChunks=0;
    bool gap=false;
    for (u64 begin=0;begin<n;begin+=o.chunkSize) {
        const auto path=shared_chunks::filename(o.output,begin);
        if (!fs::exists(path)) {gap=true;continue;}
        if (gap) throw std::runtime_error("noncontiguous committed chunks; namespace requires audit");
        const auto h=shared_chunks::read_chunk(path,lineage,layer,begin,localAliases,localValues);
        std::copy(localAliases.begin(),localAliases.end(),aliases.begin()+size_t(begin));
        std::copy(localValues.begin(),localValues.end(),layer.T.begin()+size_t(begin));
        prefix=begin+h.count; totalReps+=h.closedRepresentatives;totalLive+=h.liveEntries;
        totalChecksum+=h.valueChecksum;++resumedChunks;
    }
    std::cout << "RESUMED chunks=" << resumedChunks << " closed_prefix=" << prefix
              << " closed_representatives=" << totalReps << '\n' << std::flush;
    const auto computing=Clock::now();
    u64 newIndices=0,newChunks=0;
    while (prefix<n && !stopRequested.load() && seconds(started)<o.workSeconds) {
        const u64 count=std::min(o.chunkSize,n-prefix);
        if (count>o.limit-newIndices) break;
        localAliases.assign(size_t(count),shared_chunks::NONE);
        localValues.assign(size_t(count),0);
        std::atomic<bool> failed{false};std::mutex errorMutex;std::string error;
        u64 pairings=0,leaves=0,nodes=0;
        double fiberSeconds=0,kernelSeconds=0,lookupSeconds=0,auditSeconds=0;
        const auto chunkStarted=Clock::now();
#pragma omp parallel reduction(+:pairings,leaves,nodes,fiberSeconds,kernelSeconds,lookupSeconds,auditSeconds)
        {
            shared_f4::Worker worker;
#pragma omp for schedule(dynamic,16)
            for (int64_t j=0;j<int64_t(count);++j) {
                if (failed.load(std::memory_order_relaxed) || !layer.stab[size_t(prefix+u64(j))]) continue;
                try {
                    const auto result=worker.process(layer,u32(prefix+u64(j)));
                    localAliases[size_t(j)]=result.alias;localValues[size_t(j)]=result.F4;
                    pairings+=result.diagnostics.pairings;
                    leaves+=result.diagnostics.f4Leaves;nodes+=result.diagnostics.f4Nodes;
                    fiberSeconds+=result.diagnostics.fiberSeconds;
                    auditSeconds+=result.diagnostics.sourceAuditSeconds;
                    kernelSeconds+=result.diagnostics.f4Seconds;
                    lookupSeconds+=result.diagnostics.lookupSeconds;
                } catch (const std::exception& e) {
                    failed.store(true,std::memory_order_relaxed);
                    std::lock_guard<std::mutex> held(errorMutex); if(error.empty())error=e.what();
                }
            }
        }
        if (failed.load()) throw std::runtime_error("uncommitted chunk rejected: "+error);
        shared_chunks::Header h=lineage;h.begin=prefix;h.count=count;
        shared_chunks::validate_records(layer,h,localAliases.data(),localValues.data());
        h.payloadHash=shared_chunks::payload_hash(localAliases.data(),localValues.data(),count);
        shared_chunks::commit(shared_chunks::filename(o.output,prefix),h,localAliases.data(),localValues.data());
        std::copy(localAliases.begin(),localAliases.end(),aliases.begin()+size_t(prefix));
        std::copy(localValues.begin(),localValues.end(),layer.T.begin()+size_t(prefix));
        prefix+=count;newIndices+=count;++newChunks;
        totalReps+=h.closedRepresentatives;totalLive+=h.liveEntries;totalChecksum+=h.valueChecksum;
        std::cout << std::fixed << std::setprecision(6)
                  << "CLOSED_CHUNK begin=" << h.begin << " count=" << count
                  << " live=" << h.liveEntries << " representatives=" << h.closedRepresentatives
                  << " pairings=" << pairings << " F4leaves=" << leaves << " F4nodes=" << nodes
                  << " source_audit_cpu_s=" << auditSeconds
                  << " fiber_cpu_s=" << fiberSeconds << " kernel_cpu_s=" << kernelSeconds
                  << " lookup_cpu_s=" << lookupSeconds << " wall_s=" << seconds(chunkStarted)
                  << " closed_prefix=" << prefix << '/' << n << '\n' << std::flush;
    }
    shared_catalog::verify_source_unchanged(catalog);
    if (prefix==n) {
        u64 invalid=0;
#pragma omp parallel for schedule(static) reduction(+:invalid)
        for (int64_t i=0;i<int64_t(n);++i) {
            if (!layer.stab[size_t(i)]) continue;
            const u32 alias=aliases[size_t(i)];
            if (alias>=n || aliases[alias]!=alias || !layer.T[alias]) ++invalid;
        }
        if (invalid || totalLive!=layer.real_size()) throw std::runtime_error("complete alias/closed-value closure failed");
        verify_values(o.verify,layer,aliases);
        export_layer(o,layer,aliases);
    }
    PROCESS_MEMORY_COUNTERS finalMemory{};finalMemory.cb=sizeof(finalMemory);
    if (!GetProcessMemoryInfo(GetCurrentProcess(),&finalMemory,sizeof(finalMemory)))
        throw std::runtime_error("cannot inspect final RSS");
    const u64 measuredPeak=std::max(guard.peak.load(),u64(finalMemory.PeakWorkingSetSize));
    std::cout << std::fixed << std::setprecision(6)
              << "SUMMARY status=" << (prefix==n?"CLOSED_F4_CATALOGUE":"INCOMPLETE_RESUMABLE")
              << " domain=" << (o.sampleText.empty()?"complete_native_L4":"sample_only")
              << " new_chunks=" << newChunks << " new_indices=" << newIndices
              << " closed_prefix=" << prefix << " total_entries=" << n
              << " closed_representatives=" << totalReps << " F4_checksum_mod2_64=" << totalChecksum
              << " compute_wall_s=" << seconds(computing) << " wall_s=" << seconds(started)
              << " peak_rss_bytes=" << measuredPeak << " N6=NOT_COMPUTED\n";
    return 0;
}
} // namespace shared_run
int main(int argc,char** argv) try { return shared_run::run(argc,argv); }
catch(const std::exception& e) {std::fprintf(stderr,"ERROR: %s\n",e.what());return 1;}
