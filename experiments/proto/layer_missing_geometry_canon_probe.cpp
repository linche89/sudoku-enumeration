// Isolated, single-thread, read-only-text probe. NEVER a production key.
// A geometry-first native canonical form for degree C-2 states.
// Existing native canonicalization/producer/checkpoints are unchanged.
#define NATIVE_GATHER_NO_MAIN
#include "layer_native_gather_bench.cpp"
#include <map>

namespace missing_geometry_probe {
using Clock = native_gather::Clock;
struct Budget {
    Clock::time_point started=Clock::now(); double maximum=0; u64 peak=0;
    void check() {
        PROCESS_MEMORY_COUNTERS pm{}; pm.cb=sizeof(pm);
        if (!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))
            throw std::runtime_error("RSS unavailable");
        peak=std::max(peak,u64(pm.WorkingSetSize));
        if (peak>(1ULL<<30) || native_gather::seconds(started)>maximum)
            throw std::runtime_error("BOUND 1 GiB / requested seconds");
    }
};
struct Trie { std::array<int,6> next{}; };
struct Geometry { u64 key=0; int root=0; u64 transporters=0; };
struct Inventory {
    std::vector<Trie> trie{{}};
    std::unordered_map<u64,Geometry> info;
    u64 classes=0, maps=0;
    static u64 pack(const int a[6][6]) {
        u64 key=0; int pos=0;
        for (int i=0;i<C;++i) for(int j=i+1;j<C;++j,++pos)
            key|=u64(a[i][j])<<(3*pos);
        return key;
    }
    static void unpack(u64 key,int a[6][6]) {
        int pos=0;
        for (int i=0;i<C;++i) for(int j=i+1;j<C;++j,++pos)
            a[i][j]=a[j][i]=int((key>>(3*pos))&7);
    }
    int node() { trie.emplace_back(); return int(trie.size()-1); }
    void insert(Geometry& g,const std::array<u8,6>& p) {
        int id=g.root;
        for(int d=0;d<C;++d) {
            int next=trie[id].next[p[d]];
            if(!next) { next=node(); trie[id].next[p[d]]=next; }
            id=next;
        }
        ++g.transporters; ++maps;
    }
    void build(Budget& budget) {
        std::vector<u64> all;
        int degree[6]={4,4,4,4,4,4}, matrix[6][6]{};
        auto row=[&](auto&& self,int i,int j,int left)->void {
            if(i==C-1) { if(degree[i]==0) all.push_back(pack(matrix)); return; }
            if(j==C) { if(!left) self(self,i+1,i+2,degree[i+1]); return; }
            for(int m=0;m<=std::min(left,degree[j]);++m) {
                matrix[i][j]=matrix[j][i]=m; degree[j]-=m;
                self(self,i,j+1,left-m); degree[j]+=m;
            }
            matrix[i][j]=matrix[j][i]=0;
        };
        row(row,0,1,4); std::sort(all.begin(),all.end());
        for(u64 base:all) {
            if(info.contains(base)) continue;
            ++classes; int a[6][6]{}; unpack(base,a);
            std::array<u8,6> p{}; for(int b=0;b<C;++b)p[b]=u8(b);
            do {
                int moved[6][6]{};
                for(int i=0;i<C;++i) for(int j=i+1;j<C;++j)
                    moved[p[i]][p[j]]=moved[p[j]][p[i]]=a[i][j];
                const u64 key=pack(moved);
                if(key<base)throw std::runtime_error("geometry representative not minimum");
                auto [it,added]=info.emplace(key,Geometry{});
                if(added){it->second.key=base;it->second.root=node();}
                if(it->second.key!=base)throw std::runtime_error("geometry classes overlap");
                insert(it->second,p);
            } while(std::next_permutation(p.begin(),p.begin()+C));
            budget.check();
        }
        if(info.size()!=all.size() || maps!=classes*FACT[C])
            throw std::runtime_error("geometry inventory mismatch");
        std::map<u64,u64> hist;
        for(const auto& [key,g]:info){(void)key;++hist[g.transporters];}
        std::cout<<"inventory labelled="<<all.size()<<" classes="<<classes
                 <<" transporter_paths="<<maps<<" trie_nodes="<<trie.size()<<" aut_hist=";
        for(const auto& [s,n]:hist)std::cout<<s<<':'<<n<<',';
        std::cout<<'\n';
    }
};

