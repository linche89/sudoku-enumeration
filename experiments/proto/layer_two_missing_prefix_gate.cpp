// Isolated exact compatible-prefix qualification. Text only, one thread,
// <=120 s and <=1 GiB. No production checkpoint is opened. Include the
// actual production translation unit, including its narrowly scoped dispatch.
#define REVERSE_F5_NO_MAIN
#include "layer_reverse_f5.cpp"
#include <map>

namespace prefix_gate {
using Clock=native_gather::Clock;
struct Budget {
    Clock::time_point start=Clock::now();double maximum=0;u64 peak=0;
    void check() {
        PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
        if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("RSS unavailable");
        peak=std::max(peak,u64(pm.PeakWorkingSetSize));
        if(peak>(1ULL<<30)||native_gather::seconds(start)>maximum)throw std::runtime_error("BOUND 1GiB/requested wall");
    }
};
struct Differential {
    u64 checked=0,transforms=0,referees=0;
    std::map<u64,u64> histogram;
    std::unordered_set<State,native_gather::Hash> keys;
    void test(const State& state,Budget& budget,bool invariant=false) {
        u64 a=0,b=0;
        const State native=::canonize(state,&a);
        const State prefix=two_missing_prefix::Canon{}.strict(state,&b);
        if(!(native==prefix)||a!=b)throw std::runtime_error("native key/stabilizer differential");
        const State routed=two_missing_prefix::canonicalize(state,&b);
        if(!(native==routed)||a!=b)throw std::runtime_error("checked dispatch differential");
        ++checked;++histogram[a];keys.insert(native);
        if(invariant) {
            if(!(two_missing_prefix::Canon{}.strict(prefix)==prefix))throw std::runtime_error("idempotence");
            const size_t n=C<=4?GROUP.size():8;
            for(size_t j=0;j<n;++j) {
                const auto& g=GROUP[C<=4?j:(sample_mix64(checked*17+j)%GROUP.size())];
                if(!(two_missing_prefix::Canon{}.strict(apply_state(g,state),&b)==native)||a!=b)
                    throw std::runtime_error("invariance/stabilizer differential");
                ++transforms;
            }
            if(C<=4||referees<8) {
                if(stab_scan(state)!=a)throw std::runtime_error("independent full-group stabilizer");
                ++referees;
            }
        }
        if(checked%1024==0)budget.check();
    }
};
void guard_tests(const State& valid) {
    auto refused=[](const State& s) {
        try {two_missing_prefix::Canon{}.strict(s);}catch(const std::runtime_error&){return;}
        throw std::runtime_error("strict precondition was not enforced");
    };
    g_wlseed=true;refused(valid);
    u64 a=0,b=0;
    if(!(::canonize(valid,&a)==two_missing_prefix::canonicalize(valid,&b))||a!=b)
        throw std::runtime_error("seeded native fallback changed");
    g_wlseed=false;
    State bad=valid;bad.m[0]=u16((bad.m[0]&~3u)|1u);refused(bad);
    bad=valid;bad.m[0]|=u16(1u<<N2C);refused(bad);
    if(C<6){bad=valid;bad.m[N2C]=2;refused(bad);}
    std::cout<<"PRECONDITIONS seeded_fallback=YES bad_field=REFUSED high_bits=REFUSED padding=CHECKED\n";
}
void chain(const std::string& preload,const std::string& targets,u64 maxQueries,Budget& budget) {
    if(C!=5)throw std::runtime_error("chain is exactly C5 L3->L4");
    std::unordered_map<State,u128,native_gather::Hash> values;
    std::ifstream in(preload);if(!in)throw std::runtime_error("preload unavailable");
    native_gather::Input row;Differential diff;
    while(native_gather::next(in,3,row)) {
        if(!row.expected||!row.value)throw std::runtime_error("positive closed expected F3 required");
        const State raw=native_gather::encode(row.graph);diff.test(raw,budget,true);
        if(!values.emplace(::canonize(raw),row.value).second)throw std::runtime_error("duplicate predecessor");
    }
    if(values.size()!=16150)throw std::runtime_error("complete16150 predecessors required");
    std::ifstream input(targets);if(!input)throw std::runtime_error("targets unavailable");
    u64 checked=0,records=0,queries=0;u128 sum=0,weighted=0;
    std::unordered_set<State,native_gather::Hash> targetsSeen;
    while(native_gather::next(input,4,row)) {
        if(!row.expected||!row.value)throw std::runtime_error("positive closed expected F4 required");
        const auto root=native_gather::pivot(row.graph);
        native_gather::Batch batch{row.graph,{}, {},0,100000,50000};
        batch.chosen[0]=root.bit;batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);
        if(batch.records!=root.frequency)throw std::runtime_error("root matching count mismatch");
        records+=batch.records;queries+=batch.raw.size();
        if(queries>maxQueries)throw std::runtime_error("query bound");
        u128 nativeTotal=0,prefixTotal=0;
        for(const auto& [raw,mult]:batch.raw) {
            u64 a=0,b=0;const State oldKey=::canonize(raw,&a);
            const State newKey=two_missing_prefix::canonicalize(raw,&b);
            if(!(oldKey==newKey)||a!=b)throw std::runtime_error("operator residual key/stabilizer differs");
            const auto old=values.find(oldKey),fresh=values.find(newKey);
            if(old==values.end()||fresh==values.end())throw std::runtime_error("operator predecessor absent");
            nativeTotal+=u128(mult)*old->second;prefixTotal+=u128(mult)*fresh->second;
        }
        if(4*nativeTotal!=row.value||4*prefixTotal!=row.value)throw std::runtime_error("exact closed F4 differs");
        u64 stab=0;const State key=::canonize(native_gather::encode(row.graph),&stab);
        // One-missing inputs must use the unchanged anchored native fallback.
        const u64 before=two_missing_prefix::fallbacks;
        if(!(two_missing_prefix::canonicalize(key)==key)||two_missing_prefix::fallbacks!=before+1)
            throw std::runtime_error("one-missing fallback not retained");
        if(!targetsSeen.insert(key).second)throw std::runtime_error("duplicate target");
        sum+=row.value;weighted+=u128(FACT[C]*(1u<<C)/stab)*row.value;++checked;budget.check();
    }
    if(checked!=17120)throw std::runtime_error("complete17120 targets required");
    std::cout<<"COMPLETE_C5_L3_L4 predecessors=16150 targets="<<checked
             <<" labelled_matchings="<<records<<" weak_queries="<<queries
             <<" all_term_keys_stabs_coefficients_equal=YES all_F4_equal=YES"
             <<" predecessor_transforms="<<diff.transforms
             <<" sumF4="<<native_gather::decimal_string(sum)
             <<" sumOrbitF4="<<native_gather::decimal_string(weighted)<<'\n';
}
int run(int argc,char**argv) {
    if(argc<3)throw std::runtime_error("C degree input=TEXT limit=N maxqueries=N maxseconds=S [preload=TEXT] [invariance=N]");
    C=std::stoi(argv[1]);N2C=2*C;const int degree=std::stoi(argv[2]);
    std::string input,preload;u64 limit=0,maxQueries=0,invariance=64;double maxSeconds=0;
    for(int i=3;i<argc;++i) {
        const std::string arg=argv[i];const auto at=arg.find('=');
        if(at==std::string::npos)throw std::runtime_error("option=value required");
        const auto key=arg.substr(0,at),value=arg.substr(at+1);
        if(key=="input")input=value;else if(key=="preload")preload=value;
        else if(key=="limit")limit=std::stoull(value);else if(key=="maxqueries")maxQueries=std::stoull(value);
        else if(key=="maxseconds")maxSeconds=std::stod(value);else if(key=="invariance")invariance=std::stoull(value);
        else throw std::runtime_error("unknown option");
    }
    if(C<2||C>6||input.empty()||degree<C-2||degree>C-1||!limit||limit>20000||
       !maxQueries||maxQueries>5000000||invariance>20000||!std::isfinite(maxSeconds)||maxSeconds<=0||maxSeconds>120)
        throw std::runtime_error("bounded two-missing text gate required");
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*i;
    g_wlseed=false;omp_set_num_threads(1);build_group();Budget budget{Clock::now(),maxSeconds,0};
    if(!preload.empty()) {
        if(C!=5||degree!=4||limit!=17120)throw std::runtime_error("chain needs complete C5 L4 targets");
        chain(preload,input,maxQueries,budget);
    }else {
        std::ifstream stream(input);if(!stream)throw std::runtime_error("input unavailable");
        native_gather::Input row;Differential diff;u64 sources=0,records=0;
        while(sources<limit&&native_gather::next(stream,degree,row)) {
            ++sources;
            if(degree==C-2) {
                const State raw=native_gather::encode(row.graph);
                if(sources==1)guard_tests(raw);
                diff.test(raw,budget,diff.checked<invariance);
            }else {
                const auto root=native_gather::pivot(row.graph);
                native_gather::Batch batch{row.graph,{}, {},0,100000,50000};
                batch.chosen[0]=root.bit;batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);
                if(batch.records!=root.frequency)throw std::runtime_error("root matching count mismatch");
                records+=batch.records;
                for(const auto& [raw,mult]:batch.raw){(void)mult;diff.test(raw,budget,diff.checked<invariance);}
            }
            if(diff.checked>maxQueries)throw std::runtime_error("query bound");
            budget.check();
        }
        if(sources!=limit)throw std::runtime_error("fewer sources than requested");
        std::cout<<"DIFFERENTIAL C="<<C<<" degree="<<degree<<" sources="<<sources
                 <<" queries="<<diff.checked<<" distinct="<<diff.keys.size()<<" labelled_matchings="<<records
                 <<" transforms="<<diff.transforms<<" full_group_referees="<<diff.referees<<" histogram=";
        for(const auto& [s,n]:diff.histogram)std::cout<<s<<':'<<n<<',';
        std::cout<<'\n';
    }
    budget.check();
    std::cout<<std::fixed<<std::setprecision(6)<<"[OK] EXTRACTED_COMPATIBLE_PREFIX threads=1 seconds="
             <<native_gather::seconds(budget.start)<<" peak_rss_bytes="<<budget.peak
             <<" dispatch="<<two_missing_prefix::dispatched<<" fallback="<<two_missing_prefix::fallbacks
             <<" production_keys_unchanged=YES file_writes=none N6=NOT_COMPUTED\n";
    return 0;
}
}
int main(int argc,char**argv)try{return prefix_gate::run(argc,argv);}
catch(const std::exception&e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}
