// All-record support semantics only. No L4 values, F5 computation, or writes.
// This deliberately reuses the released native canonicalizer; it is not an
// independent canonicalization algorithm. Build to a NEW diagnostic path.
#define REVERSE_F5_NO_MAIN
#include "layer_reverse_f5.cpp"
#undef REVERSE_F5_NO_MAIN

namespace support_semantic_audit {
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

// Hold each component itself, root-to-leaf, and refuse reparse objects.
// Directory sharing permits ordinary readers but not rename/delete; the
// final regular file additionally denies writes for the whole audit.
struct ProtectedFile {
    std::vector<HANDLE> held;
    BY_HANDLE_FILE_INFORMATION info{};
    HANDLE file = INVALID_HANDLE_VALUE;
    explicit ProtectedFile(const fs::path& input) {
        const fs::path path=fs::absolute(input).lexically_normal();
        try {
            fs::path current=path.root_path();
            std::vector<fs::path> parts{current};
            for(const auto& part:path.relative_path()){current/=part;parts.push_back(current);}
            for(size_t i=0;i<parts.size();++i) {
                const bool last=i+1==parts.size();
                HANDLE h=CreateFileW(parts[i].c_str(),last?GENERIC_READ:FILE_READ_ATTRIBUTES,
                    last?FILE_SHARE_READ:FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,
                    FILE_FLAG_OPEN_REPARSE_POINT|(last?0:FILE_FLAG_BACKUP_SEMANTICS),nullptr);
                if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("cannot lock readonly source path");
                held.push_back(h);BY_HANDLE_FILE_INFORMATION details{};
                if(!GetFileInformationByHandle(h,&details)||
                    (details.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||
                    bool(details.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==last)
                    throw std::runtime_error("source path must contain only regular directories/file");
                if(last){file=h;info=details;}
            }
        } catch(...) {release();throw;}
    }
    void release(){for(auto it=held.rbegin();it!=held.rend();++it)CloseHandle(*it);held.clear();}
    ~ProtectedFile(){release();}
    ProtectedFile(const ProtectedFile&)=delete;
    u64 bytes()const{return (u64(info.nFileSizeHigh)<<32)|info.nFileSizeLow;}
    std::string sha256() {
        LARGE_INTEGER zero{};
        if(!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN))throw std::runtime_error("readonly hash seek failed");
        shared_catalog::detail::Sha256 hash;std::vector<unsigned char> buffer(8u<<20);
        u64 read=0;DWORD n=0;
        do {
            if(!ReadFile(file,buffer.data(),DWORD(buffer.size()),&n,nullptr))
                throw std::runtime_error("readonly hash read failed");
            hash.add(buffer.data(),n);read+=n;
        }while(n);
        if(read!=bytes())throw std::runtime_error("readonly source size changed");
        return hash.finish();
    }
};

struct Counts {u64 live=0,holes=0,mass=0,calls=0,nodes=0;double wall=0;};

// Exact all-record audit; never transforms keys, IDs, stabs or zero T.
// Index rebuilding checks raw-key duplicates; canonical fixed-point checks
// below then upgrade this to coordinate-orbit uniqueness.
Counts audit_all(Layer& layer,int degree,int threads) {
    const auto began=Clock::now();
    const u64 indexedMass=reverse_f5_run::rebuild_index(layer,threads,false,degree);
    const u64 group=reverse_f5_run::group_order();
    u64 live=0,holes=0,mass=0,calls=0,nodes=0;
    std::atomic<bool> failed{false};std::mutex errorMutex;std::string error;
#pragma omp parallel num_threads(threads) reduction(+:live,holes,mass,calls,nodes)
    {
        const u64 priorCalls=tl_canon_calls,priorNodes=tl_canon_nodes;
#pragma omp for schedule(dynamic,256)
        for(int64_t pos=0;pos<int64_t(layer.size());++pos) {
            if(failed.load(std::memory_order_relaxed))continue;
            const size_t id=size_t(pos);
            try {
                if(layer.T[id])throw std::runtime_error("support T must be erased, including holes");
                const u32 stab=layer.stab[id];
                if(!stab){++holes;continue;}
                reverse_f5_run::audit_native(layer.keys[id],degree,stab);
                if(layer.find(layer.keys[id])!=id)throw std::runtime_error("canonical key does not return its exact stored ID");
                ++live;mass+=group/stab;
            } catch(const std::exception& e) {
                failed.store(true,std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(errorMutex);
                if(error.empty())error="ID "+std::to_string(id)+": "+e.what();
            }
        }
        calls+=tl_canon_calls-priorCalls;nodes+=tl_canon_nodes-priorNodes;
    }
    if(failed.load())throw std::runtime_error("all-record semantic audit rejected "+error);
    if(live+holes!=layer.size()||holes!=layer.holes||live!=layer.real_size()||
       mass!=indexedMass||calls!=live)
        throw std::runtime_error("all-record semantic audit coverage/count mismatch");
    return {live,holes,mass,calls,nodes,shared_catalog::seconds(began)};
}

void small_negatives(Layer& layer,int degree,int threads) {
    if(C!=5||layer.size()>200000)throw std::runtime_error("malformed tests are small C5 only");
    u32 first=UINT32_MAX,second=UINT32_MAX;
    for(u32 i=0;i<layer.size();++i)if(layer.stab[i]){if(first==UINT32_MAX)first=i;else{second=i;break;}}
    if(second==UINT32_MAX)throw std::runtime_error("negative gate needs two live records");
    unsigned refused=0;
    auto reject=[&](const char* label,auto mutate,auto restore) {
        mutate();bool caught=false;
        try{audit_all(layer,degree,threads);}catch(const std::exception&){caught=true;}
        restore();
        if(!caught)throw std::runtime_error(std::string("malformed case accepted: ")+label);
        ++refused;std::cout<<"REFUSED "<<label<<'\n';
    };
    const State original=layer.keys[first],other=layer.keys[second];
    const u32 stab=layer.stab[first];
    reject("wrong_stabilizer",[&]{layer.stab[first]=stab==1?2:1;},[&]{layer.stab[first]=stab;});
    reject("duplicate_key",[&]{layer.keys[first]=other;},[&]{layer.keys[first]=original;});
    reject("nonzero_T",[&]{layer.T[first]=1;},[&]{layer.T[first]=0;});
    reject("invalid_field",[&]{layer.keys[first].m[0]=u16((original.m[0]&~3u)|1u);},[&]{layer.keys[first]=original;});
    reject("nonzero_padding",[&]{layer.keys[first].m[N2C]=1;},[&]{layer.keys[first]=original;});
    State unbalanced=original;bool found=false;
    for(int i=0;i<N2C&&!found;++i)for(int b=0;b<C&&!found;++b)if(fld(unbalanced.m[i],b)) {
        unbalanced.m[i]^=u16(1u<<(2*b));found=true;
    }
    sort_masks(unbalanced.m.data(),N2C);
    reject("unbalanced_slots",[&]{layer.keys[first]=unbalanced;},[&]{layer.keys[first]=original;});
    State moved{};u32 movedID=UINT32_MAX;
    for(u32 id=0;id<layer.size()&&movedID==UINT32_MAX;++id)if(layer.stab[id]) {
        GElem g{};for(int b=0;b<C;++b)g.perm[b]=u8(b);g.flips=1;
        moved=apply_state(g,layer.keys[id]);if(moved!=layer.keys[id])movedID=id;
    }
    if(movedID==UINT32_MAX)throw std::runtime_error("cannot construct noncanonical orbit image");
    const State beforeMoved=layer.keys[movedID];
    reject("valid_noncanonical_key",[&]{layer.keys[movedID]=moved;},[&]{layer.keys[movedID]=beforeMoved;});
    const u32 oldHoles=layer.holes;
    reject("unaccounted_hole",[&]{layer.stab[first]=0;},[&]{layer.stab[first]=stab;});
    layer.stab[first]=0;layer.holes=oldHoles+1;
    const Counts withHole=audit_all(layer,degree,threads);
    if(withHole.live+1!=layer.size()-oldHoles||withHole.holes!=oldHoles+1)
        throw std::runtime_error("accounted insertion hole mishandled");
    reject("nonzero_hole_T",[&]{layer.T[first]=1;},[&]{layer.T[first]=0;});
    layer.stab[first]=stab;layer.holes=oldHoles;
    const Counts restored=audit_all(layer,degree,threads);
    std::cout<<"SMALL_NEGATIVES refused="<<refused<<" restored_live="<<restored.live
             <<" legitimate_hole=PASS original_bytes_restored=yes\n";
}

int run(int argc,char** argv) {
    if(argc<3)throw std::runtime_error("usage: C degree input=PATH sha256=HEX limit=N threads=N maxseconds=S maxrssgib=G checkpointreadonly [backup=PATH] [selftest]");
    C=std::stoi(argv[1]);N2C=2*C;const int degree=std::stoi(argv[2]);
    std::string input,backup,sha;u64 limit=0;int threads=0,rss=0;double seconds=0;bool readonly=false,selftest=false;
    for(int i=3;i<argc;++i) {
        const std::string a=argv[i];if(a=="checkpointreadonly"){readonly=true;continue;}
        if(a=="selftest"){selftest=true;continue;}
        const auto at=a.find('=');if(at==std::string::npos)throw std::runtime_error("expected named option");
        const auto k=a.substr(0,at),v=a.substr(at+1);
        if(k=="input")input=v;else if(k=="backup")backup=v;else if(k=="sha256")sha=v;
        else if(k=="limit")limit=std::stoull(v);else if(k=="threads")threads=std::stoi(v);
        else if(k=="maxseconds")seconds=std::stod(v);else if(k=="maxrssgib")rss=std::stoi(v);
        else throw std::runtime_error("unknown option "+k);
    }
    for(char& ch:sha)ch=char(std::toupper(static_cast<unsigned char>(ch)));
    if(!readonly||input.empty()||sha.size()!=64||!reverse_f5_chunks::sha_shape(sha.data())||
       !limit||threads<1||threads>24||!std::isfinite(seconds)||seconds<=0||seconds>170||rss<1||rss>8||
       !((C==5&&(degree==4||degree==5))||(C==6&&degree==5))||
       (C==5&&(limit>200000||threads>2||rss>2||seconds>110||!backup.empty()))||
       (C==6&&(selftest||threads>4||rss<5||limit!=96452976||backup.empty()||sha!=reverse_f5_run::supportSha256)))
        throw std::runtime_error("require bounded readonly C5 fixture or explicitly pinned C6 L5 support");
    const auto began=Clock::now();reverse_f5_run::Guard guard(seconds,u64(rss)<<30);
    ProtectedFile source(input);std::unique_ptr<ProtectedFile> copy;
    if(C==5&&source.bytes()>(8ULL<<20))throw std::runtime_error("C5 fixture exceeds8MiB");
    if(C==6) {
        copy=std::make_unique<ProtectedFile>(backup);
        if(source.bytes()!=3472307192ULL||copy->bytes()!=source.bytes()||
           source.info.dwVolumeSerialNumber==copy->info.dwVolumeSerialNumber||copy->sha256()!=sha)
            throw std::runtime_error("C6 requires exact SHA-verified separate-volume physical backup");
    }
    g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*u64(i);
    omp_set_dynamic(0);omp_set_num_threads(threads);
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(!GlobalMemoryStatusEx(&memory)||memory.ullAvailPhys<(C==6?(6ULL<<30):(64ULL<<20)))
        throw std::runtime_error("insufficient free RAM for bounded support audit");
    Layer layer;std::string loadedSha;u64 expectedMass=0;const auto loadBegan=Clock::now();
    if(degree==5) {
        reverse_f5_run::Options o;o.support=input;o.threads=threads;o.readonly=true;
        auto loaded=reverse_f5_run::load_support(o);loadedSha=loaded.sha256;expectedMass=loaded.mass;
        if(C==6&&(loaded.layer.size()!=96452976||loaded.layer.holes!=221||loaded.layer.stab[96452974]!=1440||
           loaded.layer.stab[96452975]!=240||loaded.repairHash!=11401178190082244558ULL||g_rehearsalDenom))
            throw std::runtime_error("C6 repaired ID/configuration invariant failed");
        layer=std::move(loaded.layer);
    } else {
        shared_catalog::Options o;o.path=input;o.expectedLayer=degree;o.threads=threads;
        o.checkpointReadonly=true;o.maxEntries=200000;o.maxResidentBytes=u64(rss)<<30;o.maxSeconds=seconds;
        auto loaded=shared_catalog::load(o);loadedSha=loaded.sourceSha256;expectedMass=loaded.storedMass;
        layer=std::move(loaded.layer);
    }
    const double loadSeconds=shared_catalog::seconds(loadBegan);
    if(loadedSha!=sha||layer.size()>limit)throw std::runtime_error("full source SHA or positive record bound mismatch");
    if(selftest)small_negatives(layer,degree,threads);
    const Counts got=audit_all(layer,degree,threads);
    const u64 expectedLive=C==6?96452755ULL:(degree==4?17120ULL:355ULL);
    const u64 exactMass=C==6?4439972139072ULL:(degree==4?62185328ULL:589392ULL);
    if(got.live!=expectedLive||got.mass!=exactMass||got.mass!=expectedMass)
        throw std::runtime_error("complete support count/mass mismatch");
    if(source.sha256()!=sha||(copy&&copy->sha256()!=sha))throw std::runtime_error("final protected-file SHA changed");
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("RSS read failed");
    std::cout<<std::fixed<<std::setprecision(7)<<"SUPPORT_SEMANTIC_AUDIT C="<<C<<" degree="<<degree
        <<" entries="<<layer.size()<<" live="<<got.live<<" holes="<<got.holes<<" mass="<<got.mass
        <<" audit_calls="<<got.calls<<" audit_nodes="<<got.nodes<<" exact_ID_hits="<<got.live
        <<" zero_T_entries="<<layer.size()<<" threads="<<threads<<" load_wall_s="<<loadSeconds
        <<" audit_wall_s="<<got.wall<<" total_wall_s="<<shared_catalog::seconds(began)
        <<" peak_rss_bytes="<<pm.PeakWorkingSetSize<<" source_sha256="<<sha
        <<" native_algorithm=REUSED old_T=ERASED checkpointreadonly=yes file_writes=none F5=NOT_COMPUTED\n"
        <<"ALL RECORD SUPPORT SEMANTICS PASSED\n";
    return 0;
}
} // namespace support_semantic_audit

int main(int argc,char** argv)try{return support_semantic_audit::run(argc,argv);}
catch(const std::exception& e){std::fprintf(stderr,"ERROR: %s\n",e.what());return 1;}
