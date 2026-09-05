// Isolated third arithmetic variant. Frozen nodp helpers/core are unchanged.
// Each depth owns packed right-side DSUs for exactly its selected row prefix.
#ifndef LAYER_CPU_F4_INCREMENTAL_CORE_H
#define LAYER_CPU_F4_INCREMENTAL_CORE_H
#include "layer_gpu_f4_nodp_core.h"
namespace incremental_nodp {
using nodp::U16;using nodp::U32;using nodp::U64;using nodp::Result;
template<class Guard> inline Result solve(const U16*graph,int n,U64 nodeCap,Guard&guard){
    Result out;if(n<4||n>12||!nodeCap){out.status=5;return out;}
    if((out.status=guard())!=0)return out;
    const U16 full=U16((1u<<n)-1);
    U16 pairs[12][6],remaining1[13]={},remaining2[13]={};
    for(int row=0;row<n;++row){
        if(graph[row]&~full){out.status=5;return out;}
        U16 a=graph[row];int k=0;
        while(a){U16 low=U16(a&-a);a^=low;U16 b=a;
            while(b){U16 next=U16(b&-b);b^=next;
                if(k>=6){out.status=5;return out;}pairs[row][k++]=low|next;}}
        if(k!=6){out.status=5;return out;}
    }
    for(int column=0;column<n;++column){int degree=0;for(int row=0;row<n;++row)degree+=(graph[row]>>column)&1;
        if(degree!=4){out.status=5;return out;}}
    for(int row=n-1;row>=0;--row){
        remaining1[row]=remaining1[row+1]|graph[row];
        remaining2[row]=remaining2[row+1]|(remaining1[row+1]&graph[row]);
    }
    U16 once[13]={},twice[13]={};unsigned char cursor[13]={},components[13]={};
    U64 selected[13]={},complement[13]={},identity=0;
    for(int i=0;i<n;++i)identity|=U64(i)<<(4*i);
    selected[1]=complement[1]=identity;once[1]=pairs[0][0];
    components[1]=static_cast<unsigned char>(2*n-nodp::join(selected[1],pairs[0][0])-
                                            nodp::join(complement[1],graph[0]^pairs[0][0]));
    int row=1;out.nodes=1;U64 sum=0;
    while(true){
        ++out.iterations;
        if((out.iterations&127)==0&&(out.status=guard())!=0)return out;
        if(out.nodes>nodeCap){out.status=1;return out;}
        if(row==n){
            if(once[row]||twice[row]!=full){out.status=5;return out;}
            U64 term=1ULL<<components[row];
            if(sum>~0ULL-term){out.status=4;return out;}
            sum+=term;++out.leaves;--row;continue;
        }
        if(cursor[row]==6){if(row==1)break;--row;continue;}
        U16 pair=pairs[row][cursor[row]++];if(twice[row]&pair)continue;
        U16 nextOnce=once[row]^pair,nextTwice=twice[row]|(once[row]&pair);
        U16 zero=full^(nextOnce|nextTwice);
        if((zero&~remaining2[row+1])||(nextOnce&~remaining1[row+1]))continue;
        // Copy parent prefix words BEFORE joining this row. Siblings never mutate them.
        selected[row+1]=selected[row];complement[row+1]=complement[row];
        components[row+1]=static_cast<unsigned char>(components[row]-nodp::join(selected[row+1],pair)-
                                                    nodp::join(complement[row+1],graph[row]^pair));
        once[row+1]=nextOnce;twice[row+1]=nextTwice;cursor[row+1]=0;++row;++out.nodes;
    }
    if(sum>~0ULL/6){out.status=4;return out;}
    out.value=6*sum;return out;
}
}
#endif
