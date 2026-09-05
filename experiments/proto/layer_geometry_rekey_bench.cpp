// Isolated geometry-first RAM-key experiment. No production/checkpoint writes.
// Full C6 catalogue input is deliberately NOT supported by this small gate.
#define NATIVE_GATHER_NO_MAIN
#include "layer_native_gather_bench.cpp"
#define main geometry_probe_embedded_main
#include "layer_missing_geometry_canon_probe.cpp"
#undef main
#include "layer_two_missing_prefix_canon.h"
#include "layer_shared_catalog.h"

namespace geometry_rekey {
using Clock = native_gather::Clock;
using missing_geometry_probe::Inventory;
using missing_geometry_probe::Canon;
struct Query { State raw; u32 id=UINT32_MAX,target=0; u64 multiplicity=0; };
static_assert(sizeof(Query)==40);
struct Target { u128 expected=0; u64 stab=0; bool closed=false; };
struct Options {
    int c=0;
    std::string input,catalogue,calibration;
    u64 limit=0,maxQueries=0;
    double maxSeconds=0;
};

// A single index, rebuilt over pre-existing IDs, with bounded probe length.
u64 rebuild(Layer& layer,missing_geometry_probe::Budget& budget) {
    if(!layer.table.empty())throw std::runtime_error("old index must be freed first");
    size_t slots=64;while(slots<2*layer.size())slots*=2;
    layer.table.assign(slots,0);layer.mask=slots-1;
    u64 live=0;
    for(u32 id=0;id<layer.size();++id) {
        if(!layer.stab[id])continue;
        u64 at=state_hash(layer.keys[id])&layer.mask,probes=0;
        while(layer.table[at]) {
            if(++probes>layer.mask)throw std::runtime_error("index probe exhaustion");
            if(layer.keys[layer.table[at]-1]==layer.keys[id])
                throw std::runtime_error("duplicate representative during rebuild");
            at=(at+1)&layer.mask;
        }
        layer.table[at]=id+1;++live;
        if(id%8192==0)budget.check();
    }
    if(live!=layer.real_size())throw std::runtime_error("index live count mismatch");
    return live;
}

std::string payload_identity(const Layer& layer) {
    shared_catalog::detail::Sha256 hash;
    const u64 words[]={layer.size(),layer.real_size(),layer.holes};
    hash.add(words,sizeof(words));
    hash.add(layer.T.data(),layer.size()*sizeof(u64));
    hash.add(layer.stab.data(),layer.size()*sizeof(u32));
    return hash.finish();
}

Layer small_catalogue(const Options& o,missing_geometry_probe::Budget& budget,
                      std::string& sha) {
    CkptHeader header{};
    if(C!=5||!ck_read_header(o.catalogue,header)||header.layerIdx!=3||
       header.nEntries>20000||header.nEntries-header.holes!=16150||header.nWide||
       (header.parentLayer&&header.cursorChunk!=header.nChunks))
        throw std::runtime_error("requires COMPLETE production C5 L3 checkpoint <=20000 IDs");
    CkptImage im;
    if(!ck_read_file(o.catalogue,im))throw std::runtime_error("source payload verification failed");
    sha=shared_catalog::detail::image_sha256(im);
    Layer layer;layer.keys=std::move(im.keys);layer.T=std::move(im.T);
    layer.stab=std::move(im.stab);layer.fixedCap=true;
    layer.claimed=u32(header.nEntries);layer.holes=u32(header.holes);
    const u64 group=FACT[C]*(1ULL<<C);u64 mass=0,holes=0;
    two_missing_prefix::Canon validator;
    for(u32 id=0;id<layer.size();++id) {
        const u64 stored=layer.stab[id];
        if(!stored){++holes;if(layer.T[id])throw std::runtime_error("nonzero hole value");continue;}
        if(!validator.prepare(layer.keys[id]))
            throw std::runtime_error("source key not a balanced two-missing state");
        u64 aut=0;
        if(!(canonize(layer.keys[id],&aut)==layer.keys[id])||aut!=stored||group%aut)
            throw std::runtime_error("native source key/stabilizer audit failed");
        const u64 orbit=group/aut;mass+=orbit;
        if(!layer.T[id]||layer.T[id]%orbit)
            throw std::runtime_error("closed weighted source normalization failed");
        layer.T[id]/=orbit;
        if(layer.T[id]%FACT[C-2])throw std::runtime_error("source color factorial failed");
        if(id%1024==0)budget.check();
    }
    if(holes!=header.holes||mass!=59661280)
        throw std::runtime_error("source support/histogram mass failed");
    rebuild(layer,budget);
    std::cout<<"SOURCE domain=COMPLETE_C5_L3 SHA256="<<sha<<" ids="<<layer.size()
             <<" live="<<layer.real_size()<<" mass="<<mass<<" closed_F3=YES\n";
    return layer;
}

u64 queries(const Options& o,std::vector<Query>& out,std::vector<Target>& targets,
            missing_geometry_probe::Budget& budget) {
    std::ifstream in(o.input);if(!in)throw std::runtime_error("input unavailable");
    native_gather::Input row;u64 records=0;
    std::unordered_set<State,native_gather::Hash> targetKeys;
    while(targets.size()<o.limit&&native_gather::next(in,C-1,row)) {
        Target target;target.closed=row.expected;target.expected=row.value;
        if(C==5&&!target.closed)throw std::runtime_error("complete C5 targets need expected F4");
        if(C==6&&target.closed)throw std::runtime_error("C6 sample must not claim reference F5");
        const State raw=native_gather::encode(row.graph);
        const State targetKey=canonize(raw,&target.stab);
        if(C==5&&!targetKeys.insert(targetKey).second)
            throw std::runtime_error("duplicate native target in complete C5 gate");
        const auto pivot=native_gather::pivot(row.graph);
        native_gather::Batch batch{row.graph,{}, {},0,100000,50000};
        batch.chosen[0]=pivot.bit;
        batch.enumerate(u16(((1u<<N2C)-1)^1u),pivot.bit);
        if(batch.records!=pivot.frequency)throw std::runtime_error("pivot frequency mismatch");
        if(out.size()+batch.raw.size()>o.maxQueries)throw std::runtime_error("BOUND weak query records");
        std::vector<std::pair<State,u64>> sorted(batch.raw.begin(),batch.raw.end());
        std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.first.m<b.first.m;});
        for(const auto& [key,multiplicity]:sorted)
            out.push_back(Query{key,UINT32_MAX,u32(targets.size()),multiplicity});
        records+=batch.records;targets.push_back(target);budget.check();
    }
    if(targets.size()!=o.limit)throw std::runtime_error("input shorter than requested gate");
    if(C==5&&native_gather::next(in,C-1,row))throw std::runtime_error("C5 target file longer than complete layer");
    return records;
}

