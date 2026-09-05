// Same immutable native catalogue, two unchanged-math lookup-only cores.
// The only difference is the narrowly checked compatible canonical dispatch.
#define main prefix_compare_original_native_main
#include "layer_dp_gate.cpp"
#undef main
#include "layer_shared_catalog.h"
#include "layer_reverse_f5_core.h"
#include "layer_two_missing_prefix_canon.h"
#undef FJ_LAYER_REVERSE_F5_CORE_H
#define reverse_f5 reverse_f5_prefix
#define canonize two_missing_prefix::canonicalize
#include "layer_reverse_f5_core.h"
#undef canonize
#undef reverse_f5

namespace prefix_compare {
struct Options {
    std::string catalog,input;u64 limit=0,maxRecords=0,rss=0;
    double seconds=0;bool readonly=false,repair=false,ack=false;
};
Options parse(int argc,char**argv) {
    if(argc<2)throw std::runtime_error("C catalog=... input=... limit=... maxrecords=... maxseconds=... maxrssgib=... checkpointreadonly [repair-l4-support ack-large-c6]");
    C=std::stoi(argv[1]);N2C=2*C;Options o;
    for(int i=2;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="checkpointreadonly"){o.readonly=true;continue;}
        if(arg=="repair-l4-support"){o.repair=true;continue;}
        if(arg=="ack-large-c6"){o.ack=true;continue;}
        const auto at=arg.find('=');if(at==std::string::npos)throw std::runtime_error("option=value required");
        const auto key=arg.substr(0,at),value=arg.substr(at+1);
        if(key=="catalog")o.catalog=value;else if(key=="input")o.input=value;
        else if(key=="limit")o.limit=std::stoull(value);else if(key=="maxrecords")o.maxRecords=std::stoull(value);
        else if(key=="maxseconds")o.seconds=std::stod(value);else if(key=="maxrssgib")o.rss=std::stoull(value);
        else throw std::runtime_error("unknown option");
    }
    if((C!=5&&C!=6)||!o.readonly||o.catalog.empty()||o.input.empty()||!o.limit||o.limit>512||
       !o.maxRecords||o.maxRecords>5000000||!std::isfinite(o.seconds)||o.seconds<=0||o.seconds>180||
       !o.rss||o.rss>55||(C==6&&(!o.repair||!o.ack))||
       (C==5&&(o.repair||o.ack||o.rss>1||o.seconds>120)))
        throw std::runtime_error("bounded readonly membership-only C5/C6 required; C5 <=1GiB/120s, C6 explicit ack/repair <=55GiB/180s");
    return o;
}
struct Totals {
    u64 records=0,weak=0,hits=0,checksum=0,nodes=0,prefix=0,fallback=0;
    double wall=0,canon=0,lookup=0,pivot=0,enumeration=0,conversion=0,workers=0;
};
Totals timed(bool prefix,int threads,const std::vector<State>& sources,const Layer& layer) {
    u64 records=0,weak=0,hits=0,checksum=0,nodes=0,prefixCalls=0,fallbackCalls=0;
    double canon=0,lookup=0,pivot=0,enumeration=0,conversion=0,workers=0;
    std::atomic<bool> failed{false};std::mutex errorMutex;std::string error;
    const auto started=native_gather::Clock::now();
#pragma omp parallel num_threads(threads) reduction(+:records,weak,hits,nodes,prefixCalls,fallbackCalls,canon,lookup,pivot,enumeration,conversion,workers) reduction(^:checksum)
    {
        reverse_f5::Worker native;
        reverse_f5_prefix::Worker special;
#pragma omp for schedule(dynamic,1)
        for(int64_t i=0;i<int64_t(sources.size());++i) {
            if(failed.load(std::memory_order_relaxed))continue;
            try {
                const u64 beforePrefix=two_missing_prefix::dispatched,beforeFallback=two_missing_prefix::fallbacks;
                // Deliberately retain each core's no-value return type.
                if(prefix) {
                    const auto result=special.lookup_only(sources[size_t(i)],layer);
                    const auto& s=result.diagnostics;
                    records+=s.labelledMatchings;weak+=s.weakResiduals;hits+=s.lookupHits;checksum^=s.lookupChecksum;
                    nodes+=s.canonicalizationNodes;canon+=s.canonicalizationSeconds;lookup+=s.lookupSeconds;
                    pivot+=s.pivotSeconds;enumeration+=s.enumerationSeconds;conversion+=s.conversionSeconds;workers+=s.totalSeconds;
                }else {
                    const auto result=native.lookup_only(sources[size_t(i)],layer);
                    const auto& s=result.diagnostics;
                    records+=s.labelledMatchings;weak+=s.weakResiduals;hits+=s.lookupHits;checksum^=s.lookupChecksum;
                    nodes+=s.canonicalizationNodes;canon+=s.canonicalizationSeconds;lookup+=s.lookupSeconds;
                    pivot+=s.pivotSeconds;enumeration+=s.enumerationSeconds;conversion+=s.conversionSeconds;workers+=s.totalSeconds;
                }
                prefixCalls+=two_missing_prefix::dispatched-beforePrefix;
                fallbackCalls+=two_missing_prefix::fallbacks-beforeFallback;
            }catch(const std::exception& e) {
                failed.store(true,std::memory_order_relaxed);std::lock_guard<std::mutex> held(errorMutex);
                if(error.empty())error=e.what();
            }
        }
    }
    if(failed.load())throw std::runtime_error(error);
    return {records,weak,hits,checksum,nodes,prefixCalls,fallbackCalls,native_gather::seconds(started),
            canon,lookup,pivot,enumeration,conversion,workers};
}
int run(int argc,char**argv) {
    const Options o=parse(argc,argv);
    g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    const auto started=native_gather::Clock::now();
    shared_catalog::Options loading;
    loading.path=o.catalog;loading.threads=C==6?24:1;loading.expectedLayer=4;
    loading.checkpointReadonly=true;loading.repairC6=o.repair;
    loading.maxEntries=C==6?903398621ULL:200000ULL;
    loading.maxResidentBytes=o.rss<<30;loading.maxSeconds=o.seconds;
    shared_catalog::detail::LoadBudget budget(loading);
    std::ifstream input(o.input);if(!input)throw std::runtime_error("sample unavailable");
    std::vector<State> sources;native_gather::Input row;u64 expectedRecords=0;
    while(sources.size()<o.limit&&native_gather::next(input,5,row)) {
        expectedRecords+=native_gather::pivot(row.graph).frequency;
        if(expectedRecords>o.maxRecords)throw std::runtime_error("matching count exceeds bounded workload");
        sources.push_back(native_gather::encode(row.graph));
    }
    if(sources.size()!=o.limit)throw std::runtime_error("source count differs from positive limit");
    auto catalog=shared_catalog::load(loading);Layer& layer=catalog.layer;
    std::cout<<std::fixed<<std::setprecision(9)<<"CATALOGUE source_sha256="<<catalog.sourceSha256
             <<" source_T=discarded index_threads="<<loading.threads<<" native_live="<<layer.real_size()
             <<" read_s="<<catalog.readSeconds<<" sha256_s="<<catalog.sha256Seconds
             <<" index_s="<<catalog.indexSeconds<<" wall_s="<<catalog.wallSeconds<<'\n'<<std::flush;
    // Untimed semantic gate over EVERY actual residual query and native ID.
    const auto auditStarted=native_gather::Clock::now();u64 queries=0,records=0;
    for(const auto& source:sources) {
        const auto graph=reverse_f5::true_slot_graph(source);const auto root=native_gather::pivot(graph);
        native_gather::Batch batch{graph,{}, {},0,100000,50000};
        batch.chosen[0]=root.bit;batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);
        if(batch.records!=root.frequency)throw std::runtime_error("untimed root count mismatch");
        records+=batch.records;queries+=batch.raw.size();
        for(const auto& [raw,mult]:batch.raw) {
            (void)mult;u64 a=0,b=0;
            const State oldKey=::canonize(raw,&a),newKey=two_missing_prefix::canonicalize(raw,&b);
            if(!(oldKey==newKey)||a!=b)throw std::runtime_error("untimed complete key/stabilizer differential");
            const u32 oldId=layer.find(oldKey),newId=layer.find(newKey);
            if(oldId==UINT32_MAX||oldId!=newId||layer.stab[oldId]!=a)
                throw std::runtime_error("untimed complete native ID/stabilizer differential");
        }
    }
    if(records!=expectedRecords)throw std::runtime_error("untimed record count mismatch");
    std::cout<<"UNTIMED_DIFFERENTIAL sources="<<sources.size()<<" labelled_matchings="<<records
             <<" queries="<<queries<<" all_keys_stabilizers_IDs_equal=YES misses=0 seconds="
             <<native_gather::seconds(auditStarted)<<'\n'<<std::flush;
    u64 sharedChecksum=0;bool haveChecksum=false;
    for(int threads:(C==6?std::vector<int>{1,24}:std::vector<int>{1})) {
        for(bool prefix:(threads==1?std::vector<bool>{false,true}:std::vector<bool>{true,false})) {
            const Totals s=timed(prefix,threads,sources,layer);
            if(s.records!=expectedRecords||s.weak!=queries||s.hits!=queries||
               (haveChecksum&&s.checksum!=sharedChecksum)||
               (prefix&&C==6&&(s.prefix!=queries||s.fallback))||
               (prefix&&C==5&&(s.prefix||s.fallback!=queries)))throw std::runtime_error("timed counters/checksum/dispatch mismatch");
            haveChecksum=true;sharedChecksum=s.checksum;
            std::cout<<"TIMED variant="<<(prefix?"compatible_prefix":"native")<<" threads="<<threads
                     <<" sources="<<sources.size()<<" labelled_matchings="<<s.records<<" weak_residuals="<<s.weak
                     <<" hits="<<s.hits<<" misses=0 checksum="<<s.checksum<<" prefix_calls="<<s.prefix
                     <<" native_fallback_calls="<<s.fallback<<" canon_nodes="<<s.nodes
                     <<" query_wall_s="<<s.wall<<" canonicalization_cpu_s="<<s.canon<<" lookup_cpu_s="<<s.lookup
                     <<" pivot_cpu_s="<<s.pivot<<" enumeration_cpu_s="<<s.enumeration
                     <<" conversion_cpu_s="<<s.conversion<<" worker_total_cpu_s="<<s.workers<<'\n'<<std::flush;
        }
    }
    shared_catalog::verify_source_unchanged(catalog);
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("cannot inspect RSS");
    std::cout<<"[OK] SHARED_TABLE_COMPLETE_LOOKUP_DIFFERENTIAL total_wall_s="<<native_gather::seconds(started)
             <<" peak_rss_bytes="<<std::max(budget.peak.load(),u64(pm.PeakWorkingSetSize))
             <<" source_checkpointreadonly=yes production_T_values_read=no F5=NOT_COMPUTED N6=NOT_COMPUTED file_writes=none\n";
    return 0;
}
}
int main(int argc,char**argv)try{return prefix_compare::run(argc,argv);}
catch(const std::exception&e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}