// Allowed box sequences are exactly all transporters H -> canonical(H).
// Flips remain unrestricted. Therefore equal output iff native equivalent.
// Each minimum is attained by exactly |Stab(native)| group elements.
struct Canon {
    const Inventory& inventory;
    bool compatible=false;
    u32 maximumNeighbors[6]{},maximumEndpoints=0;
    u32 z0[6]{},z2[6][2]{},z3[6][2]{},bestSig[6]{};
    bool bestSet[6]{},haveFinal=false;
    u8 path[6]{},bestPath[6]{};
    u64 nodes=0,signatures=0,stabilizer=0;
    explicit Canon(const Inventory& value,bool nativePrefix=false):inventory(value),compatible(nativePrefix){}
    bool allowed(int depth,int node,int b,int firstBox=-1) const {
        if(!compatible)return inventory.trie[node].next[b]!=0;
        if((node>>b)&1)return false;
        if(depth==0)return (maximumEndpoints>>b)&1;
        if(depth==1)return (maximumNeighbors[firstBox>=0?firstBox:(path[0]>>1)]>>b)&1;
        return true;
    }
    int child(int node,int b)const{return compatible?(node|(1<<b)):inventory.trie[node].next[b];}
    u32 sig(int b,int f,const u32* groups,int ng) {
        ++signatures; u32 result=0;
        for(int i=0;i<ng;++i) {
            const int c0=__builtin_popcount(groups[i]&z0[b]);
            const int c2=__builtin_popcount(groups[i]&z2[b][f]);
            const int c3=__builtin_popcount(groups[i]&z3[b][f]);
            result<<=2*c0;
            result=(result<<(2*c2))|REP2[c2];
            result=(result<<(2*c3))|REP3[c3];
        }
        return result;
    }
    void dfs(int depth,const u32* groups,int ng,int node) {
        ++nodes;
        if(depth==C) {
            if(!haveFinal){std::memcpy(bestPath,path,6);haveFinal=true;stabilizer=1;}
            else ++stabilizer;
            return;
        }
        u8 candidates[12]{};int count=0;u32 minimum=UINT32_MAX;
        for(int b=0;b<C;++b)if(allowed(depth,node,b))
            for(int f=0;f<2;++f) {
                const u32 value=sig(b,f,groups,ng);
                if(value<minimum){minimum=value;count=0;}
                if(value==minimum)candidates[count++]=u8(2*b+f);
            }
        if(!count)throw std::runtime_error("empty transporter prefix");
        if(bestSet[depth]) {
            if(minimum>bestSig[depth])return;
            if(minimum<bestSig[depth]){
                bestSig[depth]=minimum;
                for(int d=depth+1;d<C;++d)bestSet[d]=false;
                haveFinal=false;stabilizer=0;
            }
        } else {bestSet[depth]=true;bestSig[depth]=minimum;}
        u32 refined[12][12]{},look[12]{}; int sizes[12]{},order[12]{};
        for(int k=0;k<count;++k) {
            const int b=candidates[k]>>1,f=candidates[k]&1;
            int n=0;
            for(int i=0;i<ng;++i){
                const u32 a0=groups[i]&z0[b],a2=groups[i]&z2[b][f],a3=groups[i]&z3[b][f];
                if(a0)refined[k][n++]=a0;
                if(a2)refined[k][n++]=a2;
                if(a3)refined[k][n++]=a3;
            }
            sizes[k]=n;order[k]=k;
            if(count>1&&depth+1<C) {
                u32 nextMin=UINT32_MAX;const int next=child(node,b);
                for(int bb=0;bb<C;++bb)if(allowed(depth+1,next,bb,depth==0?b:-1))
                    for(int ff=0;ff<2;++ff)nextMin=std::min(nextMin,sig(bb,ff,refined[k],n));
                look[k]=nextMin;
            }
        }
        if(count>1)std::sort(order,order+count,[&](int a,int b){return look[a]<look[b];});
        for(int kk=0;kk<count;++kk){
            const int k=order[kk];path[depth]=candidates[k];
            dfs(depth+1,refined[k],sizes[k],child(node,candidates[k]>>1));
        }
    }
    State run(const State& s,u64* aut=nullptr) {
        nodes=signatures=stabilizer=0;haveFinal=false;
        std::fill(bestSet,bestSet+6,false);
        for(int b=0;b<C;++b){
            u32 a0=0,a2=0,a3=0;
            for(int i=0;i<N2C;++i){
                const int value=fld(s.m[i],b);
                if(value==0)a0|=1u<<i;else if(value==2)a2|=1u<<i;else a3|=1u<<i;
            }
            z0[b]=a0;z2[b][0]=a2;z2[b][1]=a3;z3[b][0]=a3;z3[b][1]=a2;
        }
        int root=0;
        if(compatible){
            int maximum=-1;maximumEndpoints=0;std::fill(maximumNeighbors,maximumNeighbors+6,0);
            for(int b=0;b<C;++b)for(int c=b+1;c<C;++c){
                const int m=__builtin_popcount(z0[b]&z0[c]);
                if(m>maximum){maximum=m;maximumEndpoints=0;std::fill(maximumNeighbors,maximumNeighbors+6,0);}
                if(m==maximum){maximumEndpoints|=(1u<<b)|(1u<<c);maximumNeighbors[b]|=1u<<c;maximumNeighbors[c]|=1u<<b;}
            }
        }else{
            u64 missing=0;int pos=0;
            for(int b=0;b<C;++b)for(int c=b+1;c<C;++c,++pos)
                missing|=u64(__builtin_popcount(z0[b]&z0[c]))<<(3*pos);
            const auto found=inventory.info.find(missing);
            if(found==inventory.info.end())throw std::runtime_error("missing geometry outside inventory");
            root=found->second.root;
        }
        u32 group=(1u<<N2C)-1;dfs(0,&group,1,root);
        if(!haveFinal||!stabilizer)throw std::runtime_error("no canonical output");
        GElem g{};
        for(int d=0;d<C;++d){const int b=bestPath[d]>>1;g.perm[b]=u8(d);g.flips|=u8((bestPath[d]&1)<<b);}
        if(aut)*aut=stabilizer;
        return apply_state(g,s);
    }
};

