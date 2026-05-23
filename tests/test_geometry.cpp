#include "geometry.h"
#include "test_main.h"

void test_geometry() {
    Geometry g(3);
    CHECK(g.cellId(2,1)==5);
    Coord c=g.coord(5);
    CHECK(c.x==2 && c.y==1);
    CHECK(g.fullBoardMask()==((1ULL<<9)-1));
    CHECK(g.ray(g.cellId(1,1),0)==((1ULL<<g.cellId(2,1))));
    CHECK(g.ray(g.cellId(1,1),4)==(1ULL<<g.cellId(2,2)));
    CHECK(g.transformCell(0,g.cellId(0,0))==g.cellId(0,0));
    CHECK(g.inverseSymmetry(0)==0);
}
