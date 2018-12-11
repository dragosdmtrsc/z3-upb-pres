//
// Created by dragos on 11.12.2018.
//

#include <fstream>
#include <algorithm>
#include "entities.h"

db *db::read_database(const std::string &f) {
    std::ifstream ifs(f);
    assert(ifs.is_open());
    return read_database(ifs);
}

std::vector<std::string> read_csv_line(std::istream &is) {
    std::string line;
    getline(is, line);
    std::vector<std::string> vec;
    if (line[0] != '#') {
        std::istringstream ss(line);
        while (ss) {
            std::string cell;
            if (!getline(ss, cell, ','))
                break;
            vec.push_back(cell);
        }
    }
    return vec;
}

// format:
// N_profi nr profi
// N_profi randuri cu cate un nume de prof pe linie
// N_ani nr ani
// N_ani randuri cu un tuplu an,seria_1,stud_seria_1,seria_2,stud_seria_2...,seria_N_i,stud_seria_N_i
// N_cursuri nr cursuri
// N_cursuri randuri cu tupluri an,curs,durata(,serie)?
// N_catedre nr catedre
// N_catedre randuri cu tupluri nume_prof,an,curs
// N_sali nr sali
// N_sali randuri cu tupluri nume_sala,capacitate
db *db::read_database(std::istream &file) {
    auto *new_db = new db();
    int nr_profi;
    auto v = read_csv_line(file);
    nr_profi = std::stoi(*v.begin());
    assert(nr_profi > 0);
    for (int i = 0; i != nr_profi; ++i) {
        auto v = read_csv_line(file);
        auto I = v.begin();
        auto nume = *I;
        new_db->profi.emplace(nume, nume);
    }
    v = read_csv_line(file);
    int n_ani = std::stoi(*v.begin());
    for (int i = 0; i != n_ani; ++i) {
        auto serii_pe_an = read_csv_line(file);
        auto I = serii_pe_an.begin();
        auto an = static_cast<uint8_t>(std::stoi(*I));
        auto &spa = new_db->serii_pe_an[an];
        ++I;
        while (I != serii_pe_an.end()) {
            auto nume = *I;
            ++I;
            auto stud_serie = std::stoi(*I);
            ++I;
            spa.emplace(nume, an, stud_serie);
        }
    }
    v = read_csv_line(file);
    int n_cursuri = std::stoi(*v.begin());
    for (int i = 0; i != n_cursuri; ++i) {
        auto serii_pe_an = read_csv_line(file);
        auto I = serii_pe_an.begin();
        auto an = static_cast<uint8_t>(std::stoi(*I));
        ++I;
        auto curs = *I;
        ++I;
        auto durata = std::stoi(*I);
        ++I;
        if (I != serii_pe_an.end()) {
            auto serie = *I;
            new_db->cursuri_pe_an_pe_serie[an][serie].emplace(::curs( curs, an, durata ));
        } else {
            new_db->cursuri_pe_an[an].emplace(::curs( curs, an, durata ));
        }
        new_db->catalog_cursuri[an].emplace(::curs( curs, an, durata ));
    }
    v = read_csv_line(file);
    int n_catedre = std::stoi(*v.begin());
    for (int i = 0; i != n_catedre; ++i) {
        auto serii_pe_an = read_csv_line(file);
        auto I = serii_pe_an.begin();
        auto nume_prof = *I;
        ++I;
        auto an = static_cast<uint8_t>(std::stoi(*I));
        ++I;
        auto curs = *I;
        auto cursI = new_db->catalog_cursuri[an].find(::curs(curs, an));
        assert(cursI != new_db->catalog_cursuri[an].end());
        auto profI = new_db->profi.find(nume_prof);
        assert(profI != new_db->profi.end());
        profI->second.capabilitati.emplace_back(*cursI);
    }
    v = read_csv_line(file);
    int n_sali = std::stoi(*v.begin());
    for (int i = 0; i != n_sali; ++i) {
        auto serii_pe_an = read_csv_line(file);
        auto I = serii_pe_an.begin();
        auto nume_sala = *I;
        ++I;
        auto capacitate = std::stoi(*I);
        new_db->sali.emplace(nume_sala, capacitate);
    }
    return new_db;
}