void verify_values(const std::vector<u128>& sums,const std::vector<Target>& targets,
                   const char* phase) {
    if(C!=5)return;
    u128 total=0,weighted=0;
    std::map<std::pair<u64,u64>,u64> observed,expected;
    for(size_t i=0;i<targets.size();++i) {
        const u128 value=(C-1)*sums[i];
        if(value!=targets[i].expected||value>UINT64_MAX)
            throw std::runtime_error(std::string(phase)+" complete F3->F4 differential failed");
        ++observed[{u64(value),targets[i].stab}];
        ++expected[{u64(targets[i].expected),targets[i].stab}];
        total+=value;weighted+=value*(FACT[C]*(1ULL<<C)/targets[i].stab);
    }
    if(observed!=expected||total!=3972941184ULL||weighted!=14365876248576ULL)
        throw std::runtime_error("complete weighted value/stabilizer histogram failed");
    std::cout<<"VALUES phase="<<phase<<" all17120=PASS value_stab_histogram=PASS sumF4="
             <<native_gather::decimal_string(total)<<" sumOrbitF4="
             <<native_gather::decimal_string(weighted)<<'\n';
}

void calibration(const std::string& path,const Inventory& inventory,
                 missing_geometry_probe::Budget& budget) {
    if(path.empty())return;
    std::ifstream in(path);if(!in)throw std::runtime_error("calibration sample unavailable");
    native_gather::Input row;std::vector<State> sample;
    while(native_gather::next(in,C-2,row)) {
        if(sample.size()==1024)throw std::runtime_error("calibration source limit1024");
        sample.push_back(native_gather::encode(row.graph));
    }
    if(sample.size()!=1024)throw std::runtime_error("calibration needs exact uniform1024 sample");
    Canon geometry(inventory);u64 checksum=0,aut=0;
    const auto began=Clock::now();
    for(unsigned round=0;round<256;++round) {
        for(const State& s:sample) {
            const State key=geometry.run(s,&aut);checksum+=state_hash(key)^aut;
        }
        budget.check();
    }
    std::cout<<"CALIBRATION domain=REPEATED_UNIFORM_NATIVE_SAMPLE independent_sources=1024"
             <<" repeated_passes=256 calls=262144 threads=1 seconds="<<native_gather::seconds(began)
             <<" checksum="<<checksum<<" concurrent_F4=YES production_runtime_estimate=NOT_QUALIFIED\n";
}