void value_chain(const Inventory& inventory,const std::string& preloadPath,
                 const std::string& targetPath,u64 maxQueries,Budget& budget) {
    if(C!=5)throw std::runtime_error("value-chain gate is explicitly complete C5 L3 -> L4");
    std::unordered_map<State,u128,native_gather::Hash> nativeValues,geometryValues;
    Canon geometry(inventory),prefix(inventory,true);
    std::ifstream preload(preloadPath);if(!preload)throw std::runtime_error("preload unavailable");
    native_gather::Input input;
    while(native_gather::next(preload,C-2,input)){
        if(!input.expected||!input.value)throw std::runtime_error("preload needs positive closed expected=F");
        const State raw=native_gather::encode(input.graph);u64 a=0,b=0;
        const State oldKey=canonize(raw,&a),newKey=geometry.run(raw,&b);
        if(a!=b||!nativeValues.emplace(oldKey,input.value).second||!geometryValues.emplace(newKey,input.value).second)
            throw std::runtime_error("complete predecessor separation/stabilizer gate failed");
        budget.check();
    }
    if(nativeValues.size()!=16150||geometryValues.size()!=16150)
        throw std::runtime_error("need all16150 closed C5 L3 predecessors");
    std::ifstream targets(targetPath);if(!targets)throw std::runtime_error("target unavailable");
    std::unordered_set<State,native_gather::Hash> targetKeys;
    u64 queries=0,matchings=0,targetsChecked=0;u128 sumF=0,sumOrbitF=0;
    const auto started=Clock::now();
    while(native_gather::next(targets,C-1,input)){
        if(!input.expected||!input.value)throw std::runtime_error("target needs closed expected=F");
        const auto root=native_gather::pivot(input.graph);
        native_gather::Batch batch{input.graph,{}, {},0,100000,50000};
        batch.chosen[0]=root.bit;batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);
        if(batch.records!=root.frequency)throw std::runtime_error("root multiplicity mismatch");
        matchings+=batch.records;queries+=batch.raw.size();
        if(queries>maxQueries)throw std::runtime_error("BOUND value-chain queries");
        u128 totalNative=0,totalGeometry=0,totalPrefix=0;
        for(const auto& [raw,multiplicity]:batch.raw){
            u64 a=0,b=0,c=0;
            const State nkey=canonize(raw,&a),gkey=geometry.run(raw,&b),pkey=prefix.run(raw,&c);
            if(a!=b||a!=c||!(nkey==pkey))throw std::runtime_error("value-chain native-key/stabilizer differential");
            const auto n=nativeValues.find(nkey),g=geometryValues.find(gkey),p=nativeValues.find(pkey);
            if(n==nativeValues.end()||g==geometryValues.end()||p==nativeValues.end())
                throw std::runtime_error("value-chain missing predecessor");
            totalNative+=u128(multiplicity)*n->second;
            totalGeometry+=u128(multiplicity)*g->second;
            totalPrefix+=u128(multiplicity)*p->second;
        }
        totalNative*=C-1;totalGeometry*=C-1;totalPrefix*=C-1;
        if(totalNative!=input.value||totalGeometry!=input.value||totalPrefix!=input.value)
            throw std::runtime_error("closed target value differential failed");
        u64 stab=0;const State target=canonize(native_gather::encode(input.graph),&stab);
        if(!targetKeys.insert(target).second)throw std::runtime_error("duplicate target");
        sumF+=input.value;sumOrbitF+=(FACT[C]*(1u<<C)/stab)*input.value;
        ++targetsChecked;budget.check();
    }
    if(targetsChecked!=17120)throw std::runtime_error("need all17120 C5 L4 targets");
    std::cout<<"VALUE_CHAIN COMPLETE_C5_TWOMISSING predecessors=16150 targets="<<targetsChecked
             <<" labelled_matchings="<<matchings<<" weak_queries="<<queries
             <<" baseline_geometry_prefix_equal=YES per_query_stab_equal=YES closed_value_stab_histogram_equal=YES"
             <<" sumF4="<<native_gather::decimal_string(sumF)
             <<" sumOrbitF4="<<native_gather::decimal_string(sumOrbitF)
             <<" seconds="<<native_gather::seconds(started)<<" peak_bytes="<<budget.peak
             <<" concurrent_F4_window=YES N6=NOT_COMPUTED\n";
}

