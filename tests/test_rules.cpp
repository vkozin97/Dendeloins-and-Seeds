#include "state.h"
#include "test_main.h"

void test_rules() {
    Geometry g(3);
    State s{};
    CHECK((plantableMask(s,g)&(1ULL<<g.cellId(0,0)))!=0);
    s=applyPlant(s,g.cellId(0,0));
    CHECK((plantableMask(s,g)&(1ULL<<g.cellId(0,0)))==0);
    s.occupied|=(1ULL<<g.cellId(1,1));
    CHECK((plantableMask(s,g)&(1ULL<<g.cellId(1,1)))!=0);
    State w=applyWind(s,0,g);
    CHECK((w.occupied&(1ULL<<g.cellId(1,0)))!=0);
    CHECK((w.occupied&(1ULL<<g.cellId(2,0)))!=0);
    CHECK(isTerminalLoss(w,1));
}
