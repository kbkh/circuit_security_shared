# PAG algorithm not working with DES

Tu run the code one should enter:

./bin/circuit_security circuits/c432.blif tech_lib/02.genlib -t 10 -f c432 -p 4 -l 2

where c432.blif should be changed with the desired circuit, 02.genlib with the tech lib used to map this circuit. 
-t 10 runs the PAG part of the code
-f is the name used in the output files mentioned below
-p is the size of the starting PAG
-l is the fator of the threshold, 2 -> k/2, 4 -> k/4 and 0 -> 0
if one wants to only lift edges then he should add ' -a true '

The code will output results for 5 different security levels (2, 4, 8, 16 and 32)

the output files are:
    in the PAG_testing/ folder:
        <name after -f>_PAG_<number after -p>_tresh_<number after -l>_report.txt where one can find a detailed report about the execution such as, # of vertices on top tier (final G), # of vertices in bottom tier with and without dummies, number of bonds, runtime etc.
        <name after -f>_PAG_<number after -p>_tresh_<number after -l>.txt where in a form of a table are the 5 security levels (2, 4, 8, 16, 32) and the corresponding number of lifted edges.

    in the wdir/ folder:
        <name after -f>_PAG_<number after -p>_tresh_<number after -l>_lvl_<for every level>_R_circuit.gml This is a file representing the graph. It has all original vertices and edges and tags to identify in what tier it is for vertices and what type of edge it is (in the top tier ' "Top" ', bottom ' "Bottom" ' tier, crossing once (1 bond point) ' "Crossing" ' or crossing twice (2 bond points) ' "Lifted" ')
        <name after -f>_PAG_<number after -p>_tresh_<number after -l>_lvl_<for every level>_F_circuit.gml Same as above but also contains the dummy vertices and edges added in the bottom tier. A tag ' Dummy ' is there to say whether it's a dummy vertex (1) or not (0)
