// combinatorial3.cpp — M9: fully self-contained combinatorial count of N0.
//
// Everything here is built from B(sigma) alone -- no band catalogue, no 44/71
// class reduction, no mult_i, not even ed44.txt.  We use only the identity
//
//   N = sum over all band-signature triples (s1,s2,s3) that column-partition
//         of B(s1) B(s2) B(s3)
//     = 1680 * sum over (b2,b3) in 1680^2 of B(P,b2,b3) * C(P,b2,b3),
//
// fixing box1 to the standard partition P (relabelling acts transitively on the
// 1680 box-partitions, and B,C are relabel-invariant, giving the factor 1680).
//
//   B(sigma)  = sum over row0-transversals of 2^(#cycles of residual graph),
//               reduced to a dense 280x280 shape table Btab.
//   C(P,b2,b3)= sum over column-compatible (sigma2,sigma3) of B B, and it is
//               invariant under column permutation / box swap, so it depends
//               only on (shape(b2), shape(b3)) -> memoised by that pair.
//
// Conclusion this proves: the 44 equivalence classes are merely a memoisation
// of C, not a logical necessity; N0 follows from B(sigma) and symmetry alone.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <chrono>

using Box = std::array<int,3>;

static std::vector<Box> enumerate_boxes() {
    std::vector<Box> out;
    for (int c0 = 0; c0 < 512; ++c0) {
        if (__builtin_popcount(c0) != 3) continue;
        int rest = 0x1FF ^ c0;
        for (int c1 = rest; c1; c1 = (c1 - 1) & rest)
            if (__builtin_popcount(c1) == 3) out.push_back({c0, c1, rest ^ c1});
    }
    return out;
}
static inline int boxkey(const Box& b){ return (b[0]<<18)|(b[1]<<9)|b[2]; }

// ---- B(P, shapeA, shapeB) -------------------------------------------------
static int64_t g_total; static int g_T[9], g_r0[9];
static int cycles_of_residual() {
    int par[18]; for (int i=0;i<18;i++) par[i]=i;
    auto find=[&](int x){ while(par[x]!=x){par[x]=par[par[x]];x=par[x];} return x; };
    int comps=18;
    for (int j=0;j<9;j++) for (int m=g_T[j]^g_r0[j]; m; m&=m-1){
        int d=__builtin_ctz(m),a=find(j),b=find(9+d); if(a!=b){par[a]=b;--comps;} }
    return comps;
}
static void row0_dfs(int j,int used){
    if(j==9){ g_total += (int64_t)1<<cycles_of_residual(); return; }
    for(int m=g_T[j];m;m&=m-1){ int d0=m&-m; if(used&d0)continue; g_r0[j]=d0; row0_dfs(j+1,used|d0); }
}
static int64_t Bcompute(const Box& a,const Box& b){
    int P0=(1<<0)|(1<<3)|(1<<6),P1=(1<<1)|(1<<4)|(1<<7),P2=(1<<2)|(1<<5)|(1<<8);
    g_T[0]=P0;g_T[1]=P1;g_T[2]=P2; g_T[3]=a[0];g_T[4]=a[1];g_T[5]=a[2]; g_T[6]=b[0];g_T[7]=b[1];g_T[8]=b[2];
    g_total=0; row0_dfs(0,0); return g_total;
}

