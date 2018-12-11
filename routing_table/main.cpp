#include <iostream>
#include <assert.h>
#include <fstream>
#include <z3++.h>
#include <regex>

struct ip {
    unsigned v;
    ip() : v(0) {}
    ip(unsigned v) : v(v) {}
    ip(const std::string &from) {
        std::istringstream iss(from);
        std::string token;
        v = 0;
        while (std::getline(iss, token, '.')) {
            auto x = std::stoi(token);
            v = (v << 8) | x;
        }
    }
    bool operator<(ip other) const {
        return v < other.v;
    }
};
struct ippref {
    ip ipv;
    uint8_t pref;
    ippref() : ipv(), pref(0) {}
    ippref(const ip &ipv, uint8_t pref) : ipv(ipv), pref(pref) {}
    bool operator<(const ippref &other) const {
        if (pref != other.pref)
            return pref > other.pref;
        return ipv < other.ipv;
    }
};

int main(int argc, char **argv) {
    assert(argc > 0);
    std::ifstream fin(argv[1]);
    assert(fin.is_open());
    z3::context ctx;
    z3::solver slv(ctx);
    std::map<ippref, std::tuple<::ip, std::string, size_t>> rt;
    auto ipe = ctx.bv_const("ip", 32);
    size_t lineno = 0;
    while (fin) {
        std::string line;
        if (!std::getline(fin, line))
            break;
        std::string prefix;
        std::string nexthop;
        std::string nextip;
        std::regex rex("^([0-9.]+)/([0-9]+)\\s+([0-9.]+)\\s+(\\S+).*");
        std::smatch matches;
        if (!std::regex_search(line, matches, rex)) {
            std::regex rex2("^([0-9.]+)/([0-9]+)");
            if (std::regex_search(line, matches, rex2)) {
                auto pref = matches.str(1);
                ip ip1(pref);
                auto len = matches.str(2);
                auto p1 = std::stoi(len);
                ippref ipprefix(ip1, static_cast<uint8_t>(p1));
                rt.emplace(ipprefix, std::make_tuple(ip(), "", lineno));
            }
        } else {
            auto pref = matches.str(1);
            ip ip1(pref);
            auto len = matches.str(2);
            auto p1 = std::stoi(len);
            ippref ipprefix(ip1, static_cast<uint8_t>(p1));
            auto nhip = matches.str(3);
            ip nh1(nhip);
            auto nhif = matches.str(4);
            rt.emplace(ipprefix, std::make_tuple(nh1, nhif, lineno));
        }
        ++lineno;
    }
    for (auto re : rt) {
        if (re.first.pref != 0) {
            slv.push();
            auto constraint = (ipe.extract(31, 32 - re.first.pref) == ctx.bv_val(re.first.ipv.v >> (32 - re.first.pref), re.first.pref));
            slv.add(constraint);
            if (slv.check() == z3::check_result::unsat) {
                std::cerr << "dead entry at line " << std::get<2>(re.second) << '\n';
            }
            slv.pop();
            slv.add(!constraint);
        }
    }
    return 0;
}