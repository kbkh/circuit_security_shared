#!/bin/tcsh

setenv LD_LIBRARY_PATH /home/cbk289/lib:/$LD_LIBRARY_PATH

nohup ./bin/circuit_security circuits/c432.blif tech_lib/02.genlib -t 0 > areas/log_c432  &
nohup ./bin/circuit_security circuits/c3540.blif tech_lib/02.genlib -t 0 > areas/log_c3540 &
nohup ./bin/circuit_security circuits/c1908.blif tech_lib/02.genlib -t 0 > areas/log_c1908 &
nohup ./bin/circuit_security circuits/c6288.blif tech_lib/02.genlib -t 0 > areas/log_c6288 &