int main(){
    auto t0=std::chrono::steady_clock::now();
    auto boxes=enumerate_boxes(); const int NB=(int)boxes.size();        // 1680
    std::unordered_map<int,int> bidx; bidx.reserve(NB*2);
    for(int i=0;i<NB;i++) bidx[boxkey(boxes[i])]=i;

    // shapes (column-sorted box-partitions), shapeOf[box], a rep index per shape
    std::vector<Box> shapes; std::unordered_map<int,int> shapeId;
    std::vector<int> shapeOf(NB), shapeRep;
    for(int i=0;i<NB;i++){ Box s=boxes[i]; std::sort(s.begin(),s.end()); int k=boxkey(s);
        auto it=shapeId.find(k); int id;
        if(it!=shapeId.end()) id=it->second; else { id=(int)shapes.size(); shapeId[k]=id; shapes.push_back(s); shapeRep.push_back(i); }
        shapeOf[i]=id; }
    const int NS=(int)shapes.size();                                     // 280

    // Btab[a*NS+b] = #bands (P, shape a, shape b)
    std::vector<int64_t> Btab((size_t)NS*NS,0);
    for(int a=0;a<NS;a++) for(int b=a;b<NS;b++){ int64_t v=Bcompute(shapes[a],shapes[b]); Btab[(size_t)a*NS+b]=v; Btab[(size_t)b*NS+a]=v; }

    // compatibility + complement indices (per box)
    std::vector<std::vector<int>> compat(NB), compl_(NB);
    for(int i=0;i<NB;i++) for(int j=0;j<NB;j++)
        if(!(boxes[i][0]&boxes[j][0])&&!(boxes[i][1]&boxes[j][1])&&!(boxes[i][2]&boxes[j][2])){
            compat[i].push_back(j);
            Box w={0x1FF^boxes[i][0]^boxes[j][0],0x1FF^boxes[i][1]^boxes[j][1],0x1FF^boxes[i][2]^boxes[j][2]};
            compl_[i].push_back(bidx[boxkey(w)]); }
    Box P={(1<<0)|(1<<3)|(1<<6),(1<<1)|(1<<4)|(1<<7),(1<<2)|(1<<5)|(1<<8)};
    int Pidx=bidx[boxkey(P)];

    // rel[anchor*NB+box] = shape id after relabelling anchor->P (only the anchors
    // used by computeC: compat(P) and compl(P))
    std::vector<uint16_t> rel((size_t)NB*NB);
    std::vector<char> need(NB,0);
    for(int a:compat[Pidx]) need[a]=1; for(int a:compl_[Pidx]) need[a]=1;
    for(int i=0;i<NB;i++){ if(!need[i])continue; Box s=boxes[i]; std::sort(s.begin(),s.end());
        int map[9]; for(int k=0;k<3;k++){int col=s[k],t=0;for(int x=col;x;x&=x-1)map[__builtin_ctz(x)]=k+3*t++;}
        uint16_t* row=&rel[(size_t)i*NB];
        for(int j=0;j<NB;j++){ Box r; for(int c=0;c<3;c++){int v=0;for(int x=boxes[j][c];x;x&=x-1)v|=1<<map[__builtin_ctz(x)];r[c]=v;} std::sort(r.begin(),r.end()); row[j]=(uint16_t)shapeId[boxkey(r)]; } }

    // C(P, box b2, box b3) = sum over compatible (sigma2,sigma3) of B*B
    auto computeC=[&](int b2,int b3)->int64_t{
        const auto& C1=compat[Pidx]; const auto& W1=compl_[Pidx];
        const auto& C2=compat[b2];   const auto& W2=compl_[b2];
        const auto& C3=compat[b3];   const auto& W3=compl_[b3];
        const int64_t* B=Btab.data(); int64_t total=0;
        for(size_t i1=0;i1<C1.size();++i1){ const uint16_t* rU=&rel[(size_t)C1[i1]*NB]; const uint16_t* rW=&rel[(size_t)W1[i1]*NB];
            for(size_t i2=0;i2<C2.size();++i2){ const int64_t* BU=&B[(size_t)rU[C2[i2]]*NS]; const int64_t* BW=&B[(size_t)rW[W2[i2]]*NS]; int64_t acc=0;
                for(size_t i3=0;i3<C3.size();++i3) acc += BU[rU[C3[i3]]]*BW[rW[W3[i3]]];
                total+=acc; } }
        return total;
    };

    // ---- the fully self-contained sum --------------------------------------
    // N = 1680 * sum_{b2,b3 in 1680^2} B(P,b2,b3) * C(P,b2,b3)
    // C depends only on (shape(b2),shape(b3)) -> memoise by that pair.
    std::vector<int64_t> Cmemo((size_t)NS*NS,-1);
    unsigned __int128 acc=0;
    int Cdone=0;
    for(int j2=0;j2<NB;j2++){
        int s2=shapeOf[j2];
        for(int j3=0;j3<NB;j3++){
            int s3=shapeOf[j3];
            int64_t Bval=Btab[(size_t)s2*NS+s3];
            if(!Bval) continue;                                  // non-realisable
            int ca=s2<=s3?s2:s3, cb=s2<=s3?s3:s2;
            int64_t& C=Cmemo[(size_t)ca*NS+cb];
            if(C<0){ C=computeC(shapeRep[ca],shapeRep[cb]); ++Cdone; }
            acc += (unsigned __int128)(uint64_t)Bval * (uint64_t)C;
        }
    }
    unsigned __int128 N0=(unsigned __int128)1680u*acc;
    auto u128=[](unsigned __int128 x){std::string s;if(!x)s="0";while(x){s=char('0'+(int)(x%10))+s;x/=10;}return s;};
    double secs=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::printf("fully self-contained combinatorial count (B(sigma) only):\n");
    std::printf("  sum_{b2,b3} B*C (= N0/1680) = %s\n", u128(acc).c_str());
    std::printf("  N0 = 1680 * that           = %s\n", u128(N0).c_str());
    std::printf("  expected                    = 6670903752021072936960  [%s]\n",
                u128(N0)=="6670903752021072936960"?"OK":"MISMATCH");
    std::printf("  distinct C computed = %d   time = %.3f s\n", Cdone, secs);
    return 0;
}
