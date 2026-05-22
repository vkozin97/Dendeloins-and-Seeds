#include "solver.h"
#include "test_main.h"

void test_geometry();
void test_rules();

void check_case(int n, int k) {
    bool base = Solver({n,k,true,true,true}).solve().first_wins;
    CHECK(base == Solver({n,k,false,true,true}).solve().first_wins);
    CHECK(base == Solver({n,k,true,false,true}).solve().first_wins);
    CHECK(base == Solver({n,k,true,true,false}).solve().first_wins);
}

int main() {
    test_geometry();
    test_rules();
    check_case(1,1);
    check_case(2,1);
    check_case(2,2);
    check_case(3,1);
    check_case(3,2);
    return 0;
}
