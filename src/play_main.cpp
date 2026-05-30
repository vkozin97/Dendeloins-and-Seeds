#include <bit>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "oracle.h"

struct PlayNode { State state; Phase phase=Phase::P1_TO_PLANT; std::optional<int> last_planted_cell; };

static const char* res(PlayerResult r){ return r==PlayerResult::P1?"P1":(r==PlayerResult::P2?"P2":"UNKNOWN"); }
static const char* dname(int d){ static const char* n[8]={"E","W","N","S","NE","NW","SE","SW"}; return n[d]; }

void printState(const PlayNode& n, const SolverOracle& o){ const auto& g=o.geometry(); std::cout<<"round="<<roundIndex(n.state)<<" phase="<<(n.phase==Phase::P1_TO_PLANT?"P1_TO_PLANT":"P2_TO_BLOW")<<"\n"; for(int y=g.size()-1;y>=0;--y){ std::cout<<"y="<<y<<"  "; for(int x=0;x<g.size();++x){ int c=g.cellId(x,y); char ch='.'; if((n.state.occupied>>c)&1ULL) ch='*'; if((n.state.dandelions>>c)&1ULL) ch='X'; std::cout<<ch<<' '; } std::cout<<"\n";} std::cout<<"    "; for(int x=0;x<g.size();++x) std::cout<<x<<' '; std::cout<<"\nused_dirs="; for(int d=0;d<8;++d) if(n.state.used_dirs&(1u<<d)) std::cout<<dname(d)<<' '; std::cout<<"\n"; auto v=(n.phase==Phase::P1_TO_PLANT)?o.evaluateP1ToMoveState(n.state):o.evaluateP2ToBlowState(n.state); std::cout<<"current value="<<res(v)<<"\n"; }

int main(int argc, char** argv){ int n=-1,k=-1; std::string memo_path; bool autosave=false, save_on_exit=false; uint64_t max_states=0; double time_limit=0.0; for(int i=1;i<argc;++i){ std::string a=argv[i]; if(a=="--n"&&i+1<argc) n=std::stoi(argv[++i]); else if(a=="--k"&&i+1<argc) k=std::stoi(argv[++i]); else if(a=="--memo"&&i+1<argc) memo_path=argv[++i]; else if(a=="--autosave") autosave=true; else if(a=="--save-on-exit") save_on_exit=true; else if(a=="--max-states"&&i+1<argc) max_states=std::stoull(argv[++i]); else if(a=="--time-limit-sec"&&i+1<argc) time_limit=std::stod(argv[++i]); }
 if(n<=0||k<=0){ std::cerr<<"Usage: --n N --k K --memo path [--autosave] [--save-on-exit]\n"; return 1; }
 SolverConfig cfg{n,k,true,true,true,max_states,time_limit}; SolverOracle oracle(cfg); if(!memo_path.empty() && std::filesystem::exists(memo_path)){ std::string err; if(!oracle.loadMemo(memo_path,err)) std::cerr<<"memo load failed: "<<err<<"\n"; else std::cout<<"memo loaded: "<<memo_path<<"\n"; }
 std::vector<PlayNode> hist; hist.push_back({State{},Phase::P1_TO_PLANT,std::nullopt}); printState(hist.back(),oracle);
 std::string line; while(true){ std::cout<<"> "; if(!std::getline(std::cin,line)) break; std::istringstream is(line); std::string cmd; is>>cmd; if(cmd.empty()) continue; if(cmd=="q"||cmd=="quit") break; if(cmd=="help"){ std::cout<<"help state moves play <...> back save quit\n"; continue; }
 if(cmd=="state"||cmd=="s"){ printState(hist.back(),oracle); continue; }
 if(cmd=="back"||cmd=="b"){ if(hist.size()>1) hist.pop_back(); printState(hist.back(),oracle); continue; }
 if(cmd=="save"){ if(memo_path.empty()){ std::cout<<"--memo not set\n"; continue;} if(oracle.saveMemo(memo_path,true,255)) std::cout<<"saved\n"; else std::cout<<"save failed\n"; continue; }
 if(cmd=="moves"||cmd=="m"){ auto cur=hist.back(); if(cur.phase==Phase::P1_TO_PLANT){ auto v=oracle.listP1Moves(cur.state); for(auto& me:v){ std::cout<<me.move_label<<" -> "<<res(me.result); if(!me.refuting_dirs.empty()){ std::cout<<" refuting:"; for(int d:me.refuting_dirs) std::cout<<dname(d)<<' '; } if(!me.unknown_dirs.empty()){ std::cout<<" unknown:"; for(int d:me.unknown_dirs) std::cout<<dname(d)<<' '; } std::cout<<"\n"; }} else { auto v=oracle.listP2Moves(cur.state); for(auto& me:v) std::cout<<me.move_label<<" -> "<<res(me.result)<<"\n"; } continue; }
 if(cmd=="play"){ auto cur=hist.back(); if(cur.phase==Phase::P1_TO_PLANT){ int x,y; if(!(is>>x>>y)){ std::cout<<"play x y\n"; continue;} if(!oracle.geometry().inside(x,y)){ std::cout<<"bad cell\n"; continue;} int c=oracle.geometry().cellId(x,y); State ap=applyPlant(cur.state,c); PlayNode nx{ap,Phase::P2_TO_BLOW,c}; hist.push_back(nx); auto val=oracle.evaluateP2ToBlowState(nx.state); if(val==PlayerResult::Unknown){ val=oracle.solveP2ToBlowState(nx.state); std::cout<<"lazy solve => "<<res(val)<<"\n"; } printState(hist.back(),oracle); if(autosave&&!memo_path.empty()) oracle.saveMemo(memo_path,true,255);
 } else { std::string d; if(!(is>>d)){ std::cout<<"play DIR\n"; continue;} int di=-1; std::string u=d; for(char& ch:u) ch=toupper(ch); const char* arr[8]={"E","W","N","S","NE","NW","SE","SW"}; for(int i=0;i<8;++i) if(u==arr[i]) di=i; if(di<0|| (cur.state.used_dirs&(1u<<di))){ std::cout<<"bad dir\n"; continue;} State nxs=applyWind(cur.state,di,oracle.geometry()); PlayNode nx{nxs,Phase::P1_TO_PLANT,std::nullopt}; hist.push_back(nx); auto val=oracle.evaluateP1ToMoveState(nx.state); if(val==PlayerResult::Unknown){ val=oracle.solveP1ToMoveState(nx.state); std::cout<<"lazy solve => "<<res(val)<<"\n"; } printState(hist.back(),oracle); if(autosave&&!memo_path.empty()) oracle.saveMemo(memo_path,true,255);
 }
 continue; }
 std::cout<<"unknown command\n";
 }
 if(save_on_exit&&!memo_path.empty()) oracle.saveMemo(memo_path,true,255);
 return 0; }