int run(int argc,char**argv) {
    if(argc<4)throw std::runtime_error("C degree input=PATH limit=N maxqueries=N maxseconds=S [invariance=N]");
    C=std::stoi(argv[1]);N2C=2*C;const int degree=std::stoi(argv[2]);
    std::string file,preloadFile;u64 limit=0,maxQueries=0,invariance=64,rounds=1;double maxSeconds=0;
    for(int i=3;i<argc;++i){
        const std::string option=argv[i];const auto p=option.find('=');
        if(p==std::string::npos)throw std::runtime_error("need option=value");
        const auto key=option.substr(0,p),value=option.substr(p+1);
        if(key=="input")file=value;else if(key=="limit")limit=std::stoull(value);
        else if(key=="maxqueries")maxQueries=std::stoull(value);
        else if(key=="maxseconds")maxSeconds=std::stod(value);
        else if(key=="invariance")invariance=std::stoull(value);
        else if(key=="preload")preloadFile=value;
        else if(key=="rounds")rounds=std::stoull(value);
        else throw std::runtime_error("unknown option");
    }
    if(C<2||C>6||(degree!=C-2&&degree!=C-1)||file.empty()||!limit||limit>20000||
       !maxQueries||maxQueries>(preloadFile.empty()?1000000ULL:50000000ULL)||
       !std::isfinite(maxSeconds)||maxSeconds<=0||maxSeconds>120||invariance>20000||!rounds||rounds>5)
        throw std::runtime_error("invalid positive bounds/domain");
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*i;
    g_wlseed=false;Budget budget{Clock::now(),maxSeconds,0};
    Inventory inventory;const auto setup=Clock::now();inventory.build(budget);
    const double setupSeconds=native_gather::seconds(setup);
    if(!preloadFile.empty()){
        if(C!=5||degree!=4||limit!=17120)throw std::runtime_error("complete C5 L3 -> L4 gate needs C5 degree4 limit17120");
        value_chain(inventory,preloadFile,file,maxQueries,budget);return 0;
    }
    std::vector<State> queries;std::ifstream stream(file);if(!stream)throw std::runtime_error("input unavailable");
    native_gather::Input input;u64 sources=0,matchings=0;
    const auto gather=Clock::now();
    while(sources<limit&&native_gather::next(stream,degree,input)){
        ++sources;
        if(degree==C-2)queries.push_back(native_gather::encode(input.graph));
        else {
            const auto root=native_gather::pivot(input.graph);
            native_gather::Batch batch{input.graph,{}, {},0,100000,50000};
            batch.chosen[0]=root.bit;batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);
            if(batch.records!=root.frequency)throw std::runtime_error("root frequency mismatch");
            matchings+=batch.records;
            for(const auto& [state,multiplicity]:batch.raw){(void)multiplicity;queries.push_back(state);}
        }
        if(queries.size()>maxQueries)throw std::runtime_error("BOUND queries");
        budget.check();
    }
    if(queries.empty())throw std::runtime_error("no queries");
    const double gatherSeconds=native_gather::seconds(gather);
    std::vector<State> native(queries.size()),special(queries.size());
    std::vector<u64> nativeStab(queries.size()),specialStab(queries.size());
    u64 nativeNodes=0,specialNodes=0,signatures=0,prefixNodes=0,prefixSignatures=0;
    double nativeSeconds=0,specialSeconds=0,prefixSeconds=0;
    Canon canon(inventory),prefixCanon(inventory,true);
    std::vector<State> prefixKeys(queries.size());std::vector<u64> prefixStab(queries.size());
    for(u64 round=0;round<rounds;++round)for(int position=0;position<3;++position){
        const int which=int((round+position)%3);const auto stamp=Clock::now();
        for(size_t i=0;i<queries.size();++i){
            if(which==0){const u64 previous=tl_canon_nodes;native[i]=canonize(queries[i],&nativeStab[i]);nativeNodes+=tl_canon_nodes-previous;}
            else if(which==1){special[i]=canon.run(queries[i],&specialStab[i]);specialNodes+=canon.nodes;signatures+=canon.signatures;}
            else {prefixKeys[i]=prefixCanon.run(queries[i],&prefixStab[i]);prefixNodes+=prefixCanon.nodes;prefixSignatures+=prefixCanon.signatures;}
            if(i%1024==0)budget.check();
        }
        const double elapsed=native_gather::seconds(stamp);
        if(which==0)nativeSeconds+=elapsed;else if(which==1)specialSeconds+=elapsed;else prefixSeconds+=elapsed;
    }
    std::unordered_map<State,State,native_gather::Hash> toNative,toSpecial;
    std::map<u64,u64> nativeHistogram,geometryHistogram,prefixHistogram;
    u64 unequalKeys=0;
    for(size_t i=0;i<queries.size();++i){
        if(nativeStab[i]!=specialStab[i])throw std::runtime_error("stabilizer differential failed");
        if(!(prefixKeys[i]==native[i])||prefixStab[i]!=nativeStab[i])throw std::runtime_error("native-compatible prefix differential failed");
        ++nativeHistogram[nativeStab[i]];++geometryHistogram[specialStab[i]];++prefixHistogram[prefixStab[i]];
        if(!(native[i]==special[i]))++unequalKeys;
        const auto [a,aa]=toNative.emplace(special[i],native[i]);
        const auto [b,bb]=toSpecial.emplace(native[i],special[i]);
        if((!aa&&!(a->second==native[i]))||(!bb&&!(b->second==special[i])))
            throw std::runtime_error("canonical separation differential failed");
        if(i%1024==0)budget.check();
    }
    if(nativeHistogram!=geometryHistogram||nativeHistogram!=prefixHistogram)
        throw std::runtime_error("stabilizer histogram differential failed");
    std::cout<<"STAB_HISTOGRAM baseline_geometry_prefix_equal=YES values=";
    u128 orbitMass=0;
    for(const auto& [s,count]:nativeHistogram){std::cout<<s<<':'<<count<<',';orbitMass+=u128(count)*FACT[C]*(1u<<C)/s;}
    std::cout<<" query_orbit_mass="<<native_gather::decimal_string(orbitMass)<<'\n';
    build_group();std::mt19937_64 rng(20260905);u64 transforms=0,fullReferees=0;
    for(size_t i=0;i<std::min<u64>(invariance,queries.size());++i){
        if(!(canon.run(special[i])==special[i]))throw std::runtime_error("idempotence failed");
        if(!(prefixCanon.run(native[i])==native[i]))throw std::runtime_error("native-prefix idempotence failed");
        for(size_t j=0;j<(C<=4?GROUP.size():8);++j){
            const State transformed=apply_state(GROUP[C<=4?j:(rng()%GROUP.size())],queries[i]);u64 stab=0;
            if(!(canon.run(transformed,&stab)==special[i])||stab!=specialStab[i])
                throw std::runtime_error("invariance failed");
            if(!(prefixCanon.run(transformed,&stab)==native[i])||stab!=nativeStab[i])
                throw std::runtime_error("native-prefix invariance failed");
            ++transforms;
        }
        if(C<=4||i<8){
            if(stab_scan(queries[i])!=specialStab[i]||!orbit_member_scan(queries[i],special[i]))
                throw std::runtime_error("full group referee failed");
            ++fullReferees;
        }
        budget.check();
    }
    budget.check();
    std::cout<<std::fixed<<std::setprecision(9)
             <<"SUMMARY domain=NEW_NATIVE_KEY_PROBE production_compatible=NO threads=1 sources="<<sources
             <<" queries="<<queries.size()<<" labelled_matchings="<<matchings<<" distinct="<<toNative.size()
             <<" interleaved_rounds="<<rounds
             <<" changed_keys="<<unequalKeys<<" stabilizers_checked="<<queries.size()
             <<" transforms="<<transforms<<" full_group_referees="<<fullReferees
             <<" setup_s="<<setupSeconds<<" gather_s="<<gatherSeconds
             <<" native_s="<<nativeSeconds<<" geometry_s="<<specialSeconds
             <<" native_nodes="<<nativeNodes<<" geometry_nodes="<<specialNodes
             <<" geometry_signatures="<<signatures<<" ratio="<<nativeSeconds/specialSeconds
             <<" compatible_prefix_s="<<prefixSeconds<<" compatible_prefix_nodes="<<prefixNodes
             <<" compatible_prefix_signatures="<<prefixSignatures<<" compatible_prefix_ratio="<<nativeSeconds/prefixSeconds
             <<" total_s="<<native_gather::seconds(budget.started)<<" peak_bytes="<<budget.peak
             <<" concurrent_F4_window=YES F=PROBE N=NOT_COMPUTED\n";
    return 0;
}
}
int main(int argc,char**argv){try{return missing_geometry_probe::run(argc,argv);}
catch(const std::exception&e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}}
