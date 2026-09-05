// Bounded CPU execution of the new no-DP arithmetic, using retained oracle CSV.
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "layer_gpu_f4_nodp_core.h"
using nodp::U64;
static void need(bool x,const std::string&s){if(!x)throw std::runtime_error(s);}
struct Guard {std::chrono::steady_clock::time_point end;unsigned operator()(){return std::chrono::steady_clock::now()>end?3:0;}};
int main(int argc,char**argv){try{
    std::map<std::string,std::string>args;
    for(int i=1;i<argc;++i){std::string s=argv[i];auto eq=s.find('=');need(eq!=std::string::npos,"key=value required");need(args.emplace(s.substr(0,eq),s.substr(eq+1)).second,"duplicate option");}
    need(args.size()==5&&args.count("C")&&args.count("input")&&args.count("oracle")&&args.count("limit")&&args.count("maxseconds"),"C,input,oracle,limit,maxseconds required");
    int C=std::stoi(args.at("C")),limit=std::stoi(args.at("limit")),seconds=std::stoi(args.at("maxseconds"));
    need(C>=4&&C<=6&&limit>0&&limit<=26&&seconds>0&&seconds<=30,"bounded tiny gate only");
    std::ifstream input(args.at("input")),reference(args.at("oracle"));need(bool(input)&&bool(reference),"input/oracle open failure");
    std::map<int,std::array<U64,3>>oracle;std::string line;
    while(std::getline(reference,line)){if(line.empty()||!std::isdigit(static_cast<unsigned char>(line[0]))||line.find(',')==std::string::npos)continue;
        std::replace(line.begin(),line.end(),',',' ');std::istringstream in(line);int i;U64 f,l,n;std::string extra;
        need(bool(in>>i>>f>>l>>n)&&!(in>>extra)&&oracle.emplace(i,std::array<U64,3>{f,l,n}).second,"invalid oracle CSV");}
    auto started=std::chrono::steady_clock::now();Guard guard{started+std::chrono::seconds(seconds)};
    int count=0;U64 values=0,leaves=0,nodes=0,iterations=0;
    std::cout<<"row,F,leaves,new_nodes,new_iterations,oracle_nodes,seconds\n";
    while(count<limit&&std::getline(input,line)){
        line=line.substr(0,line.find('#'));std::istringstream in(line);in>>std::ws;if(in.eof())continue;
        nodp::U16 graph[12]={};for(int i=0;i<2*C;++i){unsigned v;need(bool(in>>v)&&v<(1u<<(2*C)),"invalid mask");graph[i]=nodp::U16(v);}
        std::string token;U64 expected=0;if(in>>token){need(token.rfind("expected=",0)==0,"unexpected input suffix");expected=std::stoull(token.substr(9));need(!(in>>token),"extra input suffix");}
        std::sort(graph,graph+2*C);auto stamp=std::chrono::steady_clock::now();auto out=nodp::solve(graph,2*C,2000000,guard);double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-stamp).count();
        ++count;need(out.status==0,"bounded no-DP DFS did not close, status="+std::to_string(out.status));need(oracle.count(count),"missing oracle row");
        auto gold=oracle.at(count);need(out.value==gold[0]&&out.leaves==gold[1]&&(!expected||expected==out.value),"exact F4/leaf differential");
        values+=out.value;leaves+=out.leaves;nodes+=out.nodes;iterations+=out.iterations;
        std::cout<<count<<','<<out.value<<','<<out.leaves<<','<<out.nodes<<','<<out.iterations<<','<<gold[2]<<','<<std::fixed<<std::setprecision(9)<<elapsed<<'\n';
    }
    need(count==limit,"input shorter than requested positive limit");
    std::cout<<"SUMMARY status=EXACT_TINY_GATE rows="<<count<<" F4="<<values<<" leaves="<<leaves<<" new_nodes="<<nodes<<" new_iterations="<<iterations<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<" N6=NOT_COMPUTED\n";
    std::cout<<"[OK] same fixed-root F4 and valid-leaf inventory; no oracle reevaluation\n";return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