std::ostream &operator<<(std::ostream &os, const orar &p) {
    for (const auto &s : p.o) {
        os << s << '\n';
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const sched &p) {
    std::string sapt;
    switch (p.day) {
        case L: sapt = "L"; break;
        case M:sapt = "M";break;
        case Mi:sapt = "Mi";break;
        case J:sapt = "J";break;
        case V:sapt = "V";break;
        case S:sapt = "S";break;
        case D:sapt = "D";break;
    }
    return os << static_cast<int>(p.curs1->an) << ',' << p.serie1->nume << ',' << p.curs1->nume << ',' << sapt
              << ',' << p.start << ',' << p.end << ',' << p.profesor->nume << ',' << p.sala1->nume;
}

bool schedule::get_orar(orar &orar) {
    if (!query)
        build_query();
    auto cr = query->check();
    if (cr == z3::check_result::sat) {
        orar = build_orar(query->get_model());
        return true;
    } else {
        return false;
    }
}

const char *schedule::fields[5] = { "prof", "sala", "zi", "start", "end" };
const char *schedule::wdn[7] = { "L", "M", "Mi", "J", "V", "S", "D" };

void schedule::build_query() {
    query = new z3::solver(ctx);
    sched_unit_constants.clear();
    // constraints:
    // 1: all courses will be scheduled for
    // 1.a. one series (if in per yr per serie)
    // 1.b. each series in every year (if in per yr)
    // 2: all courses will have a location
    // 3: for all entries in the time table: t are_catedra(t.prof, t.curs)
    // 4: for two distinct entries in the time table: t1, t2
    //    with t1.sala == t2.sala => (t1 /\ t2 = empty)
    // 5: for two distinct entries in the time table: t1 t2
    //    with t1.prof == t2.prof => (t1 /| t2 = empty)
    // 6: for two distinct entries in the time table: t1 t2
    //    with t1.serie == t2.serie => (t1 /| t2 == empty)
    // 7: for each norm there exists an entry t such that
    //    norm.prof == t.prof
    // 8: for all entries in the time table: t t.end <= 2200
    // 9: for all entries in the time table: t t.start >= 0800
    // 10: for all entries t in the timetable t.weekday <> Saturday && t.weekday <> Sunday
    // 11: for all entries t in the timetable t.serie.nr_studenti <= t.room.capacitate
    // 12: for all entries in the timetable: t t.end == t.start + t.curs.duration

    // start by creating your objects
    // 0: weekdays
    auto wd_sort = ctx.enumeration_sort("weekdays", 7, wdn, wd, test_wd);
    // 1: profi
    auto nprofi = static_cast<unsigned int>(database->profi.size());
    const auto **prof_names = new const char *[nprofi];
    auto profI = database->profi.begin();
    for (auto i = 0; i != nprofi; ++i) {
        prof_names[i] = profI->first.c_str();
        ++profI;
    }
    auto prof_sort = ctx.enumeration_sort("prof", nprofi, prof_names, profi, test_profi);
    auto prof_capable_macro = [&](const z3::expr &prof_expr, uint8_t an, const std::string &curs) {
        auto profI = database->profi.begin();
        auto crt = ctx.bool_val(false);
        auto i = 0;
        while (profI != database->profi.end()) {
            auto is_capable = std::find(profI->second.capabilitati.begin(),
                    profI->second.capabilitati.end(), ::curs(curs, an)) != profI->second.capabilitati.end();
            if (is_capable) {
                crt = crt || profi[i]() == prof_expr;
            }
            ++profI;
            ++i;
        }
        return crt.simplify();
    };
    // 2: rooms
    auto nrooms = static_cast<unsigned int>(database->sali.size());
    const auto **room_names = new const char *[nrooms];
    auto roomI = database->sali.begin();
    for (auto i = 0; i != nrooms; ++i) {
        room_names[i] = roomI->nume.c_str();
        ++roomI;
    }
    auto room_sort = ctx.enumeration_sort("room", nrooms, room_names, rooms, test_rooms);
    auto max_capacity_macro = [this, nrooms](const z3::expr &e) {
        auto crt = ctx.int_val(0);
        auto roomI = database->sali.begin();
        for (auto i = 0; i != nrooms; ++i) {
            crt = z3::ite(e == rooms[i](), ctx.int_val(roomI->capacitate), crt);
            ++roomI;
        }
        return crt;
    };

    // 3: scheduling units: an/serie/curs/prof/sala/zi/start/end
    z3::sort field_sorts[] = { prof_sort, room_sort, wd_sort, ctx.int_sort(), ctx.int_sort() };
    auto sched_unit_constructor = ctx.tuple_sort("sched_unit", nr_fields, fields, field_sorts, projs);
    auto sched_unit_sort = sched_unit_constructor.range();

    std::map<uint8_t, std::map<std::string, std::set<::curs>>> sched_units;
    std::vector<std::tuple<uint8_t, std::string, ::curs>> sched_units_flat;
    for (const auto &cpa : database->cursuri_pe_an) {
        auto an = cpa.first;
        for (const auto &serie : database->serii_pe_an[an]) {
            sched_units[an][serie.nume].insert(cpa.second.begin(), cpa.second.end());
            for (const auto &crs : cpa.second) {
                sched_units_flat.emplace_back(an, serie.nume, crs);
                std::stringstream ss;
                ss << "curs_" << static_cast<int>(an) << "_" << serie.nume << "_" << crs.nume;
                auto e = ctx.constant(ss.str().c_str(), sched_unit_sort);
                sched_unit_constants[an][serie.nume].emplace(crs.nume, e);
            }
        }
    }
    for (const auto &cpaps : database->cursuri_pe_an_pe_serie) {
        auto an = cpaps.first;
        for (const auto &cps : cpaps.second) {
            const auto &serie = cps.first;
            sched_units[an][serie].insert(cps.second.begin(), cps.second.end());
            for (const auto &crs : cps.second) {
                sched_units_flat.emplace_back(an, serie, crs);
                std::stringstream ss;
                ss << "curs_" << static_cast<int>(an) << "_" << serie << "_" << crs.nume;
                auto e = ctx.constant(ss.str().c_str(), sched_unit_sort);
                sched_unit_constants[an][serie].emplace(crs.nume, e);
            }
        }
    }
    // 4: entry invariants
    for (const auto &peryr : sched_units) {
        auto an = peryr.first;
        for (const auto &persr : peryr.second) {
            const auto &serie = persr.first;
            const auto &serieo = *database->serii_pe_an[an].find(::serie(serie, an));
            for (const auto &crs : persr.second) {
                auto z3_e = sched_unit_constants[an][serie].find(crs.nume)->second;
                // 8: no later than 2200
                query->add(projs[END_PR_IDX](z3_e) <= ctx.int_val(22));
                // 9: no earlier than 0800
                query->add(projs[START_PR_IDX](z3_e) >= ctx.int_val(8));
                // 12: end == start + duration
                query->add(projs[START_PR_IDX](z3_e) + ctx.int_val(crs.duration) == projs[END_PR_IDX](z3_e));
                // 10: no work during weekends
                query->add(projs[WD_PR_IDX](z3_e) != wd[5]());
                query->add(projs[WD_PR_IDX](z3_e) != wd[6]());
                // 13: max capacity constraint
                query->add(max_capacity_macro(projs[RM_PR_IDX](z3_e)) >= serieo.nr_studenti);
                // 3: prof is capable
                query->add(prof_capable_macro(projs[PF_PR_IDX](z3_e), an, crs.nume));
            }
        }
    }

    auto macro_overlaps = [&](const z3::expr &tt1, const z3::expr &tt2) {
        auto wd1 = projs[WD_PR_IDX](tt1);
        auto wd2 = projs[WD_PR_IDX](tt2);
        auto sd1 = projs[START_PR_IDX](tt1);
        auto sd2 = projs[START_PR_IDX](tt2);
        auto ed1 = projs[END_PR_IDX](tt1);
        auto ed2 = projs[END_PR_IDX](tt2);
        // [sd1,ed1] overlaps with [sd2, ed2] if ed1 > sd2 and sd1 < ed2
        return (ed1 > sd2 && sd1 < ed2 && wd1 == wd2);
    };
    // 5: relations between timetable entries
    for (auto I = sched_units_flat.begin(); I != sched_units_flat.end(); ++I) {
        const auto &z3_e1 = sched_unit_constants[std::get<0>(*I)][std::get<1>(*I)].find(std::get<2>(*I).nume)->second;
        const auto &s1 = std::get<1>(*I);
        const auto &a1 = std::get<0>(*I);
        for (auto J = I + 1; J != sched_units_flat.end(); ++J) {
            const auto &z3_e2 = sched_unit_constants[std::get<0>(*J)][std::get<1>(*J)].find(std::get<2>(*J).nume)->second;
            auto rm1 = projs[RM_PR_IDX](z3_e1);
            auto rm2 = projs[RM_PR_IDX](z3_e2);
            auto p1 = projs[PF_PR_IDX](z3_e1);
            auto p2 = projs[PF_PR_IDX](z3_e2);
            const auto &s2 = std::get<1>(*J);
            const auto &a2 = std::get<0>(*J);
            if (a1 == a2 && s1 == s2) {
                query->add(!macro_overlaps(z3_e1, z3_e2));
            } else {
                query->add(!(rm1 == rm2 && macro_overlaps(z3_e1, z3_e2)));
                query->add(!(p1 == p2 && macro_overlaps(z3_e1, z3_e2)));
            }
        }
    }
    std::cerr << *query << '\n';
}

void schedule::block(const std::vector<sched> &orar) {
    //TODO: re-create model and assert !model
}

std::vector<sched> schedule::build_orar(const z3::model &model) {
    std::vector<sched> retval;
    for (const auto &peran : sched_unit_constants) {
        auto an = peran.first;
        auto &spa = database->serii_pe_an[an];
        for (const auto &perserie : peran.second) {
            const auto &serie = perserie.first;
            for (const auto &crs : perserie.second) {
                const auto &curs = crs.first;
                const auto &ze = crs.second;
                const auto &p = get_prof(model, projs[PF_PR_IDX](ze));
                auto wd = get_weekday(model, projs[WD_PR_IDX](ze));
                const auto &room = get_sala(model, projs[RM_PR_IDX](ze));
                auto start = model.eval(projs[START_PR_IDX](ze)).get_numeral_int();
                auto end = model.eval(projs[END_PR_IDX](ze)).get_numeral_int();
                sched s;
                s.curs1 = const_cast<::curs *>(&(*database->catalog_cursuri[an].find(::curs(crs.first, an))));
                s.end = end;
                s.start = start;
                s.day = wd;
                s.sala1 = const_cast<::sala *>(&room);
                s.profesor = const_cast<::prof *>(&p);
                s.serie1 = const_cast<::serie *>(&(*spa.find(::serie(serie, an))));
                retval.emplace_back(s);
            }
        }
    }
    return retval;
}
