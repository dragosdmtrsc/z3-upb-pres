## Simple applications using z3 SMT solver

Prerequisites - install the following
* cmake >= 3.9 - binary distribution from
https://cmake.org/download/ 
```
wget https://github.com/Kitware/CMake/releases/download/v3.13.1/cmake-3.13.1-Linux-x86_64.sh
sudo bash cmake-3.13.1-Linux-x86_64.sh --skip-license --prefix=/usr/local/
```
* z3 latest binary release from 
https://github.com/Z3Prover/z3/releases/
```
sudo apt-get install -y g++ make
wget https://github.com/Z3Prover/z3/releases/download/z3-4.8.3/z3-4.8.3.7f5d66c3c299-x64-ubuntu-16.04.zip
unzip z3-4.8.3.7f5d66c3c299-x64-ubuntu-16.04.zip
sudo cp -r z3-4.8.3.7f5d66c3c299-x64-ubuntu-16.04/* /usr/local/
```

#### Build
```
mkdir build
cd build
cmake ..
make
```
If all goes well, you should get 3 executables:
`timetable/timetable`, `sudoku/sudoku`, `routing_table/routing_table`

#### Applications:

##### Sudoku solver
Very common z3 example for solving Sudoku puzzles.
To run (from build directory): 
`./sudoku/sudoku ../sudoku/in.pzl`.

##### Timetable problem
Computing a simplified timetable given:
a set of classes, teachers and their
capabilities to teach a given class, a set of
rooms and groups of students who are taking a class.

To run (from build directory): 
`./timetable/timetable ../timetable/example.txt`.

##### Dead routing table entry elimination
Compute all entries from routing table which
are _dead_ - i.e. hidden by previous ones.

To run (from build directory): 
`./routing_table/routing_table <routing_table>`.
