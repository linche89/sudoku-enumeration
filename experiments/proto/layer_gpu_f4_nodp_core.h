// Independent exact fixed-root two-factor DFS, no ternary DP/workspace.
// Portable host/device arithmetic; a caller guard supplies bounded timing.
#ifndef LAYER_GPU_F4_NODP_CORE_H
#define LAYER_GPU_F4_NODP_CORE_H
#if defined(__CUDACC__)
#define ND_HD __host__ __device__ __forceinline__
#else
#define ND_HD inline
#endif
namespace nodp {
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
struct Result {U64 value=0,leaves=0,nodes=0,iterations=0;U32 status=0;};
ND_HD int first(U32 x){
#if defined(__CUDA_ARCH__)
    return __ffs(x)-1;
#else
    return __builtin_ctz(x);
#endif
}
ND_HD int root(U64 parent,int x){
    while(int((parent>>(4*x))&15)!=x)x=int((parent>>(4*x))&15);
    return x;
}
ND_HD bool join(U64&parent,U16 pair){
    int a=root(parent,first(pair)),b=root(parent,first(pair&(pair-1)));
    if(a==b)return false;
    parent=(parent&~(15ULL<<(4*b)))|(U64(a)<<(4*b));return true;
}
ND_HD int cycleSum(const U16* graph,const U16* chosen,int n){
    U64 selected=0,complement=0;
    for(int i=0;i<n;++i)selected|=U64(i)<<(4*i);
    complement=selected;int components=2*n;
    for(int i=0;i<n;++i){components-=join(selected,chosen[i]);components-=join(complement,graph[i]^chosen[i]);}
    return components;
}
template<class Guard> ND_HD Result solve(const U16*graph,int n,U64 nodeCap,Guard&guard){
    Result out;
    if(n<4||n>12||!nodeCap){out.status=5;return out;}
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
    U16 once[13]={},twice[13]={},chosen[12]={};unsigned char cursor[13]={};
    chosen[0]=pairs[0][0];once[1]=chosen[0];int row=1;out.nodes=1;U64 sum=0;
    while(true){
        ++out.iterations;
        if((out.iterations&127)==0&&(out.status=guard())!=0)return out;
        if(out.nodes>nodeCap){out.status=1;return out;}
        if(row==n){
            if(once[row]||twice[row]!=full){out.status=5;return out;}
            U64 term=1ULL<<cycleSum(graph,chosen,n);
            if(sum>~0ULL-term){out.status=4;return out;}
            sum+=term;++out.leaves;--row;continue;
        }
        if(cursor[row]==6){if(row==1)break;--row;continue;}
        U16 pair=pairs[row][cursor[row]++];if(twice[row]&pair)continue;
        U16 nextOnce=once[row]^pair,nextTwice=twice[row]|(once[row]&pair);
        U16 zero=full^(nextOnce|nextTwice);
        if((zero&~remaining2[row+1])||(nextOnce&~remaining1[row+1]))continue;
        chosen[row]=pair;once[row+1]=nextOnce;twice[row+1]=nextTwice;cursor[row+1]=0;++row;++out.nodes;
    }
    if(sum>~0ULL/6){out.status=4;return out;}
    out.value=6*sum;return out;
}
}
#undef ND_HD
#endif
