// big.hpp — minimal arbitrary-precision unsigned integer (base 1e9), just enough for this project:
// add, multiply-by-uint64, multiply big*big, compare, decimal I/O.  Correctness over speed; the
// counts are few, so this is never the bottleneck.
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

struct Big {
    static const uint32_t BASE = 1000000000u;   // 1e9
    std::vector<uint32_t> d;                      // little-endian base-1e9 limbs; empty == 0

    Big() {}
    Big(uint64_t x){ while(x){ d.push_back((uint32_t)(x%BASE)); x/=BASE; } }

    bool isZero() const { return d.empty(); }
    void trim(){ while(!d.empty() && d.back()==0) d.pop_back(); }

    Big& operator+=(const Big& o){
        uint64_t carry=0; size_t n=std::max(d.size(),o.d.size());
        d.resize(n,0);
        for(size_t i=0;i<n;++i){
            uint64_t s=carry + d[i] + (i<o.d.size()?o.d[i]:0);
            d[i]=(uint32_t)(s%BASE); carry=s/BASE;
        }
        if(carry) d.push_back((uint32_t)carry);
        return *this;
    }
    // this += other * m  (m fits in uint64)
    void addMul(const Big& o, uint64_t m){
        if(m==0||o.isZero()) return;
        // split m into base-1e9 to keep products in uint64 safely: o limb (<1e9) * m.
        // m can be up to ~1e18 (product of two state weights); do limb*m via 128-bit.
        std::vector<uint64_t> tmp(d.size(),0);
        if(tmp.size()<o.d.size()+3) tmp.resize(o.d.size()+3,0);
        for(size_t i=0;i<d.size();++i) tmp[i]=d[i];
        unsigned __int128 carry=0;
        for(size_t i=0;i<o.d.size()||carry;++i){
            if(i>=tmp.size()) tmp.push_back(0);
            unsigned __int128 cur=carry + (unsigned __int128)tmp[i];
            if(i<o.d.size()) cur += (unsigned __int128)o.d[i]*m;
            tmp[i]=(uint64_t)(cur%BASE); carry=cur/BASE;
        }
        d.assign(tmp.size(),0);
        for(size_t i=0;i<tmp.size();++i) d[i]=(uint32_t)tmp[i];
        trim();
    }
    bool operator==(const Big& o) const { return d==o.d; }
    std::string str() const {
        if(d.empty()) return "0";
        std::string s=std::to_string(d.back());
        for(int i=(int)d.size()-2;i>=0;--i){ std::string p=std::to_string(d[i]); s+=std::string(9-p.size(),'0')+p; }
        return s;
    }
    static Big fromDec(const std::string& s){
        Big r;
        for(char c:s){ if(c<'0'||c>'9')continue; r.addMul(r, 0); /*noop*/ }
        // proper: r = r*10 + digit
        r=Big();
        for(char c:s){ if(c<'0'||c>'9')continue; Big t=r; r=Big(); r.addMul(t,10); r+=Big((uint64_t)(c-'0')); }
        return r;
    }
};
