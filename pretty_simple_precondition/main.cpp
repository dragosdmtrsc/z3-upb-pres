#include <iostream>
#include <assert.h>
#include <fstream>
#include <z3++.h>
#include <regex>

int main(int argc, char **argv) {
    assert(argc > 0);
    bool is_bad = false;
    if (std::string("--bad") == argv[1]) {
        is_bad = true;
    } else if (std::string("--good") == argv[1]) {
        is_bad = false;
    } else {
        assert(false);
    }
    z3::context ctx;
    z3::solver slv(ctx);

    auto x = ctx.bv_const("x", 8);
    auto y = ctx.bv_const("y", 8);
    // (x + y < 0 \vee y < 0) \wedge \neg{(x < 0 \vee y < 0)} \wedge \neg(x \ge 128 \vee y \ge 128)
    slv.add(x + y < 0 || y < 0);
    slv.add(!(x < 0 || y < 0));
    if (is_bad) {
        slv.add(!(x >= ctx.bv_val(128, 8) || y >= ctx.bv_val(128, 8)));
    } else {
        slv.add(!(x > ctx.bv_val(127, 8) || y >= ctx.bv_val(127, 8)));
    }
    auto cr = slv.check();
    if (cr == z3::check_result::sat) {
        std::cerr << "sat. program is buggy. And here's why." <<'\n';
        auto model = slv.get_model();
        auto xv = model.eval(x).get_numeral_uint();
        auto yv = model.eval(y).get_numeral_uint();
        std::cerr << "x=" << xv <<'\n';
        std::cerr << "y=" << yv <<'\n';
    } else {
        std::cerr << "unsat, program is correct\n";
    }
    return 0;
}