int run(int argc,char** argv) {
    if(argc<3)throw std::runtime_error("C input=PATH limit=N maxqueries=N maxseconds=S [catalogue=C5L3] [calibration=C6L4sample]");
    Options o;o.c=std::stoi(argv[1]);
    for(int i=2;i<argc;++i) {
        const std::string word=argv[i];const auto at=word.find('=');
        if(at==std::string::npos)throw std::runtime_error("need option=value");
        const auto key=word.substr(0,at),value=word.substr(at+1);
        if(key=="input")o.input=value;else if(key=="catalogue")o.catalogue=value;
        else if(key=="calibration")o.calibration=value;
        else if(key=="limit")o.limit=std::stoull(value);
        else if(key=="maxqueries")o.maxQueries=std::stoull(value);
        else if(key=="maxseconds")o.maxSeconds=std::stod(value);
        else throw std::runtime_error("unknown option");
    }
    if((o.c!=5&&o.c!=6)||o.input.empty()||!o.maxQueries||o.maxQueries>5000000||
       !std::isfinite(o.maxSeconds)||o.maxSeconds<=0||o.maxSeconds>120||
       (o.c==5&&(o.limit!=17120||o.catalogue.empty()||!o.calibration.empty()))||
       (o.c==6&&(!o.limit||o.limit>512||!o.catalogue.empty())))
        throw std::runtime_error("invalid positive limits; FULL C6 catalogue NOT SUPPORTED");
    C=o.c;N2C=2*C;g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*i;
    omp_set_num_threads(1);
    shared_catalog::Options limits;limits.maxSeconds=o.maxSeconds;limits.maxResidentBytes=1ULL<<30;
    shared_catalog::detail::LoadBudget watchdog(limits);
    missing_geometry_probe::Budget budget{Clock::now(),o.maxSeconds,0};
    Inventory inventory;inventory.build(budget);
    std::vector<Query> query;query.reserve(o.maxQueries);
    std::vector<Target> targets;targets.reserve(o.limit);
    const auto gatherStart=Clock::now();
    const u64 records=queries(o,query,targets,budget);
    const double gatherSeconds=native_gather::seconds(gatherStart);
    Layer layer;std::string sourceSha;
    std::filesystem::file_time_type sourceTime{};u64 sourceBytes=0;
    if(C==5) {
        sourceBytes=std::filesystem::file_size(o.catalogue);
        sourceTime=std::filesystem::last_write_time(o.catalogue);
        layer=small_catalogue(o,budget,sourceSha);
    } else layer.init_fixed(query.size());
    const auto idStart=Clock::now();two_missing_prefix::Canon native;
    for(size_t i=0;i<query.size();++i) {
        u64 aut=0;const State key=native.strict(query[i].raw,&aut);
        const u32 id=C==5?layer.find(key):layer.find_or_add_mt(key,u32(aut));
        if(id==UINT32_MAX||layer.stab[id]!=aut)throw std::runtime_error("native source ID/stabilizer lookup failed");
        query[i].id=id;
        if(i%1024==0)budget.check();
    }
    const double initialSeconds=native_gather::seconds(idStart);
    const std::string identity=payload_identity(layer);
    std::vector<u128> nativeValues(targets.size()),geometryValues(targets.size());
    u64 nativeChecksum=0,geometryChecksum=0,nativeNodes=0,geometryNodes=0;
    const auto nativeStart=Clock::now();
    for(size_t i=0;i<query.size();++i) {
        const auto& q=query[i];u64 aut=0;
        const State key=native.strict(q.raw,&aut);nativeNodes+=native.nodes;
        const u32 id=layer.find(key);
        if(id!=q.id||layer.stab[id]!=aut)throw std::runtime_error("native repeated ID mismatch");
        nativeChecksum+=u64(id+1)*q.multiplicity;
        if(C==5)nativeValues[q.target]+=u128(q.multiplicity)*layer.T[id];
        if(i%1024==0)budget.check();
    }
    const double nativeSeconds=native_gather::seconds(nativeStart);
    verify_values(nativeValues,targets,"NATIVE_PREFIX_INDEX");
    const auto rekeyStart=Clock::now();
    std::vector<u32>().swap(layer.table);layer.mask=0; // Release BEFORE replacing any key.
    Canon geometry(inventory);u64 changed=0,rekeyNodes=0;
    std::map<u32,u64> histogram;
    for(u32 id=0;id<layer.size();++id) {
        if(!layer.stab[id])continue;
        u64 aut=0;const State key=geometry.run(layer.keys[id],&aut);rekeyNodes+=geometry.nodes;
        if(aut!=layer.stab[id])throw std::runtime_error("RAM rekey stabilizer changed");
        if(!(key==layer.keys[id]))++changed;
        layer.keys[id]=key;++histogram[layer.stab[id]];
        if(id%1024==0)budget.check();
    }
    const double rekeySeconds=native_gather::seconds(rekeyStart);
    const auto indexStart=Clock::now();rebuild(layer,budget);
    const double indexSeconds=native_gather::seconds(indexStart);
    if(payload_identity(layer)!=identity)throw std::runtime_error("stable-ID values/stabilizers changed");
    const auto geometryStart=Clock::now();
    for(size_t i=0;i<query.size();++i) {
        const auto& q=query[i];u64 aut=0;
        const State key=geometry.run(q.raw,&aut);geometryNodes+=geometry.nodes;
        const u32 id=layer.find(key);
        if(id!=q.id||layer.stab[id]!=aut)throw std::runtime_error("geometry query changed native stable ID");
        geometryChecksum+=u64(id+1)*q.multiplicity;
        if(C==5)geometryValues[q.target]+=u128(q.multiplicity)*layer.T[id];
        if(i%1024==0)budget.check();
    }
    const double geometrySeconds=native_gather::seconds(geometryStart);
    if(nativeChecksum!=geometryChecksum||nativeValues!=geometryValues)
        throw std::runtime_error("stable ID/checksum/value differential failed");
    verify_values(geometryValues,targets,"GEOMETRY_REKEY_INDEX");
    std::cout<<"REKEY_HISTOGRAM values=";
    for(const auto& [aut,count]:histogram)std::cout<<aut<<':'<<count<<',';
    std::cout<<" duplicate_representatives=0 stable_id_payload_SHA256="<<identity<<'\n';
    calibration(o.calibration,inventory,budget);
    if(C==5&&(std::filesystem::file_size(o.catalogue)!=sourceBytes||
       std::filesystem::last_write_time(o.catalogue)!=sourceTime))
        throw std::runtime_error("readonly source changed");
    budget.check();
    std::cout<<std::fixed<<std::setprecision(9)<<"SUMMARY domain="
             <<(C==5?"COMPLETE_C5_L3_TO_L4":"C6_SAMPLE_QUERY_CATALOGUE_NOT_GLOBAL")
             <<" sources="<<targets.size()<<" labelled_matchings="<<records<<" weak_queries="<<query.size()
             <<" stable_IDs="<<layer.real_size()<<" ID_scope="<<(C==5?"SOURCE_CHECKPOINT":"LOCAL_SAMPLE")
             <<" IDs_preserved=ALL stabilizers_preserved=ALL"
             <<" changed_keys="<<changed<<" max_simultaneous_indexes=1 threads=1"
             <<" gather_s="<<gatherSeconds<<" initial_ID_s="<<initialSeconds
             <<" prefix_query_s="<<nativeSeconds<<" geometry_query_s="<<geometrySeconds
             <<" rekey_s="<<rekeySeconds<<" rebuild_s="<<indexSeconds
             <<" prefix_nodes="<<nativeNodes<<" geometry_nodes="<<geometryNodes
             <<" rekey_nodes="<<rekeyNodes<<" checksum="<<nativeChecksum
             <<" query_record_bytes="<<query.size()*sizeof(Query)
             <<" total_s="<<native_gather::seconds(budget.started)
             <<" peak_bytes="<<std::max(budget.peak,watchdog.peak.load())
             <<" concurrent_F4_window=YES production_time_estimate=NOT_QUALIFIED N6=NOT_COMPUTED\n";
    std::cout<<"GEOMETRY RAM REKEY SMALL GATE PASSED\n";
    return 0;
}
} // namespace geometry_rekey
#ifndef GEOMETRY_REKEY_NO_MAIN
int main(int argc,char** argv) try {return geometry_rekey::run(argc,argv);}
catch(const std::exception& e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}
#endif
