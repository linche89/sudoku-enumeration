#pragma once

// Allocation-free version of invRefineSig().  The old scalarsig path used
// std::string for every first-seen raw target; this key keeps the same invariant
// bytes in a fixed buffer so the hot classifier can avoid allocator traffic.
struct SigKey {
    uint16_t len=0;
    std::array<unsigned char,192> bytes{};
    bool operator==(const SigKey& o) const {
        return len==o.len && std::memcmp(bytes.data(),o.bytes.data(),len)==0;
    }
};

struct SigKeyHash {
    size_t operator()(const SigKey& k) const {
        uint64_t h=1469598103934665603ull;
        h^=k.len; h*=1099511628211ull;
        for(int i=0;i<k.len;++i){ h^=k.bytes[(size_t)i]; h*=1099511628211ull; }
        return (size_t)h;
    }
};

static bool sigKeyLess(const SigKey& a, const SigKey& b){
    if(a.len!=b.len) return a.len<b.len;
    return std::memcmp(a.bytes.data(),b.bytes.data(),a.len)<0;
}

static void refineSigKeyBranch(const uint16_t* pairs, SigKey& out){
    int xcol[8], ycol[8];
    refineColours(pairs,xcol,ycol);
    std::array<std::array<unsigned char,16>,MAXM> toks{};
    for(int i=0;i<M;++i){
        int xm=pairs[i]&FULLC, ym=(pairs[i]>>C)&FULLC;
        for(int b=xm;b;b&=b-1) toks[(size_t)i][(size_t)xcol[__builtin_ctz(b)]]++;
        for(int b=ym;b;b&=b-1) toks[(size_t)i][(size_t)(C+ycol[__builtin_ctz(b)])]++;
    }
    std::sort(toks.begin(),toks.begin()+M);
    out.len=(uint16_t)(M*2*C);
    int p=0;
    for(int i=0;i<M;++i)
        for(int j=0;j<2*C;++j)
            out.bytes[(size_t)p++]=toks[(size_t)i][(size_t)j];
}

__attribute__((noinline)) static SigKey invRefineSigKey(const Key& k){
    uint16_t orig[MAXM], sw[MAXM];
    for(int i=0;i<M;++i){ orig[i]=k[i]; sw[i]=swapPair(k[i]); }
    SigKey a,b;
    refineSigKeyBranch(orig,a);
    refineSigKeyBranch(sw,b);
    return sigKeyLess(b,a)?b:a;
}
