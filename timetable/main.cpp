#include <iostream>
#include "entities.h"

int main(int argc, char **argv) {
    assert(argc > 1);
    auto my_db = db::read_database(argv[1]);
    schedule sched(my_db);
    orar o;
    if (sched.get_orar(o)) {
        std::cout << o;
    } else {
        std::cout << "nu se poate face un orar\n";
    }
    delete my_db;
    return 0;
}