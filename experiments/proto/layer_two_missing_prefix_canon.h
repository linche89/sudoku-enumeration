// Compatible maximum-missing-edge prefix, with NO geometry inventory/trie.
// Include after the unchanged native layer engine in one translation unit.
// Proof: docs/math/two-missing-canonical-prefix.md, Lemma A. Every globally
// minimizing unseeded column path begins with a maximum-multiplicity missing
// edge. Keep all such ordered pairs, both flips, and every later box choice.
#ifndef FJ_TWO_MISSING_PREFIX_CANON_H
#define FJ_TWO_MISSING_PREFIX_CANON_H

namespace two_missing_prefix {
inline thread_local u64 dispatched=0, fallbacks=0;

struct Canon {
    u32 neighbors[6]{}, endpoints=0;
    u32 z0[6]{},z2[6][2]{},z3[6][2]{},bestSig[6]{};
    bool bestSet[6]{},haveFinal=false;
    u8 path[6]{},bestPath[6]{};
    u64 nodes=0,signatures=0,stabilizer=0;

    bool prepare(const State& s) {
        if(C<2||C>6||N2C!=2*C||g_wlseed)return false;
        const u32 low=0x555u & ((1u<<N2C)-1), high=low<<1;
        for(int i=0;i<N2C;++i) {
            const u32 w=s.m[i];
            if((w>>N2C) || (w&low&~(w>>1)) ||
               __builtin_popcount(w&high)!=C-2)return false;
        }
        for(int i=N2C;i<12;++i)if(s.m[i])return false;
        for(int b=0;b<C;++b) {
            u32 a0=0,a2=0,a3=0;
            for(int i=0;i<N2C;++i) {
                const int value=fld(s.m[i],b);
                if(!value)a0|=1u<<i;
                else if(value==2)a2|=1u<<i;
                else a3|=1u<<i;
            }
            if(__builtin_popcount(a0)!=4 || __builtin_popcount(a2)!=C-2 ||
               __builtin_popcount(a3)!=C-2)return false;
            z0[b]=a0;z2[b][0]=a2;z2[b][1]=a3;z3[b][0]=a3;z3[b][1]=a2;
        }
        int maximum=-1;endpoints=0;
        std::fill(neighbors,neighbors+6,0);
        for(int b=0;b<C;++b)for(int c=b+1;c<C;++c) {
            const int m=__builtin_popcount(z0[b]&z0[c]);
            if(m>maximum) {maximum=m;endpoints=0;std::fill(neighbors,neighbors+6,0);}
            if(m==maximum) {endpoints|=(1u<<b)|(1u<<c);neighbors[b]|=1u<<c;neighbors[c]|=1u<<b;}
        }
        return true;
    }
    bool allowed(int depth,u32 used,int b,int firstBox=-1)const {
        if((used>>b)&1)return false;
        if(depth==0)return (endpoints>>b)&1;
        if(depth==1)return (neighbors[firstBox>=0?firstBox:(path[0]>>1)]>>b)&1;
        return true;
    }
    u32 sig(int b,int f,const u32* groups,int ng) {
        ++signatures;u32 result=0;
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
    void dfs(int depth,const u32* groups,int ng,u32 used) {
        ++nodes;
        if(depth==C) {
            if(!haveFinal){std::memcpy(bestPath,path,6);haveFinal=true;stabilizer=1;}
            else ++stabilizer;
            return;
        }
        u8 candidates[12]{};int count=0;u32 minimum=UINT32_MAX;
        for(int b=0;b<C;++b)if(allowed(depth,used,b))for(int f=0;f<2;++f) {
            const u32 value=sig(b,f,groups,ng);
            if(value<minimum){minimum=value;count=0;}
            if(value==minimum)candidates[count++]=u8(2*b+f);
        }
        if(!count)throw std::runtime_error("empty compatible canonical prefix");
        if(bestSet[depth]) {
            if(minimum>bestSig[depth])return;
            if(minimum<bestSig[depth]) {
                bestSig[depth]=minimum;
                for(int d=depth+1;d<C;++d)bestSet[d]=false;
                haveFinal=false;stabilizer=0;
            }
        }else {bestSet[depth]=true;bestSig[depth]=minimum;}
        u32 refined[12][12]{},look[12]{};int sizes[12]{},order[12]{};
        for(int k=0;k<count;++k) {
            const int b=candidates[k]>>1,f=candidates[k]&1;int n=0;
            for(int i=0;i<ng;++i) {
                const u32 a0=groups[i]&z0[b],a2=groups[i]&z2[b][f],a3=groups[i]&z3[b][f];
                if(a0)refined[k][n++]=a0;
                if(a2)refined[k][n++]=a2;
                if(a3)refined[k][n++]=a3;
            }
            sizes[k]=n;order[k]=k;
            if(count>1&&depth+1<C) {
                u32 nextMin=UINT32_MAX;const u32 next=used|(1u<<b);
                for(int bb=0;bb<C;++bb)if(allowed(depth+1,next,bb,depth==0?b:-1))
                    for(int ff=0;ff<2;++ff)nextMin=std::min(nextMin,sig(bb,ff,refined[k],n));
                look[k]=nextMin;
            }
        }
        if(count>1)std::sort(order,order+count,[&](int a,int b){return look[a]<look[b];});
        for(int kk=0;kk<count;++kk) {
            const int k=order[kk];path[depth]=candidates[k];
            dfs(depth+1,refined[k],sizes[k],used|(1u<<(candidates[k]>>1)));
        }
    }
    State prepared(const State& s,u64* aut=nullptr) {
        nodes=signatures=stabilizer=0;haveFinal=false;
        std::fill(bestSet,bestSet+6,false);
        const u32 group=(1u<<N2C)-1;dfs(0,&group,1,0);
        if(!haveFinal||!stabilizer)throw std::runtime_error("no compatible canonical output");
        GElem g{};
        for(int d=0;d<C;++d) {
            const int b=bestPath[d]>>1;
            g.perm[b]=u8(d);g.flips|=u8((bestPath[d]&1)<<b);
        }
        if(aut)*aut=stabilizer;
        return apply_state(g,s);
    }
    State strict(const State& s,u64* aut=nullptr) {
        if(!prepare(s))throw std::runtime_error("maximum-missing-edge canonicalizer requires unseeded balanced C2..6 two-missing state");
        return prepared(s,aut);
    }
};

// Fallback preserves every existing one-missing/seeded/other-layer convention.
// The narrow dispatch is checked per input, not inferred from a caller's name.
inline State canonicalize(const State& s,u64* aut=nullptr) {
    Canon worker;
    if(!worker.prepare(s)) {++fallbacks;return ::canonize(s,aut);}
    const State result=worker.prepared(s,aut);
    ++dispatched;++tl_canon_calls;tl_canon_nodes+=worker.nodes;
    return result;
}
} // namespace two_missing_prefix
#endif
