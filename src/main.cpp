
/*************************************************************************//**
                                                                            *****************************************************************************
                                                                            * @file        main.cpp
                                                                            * @brief
                                                                            * @author      Frank Imeson
                                                                            * @date        2012-01-02
                                                                            *****************************************************************************
                                                                            ****************************************************************************/

#include "main.hpp"


#define DEBUG



/********************************************************************************
 * Notes:
 
 conflict_budget = 10000, min before we see changes from -i c17 -l 02 -mt4
 conflict_budget = 1000000, max before "it takes to long" for -i c432 -l 03 -t4
 
 
 ********************************************************************************/


/********************************************************************************
 * Global
 ********************************************************************************/

ofstream koutfile;
ofstream koutfile2;
int target_security;

/********************************************************************************
 * Functions
 ********************************************************************************/

void write_to_file(int lifted_edges, int G_vcount, int G_ecount, int G_v_lifted, int G_e_lifted, int H_vcount, int H_ecount, int H_v_no_dummy, int H_e_no_dummy, double took, int oneBond, int twoBonds, int Fvcount, int Fecount) {
    koutfile<<setfill(' ')<<setw(6)<<target_security<<setfill(' ')<<setw(15)<<lifted_edges<<setfill(' ')<<endl;
    koutfile2<<"Security lvl: "<<target_security<<endl;
    koutfile2<<"G initial vertex count: "<<G_vcount<<endl;
    koutfile2<<"G initial edge count: "<<G_ecount<<endl;
    koutfile2<<"G final vertex count: "<<G_v_lifted<<endl;
    koutfile2<<"G final edge count: "<<G_e_lifted<<endl;
    koutfile2<<endl;
    koutfile2<<"H final vertex count, with dummies: "<<H_vcount<<endl;
    koutfile2<<"H final edge count, with dummies: "<<H_ecount<<endl;
    koutfile2<<"H final vertex count, without dummies: "<<H_v_no_dummy<<endl;
    koutfile2<<"H final edge count, without dummies: "<<H_e_no_dummy<<endl;
    koutfile2<<endl;
    koutfile2<<"F circuit vcount: "<<Fvcount<<endl;
    koutfile2<<"F circuit ecount: "<<Fecount<<endl;
    koutfile2<<endl;
    koutfile2<<"Lifted edges: "<<lifted_edges<<endl;
    koutfile2<<"Lifted edges w/ 1 bond: "<<oneBond<<endl;
    koutfile2<<"Lifted edges w/ 2 bonds: "<<twoBonds<<endl;
    koutfile2<<"Total bonds: "<<oneBond + 2*twoBonds<<endl;
    koutfile2<<endl;
    koutfile2<<"Took: "<<took<<endl;
    koutfile2<<endl;
}


/*****************************************************************************
 * Main
 ****************************************************************************/
