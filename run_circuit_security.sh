#!/bin/tcsh
setenv LD_LIBRARY_PATH /home/cbk289/lib:/$LD_LIBRARY_PATH

foreach i (0 2 4)
	foreach j (`seq $4 $5`)
		nohup ./bin/circuit_security circuits/$1.blif tech_lib/$2.genlib -t 10 -f $1 -p $3 -l $i -k $j > logs/$1_lib$2_pag$3_tresh$i-k$j &
	end
end
