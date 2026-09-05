// Readonly, fixed-domain C6 rehearsal-support loader regression. No F4 input,
// no chunks, no factorization calculation, and no production checkpoint write.
#define REVERSE_F5_NO_MAIN
#include "layer_reverse_f5.cpp"
#undef REVERSE_F5_NO_MAIN

int main(int argc,char** argv)try {
    if(argc!=3||std::string(argv[2])!="checkpointreadonly")
        throw std::runtime_error("usage: exact-C6-rehearsal-L5.snap checkpointreadonly");
    C=6;N2C=12;g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*u64(i);
    reverse_f5_run::Guard guard(120,6ULL<<30);
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(!GlobalMemoryStatusEx(&memory)||memory.ullAvailPhys<(8ULL<<30))
        throw std::runtime_error("support smoke requires8GiB free RAM");
    reverse_f5_run::Options options;options.support=argv[1];options.threads=24;options.readonly=true;
    auto result=reverse_f5_run::load_support(options);
    u64 nonzero=0;
    for(u64 value:result.layer.T)nonzero+=value!=0;
    if(nonzero||result.layer.size()!=96452976||result.layer.real_size()!=96452755||
        result.layer.stab[96452974]!=1440||result.layer.stab[96452975]!=240||
        result.mass!=4439972139072ULL||!result.repairHash||g_rehearsalDenom||
        result.source.configHash==ck_config_hash(5)||!result.layer.table.empty())
        throw std::runtime_error("readonly support-repair smoke invariant failed");
    reverse_f5_run::unchanged(result);
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("cannot read support smoke RSS");
    std::cout<<"[OK] C6_L5_SUPPORT_ONLY entries="<<result.layer.size()<<" holes="<<result.layer.holes
             <<" real="<<result.layer.real_size()<<" old_T_all_erased=yes repairs_at=96452974,96452975"
             <<" repaired_mass="<<result.mass<<" repair_hash="<<result.repairHash
             <<" source_sha256="<<result.sha256<<" source_header_hash="<<result.source.headerHash
             <<" rehearsal_config_read=100 runtime_rehearsal_denom=0 L5_index_released=yes"
             <<" peak_rss_bytes="<<pm.PeakWorkingSetSize<<" checkpointreadonly=yes file_writes=none F5=NOT_COMPUTED\n";
    return 0;
}catch(const std::exception& error){std::fprintf(stderr,"ERROR: %s\n",error.what());return 1;}
