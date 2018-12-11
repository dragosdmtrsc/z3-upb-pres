//
// Created by dragos on 11.12.2018.
//

#ifndef Z3_PRES_UPB_ENTITIES_H
#define Z3_PRES_UPB_ENTITIES_H


#include <utility>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <z3++.h>

struct serie {
    std::string nume;
    uint8_t an;
    int nr_studenti = 0;

    serie(std::string nume, uint8_t an, int nr_studenti) : nume(std::move(nume)), an(an), nr_studenti(nr_studenti) {}
    serie(std::string nume, uint8_t an) : nume(std::move(nume)), an(an) {}
    bool operator<(const serie &o) const {
        if (an != o.an)
            return an < o.an;
        return nume < o.nume;
    }
};
struct curs {
    std::string nume;
    uint8_t an;
    int duration;
    curs(std::string nume) : nume(std::move(nume)), an(0) {}
    curs(std::string nume, uint8_t an) : nume(std::move(nume)), an(an) {}
    curs(std::string nume, uint8_t an, int duration) : nume(std::move(nume)), an(an), duration(duration) {}
    bool operator<(const curs &o) const {
        if (an != o.an)
            return an < o.an;
        return nume < o.nume;
    }
    bool operator==(const curs &o) const {
        return an == o.an && nume == o.nume;
    }
};
struct prof {
    std::vector<curs> capabilitati;
    std::string nume;
    prof(std::string nume) : nume(std::move(nume)) {}
    bool operator<(const prof &o) const {
        return nume < o.nume;
    }
};
struct sala {
    std::string nume;
    int capacitate;
    sala(std::string nume, int capacitate) : nume(std::move(nume)), capacitate(capacitate) {}
    bool operator<(const sala &o) const {
        return nume < o.nume;
    }
};

struct db {
    std::map<std::string, prof> profi;
    std::map<uint8_t, std::set<serie>> serii_pe_an;
    std::map<uint8_t, std::set<curs>> cursuri_pe_an;
    std::map<uint8_t, std::set<curs>> catalog_cursuri;
    std::map<uint8_t, std::map<std::string, std::set<curs>>> cursuri_pe_an_pe_serie;
    std::set<sala> sali;
    static db *read_database(std::istream &file);
    static db *read_database(const std::string &f);
};



enum weekday { L, M, Mi, J, V, S, D };

struct sched {
    sala *sala1;
    curs *curs1;
    serie *serie1;
    prof *profesor;
    weekday day;
    int start;
    int end;

    friend std::ostream & operator<<(std::ostream &os, const sched& p);
};
struct orar {
    std::vector<sched> o;

    orar() {}
    orar(std::vector<sched> orar) : o(std::move(orar)) {}

    friend std::ostream & operator<<(std::ostream &os, const orar& p);
};


struct schedule {
    static const char *fields[5];
    static const char *wdn[7];
    static constexpr size_t nr_fields = 5;
    const int START_PR_IDX = 3;
    const int END_PR_IDX = 4;
    const int WD_PR_IDX = 2;
    const int RM_PR_IDX = 1;
    const int PF_PR_IDX = 0;

    db *database;
    z3::context ctx;
    z3::solver *query = nullptr;
    bool weedays_only = true;
    std::map<uint8_t, std::map<std::string, std::map<std::string, z3::expr>>> sched_unit_constants;
    z3::func_decl_vector projs;
    z3::func_decl_vector rooms, test_rooms;

    z3::func_decl_vector wd, test_wd;


    z3::func_decl_vector profi, test_profi;

    schedule(db *database) : database(database), ctx(),
                             projs(ctx), rooms(ctx), test_rooms(ctx),
                             wd(ctx), test_wd(ctx),
                             profi(ctx), test_profi(ctx) {}

    const ::prof &get_prof(const z3::model &model, const z3::expr &proj) {
        auto crt = ctx.int_val(-1);
        auto prsize = profi.size();
        for (auto i = 0; i != prsize; ++i) {
            crt = z3::ite(proj == profi[i](), ctx.int_val(i), crt);
        }
        auto evald = model.eval(crt);
        assert(evald.is_int());
        auto i = evald.get_numeral_int();
        auto profI = database->profi.begin();
        while (i) {
            ++profI;
            --i;
        }
        return profI->second;
    }
    const ::sala &get_sala(const z3::model &model, const z3::expr &proj) {
        auto crt = ctx.int_val(-1);
        auto prsize = rooms.size();
        for (auto i = 0; i != prsize; ++i) {
            crt = z3::ite(proj == rooms[i](), ctx.int_val(i), crt);
        }
        auto evald = model.eval(crt);
        assert(evald.is_int());
        auto i = evald.get_numeral_int();
        auto saliI = database->sali.begin();
        while (i) {
            ++saliI;
            --i;
        }
        return *saliI;
    }
    weekday get_weekday(const z3::model &model, const z3::expr &proj) {
        auto crt = ctx.int_val(-1);
        auto prsize = wd.size();
        for (auto i = 0; i != prsize; ++i) {
            crt = z3::ite(proj == wd[i](), ctx.int_val(i), crt);
        }
        auto evald = model.eval(crt);
        assert(evald.is_int());
        auto i = evald.get_numeral_int();
        return weekday(i);
    }

    ~schedule() {
        delete query;
    }

    void build_query();

    void block(const std::vector<sched> &orar);

    std::vector<sched> build_orar(const z3::model &model);

    bool get_orar(orar &orar);
};



#endif //Z3_PRES_UPB_ENTITIES_H
