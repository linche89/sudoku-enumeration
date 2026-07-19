#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

std::uint64_t choose_count(int n,int k){std::uint64_t r=1;for(int i=1;i<=k;i++)r=r*(n-k+i)/i;return r;}

int components_2regular(const std::vector<std::vector<int>>& adj) {
    const int n=adj.size();
    std::vector<int> seenL(n),seenR(n);
    int comps=0;
    for(int s=0;s<n;s++) if(!seenL[s]) {
        comps++;
        std::vector<std::pair<int,int>> stack{{0,s}};
        seenL[s]=1;
        while(!stack.empty()) {
            auto [side,v]=stack.back();stack.pop_back();
            if(side==0){for(int j:adj[v])if(!seenR[j]){seenR[j]=1;stack.push_back({1,j});}}
            else {for(int i=0;i<n;i++)for(int j:adj[i])if(j==v&&!seenL[i]){seenL[i]=1;stack.push_back({0,i});}}
        }
    }
    return comps;
}

std::uint64_t F3(const std::vector<std::vector<int>>& adj) {
    const int n=adj.size();
    std::vector<int> match(n,-1),usedR(n);
    std::uint64_t total=0;
    auto rec=[&](auto&&self,int i)->void{
        if(i==n){
            std::vector<std::vector<int>> residual(n);
            for(int u=0;u<n;u++)for(int v:adj[u])if(v!=match[u])residual[u].push_back(v);
            total += 1ULL << components_2regular(residual);
            return;
        }
        for(int v:adj[i])if(!usedR[v]){usedR[v]=1;match[i]=v;self(self,i+1);usedR[v]=0;}
    };
    rec(rec,0);
    return total;
}

int main(){
    constexpr int C=3,N=2*C;
    std::vector<int> masks;
    for(int m=0;m<(1<<N);m++)if(__builtin_popcount((unsigned)m)==C)masks.push_back(m);
    std::uint64_t skeletons=0,sum=0;
    for(int m0:masks)for(int m1:masks)for(int m2:masks){
        int rows[C]={m0,m1,m2};
        std::vector<std::vector<int>> adj(N);
        for(int r=0;r<C;r++)for(int j=0;j<N;j++){
            int eps=(rows[r]>>j)&1;
            adj[2*r+eps].push_back(j);
        }
        sum += F3(adj);
        skeletons++;
    }
    std::cout<<"C=3 labeled_balanced_skeletons="<<skeletons<<" sum_F3="<<sum<<"\n";
}
