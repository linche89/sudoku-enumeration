// Isolated NVRTC device-only exact rooted degree-four split.
// One graph/thread; caller supplies bounded, zeroed workspaces.
// Output value is zero unless status==0 and the entire DFS is closed.
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;
struct Dsu {
    unsigned char p[24], size[24], hc[24], hr[24], hs[24];
    int h,components;
    __device__ void init(int n){h=0;components=n;for(int i=0;i<n;++i){p[i]=i;size[i]=1;}}
    __device__ int root(int a){while(p[a]!=a)a=p[a];return a;}
    __device__ void edge(int a,int b){
        a=root(a);b=root(b);if(a==b)return;
        if(size[a]<size[b]){int t=a;a=b;b=t;}
        hc[h]=b;hr[h]=a;hs[h]=size[a];++h;p[b]=a;size[a]+=size[b];--components;
    }
    __device__ void rollback(int snap){while(h>snap){--h;p[hc[h]]=hc[h];size[hr[h]]=hs[h];++components;}}
    __device__ void pair(int row,U16 bits,int n){while(bits){int c=__ffs(bits)-1;bits&=bits-1;edge(row,n+c);}}
};
extern "C" __global__ void rooted_f4(
    const U16* inputs,U64* results,unsigned char* marks,int* fronts,
    int graphs,int n,int stateCount,int frontierCap,U64 nodeCap,U64 clockCap) {
    const int id=blockIdx.x*blockDim.x+threadIdx.x;
    if(id>=graphs)return;
    const U64 begin=clock64();U64* out=results+6*id;
    for(int i=0;i<6;++i)out[i]=0;
    unsigned char* reach=marks+(U64)id*stateCount;
    int* current=fronts+(U64)id*2*frontierCap;
    int* next=current+frontierCap;
    U16 graph[12],pairs[12][6];int pairA[12][6],pairB[12][6],delta[12][6],power[12];
    power[0]=1;for(int i=1;i<n;++i)power[i]=3*power[i-1];
    for(int row=0;row<n;++row){
        graph[row]=inputs[id*12+row];int k=0;U16 a=graph[row];
        while(a){U16 first=a&-a;a^=first;U16 b=a;while(b){U16 second=b&-b;b^=second;
            if(k>=6){out[4]=5;return;}
            int x=__ffs(first)-1,y=__ffs(second)-1;
            pairs[row][k]=first|second;pairA[row][k]=x;pairB[row][k]=y;
            delta[row][k]=power[x]+power[y];++k;
        }}
        if(k!=6){out[4]=5;return;}
    }
    const U16 rootPair=pairs[0][0];const int rootState=delta[0][0];
    reach[rootState]=1;current[0]=rootState;int count=1;U64 dp=0;
    for(int row=1;row<n;++row){
        int nextCount=0;
        for(int j=0;j<count;++j){
            const int state=current[j];
            for(int k=0;k<6;++k){
                if(++dp>nodeCap){out[4]=1;out[3]=dp;out[5]=clock64()-begin;return;}
                if((dp&1023)==0&&clock64()-begin>clockCap){out[4]=3;out[3]=dp;out[5]=clock64()-begin;return;}
                if((state/power[pairA[row][k]])%3>=2||(state/power[pairB[row][k]])%3>=2)continue;
                const int child=state+delta[row][k];
                if(!reach[child]){
                    if(nextCount>=frontierCap){out[4]=2;out[3]=dp;out[5]=clock64()-begin;return;}
                    reach[child]=1;next[nextCount++]=child;
                }
            }
        }
        int* swap=current;current=next;next=swap;count=nextCount;
    }
    out[3]=dp;
    const int target=stateCount-1;
    if(!reach[target]){out[4]=5;return;}
    Dsu selected,complement;selected.init(2*n);complement.init(2*n);
    selected.pair(0,rootPair,n);complement.pair(0,graph[0]^rootPair,n);
    int states[12],cursor[12],snapS[12],snapC[12];
    int row=n-1;states[row]=target;cursor[row]=0;
    U64 nodes=1,leaves=0,value=0,iterations=0;
    while(true){
        if((++iterations&1023)==0&&clock64()-begin>clockCap){out[4]=3;break;}
        if(nodes>nodeCap){out[4]=1;break;}
        if(row==0){
            const U64 term=1ULL<<(selected.components+complement.components);
            if(value>~0ULL-term){out[4]=4;break;}
            value+=term;++leaves;++row;
            selected.rollback(snapS[row]);complement.rollback(snapC[row]);continue;
        }
        bool descended=false;
        while(cursor[row]<6){
            const int k=cursor[row]++,state=states[row];
            if((state/power[pairA[row][k]])%3==0||(state/power[pairB[row][k]])%3==0)continue;
            const int predecessor=state-delta[row][k];if(!reach[predecessor])continue;
            snapS[row]=selected.h;snapC[row]=complement.h;
            selected.pair(row,pairs[row][k],n);complement.pair(row,graph[row]^pairs[row][k],n);
            --row;states[row]=predecessor;cursor[row]=0;++nodes;descended=true;break;
        }
        if(descended)continue;
        if(row==n-1)break;
        ++row;selected.rollback(snapS[row]);complement.rollback(snapC[row]);
    }
    out[1]=leaves;out[2]=nodes;out[5]=clock64()-begin;
    if(out[4])return;
    if(value>~0ULL/6){out[4]=4;return;}
    out[0]=6*value;
}