int main(int argc, char **argv) {
    clock_t tic = clock();
    srand ( time(NULL) );
    
    /******************************
     * Command Line Parser
     ******************************/
    
    bool mono_lib(false), print_gate(false), print_solns(false), print_sat(false), print_blif(false), print_verilog(false), baseline;
    int num_threads, budget, remove_vertices_max, maxPAGsize, tresh; // Added by Karl (remove_vertices_max)
    float remove_percent(0.6);
    vector<string> test_args;
    string outName;
    string name;
    
    po::options_description optional_args("Usage: [options]");
    optional_args.add_options()
    ("circuit",         po::value<string>(),                                    "input circuit")
    ("tech_lib",        po::value<string>(),                                    "tech library")
    ("continue_file,c", po::value<string>(),                                    "input report to continue from")
    ("mono_lib,m",      po::value(&mono_lib)->zero_tokens(),                    "treat each gate as a nand")
    ("budget,b",        po::value<int>(&budget)->default_value(100000000),      "minisat conflict budget (-1 = unlimited)")
    ("remove_edges,e",  po::value<float>(&remove_percent)->default_value(0.6),  "edge percent to remove")
    ("print_graph",     po::value(&print_gate)->zero_tokens(),                  "print H graph")
    ("print_solns",     po::value(&print_solns)->zero_tokens(),                 "print isos")
    ("print_blif",      po::value(&print_blif)->zero_tokens(),                  "print G blif circuit")
    ("print_verilog",   po::value(&print_verilog)->zero_tokens(),               "print verilog circuits")
    ("test,t",          po::value< vector<string> >(&test_args),                 "Run test suite #: \n"
     "  -99: Beta Testing \n"
     "  -1: Mem Testing \n"
     "   0: L0 [count, remove_percent] \n"
     "   1: L1 [count, remove_percent] \n"
     "   2: S1 - Random [min S1] \n"
     "   3: S1 - Greedy [min S1] \n")
    // Added by Karl
    ("remove_vertices,v", po::value<int>(&remove_vertices_max)->default_value(0),   "Number of vertices to remove")
    ("maxPAGsize,p", po::value<int>(&maxPAGsize)->default_value(2), "Number of edges in a PAG")
    ("security,k", po::value<int>(&target_security)->default_value(2), "Target security")
    ("baseline,a", po::value<bool>(&baseline)->default_value(false), "Lift only edges")
    ("output data file,f", po::value<string>(&outName)->default_value("out.txt"),  "Name of the file that will be the input for gnuplot")
    ("treshhold,l", po::value<int>(&tresh)->default_value(2), "Treshhold for adding or lifting embeddings")
    ////////////////
    ("num_threads,n",   po::value<int>(&num_threads)->default_value(1),         "Number of threads")
    ("wdir,w",          po::value<string>()->default_value("./wdir"),           "Output directory")
    ("help,h",                                                                  "produce help message")
    ;
    
    po::positional_options_description positional_args;
    positional_args.add("circuit", 1);
    positional_args.add("tech_lib", 1);
    positional_args.add("test", -1);
    po::variables_map vm;
    
    try {
        po::store(po::command_line_parser(argc, argv).options(optional_args).positional(positional_args).run(), vm);
        po::notify(vm);
    } catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    } catch(...) {
        cerr << "Exception of unknown type!\n";
    }
    
    if (vm.count("help")) {
        cout << optional_args << "\n";
        return 0;
    }
    
    target_security = 16;
    
    cout<<outName<<" "<<tresh<<endl;
    stringstream ss;
    ss << tresh;
    string str = ss.str();
    
    stringstream ss2;
    ss2 << maxPAGsize;
    string str2 = ss2.str();
    
    string outname = "PAG_testing/" + outName + "_PAG_" + str2 + "_tresh_" + str;
    name = outName;
    outName = outname + ".txt";
    koutfile.open(outName.c_str());
    koutfile<<"# security"<<"     "<<"# lifted e"<<endl;
    
    outname = outname + "_report.txt";
    koutfile2.open(outname.c_str());
    
    while (target_security <= 32) {
        string circuit_filename, circuit_name, tech_filename, tech_name, working_dir, report_filename;
        
        // input circuit
        if (vm.count("circuit") == 1) {
            circuit_filename = vm["circuit"].as<string>();
            circuit_name = circuit_filename.substr(circuit_filename.rfind('/')+1);
            circuit_name = circuit_name.substr(0, circuit_name.find('.'));
        } else {
            cout << optional_args << "\n";
            return 0;
        }
        
        // input tech lib
        if (vm.count("tech_lib")) {
            tech_filename = vm["tech_lib"].as<string>();
            tech_name = tech_filename.substr(tech_filename.rfind('/')+1);
            tech_name = tech_name.substr(0, tech_name.find('.'));
        } else {
            cout << optional_args << "\n";
            return 0;
        }
        
        cout << "Circuit : " << circuit_filename << "\n";
        cout << "Tech Lib: " << tech_filename << "\n";
        
        
        /******************************
         * Setup working dir
         ******************************/
        working_dir = vm["wdir"].as<string>();
        mkdir(working_dir.c_str(), S_IRWXU);
        cout << "WDIR    : " << working_dir << "\n";
        
        
        /******************************
         * Convert circuit using tech_lib
         ******************************/
        string outfile = working_dir + circuit_filename.substr(circuit_filename.rfind('/'));
        cout << "Outfile : " << outfile << "\n";
        cout << "I'm here!\n";
        circuit_filename = outfile;
        // copy tech_lib
        if ( tech_filename != string(working_dir + tech_filename.substr(tech_filename.rfind('/')) ) ) {
            ifstream src( tech_filename.c_str() );
            ofstream dst( string(working_dir + tech_filename.substr(tech_filename.rfind('/')) ).c_str() );
            dst << src.rdbuf();
            dst.close();
            src.close();
        }
        
        /******************************
         * Load circuit
         ******************************/
        Circuit circuit;
        
        load_circuit(&circuit, circuit_filename, mono_lib);
        
        Security *security;
        Circuit G, H, F, R;
        G.copy(&circuit);
        G.remove_io();
        
        // Added By Karl
        for (int i = 0; i < igraph_ecount(&G); i++) {
            SETEAN(&G, "Lifted", i, NotLifted);
            SETEAN(&G, "Original", i, NotOriginal);
            SETEAN(&G, "ID", i, i);
            SETEAN(&G, "Removed", i, NotRemoved);
            SETEAS(&G, "Tier", i, "Lifted");
            SETEAN(&G, "Dummy", i, kNotDummy);
        }
        
        for (int i = 0; i < igraph_vcount(&G); i++) {
            SETVAN(&G, "Removed", i, NotRemoved);
            SETVAN(&G, "ID", i, i);
            SETVAS(&G, "Tier", i, "Top");
            SETVAN(&G, "Dummy", i, kNotDummy);
        }
        ////////////////
        
        circuit.save( working_dir + "/circuit.gml" );
        G.save( working_dir + "/G_circuit.gml" );
        
        
        /****************************************************************
         * print G
         ****************************************************************/
        if ( test_args.size() > 0 && -1 == atoi(test_args[0].c_str())) {
            G.print();
        }
        
        
        /****************************************************************
         * L0
         ****************************************************************/
        if ( test_args.size() > 0 && 0 == atoi(test_args[0].c_str())) {
            
            int max_count(2);
            if (test_args.size() == 2)
                max_count = atoi(test_args[1].c_str());
            
            H.copy(&G);
            H.rand_del_edges(remove_percent);
            H.save( working_dir + "/H_circuit.gml" );
            
//            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            cout << "Rand L0: |V(G)| = "  << (int) igraph_vcount(&G);
            cout << ", |E(G)| = "         << (int) igraph_ecount(&G);
            cout << ", |V(H)| = "         << (int) igraph_vcount(&H);
            cout << ", |E(H)| = "         << (int) igraph_ecount(&H) << "\n";
            
            cout << " " << boolalpha << security->L0(max_count, false) << endl;
        }
        
        
        
        /****************************************************************
         * L1
         ****************************************************************/
        if ( test_args.size() > 0 && 1 == atoi(test_args[0].c_str())) {
            cout<<"lolzi"<<endl;
            int max_L1(2);
            if (test_args.size() == 2)
                max_L1 = atoi(test_args[1].c_str());
            
            H.copy(&G);
            H.rand_del_edges(remove_percent);
            
            if (vm.count("continue_file")) {
                H.rand_del_edges((float) 1.0); // delete all edges
                string filename = vm["continue_file"].as<string>();
                ifstream file;
                try {
                    file.open(filename.c_str());
                    
                    while (file.good()) {
                        string line;
                        int L0, L1;
                        Edge edge;
                        
                        getline(file, line);
                        if (parse(line, &G, L1, L0, edge)) {
                            if (L1 >= max_L1) { // If the line in the file shows an L1 higher than max_L1 then the graph with this edge is more secure than without it, so we add it!
                                // Shouldn't we update max_L1??
                                H.add_edge(edge);
                                cout << "L1 = " << max_L1 << ", +<" << edge.first << "," << edge.second << ">" << endl;
                            }
                        }
                    }
                } catch(...) {}
            }
            
            H.save( working_dir + "/H_circuit.gml" );
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            cout << "Rand L1: |V(G)| = "  << (int) igraph_vcount(&G);
            cout << ", |E(G)| = "         << (int) igraph_ecount(&G);
            cout << ", |V(H)| = "         << (int) igraph_vcount(&H);
            cout << ", |E(H)| = "         << (int) igraph_ecount(&H) << "\n";
            
            cout << " " << boolalpha << security->L1(max_L1, false) << endl;
        }
        
        
        
        /****************************************************************
         * #L1
         ****************************************************************/
        if ( test_args.size() == 1 && 2 == atoi(test_args[0].c_str())) {
            
            int max_L1(2);
            if (test_args.size() == 2)
                max_L1 = atoi(test_args[1].c_str());
            
            H.copy(&G);
            H.rand_del_edges(remove_percent);
            
            if (vm.count("continue_file")) {
                H.rand_del_edges((float) 1.0);
                string filename = vm["continue_file"].as<string>();
                ifstream file;
                try {
                    file.open(filename.c_str());
                    
                    while (file.good()) {
                        string line;
                        int L0, L1;
                        Edge edge;
                        
                        getline(file, line);
                        if (parse(line, &G, L1, L0, edge)) {
                            H.add_edge(edge);
                            max_L1 = L1; // Shouldn't we compare max_L1 and L1 first?
                            cout << "L1 = " << max_L1 << ", +<" << edge.first << "," << edge.second << ">" << endl;
                        }
                    }
                } catch(...) {}
            }
            
            H.save( working_dir + "/H_circuit.gml" );
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            cout << "Rand L1: |V(G)| = "  << (int) igraph_vcount(&G);
            cout << ", |E(G)| = "         << (int) igraph_ecount(&G);
            cout << ", |V(H)| = "         << (int) igraph_vcount(&H);
            cout << ", |E(H)| = "         << (int) igraph_ecount(&H) << "\n";
            cout << " " << security->L1(false);
        }
        
        
        
        /****************************************************************
         * S1_rand
         ****************************************************************/
        if ( test_args.size() == 1 && 3 == atoi(test_args[0].c_str())) {
            
            int max_L1 = G.max_L1();
            H.copy(&G);
            H.rand_del_edges((float) 1.0);
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            string output;
            output = "S1_rand ("  + G.get_name() + ")";
            output = report(output, &G, &H, max_L1);
            cout << output;
            
            security->S1_rand(num_threads);
        }
        
        /****************************************************************
         * S1_greedy
         ****************************************************************/
        if ( test_args.size() >= 1 && 4 == atoi(test_args[0].c_str())) {
            int min_L1(2), max_L1 = G.max_L1();
            //kmax = max_L1;
            H.copy(&G);
            H.rand_del_edges((float) 1.0);
            bool done(false);
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            if (vm.count("continue_file")) {
                string filename = vm["continue_file"].as<string>();
                ifstream file;
                
                for (int eid = 0; eid < igraph_ecount(&G); eid++)
                    SETEAS(&G, "style", eid, "invis");
                
                try {
                    file.open(filename.c_str());
                    
                    while (file.good()) {
                        string line;
                        int L0, L1;
                        Edge edge;
                        
                        getline(file, line);
                        
                        
                        if (parse(line, &G, L1, L0, edge)) {
                            if (L1 < min_L1) {
                                done = true;
                                break;
                            }
                            
                            H.add_edge(edge);
                            max_L1 = L1;
                            cout << "L1 = " << max_L1 << ", +<" << edge.first << "," << edge.second << ">" << endl;
                        }
                    }
                } catch(...) {}
            }
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            string output;
            output = "S1_greedy ("  + G.get_name() + ")";
            output = report(output, &G, &H, max_L1);
            cout << output;
            
            fstream report;
            if (!done)
            {
                clock_t tic = clock();
                security->L1_main(outName, remove_vertices_max, num_threads, min_L1, max_L1); // Added by Karl (true, remove_vertices_max)
                clock_t toc = clock();
                cout << endl << "Heuristic took: ";
                cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
            }
        }
        
        
        /****************************************************************
         * k-isomorphism
         ****************************************************************/
        if ( test_args.size() >= 1 && 10 == atoi(test_args[0].c_str())) {
            
            int min_L1(2), max_L1 = G.max_L1();
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            igraph_vector_t match_vert;
            igraph_vector_init(&match_vert, 0);
            
            int original = igraph_vcount(&G);
            
            // Creare a vector of size of how many colors we have and fill it with how many vertices of that color exist in the graph
            //        for (int i = 0; i < igraph_vcount(&G); i++)
            //        {
            //            int color = VAN(&G, "colour", i);
            //            if (igraph_vector_size(&match_vert) < color + 1)
            //                for (int j = igraph_vector_size(&match_vert); j <= color; j++)
            //                {
            //                    igraph_vector_push_back(&match_vert, 0);
            //                }
            //            VECTOR(match_vert)[color]++;
            //        }
            cout<<"vcount1: "<<igraph_vcount(&G)<<endl;
            cout<<"ecount1: "<<igraph_ecount(&G)<<endl;
            igraph_t temp;
            //	igraph_copy(&temp, &G);
            //	igraph_destroy(&G);
            //	for (min_L1 = max_L1; min_L1 >= 1; min_L1--) {
            //		igraph_copy(&G, &temp);
            
            // if a color doesn't have a multiple of k (min_L1) vertices, add vertices with that color
            int count = 0;
            //        for (int i = 0; i < igraph_vector_size(&match_vert); i++)
            //        {
            //            int n = ((int) VECTOR(match_vert)[i]) % min_L1; if (n == 0) continue;
            //            for (int j = 0; j < min_L1 - n; j++)
            //            { count++;
            //                igraph_add_vertices(&G, 1, 0);
            //                SETVAN(&G, "colour", igraph_vcount(&G) - 1, i);
            //                SETVAN(&G, "Removed", igraph_vcount(&G) - 1, NotRemoved);
            //                SETVAN(&G, "ID", igraph_vcount(&G) - 1, igraph_vcount(&G) - 1);
            //            }
            //        }
            cout<<"vcount2: "<<igraph_vcount(&G)<<endl;
            cout<<"ecount2: "<<igraph_ecount(&G)<<endl;
            
            cout << count << " "<<endl;
            cout.flush();
            H.copy(&G);
            H.rand_del_edges((float) 1.0);
            H.del_vertices();
            
            F.copy(&G);
            R.copy(&G);
            
            bool done(false);
            
            // Added by Karl
            //        for (int i = igraph_ecount(&G)-1; i >= 0; i--) {
            //            igraph_es_t es;
            //            igraph_es_1(&es, i);
            //            igraph_delete_edges(&G,es);
            //        }
            //        cout<<"aahhh"<<endl;
            //        for (int i = igraph_vcount(&G)-1; i >= 0; i--) {
            //            igraph_vs_t es;
            //            igraph_vs_1(&es, i);
            //            igraph_delete_vertices(&G,es);
            //        }
            //        cout<<"aahhh2"<<endl;
            //
            //        igraph_add_vertices(&G, 9, 0);
            //        igraph_add_edge(&G,0,1);
            //        igraph_add_edge(&G,0,2);
            //        igraph_add_edge(&G,0,3);
            //        igraph_add_edge(&G,0,4);
            //        igraph_add_edge(&G,1,2);
            //        igraph_add_edge(&G,1,5);
            //        igraph_add_edge(&G,1,6);
            //        igraph_add_edge(&G,2,7);
            //        igraph_add_edge(&G,2,8);
            //        H.copy(&G);
            //        G.save( working_dir + "/G_circuit.gml");
            //        H.rand_del_edges((float) 1.0);
            ////////////////
            
            security = new Security(&G, &H, &F, &R);
            security->setConfBudget(budget);
            
            string output;
            output = "S1_greedy ("  + G.get_name() + ")";
            output = report(output, &G, &H, max_L1);
            //        cout << output;
            
            fstream report;
            if (!done)
            {
                clock_t tic = clock();
                security->kiso(target_security, max_L1, maxPAGsize, tresh, baseline);
                clock_t toc = clock();
                //        cout << endl << "Heuristic took: ";
                //       cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
            } //igraph_destroy(&G);}
            
        }
        
        /****************************************************************
         * Tree test
         ****************************************************************/
        if ( test_args.size() >= 1 && 7 == atoi(test_args[0].c_str())) {
            
            int min_L1(2), max_L1 = G.max_L1();
            
            //G.erase();
            igraph_vs_t vs;
            igraph_vs_all(&vs);
            
            igraph_delete_vertices(&G, vs);
            const int depth = 7;
            igraph_add_vertices(&G, pow(2,depth)-1, 0);
            for (int i=0; i < pow(2,depth-1); i++)
            {
                int level = floor(log(i+1)/log(2));
                igraph_add_edge(&G,i,pow(2,level+1) + (i-pow(2,level))*2 - 1);
                igraph_add_edge(&G,i,pow(2,level+1) + (i-pow(2,level))*2);
            }
            
            for (int i=0; i < igraph_vcount(&G); i++)
            {
                SETVAN(&G, "colour", i, 0);
                SETVAS(&G, "type", i, "invf101");
                string label = "label";
                SETVAS(&G, "label", i, label.c_str());
            }
            
            H.copy(&G);
            for (int i=0; i < igraph_vcount(&H); i++)
            {
                SETVAN(&H, "colour", i, 0);
            }
            
            H.rand_del_edges((float) 1.0);
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            cout << "I'm here!"; cout.flush();
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            string output;
            cout << "I'm here!";
            output = "S1_greedy ("  + G.get_name() + ")";
            cout << "I'm here!";
            output = report(output, &G, &H, max_L1);
            cout << output;
            
            cout << "I'm here!";
            //security->S1_greedy(num_threads, min_L1, max_L1);
            
        }
        
        /****************************************************************
         * Compute security level G if no wires are lifted
         ****************************************************************/
        if ( test_args.size() >= 1 && 5 == atoi(test_args[0].c_str())) {
            
            H.copy(&G);
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            H.rand_del_edges((float) 0.0);
            
            //	security->clean_solutions();
            
            string output;
            output = "Security of circuit ("  + G.get_name() + ") if no wires are lifted: ";
            cout << output;
            
            security->S1_self();
            
        }
        
        
        
        /****************************************************************
         * Solve LIFT(G, k, eta)
         ****************************************************************/
        
        if ( test_args.size() >= 1 && 6 == atoi(test_args[0].c_str())) {
            
            int min_L1(2), max_L1 = G.max_L1(), eta = igraph_ecount(&G);
            H.copy(&G);
            //        H.rand_del_edges((float) 1.0);
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                eta = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            
            clock_t tic = clock();
            security->rSAT(min_L1, max_L1, eta);
            clock_t toc = clock();
            cout << endl << "SAT took: ";
            cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
            
        }
        
        /****************************************************************
         * Solve LIFT(G, k, eta, u)
         ****************************************************************/
        
        if ( test_args.size() >= 1 && 8 == atoi(test_args[0].c_str())) {
            
            int min_L1(2), max_L1 = G.max_L1(), eta = igraph_ecount(&G), u;
            H.copy(&G);
            //        H.rand_del_edges((float) 1.0);
            
            if ( test_args.size() == 4 ) {
                u = atoi(test_args[1].c_str());
                min_L1 = atoi(test_args[2].c_str());
                eta = atoi(test_args[3].c_str());
            } else if ( test_args.size() == 3 )
            {
                u = atoi(test_args[1].c_str());
                min_L1 = atoi(test_args[2].c_str());
            }
            else if ( test_args.size() == 2 )
                u = atoi(test_args[1].c_str());
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            
            clock_t tic = clock();
            security->rSAT(min_L1, max_L1, eta, u, true);
            clock_t toc = clock();
            cout << endl << "SAT took: ";
            cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
            
        }
        
        /****************************************************************
         * Solve LIFT(G, k, eta, u)
         ****************************************************************/
        
        if ( test_args.size() >= 1 && 9 == atoi(test_args[0].c_str())) {
            
            int min_L1(2), max_L1 = G.max_L1(), eta = igraph_ecount(&G), u;
            H.copy(&G);
            //        H.rand_del_edges((float) 1.0);
            
            if ( test_args.size() == 4 ) {
                u = atoi(test_args[1].c_str());
                min_L1 = atoi(test_args[2].c_str());
                eta = atoi(test_args[3].c_str());
            } else if ( test_args.size() == 3 )
            {
                u = atoi(test_args[1].c_str());
                min_L1 = atoi(test_args[2].c_str());
            }
            else if ( test_args.size() == 2 )
                u = atoi(test_args[1].c_str());
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            
            clock_t tic = clock();
            security->rSAT(min_L1,max_L1, eta, u);
            clock_t toc = clock();
            cout << endl << "SAT took: ";
            cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
            
        }
        
        /****************************************************************
         * simulated annealing
         ****************************************************************/
        
        
        if ( test_args.size() >= 1 && 11 == atoi(test_args[0].c_str())) {
            const double MAX_TEMP = 100000.0;
            const int MAX_ITERATIONS = 2000;
            const double TEMP_CHANGE = 0.98;
            int no_of_edges = atoi(test_args[1].c_str());
            int min_L1(2), max_L1 = G.max_L1();
            H.copy(&G);
            H.rand_del_edges(igraph_ecount(&G) - no_of_edges);
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            int current_k_security = security->L1();
            int best_k_security = current_k_security;
            cout << "Starting with: " << current_k_security << endl;
            // delete security;
            double temperature = MAX_TEMP;
            srand( time(NULL));
            
            clock_t tic = clock();
            for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
                bool done(false);
                
                // H.rand_del_edges(1);
                vector<Edge> unlifted_edge_list;
                for (int eid = 0; eid < igraph_ecount(&H); eid++) {
                    Edge edge = H.get_edge(eid);
                    unlifted_edge_list.push_back(edge);
                }
                random_shuffle(unlifted_edge_list.begin(), unlifted_edge_list.end());
                H.del_edge(unlifted_edge_list[0]);
                
                
                vector<Edge> edge_list;
                for (int eid = 0; eid < igraph_ecount(&G); eid++) {
                    Edge edge = G.get_edge(eid);
                    if (!H.test_edge(edge)) edge_list.push_back(edge);
                }
                random_shuffle(edge_list.begin(), edge_list.end());
                H.add_edge(edge_list[0]);
                
                delete security;
                security = new Security(&G, &H, &F, &R);                security->setConfBudget(budget);
                
                int new_k_security = security->L1();
                if (new_k_security >= current_k_security) {
                    current_k_security = new_k_security;
                    if (current_k_security >= best_k_security) best_k_security = current_k_security;
                }
                else {
                    if (exp((new_k_security-current_k_security)/temperature) >= ((double) rand())/ RAND_MAX) current_k_security = new_k_security;
                    else {
                        H.add_edge(unlifted_edge_list[0]);
                        H.del_edge(edge_list[0]);
                    }
                }
                temperature *= TEMP_CHANGE;
                if ((iter + 1 )% 10 == 0) cout << " > iteration " << iter + 1 << ", temp=" << temperature << ", best=" << best_k_security << endl;
                cout.flush();
                // delete security;
            }
            clock_t toc = clock();
            cout << endl << "Annealing took: ";
            cout << (toc-tic)/CLOCKS_PER_SEC << endl;
        }
        
        
        
        /****************************************************************
         * L1(label)
         ****************************************************************/
        if ( test_args.size() >= 1 && 5 == atoi(test_args[0].c_str())) {
            
            string label = "";
            if (test_args.size() == 2)
                label = test_args[1];
            
            int max_L1(2);
            H.copy(&G);
            H.rand_del_edges(remove_percent);
            
            if (vm.count("continue_file")) {
                H.rand_del_edges((float) 1.0);
                string filename = vm["continue_file"].as<string>();
                ifstream file;
                try {
                    file.open(filename.c_str());
                    
                    while (file.good()) {
                        string line;
                        int L0, L1;
                        Edge edge;
                        
                        getline(file, line);
                        if (parse(line, &G, L1, L0, edge)) {
                            H.add_edge(edge);
                            max_L1 = L1;
                            cout << "L1 = " << max_L1 << ", +<" << edge.first << "," << edge.second << ">" << endl;
                        }
                    }
                } catch(...) {}
            }
            
            H.save( working_dir + "/H_circuit.gml" );
            
            security = new Security(&G, &H, &F, &R);            security->setConfBudget(budget);
            
            security->L1(label);
        }
        
        G.save(working_dir + "/G2_circuit.gml");
        if ( test_args.size() >= 1 && atoi(test_args[0].c_str()) >= 0) {
            // Added by Karl
            // Delete lifted vertices
            for (int i = 0; i < igraph_vcount(&G); i++)
                if (i < igraph_vcount(&H) && VAN(&H,"Lifted",i) == Lifted)
                    igraph_delete_vertices(&H,igraph_vss_1(i--)); // this will move all next vertices one inde to the left, this is why we do i--
            ///////////////
            H.save( working_dir + "/H_circuit.gml" );
            delete security;
        }
        
        stringstream ss1;
        ss1 << tresh;
        string str1 = ss1.str();
        
        stringstream ss4;
        ss4 << maxPAGsize;
        string str4 = ss4.str();
        
        stringstream ss3;
        ss3 << target_security;
        string str3 = ss3.str();

        F.save(working_dir + "/" + name + "_PAG_" + str4 + "_tresh_"+ str1 + "_lvl_" + str3 + "_F_circuit.gml");
        R.save(working_dir + "/" + name + "_PAG_" + str4 + "_tresh_"+ str1 + "_lvl_" + str3 + "_R_circuit.gml");
        
        target_security *= 2;
        
        if (print_gate)
            G.print();
        
        if (print_blif)
            print_file(circuit_filename);
        
        if (print_solns)
            security->print_solutions();
        
        if (print_verilog)
            security->print_solutions();
        
    }
    
    clock_t toc = clock();
    
    cout<<"All took: "<<(double) (toc-tic)/CLOCKS_PER_SEC<<endl;
    koutfile2<<"All took: "<<(double) (toc-tic)/CLOCKS_PER_SEC<<endl;
    koutfile.close();
    koutfile2.close();
    
    return 0;
}








