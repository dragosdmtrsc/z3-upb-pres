#include <iostream>
#include <assert.h>
#include <fstream>
#include <z3++.h>

int main(int argc, char **argv) {
    assert(argc > 0);
    std::ifstream fin(argv[1]);
    assert(fin.is_open());
    z3::context ctx;
    z3::solver slv(ctx);
    z3::expr_vector grid(ctx);
    for (unsigned i = 0; i != 9; ++i) {
        for (unsigned j = 0; j != 9; ++j) {
            std::stringstream ss;
            ss << "grid_" << i << "_" << j;
            grid.push_back(ctx.constant(ss.str().c_str(), ctx.int_sort()));
        }
    }

    int line = 0;
    while (fin) {
        int col = 0;
        std::string ln ;
        if (std::getline(fin, ln)) {
            for (auto c : ln) {
                if (c != '.') {
                    slv.add(grid[line * 9 + col] == ctx.int_val(static_cast<int>(c - '0')));
                }
                ++col;
            }
        } else {
            break;
        }
        ++line;
    }

    for (unsigned i = 0; i != 9; ++i) {
        for (unsigned j = 0; j != 9; ++j) {
            slv.add(grid[i * 9 + j] <= 9 && grid[i * 9 + j] > 0);
        }
    }
    for (unsigned i = 0; i != 9; ++i) {
        for (unsigned j = 0; j != 9; ++j) {
            for (unsigned k = j + 1; k < 9; ++k) {
                slv.add(grid[i * 9 + j] != grid[i * 9 + k]);
                slv.add(grid[k * 9 + i] != grid[j * 9 + i]);
            }
        }
    }
    for (unsigned i = 0; i < 9; i += 3) {
        for (unsigned j = 0; j < 9; j += 3) {
            z3::expr_vector evec(ctx);
            for (unsigned u = 0; u != 3; ++u) {
                for (unsigned v = 0; v != 3; ++v) {
                    evec.push_back(grid[(i + u) * 9 + (j + v)]);
                }
            }
            slv.add(z3::distinct(evec));
        }
    }
    if (slv.check() == z3::check_result::sat) {
        auto model = slv.get_model();
        auto block = ctx.bool_val(true);
        for (unsigned i =0; i != 9; ++i) {
            for (unsigned j =0; j != 9; ++j) {
                auto val = model.eval(grid[i * 9 + j]).get_numeral_int();
                std::cout << val;
                block = block && (grid[i * 9 + j] == ctx.int_val(val));
            }
            std::cout << '\n';
            slv.add(!(block.simplify()));
            if (slv.check() == z3::check_result::sat) {
                std::cerr << "solution not unique\n";
            }
        }
    } else {
        std::cerr << "no solution for this sudoku\n";
    }
}