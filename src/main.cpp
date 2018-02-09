
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
ofstream area_file;
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
    int num_threads, budget, remove_vertices_max, maxPAGsize, tresh;
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
    
    int target_securityy = target_security; //2
    
    cout<<outName<<" "<<tresh<<endl;
    stringstream ss;
    ss << tresh;
    string str = ss.str();
    
    stringstream ss2;
    ss2 << maxPAGsize;
    string str2 = ss2.str();
    
    stringstream ss3;
    ss3 << target_security;
    string str3 = ss3.str();
    
    string outname = "PAG_testing/" + outName + "_PAG_" + str2 + "_tresh_" + str + "_k_" + str3;
    name = outName;
    outName = outname + ".txt";
    koutfile.open(outName.c_str());
    koutfile<<"# security"<<"     "<<"# lifted e"<<endl;
    
    outname = outname + "_report.txt";
    koutfile2.open(outname.c_str());
    
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
    
    //Security *security;
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
    
    if ( test_args.size() > 0 && 0 == atoi(test_args[0].c_str())) {
        string area_out = "areas/" + circuit_name + "_areas.txt";
        area_file.open(area_out.c_str());
        
        float nand_area = 1.877200, inv_area = 1.407900;
        string NAND = "nanf201", INV = "invf101";
        
        float area = 0.0;
        
        for (int i = 0; i < igraph_vcount(&G); i++)
            area += (string)VAS(&G, "type", i)==NAND ? nand_area:inv_area;
        
        cout<<area<<endl;
        
        for (int pag = 4; pag < 7; pag++) {
            stringstream int_pag;
            int_pag << pag;
            string str_pag = int_pag.str();
            
            for (int tresh = 0; tresh < 5; tresh += 2) {
                stringstream int_tresh;
                int_tresh << tresh;
                string str_tresh = int_tresh.str();
                
                for (int k = 2; k < 33; k++) {
                    stringstream int_k;
                    int_k << k;
                    string str_k = int_k.str();
                    
                    Circuit T;
                    FILE* in;
                    
                    
                    string filenme = "wdir/" + circuit_name + "/" + circuit_name + "_PAG_" + str_pag + "_tresh_" + str_tresh + "_lvl_" + str_k + "_F_circuit.gml";
                    in = fopen(filenme.c_str(),"r");
                    if (in == NULL)
                        continue;
                    cout<<filenme<<endl;
                    igraph_read_graph_gml(&T, in);
                    
                    int nand = 0, inv = 0;
                    int Nand = 1, Inv = 0;
                    
                    float top_area = 0.0, bottom_area = 0.0;
                    
                    for (int i = 0; i < igraph_vcount(&T); i++) {
                        if ((string)VAS(&T, "Tier", i) == "Top") {
                            if (VAN(&T, "colour", i) == Nand)
                                top_area += nand_area;
                            else if (VAN(&T, "colour", i) == Inv)
                                top_area += inv_area;
                        }
                        else if ((string)VAS(&T, "Tier", i) == "Bottom") {
                            if (VAN(&T, "colour", i) == Nand)
                                bottom_area += nand_area;
                            else if (VAN(&T, "colour", i) == Inv)
                                bottom_area += inv_area;
                            
                            
                            igraph_vector_t eids;
                            igraph_vector_init(&eids,0);
                            igraph_incident(&T, &eids, i, IGRAPH_ALL);
                            
                            int size = igraph_vector_size(&eids);
                            int count = 0;
                            //cout<<i<<" :";
                            for (int j = 0; j < size; j++){
                                // cout<<VECTOR(eids)[j]<<" "<<EAS(&T, "Tier", VECTOR(eids)[j])<<" ";
                                if ((string)EAS(&T, "Tier", VECTOR(eids)[j]) == "Top" || (string)EAS(&T, "Tier", VECTOR(eids)[j]) == "Crossing" || (string)EAS(&T, "Tier", VECTOR(eids)[j]) == "Lifted") {
                                    count++;
                                    // cout<<"count = "<<count<<" ";
                                }
                            }
                            //cout<<endl;
                            if (count == size)
                                if (VAN(&T, "colour", i) == Nand)
                                    nand++;
                                else if (VAN(&T, "colour", i) == Inv)
                                    inv++;
                            
                            igraph_vector_destroy(&eids);
                        }
                    }
                    
                    top_area *= 2;
                    
                    if (nand > 0 && nand < 10)
                        nand = 10 - nand;
                    else if (nand >= 10)
                        nand = 0;
                    
                    if (inv > 0 && inv < 10)
                        inv = 10 - inv;
                    else if (inv >= 10)
                        inv = 0;
                    
                    bottom_area = bottom_area + nand*nand_area + inv*inv_area;
                    
                    cout<<nand<<" "<<inv<<endl;
                    
                    float bond_area = 0;
                    filenme = "PAG_testing/" + circuit_name + "/" + circuit_name + "_PAG_" + str_pag + "_tresh_" + str_tresh + "_k_" + str_k + "_report.txt";
                    ifstream infile(filenme.c_str());
                    string line;
                    boost::regex bond_rx("Total bonds:");
                    boost::regex num_rx("\\d{3}");
                    
                    string s_bond_area;
                    
                    while (infile.good()) {
                        getline(infile, line);
                        
                        if (boost::regex_search(line, bond_rx) && boost::regex_search(line, num_rx)) {
                            boost::sregex_token_iterator itera(line.begin(),  line.end(), num_rx, 0);
                            boost::sregex_token_iterator end;
                            
                            s_bond_area = *itera;
                            cout<<"helloowz: "<<s_bond_area<<endl;
                        }
                    }
                    
                    bond_area = atof(s_bond_area.c_str());
                    
                    bond_area *= 16;
                    
                    cout<<"top = "<<top_area<<" bottom = "<<bottom_area<<" bond = "<<bond_area<<endl;
                    float temp = max(top_area, bottom_area);
                    area = max(temp, bond_area);
                    
                    cout<<area<<endl;
                    area_file<<"PAG: "<<str_pag<<" Threshold: "<<str_tresh<<" security: "<<str_k<<" area = "<<area<<endl;
                }
            }
        }
        
        return 0;
    }
    
    while (target_securityy <= target_security) { // 32
        // Debug
        //string command = "ps u > usage_" + str3 + "_start.txt";
        //system(command.c_str());
        ////////
        
        /****************************************************************
         * k-isomorphism
         ****************************************************************/
        if ( test_args.size() >= 1 && 10 == atoi(test_args[0].c_str())) {
            cout<<"Kiso"<<endl;
            int min_L1(2), max_L1 = G.max_L1();
            
            if ( test_args.size() == 3 ) {
                min_L1 = atoi(test_args[1].c_str());
                max_L1 = atoi(test_args[2].c_str());
            } else if ( test_args.size() == 2 )
                min_L1 = atoi(test_args[1].c_str());
            
            int original = igraph_vcount(&G);

            cout<<"vcount1: "<<igraph_vcount(&G)<<endl;
            cout<<"ecount1: "<<igraph_ecount(&G)<<endl;
            
            int count = 0;
            
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
            
            //security = new Security(&G, &H, &F, &R);
            Security security(&G, &H, &F, &R);
            
            fstream report;
            if (!done) {
                clock_t tic = clock();
                //security->kiso(target_security, max_L1, maxPAGsize, tresh, baseline);
                security.kiso(target_security, max_L1, maxPAGsize, tresh, baseline);
                clock_t toc = clock();
            }
            
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
        }
        
        stringstream ss1;
        ss1 << tresh;
        string str1 = ss1.str();
        
        stringstream ss4;
        ss4 << maxPAGsize;
        string str4 = ss4.str();

        F.save(working_dir + "/" + name + "_PAG_" + str4 + "_tresh_"+ str1 + "_lvl_" + str3 + "_F_circuit.gml");
        R.save(working_dir + "/" + name + "_PAG_" + str4 + "_tresh_"+ str1 + "_lvl_" + str3 + "_R_circuit.gml");
        
        if (print_gate)
            G.print();
        
        if (print_blif)
            print_file(circuit_filename);
        
        // Debug
        //command = "ps u > usage_" + str3 + "_end.txt";
        //system(command.c_str());
        ////////
        
        //delete security;
        
        target_securityy *= 2;
    }
    
    // Debug
    //string command = "ps u > usage_final.txt";
    //system(command.c_str());
    
    clock_t toc = clock();
    
    cout<<"All took: "<<(double) (toc-tic)/CLOCKS_PER_SEC<<endl;
    koutfile2<<"All took: "<<(double) (toc-tic)/CLOCKS_PER_SEC<<endl;
    koutfile.close();
    koutfile2.close();
    
    return 0;
}
