// dummy
/*************************************************************************//**
                                                                            *****************************************************************************
                                                                            * @file        sat.cpp
                                                                            * @brief
                                                                            * @author      Frank Imeson
                                                                            * @date
                                                                            *****************************************************************************
                                                                            ****************************************************************************/

// http://www.ros.org/wiki/CppStyleGuide

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "security.hpp"
#include "main.hpp"

#define DEBUG
//#define PRINT_SOLUTION
//#define MEASURE_TIME
//#define MEASURE_TIME_S1
#define USE_SOLNS
//#define NRAND
//#define VF2

using namespace formula;
using namespace std;

// Added by Karl
// Lifiting heuristic
LiftingInfo LiftedVnE;
OptimalSolution optimalSolution;
int vcount = 0;
int notLifted = 0;

// PAG
int maxPAGsize = 0;
vector<set<int> > edge_neighbors;
vector<PAG> pags;
map<int,vector<int> > colors;
int H_v_dummy = 0;
int H_e_dummy = 0;
int G_v_lifted = 0;
int G_e_lifted = 0;
bool start = false;
vector<set<int> > vertex_neighbors_out;
vector<set<int> > vertex_neighbors_in;
set<int> top_tier_vertices;
map<int, set<int> >top_tier_edges;
////////////////

/************************************************************//**
                                                               * @brief
                                                               * @return            string representation of connective
                                                               * @version						v0.01b
                                                               ****************************************************************/
igraph_bool_t check_map (
                         const igraph_t *graph1,
                         const igraph_t *graph2,
                         const igraph_integer_t vid1,
                         const igraph_integer_t vid2,
                         void *arg)
{
    L1_struct *mapped = (L1_struct*) arg;
    if (vid2 == mapped->vid2) {
        //        if (mapped->mapped[vid2][vid1]) //
        //            cout << "reject vid2(" << vid2 << ") -> vid1(" << vid1 << ")" << endl;
        return !mapped->mapped[vid2][vid1];
    }
    return true;
};


/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
bool l1_edge_lt (const L1_Edge* rhs, const L1_Edge* lhs) {
    return rhs->L1_prev < lhs->L1_prev;
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
bool l1_edge_info_lt (const EdgeInfo &rhs, const EdgeInfo &lhs) {
    return rhs.max_degree < lhs.max_degree;
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
string report (string prefix, Circuit *G, Circuit *H, int L1, int L0, Edge edge) {
    
    stringstream out;
    out << prefix
    << ": |V(G)| = " << (int) igraph_vcount(G)
    << ", |E(G)| = " << (int) igraph_ecount(G)
    << ", |V(H)| = " << (int) igraph_vcount(H)
    << ", |E(H)| = " << (int) igraph_ecount(H);
    if (L0 > 0) out << ", #L0 = " << L0;
    /*if (L1 > 0)*/ out << ", L1 = "  << L1; // Removed by Karl
    if (edge.first >= 0) out << ", +<" << edge.first << "," << edge.second << ">";
    out << endl;
    
    return out.str();
}


/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
bool parse (string line, Circuit *G, int &L1, int &L0, Edge &edge) {
    
    boost::regex report_rx ("\\|V\\(G\\)\\|");
    
    if ( regex_search(line, report_rx) ) {
        
        boost::regex num_rx   ("\\d+");
        boost::regex VG_rx    ("\\|V\\(G\\)\\| = \\d+");
        boost::regex EG_rx    ("\\|E\\(G\\)\\| = \\d+");
        boost::regex L0_rx    ("L0 = \\d+");
        boost::regex L1_rx    ("L1 = \\d+");
        boost::regex edge_rx  ("<\\d+,\\d+>");
        
        boost::sregex_token_iterator VG_token   (line.begin(), line.end(), VG_rx, 0);
        boost::sregex_token_iterator EG_token   (line.begin(), line.end(), EG_rx, 0);
        boost::sregex_token_iterator L0_token   (line.begin(), line.end(), L0_rx, 0);
        boost::sregex_token_iterator L1_token   (line.begin(), line.end(), L1_rx, 0);
        boost::sregex_token_iterator Edge_token (line.begin(), line.end(), edge_rx, 0);
        boost::sregex_token_iterator end;
        string token;
        
        assert (VG_token != end);
        {
            token = *VG_token;
            boost::sregex_token_iterator num(token.begin(), token.end(), num_rx, 0);
            assert (num != end);
            if ((int) igraph_vcount(G) != atoi(string(*num).c_str())) {
                cout << "|V(G)| = " << (int) igraph_vcount(G) << ", |V(G)| = " << atoi(string(*num).c_str()) << endl;
            }
            assert ( (int) igraph_vcount(G) == atoi(string(*num).c_str()) );
        }
        
        assert (EG_token != end);
        {
            token = *EG_token;
            boost::sregex_token_iterator num(token.begin(), token.end(), num_rx, 0);
            assert (num != end);
            assert ( (int) igraph_ecount(G) == atoi(string(*num).c_str()) );
        }
        
        if (L0_token != end) {
            token = *L0_token;
            boost::sregex_token_iterator num(token.begin(), token.end(), num_rx, 0);
            num++; // L0 yeilds a num
            assert (num != end);
            L0 = atoi(string(*num).c_str());
        }
        
        if (L1_token != end) {
            token = *L1_token;
            boost::sregex_token_iterator num(token.begin(), token.end(), num_rx, 0);
            num++; // L1 yeilds a num
            assert (num != end);
            L1 = atoi(string(*num).c_str());
        } else {
            return false;
        }
        
        if (Edge_token != end) {
            token = *Edge_token;
            boost::sregex_token_iterator num(token.begin(), token.end(), num_rx, 0);
            assert (num != end);
            edge.first = atoi(string(*num).c_str());
            num++;
            assert (num != end);
            edge.second = atoi(string(*num).c_str());
        } else {
            return false;
        }
        
        return true;
    } else {
        return false;
    }
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
string report (igraph_vector_t *soln) {
    
    string output;
    stringstream out;
    
    out << "map21: ";
    for (unsigned int i = 0; i < igraph_vector_size(soln); i++)
        out << VECTOR(*soln)[i] << ", ";
    output = out.str();
    output = output.substr(0, output.size()-2) + "\n";
    
    return output;
}


/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
bool parse (string line, igraph_vector_t *soln) {
    
    boost::regex map_rx ("map21: ");
    
    if ( regex_search(line, map_rx, boost::match_continuous) ) {
        
        boost::regex num_rx ("\\d+");
        boost::sregex_token_iterator end;
        boost::sregex_token_iterator num(line.begin(), line.end(), num_rx, 0);
        num++; // map21 yeilds a num
        
        for (unsigned int i = 0; i < igraph_vector_size(soln); i++, num++) {
            assert (num != end);
            VECTOR(*soln)[i] = atoi(string(*num).c_str());
        }
        assert (num == end);
        return true;
    } else {
        return false;
    }
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
Security::Security (Circuit *_G, Circuit *_H, Circuit *_F, Circuit *_R)
{
    G = _G;
    H = _H;
    F = _F;
    R = _R;
    
    igraph_vector_int_init(&colour_G, igraph_vcount(G));
    igraph_vector_int_init(&colour_H, igraph_vcount(H));
    
    for (unsigned int i=0; i<igraph_vcount(G); i++)
        VECTOR(colour_G)[i] = (int) VAN(G, "colour", i);
    
    for (unsigned int i=0; i<igraph_vcount(H); i++)
        VECTOR(colour_H)[i] = (int) VAN(H, "colour", i);
    
    isosat = new Isosat(G, H, &colour_G, &colour_H, 0, 0, &igraph_compare_transitives, 0, 0);
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
void Security::clean_solutions () {
    for (int i = solutions.size()-1; i >= 0; i--) {
        //cout<<"here 1"<<endl;
        igraph_bool_t iso(false);
        igraph_test_isomorphic_map (G, H, &colour_G, &colour_H, 0, 0, &iso, NULL, solutions[i],
                                    &igraph_compare_transitives, 0, 0);
        //cout<<"here 2"<<endl;
        
        if (!iso) {
            igraph_vector_destroy(solutions[i]);
            //cout<<"here 3"<<endl;
            solutions.erase(solutions.begin()+i);
            //cout<<"here 4"<<endl;
        }
        
    }
}




/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
void Security::print_solutions () {
    cout << endl;
    cout << "I'm here!" << endl;
    
    cout <<  solutions.size();
    for (int i = 0; i < solutions.size(); i++) {
        cout << "map21: ";
        igraph_vector_print(solutions[i]);
    }
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
void Security::add_edge (int eid) {
    H->add_edge(G->get_edge(eid));
    
    int from, to;
    igraph_edge(G, eid, &from, &to);
    igraph_get_eid(H, &eid, from, to, IGRAPH_DIRECTED, 1 /* error */);
    isosat->add_edge(G, H, eid, &colour_G, &colour_H, 0, 0, &igraph_compare_transitives, 0, 0);
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
int set_L1 (const Circuit *G, const vector<EdgeInfo> &edge_set) {
    
    vector<bool> from, to;
    for (unsigned int i=0; i<igraph_vcount(G); i++) {
        from.push_back(false);
        to.push_back(false);
    }
    
    int from_L1(0), to_L1(0);
    for (unsigned int i=0; i<edge_set.size(); i++) {
        Edge edge = edge_set[i].edge;
        if (!from[edge.first]) {
            from_L1++;
            from[edge.first] = true;
        }
        if (!to[edge.second]) {
            to_L1++;
            to[edge.second] = true;
        }
    }
    
    return min(from_L1, to_L1);
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            * precondition H is empty
                                                                            ****************************************************************************/
void Security::add_free_edges (int L1) {
    
    /******************************
     * Catorize edges
     ******************************/
    map<Edge, EdgeStats> edges;
    map<Edge, EdgeStats>::iterator it;
    
    for (unsigned int eid=0; eid<igraph_ecount(G); eid++) {
        Edge edge, colour;
        
        igraph_edge(G, eid, &edge.first, &edge.second);
        colour.first  = (int) VAN(G, "colour", edge.first);
        colour.second = (int) VAN(G, "colour", edge.second);
        
        it = edges.find(colour);
        if (it == edges.end()) edges[colour] = EdgeStats();
        
        edges[colour].unplaced.push_back( EdgeInfo(eid, edge, max(igraph_vertex_degree(G,edge.first), igraph_vertex_degree(G,edge.second)) ));
    }
    
    vector<bool> placed;
    for (unsigned int vid1 = 0; vid1 < igraph_vcount(G); vid1++)
        placed.push_back(false);
    
    
    
    /******************************
     * Add edges
     ******************************/
    it = edges.begin();
    while ( it != edges.end() ) {
        
        // removed partialy placed edges
        int i=0;
        while (i < (*it).second.unplaced.size() && (*it).second.unplaced.size() > 0) {
            Edge edge = (*it).second.unplaced[i].edge;
            if (placed[edge.first] || placed[edge.second]) {
                (*it).second.unplaced.erase((*it).second.unplaced.begin()+i);
                i = 0;
                continue;
            }
            i++;
        }
        sort((*it).second.unplaced.begin(), (*it).second.unplaced.end(), l1_edge_info_lt);
        reverse((*it).second.unplaced.begin(), (*it).second.unplaced.end());
        
        // pick edges to add
        for (unsigned int index = 0; index < (*it).second.unplaced.size(); index++) {
            
            if ( set_L1(G, (*it).second.unplaced) + (*it).second.placed.size() < L1 ) {
                break;
            }
            
            vector<EdgeInfo> test_set = (*it).second.unplaced;
            EdgeInfo test_edge = test_set[index];
            test_set.erase(test_set.begin()+index);
            int from = test_edge.edge.first; int to = test_edge.edge.second;
            
            for (unsigned int i = 0; i < test_set.size(); i++) { // remove an edge if it's like the one erased just earlier
                if (test_set[i].edge.first   == from || test_set[i].edge.first   == to ||
                    test_set[i].edge.second  == from || test_set[i].edge.second  == to) {
                    test_set.erase(test_set.begin()+i);
                    i = -1;
                }
            }
            
            if ((*it).second.placed.size() + set_L1(G, test_set) >= L1) {
                (*it).second.placed.push_back(test_edge);
                (*it).second.unplaced = test_set;
            }
            
        }
        //cout << "L1 = " << L1 << ", L1_set = " << set_L1(G, (*it).second.unplaced) << ", placed.size() = " << (*it).second.placed.size() << endl;
        // add edges
        for (unsigned int i = 0;  i < (*it).second.placed.size(); i++) {
            add_edge((*it).second.placed[i].eid);
            placed[(*it).second.placed[i].edge.first]  = true;
            placed[(*it).second.placed[i].edge.second] = true;
            string output;
            output = "S1_4free ("  + G->get_name() + ")";
            output = report(output, G, H, L1, 0, (*it).second.placed[i].edge);
            cout << output;
        }
        it++;
    }
    
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            run circuits/c17.blif tech_lib/minimal.genlib -w tmp -t -1
                                                                            ****************************************************************************/
bool Security::L0 (int max_count, bool quite) {
    if (!quite) {
        cout << " L0(" << max_count << "): ";
        cout.flush();
    }
    
    int count = 0;
    igraph_bool_t iso(true);
    while (iso && count < max_count) {
        igraph_vector_t map21;
        igraph_vector_init(&map21, igraph_vcount(H));
        isosat->solve(&iso, NULL, &map21);
        if (iso) {
            isosat->negate(NULL, &map21);
            count++;
            if (!quite) {
                cout << '*';
                cout.flush();
            }
        } else {
            return false;
        }
        igraph_vector_destroy(&map21);
    }
    return true;
}


/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
void Security::L1 (string label) {
    //cout << "L1(label): " << label << endl;
    int index(-1);
    for (unsigned int i=0; i<igraph_vcount(G); i++) {
        //cout << VAS(G, "label", i) << endl;
        if ( VAS(G, "label", i) == label ) {
            index = i;
            //cout << "index found: " << index << endl;
        }
    }
    
    vec<Lit> reject;
    igraph_bool_t iso(true);
    while (iso) {
        
        igraph_vector_t map21;
        igraph_vector_init(&map21, igraph_vcount(H));
        
        iso = false;
        isosat->solve(&iso, NULL, &map21, &reject);
        
        if (iso) {
            reject.push( isosat->translate(M21(index, VECTOR(map21)[index], true)) );
            cout << label << " -> " << VAS(G, "label", VECTOR(map21)[index]) << endl;
        }
        
        igraph_vector_destroy(&map21);
    }
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
int Security::L1 (bool quite, bool vf2) {
    //cout<<"HEREEEEEE"<<endl;
    /******************************
     * Setup
     ******************************/
    L1_struct L1_state;
    for (unsigned int i = 0; i < igraph_vcount(H); i++) {
        L1_state.mapped.push_back( vector<bool>() );
        for (unsigned int j = 0; j < igraph_vcount(G); j++) {
            L1_state.mapped.back().push_back(false);
        }
        L1_state.reject.push_back( new vec<Lit>() );
        //Added by Karl
        L1_state.infinite.push_back(false);
        ///////////////
    }
    
    
    /******************************
     * Check all previously found solutions
     ******************************/
    cout<<"solution: "<< solutions.size()<<endl;
    for (unsigned int vid2 = 0; vid2 < igraph_vcount(H); vid2++) {
        for (unsigned int k=0; k < solutions.size(); k++) {
            if (L1_state.mapped[vid2][VECTOR(*solutions[k])[vid2]] == false) {
                L1_state.mapped[vid2][VECTOR(*solutions[k])[vid2]] = true;
                L1_state.reject[vid2]->push( isosat->translate(M21(vid2, VECTOR(*solutions[k])[vid2], true)) );
                //Added by Karl
                if (VAN(H,"Lifted",vid2) == Lifted) {
                    L1_state.infinite[vid2] = true;
                }
                ///////////////
                if (L1_state.reject[vid2]->size() == igraph_vcount(G))
                    break;
            }
        }
    }
    
    
    /******************************
     * Find level
     ******************************/
    if (!quite) {
        cout << "L1(" << G->max_L1() << "): ";
        cout.flush();
    }
    
    // Added by Mohamed
    eliminate();
    ///////////////////
    
    for (unsigned int level = G->max_L1(); level > 1; level--) {
        //cout<<"level : "<<level<<endl;
        if ( L1(level, true, &L1_state, vf2) ) {
            //Added by Karl
            if (level == G->max_L1()) { // If level != max_L1() then noway the graph is inf-secure because this means a vertex has return false once and that shouldn't happen if it has an infinite vertex mapping because it would stop generating the mapping and thus it won't know when no more mappings are possible
                //bool infinite = true;
                for (int i = 0; i < L1_state.infinite.size(); i++)
                    if (L1_state.infinite[i] == false) //{ // If a vertex has sec level = max sec level and not infinite sec lvl
                        //infinite = false; // The graph is not inf sec
                        return level; // it is level-secure
                //}
                //if (infinite) {
                return -2; // -2 = inf lvl of sec
                //}
            }
            ///////////////
            
            return level;
        }
        if (!quite) {
            cout << '*' << level;
            cout.flush();
        }
    }
    return 1;
    
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
int Security::vf2_solve (igraph_bool_t *iso, igraph_vector_t *map12, igraph_vector_t *map21, L1_struct *mapped) {
    
    L1_Thread thread;
    thread.open(true, false);
    
    /******************************
     * Child
     ******************************/
    if (thread.child()) {
        igraph_subisomorphic_vf2(G, H, &colour_G, &colour_H, 0, 0, iso, map12, map21, &check_map, 0, mapped);
        string output = report(map21);
        igraph_vector_destroy(map21);
        thread.write(output);
        thread.close(false, true);
        _exit(0);
    }
    
    /******************************
     * Parent
     ******************************/
#define MAX_COUNT 10000
    bool finished(false);
    for (unsigned int i=0; i < MAX_COUNT; i++) {
        string response = thread.read();
        if (response.size() > 0) {
            parse(response, map21);
            if (VECTOR(*map21)[0] == 0 && VECTOR(*map21)[1] == 0)
                *iso = false;
            else
                *iso = true;
            finished = true;
            break;
        }
        usleep(1000);
    }
    
    if (!finished) {
        *iso = false;
        thread.close(true, false);
        thread.kill();
    }
    
    return 0;
}



/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
bool Security::L1 (int max_count, bool quite, L1_struct *_L1_state, bool vf2) {
    //cout<<"H"<<endl;
    if (vf2 && isosat != NULL) {
        delete isosat;
        isosat = NULL;
    }
    
    if (max_count > igraph_vcount(G))
        return false;
    
    /******************************
     * Setup
     ******************************/
    L1_struct *L1_state;
    if (_L1_state == NULL) {
        L1_state = new L1_struct();
        for (unsigned int i = 0; i < igraph_vcount(H); i++) {
            L1_state->mapped.push_back( vector<bool>() );
            for (unsigned int j = 0; j < igraph_vcount(G); j++) {
                L1_state->mapped.back().push_back(false);
            }
            L1_state->reject.push_back( new vec<Lit>() );
            //Added by Karl
            L1_state->infinite.push_back(false);
            ///////////////
        }
    } else {
        L1_state = _L1_state;
    }
    
    if (!quite) {
        cout << " L1(" << max_count << "): ";
        cout.flush();
    }
    
    /******************************
     * Run tests
     ******************************/
    //cout<<"max_count "<<max_count<<endl;
    bool result(true);
    for (unsigned int l = 2; l <= max_count; l++) {
        
        /******************************
         *
         ******************************/
        igraph_vector_t *map21 = new igraph_vector_t;
        igraph_vector_init(map21, igraph_vcount(H));
        
        int count = 0;
        for (unsigned int vid2 = 0; vid2 < igraph_vcount(H); vid2++) { // ... for every vertex in G check if there can be a mapping with all the old mappings
            //cout<<"A"<<endl;
            // Added by Mohamed
            //if (VAN(H, "consider", vid2)) continue;
            count++;
            //cout<<"1 v: "<<vid2<<" l: "<<l<<endl;
            //cout<<"vid2 "<<vid2<<endl;
            //if (L1_state->reject[vid2]->size() < l && !L1_state->infinite[vid2]) { //Added by Karl: || !L1_state->infinite[vid2] ... if this vertex has more mappings that the max level of security then we don't need to compute its mapping because the lvl of sec if the min of mappings amongst all the vertices.
            //cout<<"condition "<<(L1_state->reject[vid2]->size() < l)<<" "<< L1_state->infinite[vid2]<<endl;
            //cout<<"vertex : "<<vid2<<" Lifted? "<<L1_state->infinite[vid2]<<endl;
            if (!(L1_state->reject[vid2]->size() >= l || L1_state->infinite[vid2]) == Lifted) {
                //cout<<"2 v: "<<vid2<<" l: "<<l<<endl;
                // update count and reject list
                igraph_bool_t iso(false);
                
                if (vf2) {
                    L1_state->vid2 = vid2;
                    vf2_solve(&iso, NULL, map21, L1_state);
                } else {
                    isosat->solve(&iso, NULL, map21, L1_state->reject[vid2]); // ... mapping done here. If no mapping is possible considering the assumptions then iso = false and we stop.
                }
                // Added by Karl
                //cout<<"iso "<<iso<<endl;
                ////////////////
                //cout<<"vertex: "<<vid2<<"level: "<<l<<endl;
                if (iso) {
#ifndef NDEBUG
                    igraph_bool_t test_iso(false);
                    igraph_test_isomorphic_map (G, H, &colour_G, &colour_H, 0, 0, &test_iso, NULL, map21,
                                                &igraph_compare_transitives, 0, 0);
                    if (!test_iso) {
                        H->print();
                        igraph_vector_print(map21);
                    }
                    assert(test_iso);
#endif
                    assert ( L1_state->mapped[vid2][VECTOR(*map21)[vid2]] == false );
                    // v_G = vid2; v_H = VECTOR(*map21)[vid2]
                    // if v_H is lifted then v_G has inf security level;
                    // Added by Karl
                    //cout<<"map "<<VECTOR(*map21)[vid2]<<endl;
                    //cout<<"Lifted "<<VAN(H,"Lifted",vid2)<<endl;
                    //cout<<"3 v: "<<vid2<<" l: "<<l<<endl;
                    if (VAN(H,"Lifted",vid2/*VECTOR(*map21)[vid2]*/) == Lifted)
                        L1_state->infinite[vid2] = true;
                    
                    //cout<<max_count<<" "<<vid2<<" "<<L1_state->infinite[vid2]<<endl;
                    ////////////////
                    
                    solutions.push_back(map21); // ... mapping is good.
                    for (unsigned int i = 0; i < igraph_vector_size(map21); i++) {
                        if (L1_state->mapped[i][VECTOR(*map21)[i]] == false) {
                            L1_state->mapped[i][VECTOR(*map21)[i]] = true;
                            if (vf2)
                                L1_state->reject[i]->push( mkLit(0) );
                            else
                                L1_state->reject[i]->push( isosat->translate(M21(i, VECTOR(*map21)[i], true))); // ... ++ at every mapping that works + assumption for the solver
                            // i v in H, the other one is v in G
                        }
                    }
                    map21 = new igraph_vector_t;
                    igraph_vector_init(map21, igraph_vcount(H));
                } else {
                    // Added by Karl
                    //write_levels(vid2,l);
                    ////////////////
                    if (_L1_state == NULL)
                        delete L1_state;
                    return false;
                }
                
            }
        }
        
        //cout << endl << count << endl; //cout.flush();
        
        igraph_vector_destroy(map21);
        
        if (!quite) {
            cout << '*';
            cout.flush();
        }
    }
    if (_L1_state == NULL)
        delete L1_state;
    
    return true;
}

void Security::eliminate()
{
    
    // H->print(cout);
    // cout.flush();
    igraph_vector_ptr_t components;
    
    // if (igraph_decompose(H, components, IGRAPH_WEAK, -1, 1) != IGRAPH_ENOMEM)
    // ;
    
    igraph_vector_t membership, csize;
    
    igraph_vector_init(&membership, 1);
    
    igraph_vector_init(&csize, 1);
    igraph_integer_t no;
    
    igraph_clusters(H, &membership, &csize, &no, IGRAPH_WEAK);
    igraph_vector_ptr_init(&components, no);
    
    
    
    igraph_vector_ptr_t colour_vecs;
    igraph_vector_ptr_init(&colour_vecs, no);
    
    //		cout << endl << no << "components" << endl;
    
    for (int i = 0; i < no; i++)
    {
        igraph_vector_t members;
        igraph_vector_int_t colours;
        igraph_vector_init(&members, 0);
        // igraph_vector_int_init(&colours, 0);
        igraph_vector_ptr_set(&colour_vecs, i , malloc(sizeof(igraph_vector_int_t)));
        igraph_vector_int_init((igraph_vector_int_t*) VECTOR(colour_vecs)[i], 0);
        
        for (int j = 0; j < igraph_vector_size(&membership); j++)
        {
            if (igraph_vector_e(&membership, j) == i)
            {
                igraph_vector_push_back(&members, j);
                //					igraph_vector_int_push_back(&colours, VECTOR(colour_H)[j]);
                igraph_vector_int_push_back((igraph_vector_int_t*) VECTOR(colour_vecs)[i], VECTOR(colour_H)[j]);
            }
        }
        igraph_vs_t vs;
        igraph_vs_vector(&vs, &members);
        igraph_t res;
        //			cout << endl << "Members of component " << i << " are: ";
        //			for (int j = 0; j < igraph_vector_size(&members); j++)
        //				cout << VECTOR(members)[j] << " ";
        
        //			cout << endl;
        igraph_vector_ptr_set(&components, i , malloc(sizeof(igraph_t)));
        igraph_induced_subgraph(H, (igraph_t*) VECTOR(components)[i], vs, IGRAPH_SUBGRAPH_CREATE_FROM_SCRATCH);
        //			cout << igraph_vcount(&res) << " vertices in subgraph" << endl;
        //			igraph_vector_ptr_set(&components, i , &res);
        //			igraph_vector_ptr_set(&colour_vecs, i , &colours);
        igraph_vector_destroy(&members);
    }
    
    
    
    //		igraph_vector_ptr_t colour_vecs;
    //		igraph_vector_ptr_init(&colour_vecs, no);
    //
    //		for (int i = 0; i < igraph_vector_ptr_size(&components); i++)
    //		{
    //			igraph_vector_t res;
    //			igraph_vector_init(&res, 1);
    //			VANV((igraph_t*)igraph_vector_ptr_e(&components, i), "colour", &res);
    //			igraph_vector_ptr_set(&colour_vecs, i, &res);
    //		}
    
    //		for (int i = 0; i < igraph_vector_ptr_size(&components); i ++)
    //			cout << endl << "Number of vertices in component " << i << ": " << igraph_vcount((igraph_t*) VECTOR(components)[i]) << ", Colour vector size: " << igraph_vector_int_size((igraph_vector_int_t*) VECTOR(colour_vecs)[i]);
    
    //cout.flush();
    //		for (int i = 0; i < igraph_vector_size(&membership); i ++)
    //			cout << endl << VECTOR(membership)[i];
    
    igraph_vector_t consider;
    igraph_vector_init(&consider, igraph_vcount(H));
    
    SETVANV(H, "consider", &consider);
    
    igraph_vector_bool_t isomorphic;
    igraph_vector_bool_init(&isomorphic, no);
    for (int i = 0; i < igraph_vector_ptr_size(&components); i++)
    {
        for (int j = i+1; j < igraph_vector_ptr_size(&components); j++)
        {
            if (VECTOR(isomorphic)[j]) continue;
            igraph_bool_t iso;
            //				cout << "boo";cout.flush();
            if (igraph_vcount((igraph_t*)VECTOR(components)[i]) != igraph_vcount((igraph_t*)VECTOR(components)[j])
                || igraph_ecount((igraph_t*)VECTOR(components)[i]) != igraph_ecount((igraph_t*)VECTOR(components)[j])) continue;
            igraph_isomorphic_vf2((igraph_t*)VECTOR(components)[i], (igraph_t*) VECTOR(components)[j], (igraph_vector_int_t *) VECTOR(colour_vecs)[i], (igraph_vector_int_t *) VECTOR(colour_vecs)[j], NULL, NULL, &iso, NULL, NULL, 0, 0, NULL);
            if (iso)
            {
                igraph_vector_bool_set(&isomorphic, j, true);
                for (int k = 0; k < igraph_vector_size(&membership); k++)
                {
                    if (igraph_vector_e(&membership, k) == j)
                        SETVAN(H, "consider", k, 1);
                }
                //					igraph_destroy((igraph_t*)VECTOR(components)[j]);
                //					free(VECTOR(components)[j]);
                //					igraph_vector_ptr_remove(&components, j);
                //					igraph_vector_int_destroy((igraph_vector_int_t*) VECTOR(colour_vecs)[j]);
                //					free(VECTOR(colour_vecs)[j]);
                //					igraph_vector_ptr_remove(&colour_vecs, j);
                //					j--;
            }
        }
    }
    
    for (int i = 0; i < igraph_vector_ptr_size(&components); i++)
    {
        igraph_destroy((igraph_t*)igraph_vector_ptr_e(&components, i));
        //			igraph_vector_destroy((igraph_vector_t *) igraph_vector_ptr_e(&colour_vecs, i));
        //free(VECTOR(components)[j]);
        igraph_vector_int_destroy((igraph_vector_int_t*) VECTOR(colour_vecs)[i]);
    }
    igraph_vector_ptr_destroy_all(&components);
    igraph_vector_ptr_destroy_all(&colour_vecs);
    igraph_vector_destroy(&membership);
    igraph_vector_destroy(&csize);
    igraph_vector_destroy(&consider);
    
    //cout.flush();
}

/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            run circuits/c17.blif tech_lib/minimal.genlib -w tmp -t -1
                                                                            ****************************************************************************/
void Security::S1_rand (int threads, int min_L1, bool quite) { //2//true
    
    /******************************
     * Setup
     ******************************/
    vector<int> good;
    for (unsigned int eid = 0; eid < igraph_ecount(G); eid++) {
        //cout<<"test edge: "<<!H->test_edge(G->get_edge(eid))<<endl;
        if (!H->test_edge(G->get_edge(eid))) {
            good.push_back(eid);
        }
    }
    
#ifndef NRAND
    random_shuffle(good.begin(), good.end());
#endif
    
    vector<L1_Thread*> busy_threads, free_threads;
    for (unsigned int i=0; i<threads; i++)
        free_threads.push_back( new L1_Thread() );
    
    //cout<<"Free threads size: "<<free_threads.size()<<endl;
    
    
    /******************************
     * Add edges until L1 == min_L1
     ******************************/
    bool done(false);
    while (!done || busy_threads.size() > 0) {
        
        /******************************
         * Load Threads (create sub-processes)
         ******************************/
        if (!done && free_threads.size() > 0) {
            int test_index = good.back();
            good.pop_back();
            add_edge(test_index);
            
            busy_threads.push_back(free_threads.back());
            //cout<<"Busy threads size: "<<busy_threads.size()<<endl;
            free_threads.pop_back();
            //cout<<"Free threads size 1: "<<free_threads.size()<<endl;
            busy_threads.back()->open(true,false);
            
            /******************************
             * Child
             ******************************/
            if ( busy_threads.back()->child() ) {              // Child (PID == 0)
                
                Edge test_edge = G->get_edge(test_index);
                int test_L1 = L1();
                
                string output;
                output = "S1_rand ("  + G->get_name() + ").child(" + num2str(getpid()) + ")";
                output = report(output, G, H, test_L1, solutions.size(), test_edge);
                
#ifdef DEBUG
                cout << output << endl;
#endif
                
                //cout<<"Nothing to see here"<<endl;
                busy_threads.back()->write(output);
                //cout<<"Busy threads size1: "<<busy_threads.size()<<endl;
                busy_threads.back()->close(false, true);
                _exit(0);
            }
            
        }
        
        /******************************
         * Unload Threads (Parent)
         ******************************/
        do {
            //cout<<"YAALLLAAAAA: "<<endl<<endl<<endl;
            //cout<<"Busy threads size2: "<<busy_threads.size()<<endl;
            for (unsigned int j=0; j<busy_threads.size(); j++) {
                string response = busy_threads[j]->read();
                // do something with response
                if (response.size() > 0) {
                    
                    int L0, test_L1;
                    Edge test_edge;
                    
                    stringstream r_stream(response);
                    string line;
                    while( getline(r_stream, line) )
                        parse(line, G, test_L1, L0, test_edge);
                    
                    string output;
                    output = "S1_rand ("  + G->get_name() + ")";
                    output = report(output, G, H, test_L1, L0, test_edge);
                    cout << endl << output;
                    
                    free_threads.push_back(busy_threads[j]);
                    //cout<<"Free threads size 2: "<<free_threads.size()<<endl;
                    busy_threads.erase(busy_threads.begin()+j);
                    //cout<<"Busy threads size3: "<<busy_threads.size()<<endl;
                    
                    if (test_L1 < min_L1) done = true;
                }
            }
        } while (free_threads.size() == 0);
        //cout<<"YAALLLAAAAA1: "<<busy_threads.size()<<endl<<endl<<endl;
    }
}

void Security::S1_self()
{
    clean_solutions();
    cout << L1(false, false);
}

int C_SAT::e(int i) { return marker - igraph_ecount(self->G) + i; }

int C_SAT::phi(int u, int v, int i, int j) {
    Circuit* G = self->G;
    int color = VAN(G, "colour", u);
    int index = 0;
    int l;
    for (l = 0; l < color; l++)
    {
        int n = igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[l]);
        index += n * n * (igraph_vcount(G)) * (igraph_vcount(G));
    }
    bool found1 = false, found2 = false;
    int k1=-1, k2=-1;
    
    //	l = VAN()
    while (!found1 || !found2)
    {
        if (!found1) if (igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[l],++k1) == u) found1 = true;
        if (!found2) if (igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[l],++k2) == v) found2 = true;
    }
    //	k2 = v;
    index += ( k1 * (igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[l])) + k2 ) * igraph_vcount(G) * igraph_vcount(G) + i * igraph_vcount(G) + j;
    return index + 1;
}

Formula* C_SAT::leq( const vec<Formula*>& fvector, string n, int start)
{
    if (start == n.length()) return NULL;
    Formula* ret;
    int index = n.length() - start - 1;
    
    if (n[start]=='0')
    {
        ret = new Formula(F_AND);
        Formula * tmp = new Formula(F_AND), *neg = new Formula(*fvector[index]); neg->negate();
        ret->add(neg);
        ret->add(leq(fvector, n, ++start));
    }
    else
    {
        ret = new Formula(F_OR);
        Formula * tmp = new Formula(F_AND), *neg = new Formula(*fvector[index]); neg->negate();
        ret->add(neg);
        tmp->add(fvector[index]);
        tmp->add(leq(fvector, n, ++start));
        ret->add(tmp);
    }
    return ret;
}

Formula* C_SAT::leq( const vec<Var>& fvector, string n, int start)
{
    if (start == n.length()) return NULL;
    Formula* ret;
    int index = n.length() - start - 1;
    
    if (n[start]=='0')
    {
        ret = new Formula(F_AND);
        //Formula * tmp = new Formula(F_AND), *neg = new Formula(*fvector[index]); neg->negate();
        Lit neg = mkLit(fvector[index],true);
        ret->add(neg);
        ret->add(leq(fvector, n, ++start));
    }
    else
    {
        ret = new Formula(F_OR);
        Formula * tmp = new Formula(F_AND);//, *neg = new Formula(*fvector[index]); neg->negate();
        Lit neg = mkLit(fvector[index], true);
        ret->add(neg);
        tmp->add(mkLit(fvector[index]));
        tmp->add(leq(fvector, n, ++start));
        ret->add(tmp);
    }
    return ret;
}

Formula* C_SAT::geq( const vec<Formula*>& fvector, string n, int start)
{
    if (start == n.length()) return NULL;
    int index = n.length() - start - 1;
    Formula* ret;
    
    if (n[start]=='1')
    {
        ret = new Formula(F_AND);
        ret->add(fvector[index]);
        ret->add(geq(fvector, n, ++start));
    }
    else
    {
        ret = new Formula(F_OR);
        Formula * tmp = new Formula(F_AND), *neg = new Formula(*fvector[index]); neg->negate();
        ret->add(fvector[index]);
        tmp->add(neg);
        tmp->add(geq(fvector, n, ++start));
        ret->add(tmp);
    }
    
    return ret;
}

Formula* C_SAT::geq( const vec<Var>& fvector, string n, int start)
{
    if (start == n.length()) return NULL;
    int index = n.length() - start - 1;
    Formula* ret;
    
    if (n[start]=='1')
    {
        ret = new Formula(F_AND);
        ret->add(mkLit(fvector[index]));
        ret->add(geq(fvector, n, ++start));
    }
    else
    {
        ret = new Formula(F_OR);
        Formula * tmp = new Formula(F_AND);//, *neg = new Formula(*fvector[index]); neg->negate();
        Lit neg = mkLit(fvector[index],true);
        ret->add(mkLit(fvector[index]));
        tmp->add(neg);
        tmp->add(geq(fvector, n, ++start));
        ret->add(tmp);
    }
    
    return ret;
}

string C_SAT::convertInt(int number)
{
    if (number == 0)
        return "0";
    string temp="";
    string returnvalue="";
    while (number>0)
    {
        temp+=number%2+48;
        number/=2;
    }
    for (int i=0;i<temp.length();i++)
        returnvalue+=temp[temp.length()-i-1];
    return returnvalue;
}

C_SAT::C_SAT(Security* security, int min_L1, int max_L1, int eta) {
    
    self = security;
    Circuit* G = self->G;
    sat = new Formula(F_AND);
    igraph_vector_ptr_init(&match_vert, 0);
    for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        if (igraph_vector_ptr_size(&match_vert) < color + 1)
            for (int j = igraph_vector_ptr_size(&match_vert); j <= color; j++)
            {
                igraph_vector_t* v = (igraph_vector_t*) malloc(sizeof(igraph_vector_t));
                igraph_vector_init(v, 0);
                igraph_vector_ptr_push_back(&match_vert, v);
            }
        igraph_vector_push_back((igraph_vector_t*) VECTOR(match_vert)[color], i);
    }
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        for (int j=0; j < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]); j++)
            cout << igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[i], j) << "";
        cout << endl;
    }
    
    cout.flush();
    
    //	int nVars
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        int n = igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]);
        for (int j=0; j < n * n * (igraph_vcount(G))*(igraph_vcount(G)); j++)
            sat->newVar();
    }
    
    for (int i=0; i < igraph_ecount(G); i++) sat->newVar();
    
    marker = sat->maxVar();
    
    int n = (int) floor(log(igraph_ecount(G))/log(2)+1);
    //	char* eta_b = (char*) malloc(n);
    
    string eta_b = convertInt(eta);
    string temp="";
    for (int i = eta_b.length(); i < n; i++)
        temp+='0';
    
    eta_b=temp+eta_b;
    vec<Formula*> sum;
    
    for (int i = 0; i < n; i++)
        sum.push(NULL);
    
    Formula* tmp1 = new Formula(), *tmp2 = new Formula(), *tmp3 = new Formula(), *carry = new Formula(), *sum_neg = new Formula(), *carry_neg = new Formula();
    tmp1->add(mkLit(e(1)));
    tmp1->add(mkLit(e(2), true));
    
    tmp2->add(mkLit(e(1), true));
    tmp2->add(mkLit(e(2)));
    
    sum[0] = new Formula(F_OR);
    sum[0]->add(tmp1);
    sum[0]->add(tmp2);
    
    carry->add(mkLit(e(1)));
    carry->add(mkLit(e(2)));
    
    sum[1] = carry;
    
    for (int i = 3; i <= igraph_ecount(G); i++)
    {
        sum_neg = new Formula(*sum[0]); sum_neg->negate();
        tmp1 = new Formula();
        tmp1->add(sum[0]);
        tmp1->add(mkLit(e(i), true));
        
        tmp2 = new Formula();
        tmp2->add(sum_neg);
        tmp2->add(mkLit(e(i)));
        
        carry = new Formula();
        carry->add(sum[0]);
        carry->add(mkLit(e(i)));
        
        sum[0] = new Formula(F_OR);
        sum[0]->add(tmp1);
        sum[0]->add(tmp2);
        
        int j;
        for (j = 1; sum[j]!=NULL && j < sum.size(); j++)
        {
            Formula* carry_neg = new Formula(*carry);
            sum_neg = new Formula(*sum[j]);
            
            sum_neg->negate(); carry_neg->negate();
            
            tmp1 = new Formula();
            tmp1->add(sum[j]);
            tmp1->add(carry_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(carry);
            
            tmp3 = new Formula();
            tmp3->add(sum[j]);
            tmp3->add(carry);
            
            sum[j] = new Formula(F_OR);
            sum[j]->add(tmp1);
            sum[j]->add(tmp2);
            
            carry = tmp3;
            
        }
        if (j < sum.size()) sum[j] = carry;
    }
    
    
    Formula* ecount_less_than_eta = leq(sum, eta_b, 0);
    //cout << ecount_less_than_eta->str();
    sat->add(ecount_less_than_eta);
    
    Formula* k_secure = new Formula(F_AND);
    
    
    //	vec<Formula*> formuli;
    for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        vec<Formula*> formuli;
        for (int j1 = 0; j1 < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[color]); j1++)
        {
            int j = igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[color], j1);
            Formula* F1 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k == i) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //				for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l==j) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == i && l != j) continue;
                    if (k != i || l != j) nested1->add(mkLit(phi(i, j, k, l)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                    {
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != j) nested1->add(mkLit(phi(i, j, k, VECTOR(*vec)[h]), true));
                    }
                    nested->add(nested1);
                }
                F1->add(nested);
            }
            
            Formula* F2 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k==j) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l==i) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == j && l != i) continue;
                    if (k != j || l != i) nested1->add(mkLit(phi(i, j, l, k)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                    {
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != i) nested1->add(mkLit(phi(i, j, VECTOR(*vec)[h], k), true));
                    }
                    nested->add(nested1);
                }
                F2->add(nested);
            }
            
            Formula* F3 = new Formula(F_AND);
            for (int k = 0; k < igraph_ecount(G); k++)
            {
                //				F3->add(mkLit(e(k),true));
                Formula* nested = new Formula(F_OR);
                nested->add(mkLit(e(k+1)));
                for (int l = 0; l < igraph_ecount(G); l++)
                {
                    int src_l, dest_l, src_k, dest_k;
                    igraph_edge(G, l, &src_l, &dest_l);
                    int src_col_k, dest_col_k, src_col_l, dest_col_l;
                    src_col_l = VAN(G, "colour", src_l);
                    dest_col_l = VAN(G, "colour", dest_l);
                    //					if (src_col != dest_col) continue;
                    igraph_edge(G, k, &src_k, &dest_k);
                    src_col_k = VAN(G, "colour", src_k);
                    dest_col_k = VAN(G, "colour", dest_k);
                    if (src_col_l != src_col_k || dest_col_l != dest_col_k) continue;
                    //					if (src_k == i && src_l == j && dest_k == i && dest_l == j) break;
                    if (src_k == i && src_l != j || dest_k == i && dest_l != j) continue;
                    if (src_k != i && src_l == j || dest_k != i && dest_l == j) continue;
                    if (src_k == i && src_l == j) { nested->add(mkLit(phi(i,j,dest_k,dest_l))); continue; }
                    if (dest_k == i && dest_l == j) { nested->add(mkLit(phi(i,j,src_k,src_l))); continue; }
                    Formula* p = new Formula(F_AND);
                    p->add(mkLit(phi(i,j,src_k,src_l)));
                    p->add(mkLit(phi(i,j,dest_k,dest_l)));
                    nested->add(p);
                }
                F3->add(nested);
            }
            
            Formula* F = new Formula(F_AND);
            F->add(F1);
            F->add(F2);
            F->add(F3);
            formuli.push(F);
            if (i == 10 && j == 1) cout << F->str();
        }
        
        int n = (int) floor(log(igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[color]))/log(2)+1);
        //		char* min_L1_b = (char*) malloc(n);
        
        string min_L1_b = convertInt(min_L1);
        
        string temp="";
        for (int j = min_L1_b.length(); j < n; j++)
            temp+="0";
        min_L1_b=temp+min_L1_b;
        cout << min_L1_b; cout << endl;
        vec<Formula*> sum;
        
        for (int j = 0; j < n; j++)
            sum.push(NULL);
        
        Formula* tmp1, *tmp2, *carry, *sum_neg, *carry_neg, *add_neg;
        
        sum[0] = new Formula(*formuli[0]);
        for (int j = 1; j < formuli.size(); j++)
        {
            sum_neg = new Formula(*sum[0]); sum_neg->negate();
            add_neg = new Formula(*formuli[j]); add_neg->negate();
            
            tmp1 = new Formula();
            tmp1->add(sum[0]);
            tmp1->add(add_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(formuli[j]);
            
            carry = new Formula();
            carry->add(sum[0]);
            carry->add(formuli[j]);
            
            sum[0] = new Formula(F_OR);
            sum[0]->add(tmp1);
            sum[0]->add(tmp2);
            
            int k;
            for (k = 1; sum[k]!=NULL && k < sum.size(); k++)
            {
                carry_neg = new Formula(*carry);
                sum_neg = new Formula(* (Formula*) sum[k]);
                
                sum_neg->negate(); carry_neg->negate();
                
                tmp1 = new Formula();
                tmp1->add(sum[k]);
                tmp1->add(carry_neg);
                
                tmp2 = new Formula();
                tmp2->add(sum_neg);
                tmp2->add(carry);
                
                tmp3 = new Formula();
                tmp3->add(sum[k]);
                tmp3->add(carry);
                
                sum[k] = new Formula(F_OR);
                sum[k]->add(tmp1);
                sum[k]->add(tmp2);
                
                carry = tmp3;
                
            }
            if (k < sum.size()) sum[k] = carry;
        }
        
        
        k_secure->add(geq(sum, min_L1_b, 0));
        //		cout << "end of loop" << endl;
    }
    
    sat->add(k_secure);
    cout << sat->maxVar();
    //	cout << sat->str();
    
    Formula cnf_sat;
    Lit out;
    Solver mySolver;
    sat->export_cnf(out, NULL, &mySolver);
    sat->export_cnf(out, &cnf_sat, NULL);
    cnf_sat.add(out);
    
    mySolver.addClause(out);
    //	cout << endl << cnf_sat.maxVar();
    //	mySolver.solve();
    cout << endl << "done";
    //	cout.flush();
    
    if (!mySolver.solve()) cout << endl << "Problem is not in LIFT" << endl;
    else
    {
        for (int i=0; i<igraph_ecount(G); i++)
            
            if (mySolver.modelValue(e(i+1)) != l_False) { Edge edge; igraph_edge(G, i, &edge.first, &edge.second); self->H->del_edge(edge); }
        //		for (int j=0; j < igraph_vector_ptr_size(&match_vert); j++)
        //		{
        //			cout << j << " ";
        //			igraph_vector_t* v = (igraph_vector_t*) VECTOR(match_vert)[j];
        //			for (int k=0; k < igraph_vector_size(v); k++) cout << VECTOR(*v)[k] << " ";
        //			cout << endl;
        //		}
        //		for (int j=0; j < mySolver.model.size(); j++)
        //			cout << (mySolver.model[j]==l_False? "false" : "true") << " ";
        for (int j=0; j < igraph_vcount(G); j++)
        {
            for (int k=0; k < igraph_vcount(G); k++)
            {
                cout << j << "->" << k << ": " << (mySolver.modelValue(phi(10,1,j,k))==l_False? "false": "true"); cout << " ";
            }
            cout << endl;
        }
    }
    
    cout << "ecount_less_than_eta is: " << (ecount_less_than_eta->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "k_secure is: " << (k_secure->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "sat is: " << (sat->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "cnf_sat is: " << (cnf_sat.evaluate(&mySolver)? "True" : "False") << endl;
    cout.flush();
    //	Grabage Collection
    for (int i =0; i < igraph_vector_ptr_size(&match_vert); i++)
        igraph_vector_destroy((igraph_vector_t*) VECTOR(match_vert)[i]);
    igraph_vector_ptr_destroy(&match_vert);
    
    //	for (int i = 0; i < sum.size(); i++) delete sum[i];
    
    //	delete carry, tmp1, tmp2, tmp3, carry_neg, sum_neg, sat;
    //	delete sat;
}

int Security::rSAT(int min_L1, int max_L1, int eta) { return C_SAT(this, min_L1, max_L1, eta); };

int Security::rSAT(int min_L1, int max_L1, int eta, int u, bool quite) { return C_SAT(this, min_L1, max_L1, eta, u, quite); };

int Security::rSAT(int min_L1, int max_L1, int eta, int u) { return C_SAT(this, min_L1, max_L1, eta, u); };

C_SAT::C_SAT(Security* security, int min_L1, int max_L1, int eta, int u, bool quite) {
    
    self = security;
    Circuit* G = self->G;
    sat = new Formula(F_AND);
    igraph_vector_ptr_init(&match_vert, 0);
    for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        if (igraph_vector_ptr_size(&match_vert) < color + 1)
            for (int j = igraph_vector_ptr_size(&match_vert); j <= color; j++)
            {
                igraph_vector_t* v = (igraph_vector_t*) malloc(sizeof(igraph_vector_t));
                igraph_vector_init(v, 0);
                igraph_vector_ptr_push_back(&match_vert, v);
            }
        igraph_vector_push_back((igraph_vector_t*) VECTOR(match_vert)[color], i);
    }
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        for (int j=0; j < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]); j++)
            cout << igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[i], j) << "";
        cout << endl;
    }
    
    cout.flush();
    
    //	int nVars
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        int n = igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]);
        for (int j=0; j < n * n * (igraph_vcount(G))*(igraph_vcount(G)); j++)
            sat->newVar();
    }
    
    for (int i=0; i < igraph_ecount(G); i++) sat->newVar();
    marker = sat->maxVar();
    int n = (int) floor(log(igraph_ecount(G))/log(2)+1);
    //	char* eta_b = (char*) malloc(n);
    
    string eta_b = convertInt(eta);
    string temp="";
    for (int i = eta_b.length(); i < n; i++)
        temp+='0';
    
    eta_b=temp+eta_b;
    vec<Formula*> sum;
    
    for (int i = 0; i < n; i++)
        sum.push(NULL);
    
    Formula* tmp1 = new Formula(), *tmp2 = new Formula(), *tmp3 = new Formula(), *carry = new Formula(), *sum_neg = new Formula(), *carry_neg = new Formula();
    tmp1->add(mkLit(e(1)));
    tmp1->add(mkLit(e(2), true));
    
    tmp2->add(mkLit(e(1), true));
    tmp2->add(mkLit(e(2)));
    
    sum[0] = new Formula(F_OR);
    sum[0]->add(tmp1);
    sum[0]->add(tmp2);
    
    carry->add(mkLit(e(1)));
    carry->add(mkLit(e(2)));
    
    sum[1] = carry;
    
    for (int i = 3; i <= igraph_ecount(G); i++)
    {
        sum_neg = new Formula(*sum[0]); sum_neg->negate();
        tmp1 = new Formula();
        tmp1->add(sum[0]);
        tmp1->add(mkLit(e(i), true));
        
        tmp2 = new Formula();
        tmp2->add(sum_neg);
        tmp2->add(mkLit(e(i)));
        
        carry = new Formula();
        carry->add(sum[0]);
        carry->add(mkLit(e(i)));
        
        sum[0] = new Formula(F_OR);
        sum[0]->add(tmp1);
        sum[0]->add(tmp2);
        
        int j;
        for (j = 1; sum[j]!=NULL && j < sum.size(); j++)
        {
            Formula* carry_neg = new Formula(*carry);
            sum_neg = new Formula(*sum[j]);
            
            sum_neg->negate(); carry_neg->negate();
            
            tmp1 = new Formula();
            tmp1->add(sum[j]);
            tmp1->add(carry_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(carry);
            
            tmp3 = new Formula();
            tmp3->add(sum[j]);
            tmp3->add(carry);
            
            sum[j] = new Formula(F_OR);
            sum[j]->add(tmp1);
            sum[j]->add(tmp2);
            
            carry = tmp3;
            
        }
        if (j < sum.size()) sum[j] = carry;
    }
    
    
    Formula* ecount_less_than_eta = leq(sum, eta_b, 0);
    //cout << ecount_less_than_eta->str();
    sat->add(ecount_less_than_eta);
    
    Formula* k_secure = new Formula(F_AND);
    
    
    //	vec<Formula*> formuli;
    
    int i = u;
    //	for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        vec<Formula*> formuli;
        for (int j1 = 0; j1 < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[color]); j1++)
        {
            int j = igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[color], j1);
            Formula* F1 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k == i) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //				for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l == j) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == i && l != j) continue;
                    if (k != i || l != j) nested1->add(mkLit(phi(i, j, k, l)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                    {
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != j) nested1->add(mkLit(phi(i, j, k, VECTOR(*vec)[h]), true));
                    }
                    nested->add(nested1);
                }
                F1->add(nested);
            }
            
            Formula* F2 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k==j) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l==i) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == j && l != i) continue;
                    if (k != j || l != i) nested1->add(mkLit(phi(i, j, l, k)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                    {
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != i) nested1->add(mkLit(phi(i, j, VECTOR(*vec)[h], k), true));
                    }
                    nested->add(nested1);
                }
                F2->add(nested);
            }
            
            Formula* F3 = new Formula(F_AND);
            for (int k = 0; k < igraph_ecount(G); k++)
            {
                //				F3->add(mkLit(e(k),true));
                Formula* nested = new Formula(F_OR);
                nested->add(mkLit(e(k+1)));
                for (int l = 0; l < igraph_ecount(G); l++)
                {
                    int src_l, dest_l, src_k, dest_k;
                    igraph_edge(G, l, &src_l, &dest_l);
                    int src_col_k, dest_col_k, src_col_l, dest_col_l;
                    src_col_l = VAN(G, "colour", src_l);
                    dest_col_l = VAN(G, "colour", dest_l);
                    //					if (src_col != dest_col) continue;
                    igraph_edge(G, k, &src_k, &dest_k);
                    src_col_k = VAN(G, "colour", src_k);
                    dest_col_k = VAN(G, "colour", dest_k);
                    if (src_col_l != src_col_k || dest_col_l != dest_col_k) continue;
                    //					if (src_k == i && src_l == j && dest_k == i && dest_l == j) break;
                    if (src_k == i && src_l != j || dest_k == i && dest_l != j) continue;
                    if (src_k != i && src_l == j || dest_k != i && dest_l == j) continue;
                    if (src_k == i && src_l == j) { nested->add(mkLit(phi(i,j,dest_k,dest_l))); continue; }
                    if (dest_k == i && dest_l == j) { nested->add(mkLit(phi(i,j,src_k,src_l))); continue; }
                    Formula* p = new Formula(F_AND);
                    p->add(mkLit(phi(i,j,src_k,src_l)));
                    p->add(mkLit(phi(i,j,dest_k,dest_l)));
                    nested->add(p);
                }
                F3->add(nested);
            }
            
            Formula* F = new Formula(F_AND);
            F->add(F1);
            F->add(F2);
            F->add(F3);
            formuli.push(F);
            if (i == 10 && j == 1) cout << F->str();
        }
        
        int n = (int) floor(log(igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[color]))/log(2)+1);
        //		char* min_L1_b = (char*) malloc(n);
        
        string min_L1_b = convertInt(min_L1);
        
        string temp="";
        for (int j = min_L1_b.length(); j < n; j++)
            temp+="0";
        min_L1_b=temp+min_L1_b;
        cout << min_L1_b; cout << endl;
        vec<Formula*> sum;
        
        for (int j = 0; j < n; j++)
            sum.push(NULL);
        
        Formula* tmp1, *tmp2, *carry, *sum_neg, *carry_neg, *add_neg;
        
        sum[0] = new Formula(*formuli[0]);
        for (int j = 1; j < formuli.size(); j++)
        {
            sum_neg = new Formula(*sum[0]); sum_neg->negate();
            add_neg = new Formula(*formuli[j]); add_neg->negate();
            
            tmp1 = new Formula();
            tmp1->add(sum[0]);
            tmp1->add(add_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(formuli[j]);
            
            carry = new Formula();
            carry->add(sum[0]);
            carry->add(formuli[j]);
            
            sum[0] = new Formula(F_OR);
            sum[0]->add(tmp1);
            sum[0]->add(tmp2);
            
            int k;
            for (k = 1; sum[k]!=NULL && k < sum.size(); k++)
            {
                carry_neg = new Formula(*carry);
                sum_neg = new Formula(* (Formula*) sum[k]);
                
                sum_neg->negate(); carry_neg->negate();
                
                tmp1 = new Formula();
                tmp1->add(sum[k]);
                tmp1->add(carry_neg);
                
                tmp2 = new Formula();
                tmp2->add(sum_neg);
                tmp2->add(carry);
                
                tmp3 = new Formula();
                tmp3->add(sum[k]);
                tmp3->add(carry);
                
                sum[k] = new Formula(F_OR);
                sum[k]->add(tmp1);
                sum[k]->add(tmp2);
                
                carry = tmp3;
                
            }
            if (k < sum.size()) sum[k] = carry;
        }
        
        
        k_secure->add(geq(sum, min_L1_b, 0));
        //		cout << "end of loop" << endl;
    }
    
    sat->add(k_secure);
    cout << sat->maxVar();
    //	cout << sat->str();
    
    Formula cnf_sat;
    Lit out;
    Solver mySolver;
    sat->export_cnf(out, NULL, &mySolver);
    sat->export_cnf(out, &cnf_sat, NULL);
    cnf_sat.add(out);
    
    mySolver.addClause(out);
    //	cout << endl << cnf_sat.maxVar();
    //	mySolver.solve();
    cout << endl << "done";
    //	cout.flush();
    
    if (!mySolver.solve()) cout << endl << "Problem is not in LIFT" << endl;
    else
    {
        for (int i=0; i<igraph_ecount(G); i++)
            
            if (mySolver.modelValue(e(i+1)) != l_False) { Edge edge; igraph_edge(G, i, &edge.first, &edge.second); self->H->del_edge(edge); }
        //		for (int j=0; j < igraph_vector_ptr_size(&match_vert); j++)
        //		{
        //			cout << j << " ";
        //			igraph_vector_t* v = (igraph_vector_t*) VECTOR(match_vert)[j];
        //			for (int k=0; k < igraph_vector_size(v); k++) cout << VECTOR(*v)[k] << " ";
        //			cout << endl;
        //		}
        //		for (int j=0; j < mySolver.model.size(); j++)
        //			cout << (mySolver.model[j]==l_False? "false" : "true") << " ";
        for (int j=0; j < igraph_vcount(G); j++)
        {
            for (int k=0; k < igraph_vcount(G); k++)
            {
                cout << j << "->" << k << ": " << (mySolver.modelValue(phi(10,1,j,k))==l_False? "false": "true"); cout << " ";
            }
            cout << endl;
        }
    }
    
    cout << "ecount_less_than_eta is: " << (ecount_less_than_eta->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "k_secure is: " << (k_secure->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "sat is: " << (sat->evaluate(&mySolver)? "True" : "False") << endl;
    cout << "cnf_sat is: " << (cnf_sat.evaluate(&mySolver)? "True" : "False") << endl;
    cout.flush();
    //	Grabage Collection
    for (int i =0; i < igraph_vector_ptr_size(&match_vert); i++)
        igraph_vector_destroy((igraph_vector_t*) VECTOR(match_vert)[i]);
    igraph_vector_ptr_destroy(&match_vert);
    
    //	for (int i = 0; i < sum.size(); i++) delete sum[i];
    
    //	delete carry, tmp1, tmp2, tmp3, carry_neg, sum_neg, sat;
    //	delete sat;
}

C_SAT::C_SAT(Security* security, int min_L1, int max_L1, int eta, int u) {
    
    self = security;
    Circuit* G = self->G;
    sat = new Formula(F_AND);
    igraph_vector_ptr_init(&match_vert, 0);
    for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        if (igraph_vector_ptr_size(&match_vert) < color + 1)
            for (int j = igraph_vector_ptr_size(&match_vert); j <= color; j++)
            {
                igraph_vector_t* v = (igraph_vector_t*) malloc(sizeof(igraph_vector_t));
                igraph_vector_init(v, 0);
                igraph_vector_ptr_push_back(&match_vert, v);
            }
        igraph_vector_push_back((igraph_vector_t*) VECTOR(match_vert)[color], i);
    }
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        for (int j=0; j < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]); j++)
            cout << igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[i], j) << "";
        cout << endl;
    }
    
    cout.flush();
    
    //	int nVars
    
    for (int i=0; i < igraph_vector_ptr_size(&match_vert); i++)
    {
        int n = igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[i]);
        for (int j=0; j < n * n * (igraph_vcount(G))*(igraph_vcount(G)); j++)
            sat->newVar();
    }
    
    for (int i=0; i < igraph_ecount(G); i++) sat->newVar();
    
    marker = sat->maxVar();
    
    int n = (int) floor(log(igraph_ecount(G))/log(2)+1);
    //	char* eta_b = (char*) malloc(n);
    
    vec<Formula*> sum;
    vec<Var> sum_var;
    
    for (int i = 0; i < n; i++)
    { sum.push(NULL); sum_var.push(0); }
    
    Formula* tmp1 = new Formula(), *tmp2 = new Formula(), *tmp3 = new Formula(F_OR), *carry = new Formula(), *sum_neg, *carry_neg;
    tmp1->add(mkLit(e(1)));
    tmp1->add(mkLit(e(2), true));
    
    tmp2->add(mkLit(e(1), true));
    tmp2->add(mkLit(e(2)));
    
    sum_var[0] = sat->newVar();
    sum[0] = new Formula(F_OR);
    sum[0]->add(tmp1);
    sum[0]->add(tmp2);
    sum_neg = new Formula(*sum[0]);
    sum_neg->negate();
    Formula* tmp4 = new Formula();
    tmp4->add(mkLit(sum_var[0]));
    tmp4->add(sum[0]);
    
    tmp3->add(tmp4);
    tmp4 = new Formula();
    tmp4->add(mkLit(sum_var[0], true));
    tmp4->add(sum_neg);
    tmp3->add(tmp4);
    
    sum[0]=new Formula();
    sat->add(tmp3); //sum[0]->add(tmp3);
    
    carry->add(mkLit(e(1)));
    carry->add(mkLit(e(2)));
    carry_neg = new Formula(*carry);
    carry_neg->negate();
    int carry_var = sat->newVar();
    tmp3 = new Formula(F_OR);
    tmp4 = new Formula();
    tmp4->add(mkLit(carry_var));
    tmp4->add(carry);
    
    tmp3->add(tmp4);
    tmp4 = new Formula();
    tmp4->add(mkLit(carry_var, true));
    tmp4->add(carry_neg);
    tmp3->add(tmp4);
    
    sum[1] = new Formula();
    sat->add(tmp3); // sum[1]->add(tmp3);
    sum_var[1] = carry_var;
    
    //cout << "Before loop: " << sum[1]->maxVar() << endl;
    for (int i = 3; i <= igraph_ecount(G); i++)
    {
        Lit sum_neg = mkLit(sum_var[0], true); //sum_neg->negate();
        tmp1 = new Formula();
        tmp1->add(mkLit(sum_var[0]));
        tmp1->add(mkLit(e(i), true));
        
        tmp2 = new Formula();
        tmp2->add(sum_neg);
        tmp2->add(mkLit(e(i)));
        
        carry = new Formula();
        carry->add(mkLit(sum_var[0]));
        carry->add(mkLit(e(i)));
        
        sum_var[0] = sat->newVar();
        Formula* tmp5 = new Formula(F_OR);
        tmp5->add(tmp1);
        tmp5->add(tmp2);
        
        tmp3 = new Formula(); tmp4 = new Formula(F_OR);
        Formula* tmp5_neg = new Formula(*tmp5); tmp5_neg->negate();
        tmp3->add(mkLit(sum_var[0])); tmp3->add(tmp5);
        tmp4->add(tmp3);
        tmp3 = new Formula();
        tmp3->add(mkLit(sum_var[0], true)); tmp3->add(tmp5_neg);
        tmp4->add(tmp3);
        
        sat->add(tmp4); //sum[0]->add(tmp4);
        
        int carry_var = sat->newVar();
        carry_neg = new Formula(*carry); carry_neg->negate();
        tmp3 = new Formula(); //tmp4 = new Formula(F_OR);
        tmp3->add(mkLit(carry_var)); tmp3->add(carry);
        carry = new Formula(F_OR); carry->add(tmp3);
        tmp3 = new Formula();
        tmp3->add(mkLit(carry_var, true)); tmp3->add(carry_neg);
        carry->add(tmp3);
        //		sum[1]->add(carry);
        
        int j;
        for (j = 1; sum[j]!=NULL && j < sum.size(); j++)
        {
            //		cout << sum[j]->maxVar() << endl;
            sat->add(carry);
            // sum[j]->add(carry);
            
            //		cout << sum[j]->maxVar() << " " << carry->maxVar() << endl;
            // Formula* carry_neg = new Formula(*carry);
            Lit carry_neg = mkLit(carry_var, true);
            Lit sum_neg = mkLit(sum_var[j], true); //new Formula(*sum[j]);
            
            //			sum_neg->negate(); carry_neg->negate();
            
            tmp1 = new Formula();
            tmp1->add(mkLit(sum_var[j]));
            tmp1->add(carry_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(mkLit(carry_var));
            
            tmp3 = new Formula();
            tmp3->add(mkLit(sum_var[j]));
            tmp3->add(mkLit(carry_var));
            
            
            tmp4 = new Formula(F_OR);
            tmp4->add(tmp1);
            tmp4->add(tmp2);
            
            sum_var[j] = sat->newVar();
            
            Formula* tmp4_neg = new Formula(*tmp4); tmp4_neg->negate();
            Formula* tmp5 = new Formula(), *tmp6 = new Formula(F_OR);
            
            //		cout << "tmp4: " << tmp4->maxVar() << endl;
            //			cout << "sum_var[j]: " << mkLit(sum_var[j]) << endl;
            tmp5->add(mkLit(sum_var[j])); tmp5->add(tmp4);
            tmp6->add(tmp5);
            tmp5 = new Formula();
            tmp5->add(mkLit(sum_var[j], true)); tmp5->add(tmp4_neg);
            
            //			cout << "tmp5: " << tmp5->maxVar() << endl;
            tmp6->add(tmp5);
            
            //			cout << "tmp6: " << tmp6->maxVar() << endl;
            // sum[j]->add(tmp6);
            sat->add(tmp6);
            
            carry_var = sat->newVar();
            Formula* tmp3_neg = new Formula(*tmp3); tmp3_neg->negate();
            tmp4 = new Formula(); tmp4->add(mkLit(carry_var)); tmp4->add(tmp3);
            carry = new Formula(F_OR); carry->add(tmp4);
            tmp4 = new Formula(); tmp4->add(mkLit(carry_var, true)); tmp4->add(tmp3_neg);
            carry->add(tmp4);
            
            //		cout << "End loop body: " << sum[j]->maxVar() << endl;
        }
        if (j < sum.size()) { sum[j] = new Formula(); sat->add(carry); sum_var[j] = carry_var; }
    }
    
    //	cout << endl << ecount_less_than_eta->str();
    //	cout << endl << leqf->str();
    
    //	sat->add(ecount_less_than_eta);
    
    Formula* k_secure = new Formula(F_AND);
    
    vec<Formula*> formuli;
    
    int i = u;
    
    //	for (int i = 0; i < igraph_vcount(G); i++)
    {
        int color = VAN(G, "colour", i);
        //		vec<Formula*> formuli;
        //		Formula* formula1 = new Formula(F_OR);
        for (int j1 = 0; j1 < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[color]); j1++)
        {
            int j = igraph_vector_e((igraph_vector_t*) VECTOR(match_vert)[color], j1);
            if (j == i) continue;
            Formula* F1 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k == i) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //				for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l == j) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == i && l != j) continue;
                    if (k != i || l != j) nested1->add(mkLit(phi(i, j, k, l)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                    {
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != j) nested1->add(mkLit(phi(i, j, k, VECTOR(*vec)[h]), true));
                    }
                    nested->add(nested1);
                }
                F1->add(nested);
            }
            
            Formula* F2 = new Formula(F_AND);
            for (int k = 0; k < igraph_vcount(G); k++)
            {	if (k==j) continue;
                int k_color = VAN(G, "colour", k);
                Formula* nested = new Formula(F_OR);
                //for (int l = 0; l < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); l++)
                for (int l = 0; l < igraph_vcount(G); l++)
                {	if (l==i) continue;
                    Formula* nested1 = new Formula(F_AND);
                    int l_color = VAN(G, "colour", l);
                    if (k_color != l_color) continue;
                    if (k == j && l != i) continue;
                    if (k != j || l != i) nested1->add(mkLit(phi(i, j, l, k)));
                    
                    for (int h = 0; h < igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[k_color]); h++)
                    {
                        //					for (int h = 0; h < igraph_vcount(G); h++)
                        igraph_vector_t *vec = (igraph_vector_t *) VECTOR(match_vert)[k_color];
                        if (VECTOR(*vec)[h] != l && VECTOR(*vec)[h] != i) nested1->add(mkLit(phi(i, j, VECTOR(*vec)[h], k), true));
                    }
                    nested->add(nested1);
                }
                F2->add(nested);
            }
            
            Formula* F3 = new Formula(F_AND);
            for (int k = 0; k < igraph_ecount(G); k++)
            {
                Formula* nested = new Formula(F_OR);
                nested->add(mkLit(e(k+1)));
                for (int l = 0; l < igraph_ecount(G); l++)
                {
                    int src_l, dest_l, src_k, dest_k;
                    igraph_edge(G, l, &src_l, &dest_l);
                    int src_col_k, dest_col_k, src_col_l, dest_col_l;
                    src_col_l = VAN(G, "colour", src_l);
                    dest_col_l = VAN(G, "colour", dest_l);
                    igraph_edge(G, k, &src_k, &dest_k);
                    src_col_k = VAN(G, "colour", src_k);
                    dest_col_k = VAN(G, "colour", dest_k);
                    if (src_col_l != src_col_k || dest_col_l != dest_col_k) continue;
                    if (src_k == i && src_l != j || dest_k == i && dest_l != j) continue;
                    if (src_k != i && src_l == j || dest_k != i && dest_l == j) continue;
                    if (src_k == i && src_l == j) { nested->add(mkLit(phi(i,j,dest_k,dest_l))); continue; }
                    if (dest_k == i && dest_l == j) { nested->add(mkLit(phi(i,j,src_k,src_l))); continue; }
                    Formula* p = new Formula(F_AND);
                    p->add(mkLit(phi(i,j,src_k,src_l)));
                    p->add(mkLit(phi(i,j,dest_k,dest_l)));
                    nested->add(p);
                }
                F3->add(nested);
            }
            
            Formula* F = new Formula(F_AND);
            F->add(F1);
            F->add(F2);
            F->add(F3);
            //			formula1->add(F);
            formuli.push(F);
        }
        //		k_secure->add(formula1);
    }
    
    {
        igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[(int) VAN(G, "colour", i)]);
        int n1 = (int) floor(log(	igraph_vector_size((igraph_vector_t*) VECTOR(match_vert)[(int) VAN(G, "colour", i)])-1)/log(2)+1);
        //	char* eta_b = (char*) malloc(n);
        
        vec<Formula*> sum;
        //	sum.clear();
        vec<Var> sum_var;
        //	sum_var.clear();
        
        for (int i = 0; i < n1; i++)
        { sum.push(NULL); sum_var.push(0); }
        
        
        Formula* tmp1 = new Formula(), *tmp2 = new Formula(), *tmp3 = new Formula(F_OR), *carry = new Formula(), *sum_neg, *carry_neg;
        tmp1->add(formuli[0]);
        Formula* formula1_neg = new Formula(*formuli[1]); formula1_neg->negate();
        tmp1->add(formula1_neg);
        
        Formula* formula0_neg = new Formula(*formuli[0]); formula0_neg->negate();
        tmp2->add(formula0_neg);
        tmp2->add(formuli[1]);
        
        sum_var[0] = sat->newVar();
        sum[0] = new Formula(F_OR);
        sum[0]->add(tmp1);
        sum[0]->add(tmp2);
        sum_neg = new Formula(*sum[0]);
        sum_neg->negate();
        Formula* tmp4 = new Formula();
        tmp4->add(mkLit(sum_var[0]));
        tmp4->add(sum[0]);
        
        tmp3->add(tmp4);
        tmp4 = new Formula();
        tmp4->add(mkLit(sum_var[0], true));
        tmp4->add(sum_neg);
        tmp3->add(tmp4);
        
        sum[0]=new Formula();
        sat->add(tmp3); //sum[0]->add(tmp3);
        
        carry->add(formuli[0]);
        carry->add(formuli[1]);
        carry_neg = new Formula(*carry);
        carry_neg->negate();
        int carry_var = sat->newVar();
        tmp3 = new Formula(F_OR);
        tmp4 = new Formula();
        tmp4->add(mkLit(carry_var));
        tmp4->add(carry);
        
        tmp3->add(tmp4);
        tmp4 = new Formula();
        tmp4->add(mkLit(carry_var, true));
        tmp4->add(carry_neg);
        tmp3->add(tmp4);
        
        sum[1] = new Formula();
        sat->add(tmp3); // sum[1]->add(tmp3);
        sum_var[1] = carry_var;
        
        //cout << "Before loop: " << sum[1]->maxVar() << endl;
        for (int i = 2; i < formuli.size(); i++)
        {
            Lit sum_neg = mkLit(sum_var[0], true); //sum_neg->negate();
            tmp1 = new Formula();
            tmp1->add(mkLit(sum_var[0]));
            Formula* formulii_neg = new Formula(*formuli[i]); formulii_neg->negate();
            tmp1->add(formulii_neg);
            
            tmp2 = new Formula();
            tmp2->add(sum_neg);
            tmp2->add(formuli[i]);
            
            carry = new Formula();
            carry->add(mkLit(sum_var[0]));
            carry->add(formuli[i]);
            
            sum_var[0] = sat->newVar();
            Formula* tmp5 = new Formula(F_OR);
            tmp5->add(tmp1);
            tmp5->add(tmp2);
            
            tmp3 = new Formula(); tmp4 = new Formula(F_OR);
            Formula* tmp5_neg = new Formula(*tmp5); tmp5_neg->negate();
            tmp3->add(mkLit(sum_var[0])); tmp3->add(tmp5);
            tmp4->add(tmp3);
            tmp3 = new Formula();
            tmp3->add(mkLit(sum_var[0], true)); tmp3->add(tmp5_neg);
            tmp4->add(tmp3);
            
            sat->add(tmp4); //sum[0]->add(tmp4);
            
            int carry_var = sat->newVar();
            carry_neg = new Formula(*carry); carry_neg->negate();
            tmp3 = new Formula(); //tmp4 = new Formula(F_OR);
            tmp3->add(mkLit(carry_var)); tmp3->add(carry);
            carry = new Formula(F_OR); carry->add(tmp3);
            tmp3 = new Formula();
            tmp3->add(mkLit(carry_var, true)); tmp3->add(carry_neg);
            carry->add(tmp3);
            //		sum[1]->add(carry);
            
            int j;
            for (j = 1; sum[j]!=NULL && j < sum.size(); j++)
            {
                //		cout << sum[j]->maxVar() << endl;
                sat->add(carry);
                // sum[j]->add(carry);
                
                //		cout << sum[j]->maxVar() << " " << carry->maxVar() << endl;
                // Formula* carry_neg = new Formula(*carry);
                Lit carry_neg = mkLit(carry_var, true);
                Lit sum_neg = mkLit(sum_var[j], true); //new Formula(*sum[j]);
                
                //			sum_neg->negate(); carry_neg->negate();
                
                tmp1 = new Formula();
                tmp1->add(mkLit(sum_var[j]));
                tmp1->add(carry_neg);
                
                tmp2 = new Formula();
                tmp2->add(sum_neg);
                tmp2->add(mkLit(carry_var));
                
                tmp3 = new Formula();
                tmp3->add(mkLit(sum_var[j]));
                tmp3->add(mkLit(carry_var));
                
                
                tmp4 = new Formula(F_OR);
                tmp4->add(tmp1);
                tmp4->add(tmp2);
                
                sum_var[j] = sat->newVar();
                
                Formula* tmp4_neg = new Formula(*tmp4); tmp4_neg->negate();
                Formula* tmp5 = new Formula(), *tmp6 = new Formula(F_OR);
                
                //		cout << "tmp4: " << tmp4->maxVar() << endl;
                //			cout << "sum_var[j]: " << mkLit(sum_var[j]) << endl;
                tmp5->add(mkLit(sum_var[j])); tmp5->add(tmp4);
                tmp6->add(tmp5);
                tmp5 = new Formula();
                tmp5->add(mkLit(sum_var[j], true)); tmp5->add(tmp4_neg);
                
                //			cout << "tmp5: " << tmp5->maxVar() << endl;
                tmp6->add(tmp5);
                
                //			cout << "tmp6: " << tmp6->maxVar() << endl;
                // sum[j]->add(tmp6);
                sat->add(tmp6);
                
                carry_var = sat->newVar();
                Formula* tmp3_neg = new Formula(*tmp3); tmp3_neg->negate();
                tmp4 = new Formula(); tmp4->add(mkLit(carry_var)); tmp4->add(tmp3);
                carry = new Formula(F_OR); carry->add(tmp4);
                tmp4 = new Formula(); tmp4->add(mkLit(carry_var, true)); tmp4->add(tmp3_neg);
                carry->add(tmp4);
                
                //		cout << "End loop body: " << sum[j]->maxVar() << endl;
            }
            if (j < sum.size()) { sum[j] = new Formula(); sat->add(carry); sum_var[j] = carry_var; }
        }
        
        string min_L1_b = convertInt(min_L1-1);
        string temp="";
        for (int i = min_L1_b.length(); i < n1; i++)
            temp+='0';
        
        min_L1_b=temp+min_L1_b;
        
        Formula* geqf = geq(sum_var,min_L1_b,0);
        Formula* geqf_neg = new Formula(*geqf); geqf_neg->negate();
        
        Var comp_var = sat->newVar();
        Formula* temp1 = new Formula(F_OR), * temp2 = new Formula();
        temp2->add(geqf); temp2->add(mkLit(comp_var));
        temp1->add(temp2); temp2 = new Formula();
        temp2->add(geqf_neg); temp2->add(mkLit(comp_var,true));
        temp1->add(temp2);
        
        
        sat->add(mkLit(comp_var)); sat->add(temp1);
        
    }
    
    
    //	sat->add(k_secure);
    
    int upper = igraph_ecount(G), lower = 0;
    vec<lbool> solution;
    
    while (upper > lower + 1)
    {
        int eta = (upper + lower) / 2;
        string eta_b = convertInt(eta);
        string temp="";
        for (int i = eta_b.length(); i < n; i++)
            temp+='0';
        
        eta_b=temp+eta_b;
        
        Formula* leqf = leq(sum_var,eta_b,0);
        Formula* leqf_neg = new Formula(*leqf); leqf_neg->negate();
        
        Formula* final_sat = new Formula(); final_sat->add(sat);
        Var comp_var = final_sat->newVar();
        Formula* temp1 = new Formula(F_OR), * temp2 = new Formula();
        temp2->add(leqf); temp2->add(mkLit(comp_var));
        temp1->add(temp2); temp2 = new Formula();
        temp2->add(leqf_neg); temp2->add(mkLit(comp_var,true));
        temp1->add(temp2);
        
        
        final_sat->add(mkLit(comp_var)); final_sat->add(temp1);
        
        Lit out;
        Solver mySolver;
        cout << "here..." << endl;
        final_sat->export_cnf(out, NULL, &mySolver);
        cout << "done";
        mySolver.addClause(out);
        delete temp1;
        
        //delete sat;
        cout << endl << "done";
        
        if (!mySolver.solve()) { lower = eta; cout << endl << "Problem is not in LIFT for eta = " << eta << endl; }
        else
        {
            upper = eta;
            mySolver.model.copyTo(solution);
            cout << endl << "Problem is in LIFT for eta = " << eta << endl;
        }
        
    }
    
    formuli.clear(); sum.clear();
    //delete sat;
    for (int i=0; i<igraph_ecount(G); i++)
        if (solution[e(i+1)] != l_False) { Edge edge; igraph_edge(G, i, &edge.first, &edge.second); self->H->del_edge(edge); }
    
    //	Grabage Collection
    for (int i =0; i < igraph_vector_ptr_size(&match_vert); i++)
        igraph_vector_destroy((igraph_vector_t*) VECTOR(match_vert)[i]);
    igraph_vector_ptr_destroy(&match_vert);
    
}

// Added by Karl
void Security::get_edge_neighbors() {
    for (int i = 0; i < igraph_ecount(G); i++) {
        set<int> temp;
        edge_neighbors.push_back(temp);
        // Get vertices the edge is connected to
        int from, to;
        igraph_edge(G,i,&from,&to);
        
        // get neighbor vertices of the first vertex
        igraph_vector_t nvids;
        igraph_vector_init(&nvids, 0);
        igraph_neighbors(G, &nvids, from, IGRAPH_ALL);
        
        for (int j = 0; j < igraph_vector_size(&nvids); j++) {
            if (VECTOR(nvids)[j] != to) {
                // get id of edge connecting the 2 vertices as it's a neighbor to the target edge
                int eid;
                int nto = VECTOR(nvids)[j];
                igraph_get_eid(G, &eid, from, nto, IGRAPH_UNDIRECTED, 1);
                
                // Add the id of that edge to the set of neighbors of the target edge
                set<int>::const_iterator got = edge_neighbors[i].find(eid);
                if (got == edge_neighbors[i].end() && eid != i) // not in set + security check
                    edge_neighbors[i].insert(eid);
            }
        }
        
        // get neighbor vertices of the first vertex
        igraph_vector_destroy(&nvids);
        igraph_vector_init(&nvids, 0);
        igraph_neighbors(G, &nvids, to, IGRAPH_ALL);
        
        for (int j = 0; j < igraph_vector_size(&nvids); j++) {
            if (VECTOR(nvids)[j] != from) {
                // get id of edge connecting the 2 vertices as it's a neighbor to the target edge
                int eid;
                int nfrom = VECTOR(nvids)[j];
                igraph_get_eid(G, &eid, nfrom, to, IGRAPH_UNDIRECTED, 1);
                
                // Add the id of that edge to the set of neighbors of the target edge
                set<int>::const_iterator got = edge_neighbors[i].find(eid);
                if (got == edge_neighbors[i].end() && eid != i) // not in set + security check
                    edge_neighbors[i].insert(eid);
            }
        }
    }
}

void Security::create_graph(igraph_t* g, set<int> edges, map<int,int>& map12, set<int>& vertices_set, int* max_degree, bool mapping, bool create) {
    map<int,int> vertices;
    
    if (create)
        igraph_empty(g,0,IGRAPH_DIRECTED);
    
    set<int>::iterator it;
    for (it = edges.begin(); it != edges.end(); it++) {
        int from, to;
        int new_from, new_to;
        int F_from, F_to;
        
        igraph_edge(G,*it,&from,&to);
        
        // if adding vertices and egdes to H, "delete" them from G
        if (!create) {
            SETVAN(G, "Removed", from, Removed);
            SETVAN(G, "Removed", to, Removed);
            SETEAN(G, "Removed", *it, Removed);
        }
        
        map<int,int>::iterator got = vertices.find(from);
        if (got == vertices.end()) { // not in set
            igraph_add_vertices(g, 1, 0);
            
            int vid = igraph_vcount(g) - 1;
            
            if (start && !mapping && !create) {
                H_v_dummy++;
                igraph_add_vertices(F, 1, 0);
                SETVAN(F, "Dummy", igraph_vcount(F)-1, kDummy);
                SETVAN(F, "colour", igraph_vcount(F)-1, VAN(G, "colour", from));
                SETVAN(F, "ID", igraph_vcount(F)-1, igraph_vcount(F)-1);
                SETVAS(F, "Tier", igraph_vcount(F)-1, "Bottom");
                F_from = igraph_vcount(F)-1;
            } else if (!mapping && !create) {
                SETVAS(F, "Tier", VAN(G, "ID", from), "Bottom");
                SETVAS(R, "Tier", VAN(G, "ID", from), "Bottom");
                F_from = VAN(G, "ID", from);
            }
            
            SETVAN(g, "colour", vid, VAN(G, "colour", from));
            SETVAN(g, "ID", vid, VAN(G, "ID", from));
            
            vertices.insert(pair<int,int>(from,vid));
            new_from = vid;
            
            // add to vertices set
            set<int>::iterator has = vertices_set.find(from);
            if (has == vertices_set.end()) // not in set, security check
                vertices_set.insert(from);
            
            if (igraph_vertex_degree(G, from) > *max_degree)
                *max_degree = igraph_vertex_degree(G, from);
            
            // "mapping" of the vertices of the new pag to vertices in G
            if (mapping)
                map12.insert(pair<int,int>(vid,from));
            
            // "mapping" of the vertices of H to vertices in G
            if (!create) {
                map<int,int>::iterator in = map12.find(VAN(G,"ID",from));
                
                if (in == map12.end()) // not in set
                    map12.insert(pair<int,int>(VAN(G,"ID",from),vid));
            }
        } else new_from = vertices[from];
        
        got = vertices.find(to);
        if (got == vertices.end()) { // not in set
            igraph_add_vertices(g, 1, 0);
            
            int vid = igraph_vcount(g) - 1;
            
            if (start && !mapping && !create) {
                H_v_dummy++;
                igraph_add_vertices(F, 1, 0);
                SETVAN(F, "Dummy", igraph_vcount(F)-1, kDummy);
                SETVAN(F, "colour", igraph_vcount(F)-1, VAN(G, "colour", to));
                SETVAN(F, "ID", igraph_vcount(F)-1, igraph_vcount(F)-1);
                SETVAS(F, "Tier", igraph_vcount(F)-1, "Bottom");
                F_to = igraph_vcount(F)-1;
            } else if (!mapping && !create) {
                SETVAS(F, "Tier", VAN(G, "ID", to), "Bottom");
                SETVAS(R, "Tier", VAN(G, "ID", to), "Bottom");
                F_to = VAN(G, "ID", to);
            }
            
            SETVAN(g, "colour", vid, VAN(G, "colour", to));
            SETVAN(g, "ID", vid, VAN(G, "ID", to));
            vertices.insert(pair<int,int>(to,vid));
            new_to = vid;
            
            // add to vertices set
            set<int>::iterator has = vertices_set.find(to);
            if (has == vertices_set.end()) // not in set, security check
                vertices_set.insert(to);
            
            if (igraph_vertex_degree(G, to) > *max_degree)
                *max_degree = igraph_vertex_degree(G, to);
            
            if (mapping)
                map12.insert(pair<int,int>(vid,to));
            
            if (!create) {
                map<int,int>::iterator in = map12.find(VAN(G,"ID",to));
                
                if (in == map12.end()) // not in set
                    map12.insert(pair<int,int>(VAN(G,"ID",to),vid));
            }
        } else new_to = vertices[to];
        
        igraph_add_edge(g, new_from, new_to);
        
        if (start && !mapping && !create) {
            H_e_dummy++;
            igraph_add_edge(F, F_from, F_to);
            SETEAN(F, "Dummy", igraph_ecount(F)-1, kDummy);
//            SETEAN(F, "colour", igraph_ecount(F)-1, EAN(G, "colour", to));
            SETEAS(F, "Tier", igraph_ecount(F)-1, "Bottom");
        } else if (!mapping && !create) {
            SETEAS(F, "Tier", EAN(G, "ID", *it), "Bottom");
            SETEAS(R, "Tier", EAN(G, "ID", *it), "Bottom");
        }
        
        SETEAN(g, "ID", igraph_ecount(g)-1, EAN(G, "ID", *it));
    }
}

void Security::isomorphic_test(set<int> current_subgraph) {
    map<int,int> mapPAGG; // mapping of the new pag from its vertices to G
    set<int> vert;
    igraph_t new_pag;
    int max_degree = 0;
    
    create_graph(&new_pag, current_subgraph, mapPAGG, vert, &max_degree);
    
    igraph_vector_t color11;
    igraph_vector_init(&color11, 0);
    igraph_vector_int_t color1;
    igraph_vector_int_init(&color1, 0);
    
    VANV(&new_pag, "colour", (igraph_vector_t*) &color11);
    for (int i = 0; i < igraph_vector_size(&color11); i++)
        igraph_vector_int_push_back(&color1, VECTOR(color11)[i]);
    
    //            // debug
    //            for (int i = 0; i < igraph_vcount(&new_pag); i++) {
    //                if (i == 0)
    //                    cout<<"new_pag:"<<endl;
    //                cout<<"     ID: "<<VAN(&new_pag, "ID", i)<<" "<<VAN(&new_pag, "colour", i)<<endl;
    //            }
    //            //--
    
    // Check if this subgraph alrady has an isomorphic pag. If it does, it's an embedding of that pag
    igraph_bool_t iso = false;
    // debug
    int index = 0;
    //--
    
    igraph_vector_t* map12 = new igraph_vector_t;
    igraph_vector_init(map12, 0);
    
    for (int i = 0; i < pags.size(); i++) {
        // debug
        index = i;
        //--
        
        // create the subgraphs
        map<int,int> dummy;
        set<int> dumy;
        int dum;
        igraph_t pag;
        create_graph(&pag, pags[i].pag, dummy, dumy, &dum, false);
        
        if (igraph_vcount(&pag) != igraph_vcount(&new_pag))
            continue;
        
        igraph_vector_t color22;
        igraph_vector_init(&color22, 0);
        igraph_vector_int_t color2;
        igraph_vector_int_init(&color2, 0);
        
        VANV(&pag, "colour", (igraph_vector_t*) &color22);
        for (int j = 0; j < igraph_vector_size(&color22); j++)
            igraph_vector_int_push_back(&color2, VECTOR(color22)[j]);
        //                cout<<"1: "<<igraph_vector_size(&color11)<<" 2: "<<igraph_vector_size(&color22)<<endl;
        igraph_isomorphic_vf2(&pag, &new_pag, &color2, &color1, NULL, NULL, &iso, map12, NULL, NULL, NULL, NULL);
        
        if (iso)
            break;
    }
    
    if (!iso && pags.size() == 0) {
        //                // debug
        //                cout<<"nope: ";
        //                set<int>::iterator it;
        //                for (it = current_subgraph.begin(); it != current_subgraph.end(); it++)
        //                    cout<<*it<<" ";
        //                cout<<endl;
        //                //--
        
        // add this subgraph to the pags
        PAG temp_pag;
        pags.push_back(temp_pag);
        pags[pags.size()-1].pag = current_subgraph;
        pags[pags.size()-1].mapPAGG = mapPAGG;
        pags[pags.size()-1].vertices = vert;
        pags[pags.size()-1].max_degree = max_degree;
        pags[pags.size()-1].processed = false;
    } else if (iso) {
        //                // debug
        //                cout<<"yup: ";
        //                set<int>::iterator it;
        //                for (it = pags[index].pag.begin(); it != pags[index].pag.end(); it++)
        //                    cout<<*it<<" ";
        //                cout<<"and ";
        //                for (it = current_subgraph.begin(); it != current_subgraph.end(); it++)
        //                    cout<<*it<<" ";
        //                cout<<endl;
        //                //--
        
        // add it to the list of embedding for that PAG
        EMBEDDINGS temp;
        pags[index].embeddings.push_back(temp);
        pags[index].embeddings[pags[index].embeddings.size()-1].edges = current_subgraph;
        
        for (int i = 0; i < igraph_vector_size(map12); i++)
            igraph_vector_set(map12, i, VAN(&new_pag, "ID", igraph_vector_e(map12,i)));
        
        pags[index].embeddings[pags[index].embeddings.size()-1].map = map12;
        pags[index].embeddings[pags[index].embeddings.size()-1].max_degree = 0;
        pags[index].embeddings[pags[index].embeddings.size()-1].vertices = vert;
        pags[index].embeddings[pags[index].embeddings.size()-1].max_degree = max_degree;
    }
}

void Security::subgraphs(int v, set<int> current_subgraph, set<int> possible_edges, set<int> neighbors) {
    if (maxPAGsize == 1)
        isomorphic_test(current_subgraph);
    else if (current_subgraph.size() == maxPAGsize-1) {
        //        // debug
        //        cout<<"subgraphs of size "<<maxPAGsize<<":"<<endl;
        //        string first;
        //
        //        set<int>::iterator it1;
        //        for (it1 = current_subgraph.begin(); it1 != current_subgraph.end(); it1++) {
        //            stringstream ss;
        //            ss << *it1;
        //            string str = ss.str();
        //            first = first + " " + str;
        //        }
        
        set<int>::iterator it;
        //        for (it = possible_edges.begin(); it != possible_edges.end(); it++)
        //            cout<<first<<" "<<*it<<endl;
        //        //--
        
        // Every edge in the possible list will give a new subgrph so to save them they are added to the current subgraph one after the other
        for (it = possible_edges.begin(); it != possible_edges.end(); it++) {
            // add an edge to the current subgraph
            current_subgraph.insert(*it);
            
            isomorphic_test(current_subgraph);
            
            // bring back the current subgraph to how it was to add another edge from the list
            current_subgraph.erase(*it);
        }
        
        return;
    }
    
    while (!possible_edges.empty()) {
        // don't want to modify the graph for next edge to be added to form a new subgraph
        set<int> temp_subgraph = current_subgraph;
        // add the first edge in the list
        int new_edge = *possible_edges.begin();
        
        set<int>::iterator got = temp_subgraph.find(new_edge);
        if (got == temp_subgraph.end()) // not in set
            temp_subgraph.insert(new_edge);
        // remove it from the list because it can't be part of it for this graph of the next since it's a neighbor of an edge in the graph
        possible_edges.erase(possible_edges.begin());
        
        // add the rest of the list to the list of the new subgraph
        set<int> temp_edges = possible_edges;
        set<int>::iterator it;
        // for every neighbor of the newly added edge
        for (it = edge_neighbors[new_edge].begin(); it != edge_neighbors[new_edge].end(); it++) {
            int neighbor = *it;
            // check if it's bigger than the original edge of this family of subgraphs, if it's not then the subgraphs containing these 2 are already created
            if (neighbor > v) {
                set<int>::iterator got = neighbors.find(neighbor);
                // check if it's the neighbor of one of the other edges in the graph as well. If it is, the subgraphs containing these edges are already created
                if (got == neighbors.end()) // not in set
                    temp_edges.insert(neighbor);
            }
        }
        
        // the neighbors should stay the same for the next edge so a copy is created so that the updated version can be sent to the subgraphs created from these edges
        set<int> temp_neighbors = neighbors;
        temp_neighbors.insert(edge_neighbors[new_edge].begin(), edge_neighbors[new_edge].end());
        
        subgraphs(v, temp_subgraph, temp_edges, temp_neighbors);
    }
}

void Security::find_VD_embeddings(int i) {
    //cout<<"pags: "<<i<<endl;
    // add the pag itself to the embeddings to be studied because it is a part of the graph
    EMBEDDINGS temp;
    pags[i].embeddings.push_back(temp);
    pags[i].embeddings[pags[i].embeddings.size()-1].edges = pags[i].pag;
    pags[i].embeddings[pags[i].embeddings.size()-1].max_degree = pags[i].max_degree;
    pags[i].embeddings[pags[i].embeddings.size()-1].vertices = pags[i].vertices;
    
    pags[i].vd_embeddings.vd_embeddings.clear();
    pags[i].vd_embeddings.max_degree = 0;
    pags[i].vd_embeddings.max_count = 0;
    
    // Initialization
    for (int j = 0; j < pags[i].embeddings.size(); j++) {
        pags[i].embeddings[j].connected_embeddings.clear();
        pags[i].embeddings[j].size = 0;
    }
    
    map<int,set<int> > size;
    
    // for every embedding ...
    for (int j = 0; j < pags[i].embeddings.size(); j++) {
        // ... look at the other embeddings ...
        for (int k = j+1; k < pags[i].embeddings.size(); k++) {
            set<int>::iterator it;
            // ... for every vertex in the embedding studied ...
            for (it = pags[i].embeddings[j].vertices.begin(); it != pags[i].embeddings[j].vertices.end(); it++) {
                set<int>::iterator got = pags[i].embeddings[k].vertices.find(*it);
                // ... see if it is in the other embeddings of the same pag
                if (got != pags[i].embeddings[k].vertices.end()) { // in set, they are connected
                    pags[i].embeddings[j].connected_embeddings.insert(k);
                    pags[i].embeddings[k].connected_embeddings.insert(j);
                    break; // break and move to the next embeding
                }
            }
        }
        
        int temp_size = pags[i].embeddings[j].connected_embeddings.size();
        pags[i].embeddings[j].size = temp_size;
        // see if we already found an embedding with this many not vd embeddings
        // in both cases add the id of the new embedding to that
        map<int, set<int> >::iterator got = size.find(temp_size);;
        if (got == size.end()) { // not in set
            set<int> temp;
            temp.insert(j);
            size.insert(pair<int,set<int> >(temp_size, temp));
        } else size[temp_size].insert(j); // in set
    }
    
    // while there are embeddings that have connected embeddings (they are not vd embeddings) we remove from the candidate list the embedding that is conncted to the most embeddings
    // size->first is the # of connected embeddings, size->second is the ids of the embeddings that have that many conneceted embeddings
    while (size.size() > 1 || size.begin()->first != 0) {
        //            cout<<"YAS"<<endl;
        // access last element (in c++ map is sorted automatically everytime we insert an element)
        map<int,set<int> >::iterator itr = size.end();
        itr--;
        
        // get id of embedding to remove
        set<int> remove = itr->second;
        // if more than one have that much connected embeds then we take the first one
        int to_remove = *remove.begin();
        //            cout<<to_remove<<endl;
        // remove it from the set of embeds that have that much connected embeds
        itr->second.erase(to_remove);
        // to mark that it already have been considered
        pags[i].embeddings[to_remove].size = -1;
        
        // if it was the last one with that much connected embeds, update map
        if (itr->second.empty())
            size.erase(itr->first);
        
        // update size map and size for every connected embedding
        set<int>::iterator itera;
        for (itera = pags[i].embeddings[to_remove].connected_embeddings.begin(); itera != pags[i].embeddings[to_remove].connected_embeddings.end(); itera++) {
            int old_size = pags[i].embeddings[*itera].size;
            // if it was considered, skip, we don't want to put it back in the size map
            if (old_size == -1)
                continue;
            // remove from size map at the old size element
            map<int,set<int> >::iterator gotsize = size.find(old_size);
            gotsize->second.erase(*itera);
            if (gotsize->second.empty())
                size.erase(old_size);
            
            // place it at the right element in the size map (with the old size decreased by 1)
            --pags[i].embeddings[*itera].size;
            int new_size = pags[i].embeddings[*itera].size;
            gotsize = size.find(new_size);
            if (gotsize == size.end()) { // not in map
                set<int> temp;
                temp.insert(*itera);
                size.insert(pair<int,set<int> >(new_size, temp));
            } else size[new_size].insert(*itera);
        }
        
        //            // debug
        //            cout<<size.size()<<endl;
        //            map<int,set<int> >::iterator itrat;
        //            for (itrat = size.begin(); itrat != size.end(); itrat++)
        //                cout<<itrat->first<<" ";
        //            cout<<endl;
        //            cout<<size.begin()->first<<endl;
        //            //--
    }
    
    // save the VD-embeddings
    pags[i].vd_embeddings.max_degree = 0;
    
    map<int, set<int> >::iterator itervd;
    
    for (itervd = size.begin(); itervd != size.end(); itervd++) {
        set<int> temp = itervd->second;
        set<int>::iterator itr;
        for (itr = temp.begin(); itr != temp.end(); itr++) {
            pags[i].vd_embeddings.vd_embeddings.insert(pair<int, set<int> >(*itr, pags[i].embeddings[*itr].edges));
            if (pags[i].embeddings[*itr].max_degree > pags[i].vd_embeddings.max_degree) {
                pags[i].vd_embeddings.max_degree = pags[i].embeddings[*itr].max_degree;
                pags[i].vd_embeddings.max_count = 1;
            } else if (pags[i].embeddings[*itr].max_degree == pags[i].vd_embeddings.max_degree)
                pags[i].vd_embeddings.max_count++;
        }
    }
}

void Security::find_subgraphs() {
    set<int> not_permitted;
    
    if (maxPAGsize > 0){
        for (int i = 0; i < igraph_ecount(G); i++) {
            // Create the subgraph
            set<int> current_subgraph;
            set<int>::iterator got = current_subgraph.find(i);
            if (got == current_subgraph.end()) // not in set
                current_subgraph.insert(i);
            
            set<int> neighbors;
            set<int> permitted_neighbors;
            
            if (maxPAGsize > 1) {
                // Enumerate the neighbors of the edge already in the subgraph. This will reduce the complexity of adding the neighbors of a newly added edge to the list of possible edges
                neighbors = edge_neighbors[i];
                
                // When we start, every edge we add is not permitted in the future
                set<int>::const_iterator got2 = not_permitted.find(i);
                if (got2 == not_permitted.end()) // not in set
                    not_permitted.insert(i);
                
                //        // debug
                //        cout<<setfill('-')<<setw(100)<<"subgraphs enumaration"<<setfill('-')<<setw(99)<<"-"<<endl;
                //        cout<<i<<":";
                //        //--
                
                // add the neighbors of the edge to the possible edges
                set<int>::iterator it;
                for (it = edge_neighbors[i].begin(); it != edge_neighbors[i].end(); it++) {
                    set<int>::iterator got = not_permitted.find(*it);
                    // if it is permitted (not not permitted) then we can add it
                    if (got == not_permitted.end()) { // not in set
                        set<int>::iterator got2 = permitted_neighbors.find(*it);
                        // If it's not already in the set add it -> security check
                        if (got2 == permitted_neighbors.end()) // not in set
                            permitted_neighbors.insert(*it);
                    }
                }
            }
            
            //        // debug
            //        set<int>::iterator it2;
            //        for (it2 = permitted_neighbors.begin(); it2 != permitted_neighbors.end(); it2++)
            //            cout<<" "<<*it2;
            //        cout<<endl;
            //        //--
            
            // start constructing subgraphs of size maxPAGsize
            subgraphs(i, current_subgraph, permitted_neighbors, neighbors);
        }
    }
    
    if (maxPAGsize == 0) {
        for (int i = 0; i < igraph_vcount(G); i++) {
            int color = VAN(G,"colour",i);
            map<int,vector<int> >::iterator got = colors.find(color);
            if (got == colors.end()) { // not in set
                // new pag
                vector<int> tmp;
                tmp.push_back(i);
                colors.insert(pair<int, vector<int> >(color, tmp));
            } else colors[color].push_back(i);
        }
    }
}

void Security::update_pags() {
    // for every pag...
    int pag_loop = pags.size() - 1;
    for (int i = pag_loop; i >= 0; i--) {
        int to_remove = pags[i].embeddings.size()-1;
        int loop = pags[i].embeddings.size() - 1;
        // .. for every embedding
        for (int j = loop; j >= 0 ; j--) {
            bool remove = false;
            set<int>::iterator it;
            int to_remove = -1;
            // see if any of its vertices was removed when copying the vd-embeddings to H
            for (it = pags[i].embeddings[j].vertices.begin(); it != pags[i].embeddings[j].vertices.end(); it++) {
                if (VAN(G, "Removed", *it) == Removed) { // if it removed
                    to_remove = j;
                    remove = true;
                    break;
                }
            }
            
            // if it was, remove it from the embeddings and from the vd-embeddings
            if (remove)
                pags[i].embeddings.erase(pags[i].embeddings.begin()+j);
        }
        
        // the pag is a part of the embeddings, so if it's 0 then the pag should be removed
        if (pags[i].embeddings.size() == 0)
            pags.erase(pags.begin()+i);
        else { // if not
            bool remove = false;
            set<int>::iterator it;
            for (it = pags[i].vertices.begin(); it != pags[i].vertices.end(); it++) {
                if (VAN(G, "Removed", *it) == Removed) { // if it removed
                    remove = true;
                    break;
                }
            }
            
            // if the pag itself has a vertex that is removed then it is not in the embeddings anymore and so a new pag must be chosen
            if (remove == true) {
                // copy the information from the embedding to the pag
                pags[i].pag = pags[i].embeddings[0].edges;
                pags[i].vertices = pags[i].embeddings[0].vertices;
                pags[i].max_degree = pags[i].embeddings[0].max_degree;
                pags[i].mapPAGG.clear();
                
                for (int j = 0; j < igraph_vector_size(pags[i].embeddings[0].map); j++)
                    pags[i].mapPAGG.insert(pair<int,int>(j,igraph_vector_e(pags[i].embeddings[0].map, j)));
                
                // delete the pag
                pags[i].embeddings.erase(pags[i].embeddings.begin());
            } else pags[i].embeddings.erase(pags[i].embeddings.end()-1); // If the pag is a vd-embedding then remove it from list of embeddings because we will add it back when searching for the VD-embeddings
        }
    }
}

void Security::VD_embeddings(int* max_degree, int* max_count, int* first_pag, int min_L1) {
    for (int i = 0; i < pags.size(); i++) {
        find_VD_embeddings(i);
        
        if (pags[i].vd_embeddings.vd_embeddings.size() >= min_L1) {
            if (pags[i].vd_embeddings.max_degree > *max_degree) {
                *max_degree = pags[i].vd_embeddings.max_degree;
                *max_count = pags[i].vd_embeddings.max_count;
                *first_pag = i;
            } else if (pags[i].vd_embeddings.max_degree == *max_degree) {
                if (pags[i].vd_embeddings.max_count > *max_count) {
                    *max_count = pags[i].vd_embeddings.max_count;
                    *first_pag = i;
                }
            }
        }
        
        // debug
        cout<<endl<<setw(100)<<setfill('-')<<"pags, embeddings, VD embeddings"<<setfill('-')<<setw(99)<<"-"<<endl;
        cout<<"pag #"<<i<<": ";
        set<int>::iterator iter;
        for (iter = pags[i].pag.begin(); iter != pags[i].pag.end(); iter++)
            cout<<EAN(G,"ID",*iter)<<" ";
        cout<<endl;
        
        cout<<"vertices: ";
        for (iter = pags[i].vertices.begin(); iter != pags[i].vertices.end(); iter++)
            cout<<VAN(G, "ID",*iter)<<" ";
        cout<<endl;
        
        for (int j = 0; j < pags[i].embeddings.size(); j++) {
            cout<<endl;
            cout<<"embeding #"<<j<<": ";
            set<int>::iterator iter2;
            for (iter2 = pags[i].embeddings[j].edges.begin(); iter2 != pags[i].embeddings[j].edges.end(); iter2++)
                cout<<*iter2<<" ";
            cout<<endl;
            cout<<"connected embeddings: ";
            set<int>::iterator it;
            for (it = pags[i].embeddings[j].connected_embeddings.begin(); it != pags[i].embeddings[j].connected_embeddings.end(); it++)
                cout<<*it<<" ";
            cout<<endl;
            cout<<"map: ";
            if (j != pags[i].embeddings.size()-1) {
                for (int k = 0; k < igraph_vector_size(pags[i].embeddings[j].map); k++)
                    cout<<igraph_vector_e(pags[i].embeddings[j].map, k)<<" ";
            }
            cout<<endl;
            cout<<"vertices: ";
            for (it = pags[i].embeddings[j].vertices.begin(); it != pags[i].embeddings[j].vertices.end(); it++)
                cout<<*it<<" ";
            cout<<endl;
            
            cout<<"max_deg: "<<pags[i].embeddings[j].max_degree<<endl;
        }
        
        cout<<endl;
        
        map<int, set<int> >::iterator iter2;
        cout<<"VD-embeddings: ";
        for (iter2 = pags[i].vd_embeddings.vd_embeddings.begin(); iter2 != pags[i].vd_embeddings.vd_embeddings.end(); iter2++)
            cout<<iter2->first<<" ";
        cout<<endl<<"max deg: "<<pags[i].vd_embeddings.max_degree<<" max count: "<<pags[i].vd_embeddings.max_count<<endl;
        
        for (iter2 = pags[i].vd_embeddings.vd_embeddings.begin(); iter2 != pags[i].vd_embeddings.vd_embeddings.end(); iter2++) {
            cout<<"embedding "<<iter2->first<<": ";
            set<int> temp = iter2->second;
            set<int>::iterator it;
            for (it = temp.begin(); it != temp.end(); it++)
                cout<<*it<<" ";
            cout<<"deg: "<<pags[i].embeddings[iter2->first].max_degree;
            cout<<endl;
        }
        //--
    }
}

void Security::get_vertex_neighbors() {
    for (int i = 0; i < igraph_vcount(G); i++) {
        set<int> neighbors;
        
        igraph_vector_t nvids;
        igraph_vector_init(&nvids, 0);
        igraph_neighbors(G, &nvids, i, IGRAPH_OUT);
        
        for (int j = 0; j < igraph_vector_size(&nvids); j++)
            neighbors.insert(VECTOR(nvids)[j]);
        
        vertex_neighbors_out.push_back(neighbors);
        
        
        neighbors.clear();
        igraph_vector_clear(&nvids);
        igraph_neighbors(G, &nvids, i, IGRAPH_IN);
        
        for (int j = 0; j < igraph_vector_size(&nvids); j++)
            neighbors.insert(VECTOR(nvids)[j]);
        
        vertex_neighbors_in.push_back(neighbors);
    }
}

void Security::kiso(int min_L1, int max_L1, int maxPsize, int tresh, bool baseline) {
    // Initialization
    maxPAGsize = 0;
    edge_neighbors.clear();
    vertex_neighbors_in.clear();
    vertex_neighbors_out.clear();
    top_tier_vertices.clear();
    top_tier_edges.clear();
    pags.clear();
    colors.clear();
    H_v_dummy = 0;
    H_e_dummy = 0;
    G_v_lifted = 0;
    G_e_lifted = 0;
    start = false;
    
    clock_t tic = clock();
    maxPAGsize = maxPsize;
    
    int treshold = tresh == 0 ? 0 : min_L1/tresh;
    cout<<"treshold: "<<treshold<<endl;
    int G_ecount = igraph_ecount(G);
    int G_vcount = igraph_vcount(G);
    
    get_vertex_neighbors();
    
    while (igraph_vcount(G) != 0) {
        while(1) {
            start = false;
            vector<PAG> tmp;
            pags.clear();
            pags.swap(tmp);
            edge_neighbors.clear();
            colors.clear();
            cout<<"PAG: "<<maxPAGsize<<endl;
            cout<<"G: "<<igraph_vcount(G)<<endl;
            cout<<"H: "<<igraph_vcount(H)<<endl;
            
            if (maxPAGsize > 1)
                get_edge_neighbors();
            
            //        // debug
            //        cout<<setfill('-')<<setw(100)<<"neighbors of edges"<<setfill('-')<<setw(99)<<"-"<<endl;
            //
            //        for (int i = 0; i < edge_neighbors.size(); i++) {
            //            cout<<i<<"'s neighbors:";
            //            set<int>::iterator it;
            //            for (it = edge_neighbors[i].begin(); it != edge_neighbors[i].end(); it++)
            //                cout<<" "<<*it;
            //            cout<<endl;
            //        }
            //        //--
            
            find_subgraphs();
            
            if (pags.size() == 0 && maxPAGsize > 0)
                break;
            
            if (maxPAGsize == 0) {
                //            cout<<"color size: "<<colors.size()<<endl;
                map<int, vector<int> >::iterator it;
                for (it = colors.begin(); it != colors.end(); it++) {
                    vector<int> temp = it->second;
                    
                    //int multiple = temp.size()%min_L1;
                    //int div = floor(temp.size()/min_L1);
                    //                cout<<"temp size: "<<temp.size()<<endl;
                    //                cout<<"tresh: "<<treshold<<endl;
                    int loop = temp.size()-1;
                    //int counter = 0;
                    if (temp.size() >= min_L1) {
                        for (int i = loop; i >= 0; i--) {
                            //                    if (multiple != 0 && counter == div*min_L1)
                            //                        break;
                            
                            // add v to H
                            igraph_add_vertices(H, 1, 0);
                            int vid = igraph_vcount(H) - 1;
                            SETVAN(H, "colour", vid, VAN(G, "colour", temp[i]));
                            SETVAN(H, "ID", vid, VAN(G, "ID", temp[i]));
                            
                            // delete v from G
                            //                    igraph_vs_t id;
                            //                    igraph_vs_1(&id, temp[i]);
                            //                    igraph_delete_vertices(G,id);
                            SETVAN(G, "Removed", temp[i], Removed);
                            
                            SETVAS(F, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                            SETVAS(R, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                            // delete v from vector
                            temp.erase(temp.begin()+i);
                            
                            //counter++;
                        }
                    } else {
                        //                if (multiple != 0) {
                        if (temp.size() >= treshold || baseline) {
                            // add
                            for (int i = min_L1-1; i >= 0; i--) {
                                // add v to H
                                igraph_add_vertices(H, 1, 0);
                                
                                int vid = igraph_vcount(H) - 1;
                                if (i < temp.size()) {
                                    SETVAN(H, "colour", vid, VAN(G, "colour", temp[i]));
                                    SETVAN(H, "ID", vid, VAN(G, "ID", temp[i]));
                                    
                                    // delete v from G
                                    //                                igraph_vs_t id;
                                    //                                igraph_vs_1(&id, temp[i]);
                                    //                                igraph_delete_vertices(G,id);
                                    SETVAN(G, "Removed", temp[i], Removed);
                                    
                                    SETVAS(F, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                                    SETVAS(R, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                                    // delete v from vector
                                    temp.erase(temp.begin()+i);
                                } else {
                                    H_v_dummy++;
                                    
                                    int tmp = it->first;
                                    SETVAN(H, "colour", vid, tmp);
                                    SETVAN(H, "ID", vid, vid);
                                    
                                    igraph_add_vertices(F, 1, 0);
                                    SETVAS(F, "Tier", igraph_vcount(F) - 1, "Bottom");
                                    SETVAN(F, "Dummy", igraph_vcount(F) - 1, kDummy);
                                }
                            }
                        } else {
                            loop = temp.size()-1;
                            for (int i = loop; i >= 0; i--) {
                                G_v_lifted++;
                                
                                set<int>::iterator has = top_tier_vertices.find(VAN(G, "ID",temp[i]));
                                if (has == top_tier_vertices.end()) // not in set
                                    top_tier_vertices.insert(VAN(G, "ID", temp[i]));
                                
                                // delete v from G
                                //                            igraph_vs_t vid;
                                //                            igraph_vs_1(&vid, temp[i]);
                                //                            igraph_delete_vertices(G,vid);
                                SETVAN(G, "Removed", temp[i], Removed);
                                SETVAS(F, "Tier", VAN(G, "ID", temp[i]), "Top");
                                SETVAS(R, "Tier", VAN(G, "ID", temp[i]), "Top");
                                // delete v from vector
                                temp.erase(temp.begin()+i);
                            }
                        }
                    }
                }
                
                int vcount = igraph_vcount(G) - 1;
                for (int i = vcount; i >= 0; i--) {
                    if (VAN(G, "Removed", i) == Removed) {
                        igraph_vs_t vid;
                        igraph_vs_1(&vid, i);
                        igraph_delete_vertices(G,vid);
                    }
                }
                
                break;
            }
            
            // find the VD embeddings of every PAG
            int max_degree = 0;
            int max_count = 0;
            int first_pag = -1;
            vector<vector<int> > VM;
            
            for (int i = 0; i < min_L1; i++) {
                vector<int> temp;
                VM.push_back(temp);
            }
            
            //            do {
            //                max_degree = 0;
            //                max_count = 0;
            //                first_pag = -1;
            
            VD_embeddings(&max_degree, &max_count, &first_pag, min_L1);
            
            cout<<endl<<setfill('-')<<setw(100)<<"Add embeddings to H"<<setfill('-')<<setw(99)<<"-"<<endl;
            cout<<"first: "<<first_pag<<endl;
            
            //                if (first_pag == -1)
            //                    continue;
            if (first_pag != -1) {
                if (pags[first_pag].vd_embeddings.vd_embeddings.size() >= min_L1) {
                    pags[first_pag].processed = true;
                    //int multiple = pags[first_pag].vd_embeddings.vd_embeddings.size()%min_L1;
                    map<int, set<int> >::iterator itr;
                    int counter = 0;
                    // for every vd-embedding
                    for (itr = pags[first_pag].vd_embeddings.vd_embeddings.begin(); itr != pags[first_pag].vd_embeddings.vd_embeddings.end(); itr++) {
                        // if we have a multiple of k vd-embeddings, go through all of them
                        //                    if (multiple == 0) {
                        //                        if (counter == pags[first_pag].vd_embeddings.vd_embeddings.size())
                        //                            break;
                        //                    } else {
                        //                        // if we have a non multiple, we take min_L1 embeddings. If the non multiple is bigger than x*L1_min then we take x*L1_min embeddings
                        //                        int div = floor(pags[first_pag].vd_embeddings.vd_embeddings.size()/min_L1);
                        //                        if (counter == div*min_L1)
                        //                            break;
                        //                    }
                        
                        cout<<"embedding added to H: "<<itr->first<<endl;
                        
                        map<int,int> mapGH;
                        set<int> dummy;
                        int dum;
                        set<int> edges = itr->second;
                        
                        // add embedding to H
                        create_graph(H, edges,  mapGH, dummy, &dum, false, false);
                        counter++;
                        
                        /*****************************************************************
                         This works because the mapping saves the corresponding vertex in
                         an embedding for a pag in ascending order (0->end) do when going
                         through the vertices of the pag in that order we garentee that the
                         mapping is valid. Even if the pag is not in the vd-embeddings, if
                         we get the corresponding vertices for every vd-embed by taking
                         the mapping done between it and the pag, for every vd-embed, then
                         we garentee that every vertex in any embed that maps to vertex i
                         in the pag will map to the vertex of any vd_embed that also maps
                         to vertex i in the pag
                         ****************************************************************/
                        
                        // if it's an embedding of the pag, then the isomorphic test generated a mapping
                        if (itr->first != pags[first_pag].embeddings.size()-1)
                            // go through that mapping and get the id of the vertex in H and insert it in the corresponding column in VM
                            for (int i = 0; i < igraph_vector_size(pags[first_pag].embeddings[itr->first].map); i++) {
                                map<int,int>::iterator got = mapGH.find(igraph_vector_e(pags[first_pag].embeddings[itr->first].map, i));
                                VM[counter%min_L1].push_back(got->second);
                            }
                        else { // if it's the pag itself, no mapping was created other than the one I did
                            map<int,int>::iterator it;
                            // go through that mapping and get the id of the vertex in H and insert in the corresponding column in VM
                            for (it = pags[first_pag].mapPAGG.begin(); it != pags[first_pag].mapPAGG.end(); it++) {
                                map<int,int>::iterator got = mapGH.find(VAN(G,"ID",it->second));
                                VM[counter%min_L1].push_back(got->second);
                            }
                        }
                        
                        // debug
                        if (itr->first != pags[first_pag].embeddings.size()-1) {
                            cout<<"G H"<<endl;
                            for (int i = 0; i < igraph_vector_size(pags[first_pag].embeddings[itr->first].map); i++) {
                                map<int,int>::iterator got = mapGH.find(igraph_vector_e(pags[first_pag].embeddings[itr->first].map, i));
                                cout<<got->first<<" "<<got->second<<endl;
                            }
                        } else {
                            cout<<"G H pag"<<endl;
                            map<int,int>::iterator it;
                            //cout<<pags[first_pag].mapPAGG.size();
                            for (it = pags[first_pag].mapPAGG.begin(); it != pags[first_pag].mapPAGG.end(); it++) {
                                map<int,int>::iterator got = mapGH.find(VAN(G,"ID",it->second));
                                cout<<got->first<<" "<<got->second<<" "<<it->first<<endl;
                            }
                        }
                        //--
                    }
                    
                    // Update PAGs
                    //update_pags();
                }
            }
            //            } while (first_pag != -1);
            
            if (!baseline) {
                cout<<endl<<setfill('-')<<setw(100)<<"Anonimyze"<<setfill('-')<<setw(99)<<"-"<<endl;
                
                while (!pags.empty()) {
                    start = false;
                    map<int,vector<int> > vd_embeddings;
                    // find the pag that has the most vd-embeddings left
                    for (int i = 0; i < pags.size(); i++) {
                        int vd_size = pags[i].vd_embeddings.vd_embeddings.size();
                        
                        map<int,vector<int> >::iterator got = vd_embeddings.find(vd_size);
                        if (got == vd_embeddings.end()) { // not in set
                            vector<int> temp;
                            temp.push_back(i);
                            vd_embeddings.insert(pair<int,vector<int> >(vd_size, temp));
                        } else vd_embeddings[vd_size].push_back(i);
                    }
                    
                    map<int,vector<int> >::iterator got = vd_embeddings.end();
                    got--;
                    
                    int pag = got->second[got->second.size() - 1];
                    got->second.erase(got->second.begin() + got->second.size() - 1);
                    if (got->first >= treshold) {
                        cout<<endl<<setfill('-')<<setw(100)<<"Add embeddings to H"<<setfill('-')<<setw(99)<<"-"<<endl;
                        cout<<"pag #"<<pag<<endl;
                        map<int,set<int> >::iterator it;
                        it = pags[pag].vd_embeddings.vd_embeddings.begin();
                        
                        for (int i = 0; i < min_L1; i++) {
                            if (it == pags[pag].vd_embeddings.vd_embeddings.end()) {
                                it = pags[pag].vd_embeddings.vd_embeddings.begin();
                                start = true;
                            }
                            
                            cout<<"embedding added to H: "<<it->first<<endl;
                            set<int>::iterator itr;
                            for (itr = it->second.begin(); itr != it->second.end(); itr++)
                                cout<<*itr<<" ";
                            cout<<endl;
                            map<int,int> mapGH;
                            set<int> dummy;
                            int dum;
                            set<int> edges = it->second;
                            // add embedding to H
                            create_graph(H, edges, mapGH, dummy, &dum, false, false);
                            cout<<"H vcount = "<<igraph_vcount(H)<<endl<<endl;
                            
                            it++;
                        }
                    } else {
                        // lift those embeddings
                        cout<<endl<<setfill('-')<<setw(100)<<"Remove embeddings from H"<<setfill('-')<<setw(99)<<"-"<<endl;
                        cout<<"pag #"<<pag<<endl;
                        map<int,set<int> >::iterator it;
                        for (it = pags[pag].vd_embeddings.vd_embeddings.begin(); it != pags[pag].vd_embeddings.vd_embeddings.end(); it++) {
                            cout<<"embedding removed from H: "<<it->first<<endl;
                            set<int> removed;
                            set<int>::iterator itr;
                            for (itr = it->second.begin(); itr != it->second.end(); itr++) {
                                int from, to;
                                
                                igraph_edge(G,*itr,&from,&to);
                                
                                set<int>::iterator got = removed.find(from);
                                if (got == removed.end()) { // not in set
                                    G_v_lifted++;
                                    set<int>::iterator has = top_tier_vertices.find(VAN(G, "ID",from));
                                    if (has == top_tier_vertices.end()) // not in set
                                        top_tier_vertices.insert(VAN(G, "ID",from));
                                    removed.insert(from);
                                }
                                SETVAS(F, "Tier", VAN(G, "ID",from), "Top");
                                SETVAS(R, "Tier", VAN(G, "ID",from), "Top");
                                
                                got = removed.find(to);
                                if (got == removed.end()) { // not in set
                                    G_v_lifted++;
                                    set<int>::iterator has = top_tier_vertices.find(VAN(G, "ID",to));
                                    if (has == top_tier_vertices.end()) // not in set
                                        top_tier_vertices.insert(VAN(G, "ID",to));
                                    removed.insert(to);
                                }
                                SETVAS(F, "Tier", VAN(G, "ID", to), "Top");
                                SETVAS(R, "Tier", VAN(G, "ID", to), "Top");
                                
                                G_e_lifted++;
                                map<int, set<int> >::iterator in = top_tier_edges.find(VAN(G, "ID",from));
                                if (in == top_tier_edges.end()) { // not in set
                                    set<int> tmp;
                                    tmp.insert(VAN(G, "ID",to));
                                    top_tier_edges.insert(pair<int, set<int> >(VAN(G, "ID",from), tmp));
                                } else in->second.insert(VAN(G, "ID",to));
                                
                                int eid;
                                igraph_get_eid(F, &eid, VAN(G, "ID",from), VAN(G, "ID",to), IGRAPH_DIRECTED, 0);
                                SETEAS(F, "Tier", eid, "Top");
                                SETEAS(R, "Tier", eid, "Top");
                                
                                SETVAN(G, "Removed", from, Removed);
                                SETVAN(G, "Removed", to, Removed);
                                SETEAN(G, "Removed", *itr, Removed);
                            }
                        }
                    }
                    
                    // Upadate PAGs
                    pags.erase(pags.begin() + pag);
//                    update_pags();
//                    
//                    max_degree = 0;
//                    max_count = 0;
//                    first_pag = -1;
//                    
//                    VD_embeddings(&max_degree, &max_count, &first_pag, min_L1);
                }
            }
            
            // debug
            cout<<endl;
            cout<<"VM:"<<endl;
            for (int i = 0; i < VM.size(); i++)
                cout<<i<<" ";
            cout<<"columns"<<endl;
            
            for (int i = 0; i < VM[0].size(); i++) {
                for(int j = 0; j < VM.size(); j++)
                    cout<<VM[j][i]<<" ";
                cout<<endl;
            }
            //--
            
            cout<<endl;
            
            int vcount = igraph_vcount(G) - 1;
            for (int i = vcount; i >= 0; i--) {
                if (VAN(G, "Removed", i) == Removed) {
                    igraph_vs_t vid;
                    igraph_vs_1(&vid, i);
                    igraph_delete_vertices(G,vid);
                }
            }
            
            int ecount = igraph_ecount(G) - 1;
            for (int i = ecount; i >= 0; i--) {
                if (EAN(G, "Removed", i) == Removed) {
                    igraph_es_t eid;
                    igraph_es_1(&eid, i);
                    igraph_delete_edges(G,eid);
                }
            }
        }
        
        if (maxPAGsize == 0) {
            int vcount = igraph_vcount(G) - 1;
            for (int i = vcount; i >= 0; i--) {
                if (VAN(G, "Removed", i) == Removed) {
                    igraph_vs_t vid;
                    igraph_vs_1(&vid, i);
                    igraph_delete_vertices(G,vid);
                }
            }
            
            int ecount = igraph_ecount(G) - 1;
            for (int i = ecount; i >= 0; i--) {
                if (EAN(G, "Removed", i) == Removed) {
                    igraph_es_t eid;
                    igraph_es_1(&eid, i);
                    igraph_delete_edges(G,eid);
                }
            }
        }
        
        maxPAGsize--;
        
        cout<<endl;
        cout<<"G ecount - H ecount = "<<G_ecount - igraph_ecount(H)<<endl;
        cout<<"H vcount = "<<igraph_vcount(H)<<endl;
        cout<<"G vcount = "<<igraph_vcount(G)<<endl;
        cout<<endl;
    }
    
    int oneBond = 0;
    set<int>::iterator it;
    for (it = top_tier_vertices.begin(); it != top_tier_vertices.end(); it++) {
        set<int>::iterator iter;
        for (iter = vertex_neighbors_out[*it].begin(); iter != vertex_neighbors_out[*it].end(); iter++) {
            set<int>::iterator got = top_tier_vertices.find(*iter);
            if (got == top_tier_vertices.end()) { // not in set
                oneBond++;
                // edge going from to to is crossing
                // edge already exists
                int eid = -1;
                igraph_get_eid(F, &eid, *it, *iter, IGRAPH_DIRECTED, 1);
                if (eid != -1) {
                    SETEAS(F, "Tier", eid, "Crossing");
                    SETEAS(R, "Tier", eid, "Crossing");
                }
            }
            else {
                set<int>::iterator has = top_tier_edges[*it].find(*iter);
                if (has == top_tier_edges[*it].end()) { // not in set
                    G_e_lifted++;
                    top_tier_edges[*it].insert(*iter);
                    // edge is in top tier
                    // edge already exists
                    int eid = -1;
                    igraph_get_eid(F, &eid, *it, *iter, IGRAPH_DIRECTED, 1);
                    if (eid != -1) {
                        SETEAS(F, "Tier", eid, "Top");
                        SETEAS(R, "Tier", eid, "Top");
                    }
                }
            }
        }
        
        for (iter = vertex_neighbors_in[*it].begin(); iter != vertex_neighbors_in[*it].end(); iter++) {
            set<int>::iterator got = top_tier_vertices.find(*iter);
            if (got == top_tier_vertices.end()) {// not in set
                oneBond++;
                // edge going to to from is crossing
                // edge already exists
                int eid = -1;
                igraph_get_eid(F, &eid, *iter, *it, IGRAPH_DIRECTED, 1);
                if (eid != -1) {
                    SETEAS(F, "Tier", eid, "Crossing");
                    SETEAS(R, "Tier", eid, "Crossing");
                }
            }
        }
    }
    
    int lifted_edges = G_ecount - (igraph_ecount(H) - H_e_dummy) - G_e_lifted;
    set_topV(G_v_lifted);
    set_topE(G_e_lifted);
    set_bottomV(igraph_vcount(H));
    set_bottomE(igraph_ecount(H));
    int twoBond = lifted_edges-oneBond;
    set_twoBondLiftedEdge(twoBond);
    set_oneBondLiftedEdge(oneBond);
    set_bonds(oneBond + 2*twoBond);
    
    cout<<endl<<setfill('-')<<setw(100)<<"Report"<<setfill('-')<<setw(99)<<"-"<<endl;
    cout<<"Security lvl: "<<min_L1<<endl;
    cout<<"G initial vertex count: "<<G_vcount<<endl;
    cout<<"G initial edge count: "<<G_ecount<<endl;
    cout<<"G final vertex count: "<<G_v_lifted<<endl;
    cout<<"G final edge count: "<<G_e_lifted<<endl;
    cout<<endl;
    cout<<"H final vertex count, with dummies: "<<igraph_vcount(H)<<endl;
    cout<<"H final edge count, with dummies: "<<igraph_ecount(H)<<endl;
    cout<<"H final vertex count, without dummies: "<<igraph_vcount(H) - H_v_dummy<<endl;
    cout<<"H final edge count, without dummies: "<<igraph_ecount(H) - H_e_dummy<<endl;
    cout<<endl;
    cout<<"Lifted edges: "<<lifted_edges<<endl;
    cout<<endl;
    
    //    // debug
    //
    //    for (int i = 0; i < pags.size(); i++ ){
    //        cout<<endl<<setw(100)<<setfill('-')<<"updated pags, embeddings, VD embeddings"<<setfill('-')<<setw(99)<<"-"<<endl;
    //        cout<<"pag #"<<i<<": ";
    //        set<int>::iterator iter;
    //        for (iter = pags[i].pag.begin(); iter != pags[i].pag.end(); iter++)
    //            cout<<*iter<<" ";
    //        cout<<endl;
    //        cout<<"map: ";
    //        map<int,int>::iterator itr;
    //        for (itr = pags[i].mapPAGG.begin(); itr != pags[i].mapPAGG.end(); itr++)
    //            cout<<itr->second<<" ";
    //        cout<<endl;
    //
    //        for (int j = 0; j < pags[i].embeddings.size(); j++) {
    //            cout<<"embeding #"<<j<<": ";
    //            set<int>::iterator iter2;
    //            for (iter2 = pags[i].embeddings[j].edges.begin(); iter2 != pags[i].embeddings[j].edges.end(); iter2++)
    //                cout<<*iter2<<" ";
    //            cout<<endl;
    //            cout<<"connected embeddings: ";
    //            set<int>::iterator it;
    //            for (it = pags[i].embeddings[j].connected_embeddings.begin(); it != pags[i].embeddings[j].connected_embeddings.end(); it++)
    //                cout<<*it<<" ";
    //            cout<<endl;
    //        }
    //
    //        map<int, set<int> >::iterator iter2;
    //        cout<<"VD-embeddings: ";
    //        for (iter2 = pags[i].vd_embeddings.vd_embeddings.begin(); iter2 != pags[i].vd_embeddings.vd_embeddings.end(); iter2++)
    //            cout<<iter2->first<<" ";
    //        cout<<endl<<"max deg: "<<pags[i].vd_embeddings.max_degree<<" max count: "<<pags[i].vd_embeddings.max_count<<endl;
    //
    //        for (iter2 = pags[i].vd_embeddings.vd_embeddings.begin(); iter2 != pags[i].vd_embeddings.vd_embeddings.end(); iter2++) {
    //            cout<<"embedding "<<iter2->first<<": ";
    //            set<int> temp = iter2->second;
    //            set<int>::iterator it;
    //            for (it = temp.begin(); it != temp.end(); it++)
    //                cout<<*it<<" ";
    //            cout<<"deg: "<<pags[i].embeddings[iter2->first].max_degree;
    //            cout<<endl;
    //        }
    //    }
    //
    //    cout<<setw(100)<<setfill('-')<<"initial subgraphs"<<setfill('-')<<setw(99)<<"-"<<endl;
    //    cout<<endl;
    //    set<int>::iterator it;
    //    for (it = not_permitted.begin(); it != not_permitted.end(); it++)
    //        cout<<*it<<" ";
    //    cout<<endl;
    //
    //    cout<<setw(100)<<setfill('-')<<"Pags and their embeddings"<<setfill('-')<<setw(99)<<"-"<<endl;
    //    for (int i = 0; i < pags.size(); i++) {
    //        cout<<endl;
    //        set<int>::iterator iter;
    //        for (iter = pags[i].pag.begin(); iter != pags[i].pag.end(); iter++)
    //            cout<<*iter<<" ";
    //        cout<<endl<<"   ";
    //        for (int j = 0; j < pags[i].embeddings.size(); j++) {
    //            set<int>::iterator iter2;
    //            for (iter2 = pags[i].embeddings[j].begin(); iter2 != pags[i].embeddings[j].end(); iter2++)
    //                cout<<*iter2<<" ";
    //            cout<<endl<<"   ";
    //        }
    //    }
    //    //--
    
    clock_t toc = clock();
    
    cout<<"Took: "<<(double) (toc-tic)/CLOCKS_PER_SEC<<endl;
    
    write_to_file(lifted_edges, G_vcount, G_ecount, G_v_lifted, G_e_lifted, igraph_vcount(H), igraph_ecount(H), igraph_vcount(H) - H_v_dummy, igraph_ecount(H) - H_e_dummy, (double) (toc-tic)/CLOCKS_PER_SEC, oneBond, twoBond, igraph_vcount(F), igraph_ecount(F));

    // debug
    
    cout<<"oneBond: "<<oneBond<<endl;
    cout<<"twoBond: "<<twoBond<<endl;
    cout<<"bonds: "<<oneBond + 2*twoBond<<endl;
    
    for (it = top_tier_vertices.begin(); it != top_tier_vertices.end(); it++)
        cout<<*it<<" ";
    cout<<endl;
    
    map<int, set<int> >::iterator itmap;
    for (itmap = top_tier_edges.begin(); itmap != top_tier_edges.end(); itmap++) {
        cout<<"edge: "<<itmap->first<<": ";
        set<int> tmp = itmap->second;
        set<int>::iterator itset;
        for (itset = tmp.begin(); itset != tmp.end(); itset++)
            cout<<*itset<<" ";
        cout<<endl;
    }

//    for (int i = 0; i < vertex_neighbors_out.size(); i++) {
//        set<int>::iterator it;
//        cout<<"vertex: "<<i<<endl;
//        for (it = vertex_neighbors_out[i].begin(); it != vertex_neighbors_out[i].end(); it++)
//            cout<<*it<<" ";
//        for (it = vertex_neighbors_in[i].begin(); it != vertex_neighbors_in[i].end(); it++)
//            cout<<*it<<" ";
//        cout<<endl;
//    }
    //--
    cout<<"top tier: "<<top_tier_vertices.size()<<endl;
}
/*************************************************************************//**
                                                                            * @brief
                                                                            * @version						v0.01b
                                                                            ****************************************************************************/
void Security::S1_greedy (bool save_state, int threads, int min_L1, int max_L1, bool quite) { // Added by Karl (int remove_vertex_max)
    
    /******************************
     * Setup
     ******************************/
    
    // Added by Karl
    int count = 0;
    ////////////////
    
    if (max_L1 == -1) max_L1 = G->max_L1();
    cout<<igraph_ecount(H)<<endl;
    if (igraph_ecount(H) == 0) add_free_edges(max_L1);
    
    vector<L1_Edge*> edges;
    vector<L1_Edge*> edge_list;
    
    for (unsigned int eid = 0; eid < igraph_ecount(G); eid++) {
        if (!H->test_edge(G->get_edge(eid)) && !(EAN(G,"Lifted",eid) == Lifted)) {// edges already in H will not be considered in the list of candidates as well as edges that are connected to lifted vertices
            int from, to;
            igraph_edge(G, eid, &from, &to);
            edges.push_back( new L1_Edge(eid, Edge(from,to), max_L1) );
            edge_list.push_back( edges.back() );
        }
    }
    
#ifndef NRAND
    random_shuffle(edge_list.begin(), edge_list.end());
#endif
    
#ifdef VF2
    bool vf2_flippy(false);
#endif
    vector<L1_Thread*> busy_threads, free_threads;
    for (unsigned int i=0; i<threads; i++) {
        free_threads.push_back( new L1_Thread() );
#ifdef VF2
        if (vf2_flippy)
            free_threads.back()->vf2 = true;
        vf2_flippy = !vf2_flippy;
#endif
    }
    
    /******************************
     * Add edges until L1 == min_L1
     ******************************/
    
    //	Added by Mohamed
    int prev_L1 = max_L1;
    
    ofstream myfile;
    myfile.open ("tradeoff.dat");
    
    while ((max_L1 >= min_L1 || max_L1 == -2) && edge_list.size() > 0) { // Added by Karl: || max_L1 == -2 to account for inf lvl graphs
        
        // Added by Karl
        if (save_state) {
            optimalSolution.L1 = -1;
            optimalSolution.liftedEdges = -1;
            optimalSolution.liftedVertices = 0;
            cout<<setfill('/')<<setw(250)<<"in"<<endl;
        }
        ////////////////
        
        
        cout << "  E(" << edge_list.size() << ") ";
        cout.flush();
        
        /******************************
         * Presort eids int max(L1)
         ******************************/
        L1_Edge *best_edge = edge_list[0];
        sort    (edge_list.begin(), edge_list.end(), l1_edge_lt);
        reverse (edge_list.begin(), edge_list.end());
        int sat_index(0), vf2_index(0);
        
#ifdef DEBUG
        cout << endl << "edge_list.sort(" << edge_list.size() << ") : ";
        for (unsigned int i = 0; i < edge_list.size(); i++)
            cout << "(" << edge_list[i]->eid << ", " << edge_list[i]->L1_prev << "," << edge_list[i]->L1_sat << "," << edge_list[i]->L1_vf2 << ") ";
        cout << endl;
#endif
        
        while (sat_index < edge_list.size()) {
            
            /******************************
             * Load Threads (create sub-processes)
             ******************************/
            if (free_threads.size() > 0) {
                busy_threads.push_back(free_threads.back());
                free_threads.pop_back();
                
                if (busy_threads.back()->vf2)
                    if (vf2_index >= edge_list.size())
                        busy_threads.back()->vf2 = false;
                
                if (busy_threads.back()->vf2) {
                    busy_threads.back()->test_edge = edge_list[vf2_index++];
                } else {
                    busy_threads.back()->test_edge = edge_list[sat_index++];
                }
                
                busy_threads.back()->open(true,false);
                
                /******************************
                 * Child
                 ******************************/
                if ( busy_threads.back()->child() ) {              // Child (PID == 0)
                    
                    L1_Edge *test_edge = busy_threads.back()->test_edge;
                    
#ifdef MEASURE_TIME_S1
                    clock_t tic = clock();
#endif
                    
                    add_edge(test_edge->eid);
                    
#ifdef DEBUG
                    cout << endl;
                    cout << "Child(" << getpid() << ") : before clean "<< solutions.size() << endl;
#endif
                    clean_solutions();
                    
#ifdef DEBUG
                    cout << "Child(" << getpid() << ") :  after clean "<< solutions.size() << endl;
#endif
                    int old_size = solutions.size();
#ifdef MEASURE_TIME_S1
                    clock_t toc = clock();
#ifdef DEBUG
                    cout << "Child(" << getpid() << ") :   time clean ";
                    << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
#endif
#endif
                    
                    if (busy_threads.back()->vf2) { // Can we improve the best case lvl so far by adding an edge to this graph? If the lvl is already lower or equal then noway, otherwise we might.
                        if (test_edge->L1_prev < min_L1)
                            test_edge->L1_vf2 = 1;
                        if (test_edge->L1_prev <= best_edge->L1())
                            test_edge->L1_vf2 = test_edge->L1_prev;
                        else
                            test_edge->L1_vf2 = L1(true, true);
                    } else {
                        if (test_edge->L1_prev < min_L1 && test_edge->L1_prev > -2) // Added by Karl: && test_edge->L1_prev > -2
                            test_edge->L1_sat = 1;
                        if (test_edge->L1_prev <= best_edge->L1() && test_edge->L1_prev > -2) // Added by Karl: && test_edge->L1_prev > -2 // shouldn't we add an else otherwise everytime the first if is met, this one will and it will change the L1_sat
                            test_edge->L1_sat = test_edge->L1_prev;
                        else
                            test_edge->L1_sat = L1();
                    }
                    
                    string output;
                    if (busy_threads.back()->vf2) {
                        output = "S1_greedy.vf2 ("  + G->get_name() + ").child(" + num2str(getpid()) + ")";
                        output = report(output, G, H, test_edge->L1_vf2, solutions.size(), test_edge->edge);
                    } else {
                        output = "S1_greedy.sat ("  + G->get_name() + ").child(" + num2str(getpid()) + ")";
                        output = report(output, G, H, test_edge->L1_sat, solutions.size(), test_edge->edge);
                    }
                    
#ifdef DEBUG
                    cout << output;
#endif
                    
#ifdef USE_SOLNS
                    vector<igraph_vector_t*>::iterator it_begin = solutions.begin();
                    for (unsigned int i = 0; i < old_size; i++) {
                        it_begin++;
                        if (it_begin == solutions.end()) break;
                    }
                    for (vector<igraph_vector_t*>::iterator it = it_begin; it != solutions.end(); ++it)
                        output += report(*it);
#endif
                    
                    busy_threads.back()->write(output);
                    
                    busy_threads.back()->close(false, true);
                    
#ifdef MEASURE_TIME_S1
                    toc = clock();
                    cout << endl << "Child(" << getpid() << ") : Total Time: ";
                    cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
#endif
                    
                    _exit(0);
                }
            }
            
            /******************************
             * Unload Threads (Parent)
             ******************************/
            do {
                
                for (unsigned int j=0; j<busy_threads.size(); j++) {
                    string response = busy_threads[j]->read();
                    // do something with response
                    if (response.size() > 0) {
                        
#ifdef MEASURE_TIME_S1
                        clock_t tic = clock();
#endif
                        
                        L1_Edge *test_edge = busy_threads[j]->test_edge;
                        int L0;
                        
                        stringstream r_stream(response);
                        string line;
                        while( getline(r_stream, line) ) {
                            
                            Edge tmp;
                            if (busy_threads[j]->vf2) {
                                parse(line, G, test_edge->L1_vf2, L0, tmp);
                                test_edge->L1_vf2 = min(max_L1, test_edge->L1_vf2);
                            } else {
                                parse(line, G, test_edge->L1_sat, L0, tmp);
                                test_edge->L1_sat = min(max_L1, test_edge->L1_sat);
                            }
                            
#ifdef USE_SOLNS
                            // recive solutions
                            igraph_vector_t *map21 = new igraph_vector_t();
                            igraph_vector_init (map21, igraph_vcount(H));
                            if ( parse(line, map21) ) {
                                solutions.push_back(map21);
                            } else {
                                igraph_vector_destroy (map21);
                                delete map21;
                            }
#endif
                            
                        }
                        ////////////////
                        if ((test_edge->L1() > best_edge->L1()) || (test_edge->L1() != best_edge->L1() && test_edge->L1() == -2)) { //Added by Karl: || (test_edge->L1() != best_edge->L1() && test_edge->L1() == -2. We want them to be different when == -2 because if it's the same it means that both are inf lvl so no need to update, we can use the old edge.
                            best_edge = test_edge;
                        }
                        
                        if (busy_threads[j]->vf2)
                            cout << 'v';
                        else
                            cout << 's';
                        cout.flush();
                        
#ifdef DEBUG
                        string output;
                        if (busy_threads[j]->vf2) {
                            output = "S1_greedy.vf2 ("  + G->get_name() + ").parent(" + num2str(busy_threads[j]->pid) + ")";
                            output = report(output, G, H, test_edge->L1_vf2, solutions.size(), test_edge->edge);
                        } else {
                            output = "S1_greedy.sat ("  + G->get_name() + ").parent(" + num2str(busy_threads[j]->pid) + ")";
                            output = report(output, G, H, test_edge->L1_sat, solutions.size(), test_edge->edge);
                        }
                        cout << endl << output;
#endif
                        
                        free_threads.push_back(busy_threads[j]);
                        busy_threads.erase(busy_threads.begin()+j);
                        j = -1; // j++
                        
#ifdef MEASURE_TIME_S1
                        clock_t toc = clock();
                        cout << endl << "Parent: pipe Time: ";
                        cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
#endif
                    }
                }
                
            } while (free_threads.size() == 0);
        }
        
        // empty left over threads
        while (busy_threads.size() > 0) {
            if (busy_threads[0]->read().size() == 0 ) {
#ifdef DEBUG
                cout << "Parent: Kill left over thread(" << busy_threads[0]->pid << ")" << endl;
#endif
                busy_threads[0]->close(true, false);
                busy_threads[0]->kill();
            }
            free_threads.push_back(busy_threads[0]);
            busy_threads.erase(busy_threads.begin());
        }
        
        if (best_edge->L1() != prev_L1)
        {
            for (int m = prev_L1; m > best_edge->L1(); m--)
                myfile << igraph_ecount(H) - 1 << " " << m << endl;
            prev_L1 = best_edge->L1();
        }
        
        if (best_edge->L1() < min_L1 && best_edge->L1() != -2) // Added by Karl: && best_edge->L1() != -2
            break;
        
        // add to graph, remove from list, reset edges
        add_edge(best_edge->eid);
        
        // Added by Karl
        if (!save_state) {
            int lifted_edges = igraph_ecount(G) - igraph_ecount(H) - notLifted;
            
            if (optimalSolution.L1 != -1) {
                if (lifted_edges < optimalSolution.liftedEdges)
                    updateOptimalSolution(best_edge->L1(), lifted_edges, vcount);
                else if (lifted_edges == optimalSolution.liftedEdges) {
                    if (vcount < optimalSolution.liftedVertices)
                        updateOptimalSolution(best_edge->L1(), lifted_edges, vcount);
                    else if (vcount == optimalSolution.liftedVertices)
                        if (best_edge->L1() > optimalSolution.L1)
                            updateOptimalSolution(best_edge->L1(), lifted_edges, vcount);
                }
            } else updateOptimalSolution(best_edge->L1(), lifted_edges, vcount);
        }
        
        cout<<setfill('/')<<setw(250)<<optimalSolution.L1<<" "<<optimalSolution.liftedEdges<<" "<<optimalSolution.liftedVertices<<endl;
        
        if (save_state) {
            SETEAN(G, "Original", best_edge->eid, Original);
            
            set<int>::const_iterator got = LiftedVnE.edgeIDsSet.find(best_edge->eid);
            if (got == LiftedVnE.edgeIDsSet.end()) // not in set
                LiftedVnE.edgeIDsSet.insert(best_edge->eid);
        }
        ////////////////
        
        max_L1 = best_edge->L1();
        
        // Added by Karl
        maxL1 = max_L1;
        ////////////////
        
        int best_edge_index(-1);
        for (unsigned int i=0; i<edge_list.size(); i++) {
            if ( edge_list[i] == best_edge) best_edge_index = i;
            if ( edge_list[i]->L1() > 0 )   edge_list[i]->L1_prev = edge_list[i]->L1();
            
            edge_list[i]->L1_sat = -1;
            edge_list[i]->L1_vf2 = -1;
        }
        edge_list.erase(edge_list.begin()+best_edge_index);
        
#ifdef MEASURE_TIME_S1
        clock_t tic = clock();
#endif
#ifdef USE_SOLNS
        clean_solutions();
#endif
#ifdef MEASURE_TIME_S1
        clock_t toc = clock();
        cout << endl << "Master: clean_solution() Time: ";
        cout << (double) (toc-tic)/CLOCKS_PER_SEC << endl;
#endif
        
        string output;
        output = "S1_greedy ("  + G->get_name() + ")";
        output = report(output, G, H, best_edge->L1_prev, solutions.size(), best_edge->edge);
        cout << endl << output;
        
        // Added by Karl
        // To be done only for the first iteration not the ones used inside the lift vertex thing. Add a bool
        if (save_state) {
            cout<<setfill('/')<<setw(250)<<"target: "<<maxL1<<endl;
            
            LiftedVnE.vertexIDs.clear();
            LiftedVnE.lifted.clear();
            
            k2outfile<<setfill(' ')<<setw(6)<<maxL1<<setfill(' ')<<setw(15)<<igraph_ecount(G)-igraph_ecount(H)<<endl;
            
            int temp_maxL1 = maxL1;
            int temp_lifted = igraph_ecount(G)-igraph_ecount(H);
            
            cout<<setfill(':')<<setw(225)<<igraph_ecount(H)<<" Id: "<<*LiftedVnE.edgeIDsSet.begin()<<" set: "<<LiftedVnE.edgeIDsSet.size()<<endl;
            
            set<int>::iterator it3;
            for (it3 = LiftedVnE.edgeIDsSet.begin(); it3 != LiftedVnE.edgeIDsSet.end(); it3++) {
                cout<<"Id: "<<*it3<<endl;
            }
            
            for (int i = 0; i < igraph_ecount(H); i++) {
                int from, to;
                int eid;
                igraph_edge(H,i,&from,&to);
                igraph_get_eid(G, &eid, from, to, IGRAPH_DIRECTED, 1);
                cout<<"ID: "<<eid<<endl;
            }
            
            clean_solutions();
            vcount = 0;
            notLifted = 0;
            lift_vertex(maxL1, threads);
            cout<<setfill('.')<<setw(250)<<notLifted<<endl;
            cout<<setfill('/')<<setw(250)<<optimalSolution.L1<<" "<<optimalSolution.liftedEdges<<" "<<optimalSolution.liftedVertices<<endl;
            
            // Write to file
            file(WRITE);
            if (temp_lifted < optimalSolution.liftedEdges)
                k3outfile<<setfill(' ')<<setw(6)<<temp_maxL1<<setfill(' ')<<setw(15)<<temp_lifted<<setfill(' ')<<setw(15)<<"0"<<endl;
            else if (temp_lifted == optimalSolution.liftedEdges) {
                if (optimalSolution.liftedVertices > 0)
                    k3outfile<<setfill(' ')<<setw(6)<<temp_maxL1<<setfill(' ')<<setw(15)<<temp_lifted<<setfill(' ')<<setw(15)<<"0"<<endl;
                else if (optimalSolution.liftedVertices == 0) {
                    if (temp_maxL1 >= optimalSolution.L1)
                        k3outfile<<setfill(' ')<<setw(6)<<temp_maxL1<<setfill(' ')<<setw(15)<<temp_lifted<<setfill(' ')<<setw(15)<<"0"<<endl;
                    else
                        k3outfile<<setfill(' ')<<setw(6)<<optimalSolution.L1<<setfill(' ')<<setw(15)<<optimalSolution.liftedEdges<<setfill(' ')<<setw(15)<<optimalSolution.liftedVertices<<endl;
                }
            }
            else if (optimalSolution.liftedEdges < temp_lifted)
                k3outfile<<setfill(' ')<<setw(6)<<optimalSolution.L1<<setfill(' ')<<setw(15)<<optimalSolution.liftedEdges<<setfill(' ')<<setw(15)<<optimalSolution.liftedVertices<<endl;
            
            // Unlift all vertices to go back to "original" circuit
            for (int i = 0; i < LiftedVnE.vertexIDs.size(); i++)
                if (VAN(H,"Lifted",LiftedVnE.vertexIDs[i]) == Lifted) // Security check
                    SETVAN(H, "Lifted", LiftedVnE.vertexIDs[i], NotLifted);
            
            // Unlift all edges
            for (int i = 0; i < LiftedVnE.lifted.size(); i++)
                SETEAN(G, "Lifted", LiftedVnE.lifted[i], NotLifted);
            
            notLifted = 0;
            
            set<int> temp (LiftedVnE.edgeIDsSet);
            cout<<setfill(':')<<setw(225)<<igraph_ecount(H)<<" "<<LiftedVnE.edgeIDsSet.size()<<" "<<temp.size()<<endl;
            set<int>::iterator it2;
            for (it2 = temp.begin(); it2 != temp.end(); it2++) {
                cout<<"Id: "<<*it2<<endl;
            }
            // Bring back H to the way it was
            for (int i = 0; i < igraph_ecount(H); i++) {
                cout<<i<<endl;
                // Get ID in G
                int from, to;
                igraph_edge(H,i,&from,&to);
                int eid;
                igraph_get_eid(G, &eid, from, to, IGRAPH_DIRECTED, 1); // get id of the edge in H
                
                // In set?
                set<int>::const_iterator got = temp.find(eid);
                if (got == temp.end()) {// not in set
                    igraph_delete_edges(H, igraph_ess_1(i));
                    cout<<"not"<<endl;
                    i--;
                }
                else {
                    temp.erase(eid);
                    cout<<"in"<<endl;
                }
            }
            cout<<setfill(':')<<setw(225)<<igraph_ecount(H)<<" "<<LiftedVnE.edgeIDsSet.size()<<" "<<temp.size()<<endl;
            set<int>::iterator it;
            for (it = temp.begin(); it != temp.end(); it++) {
                cout<<"Id: "<<*it<<endl;
                add_edge(*it);
            }
            
            for (int i = 0; i < igraph_ecount(H); i++) {
                int from, to;
                int eid;
                igraph_edge(H,i,&from,&to);
                igraph_get_eid(G, &eid, from, to, IGRAPH_DIRECTED, 1);
                cout<<"ID: "<<eid<<endl;
            }
            
            cout<<setfill(':')<<setw(225)<<igraph_ecount(H)<<" "<<LiftedVnE.edgeIDsSet.size()<<endl;
            
            LiftedVnE.vertexIDs.clear();
            LiftedVnE.lifted.clear();
            
            clean_solutions();
        }
        ////////////////
    }
    
    for (unsigned int i=0; i<edges.size(); i++)
        delete edges[i];
    
    myfile.close();
}

// Added by Karl
void Security::L1_main (string outFileName, int _remove_vertices_max, int threads, int min_L1, int max_L1, bool quite) {
    maxL1 = -1;
    optimalSolution.L1 = -1;
    optimalSolution.liftedEdges = -1;
    optimalSolution.liftedVertices = 0;
    
    string outFile = "gnuplotOutput/" + outFileName;
    file(OPEN, outFile);
    
    remove_vertices_max = _remove_vertices_max;
    
    S1_greedy(true, threads, min_L1, max_L1);
    
    file(CLOSE);
}

void Security::lift_vertex(/*int max_L1*/) {
    
    int index = -1;
    int max_L1 = maxL1;
    
    for (int i = 0; i < igraph_vcount(H); i++) {
        vector<int> deleted;
        if (VAN(H,"Lifted",i) == NotLifted) {
            // remove mappings that don't work
            clean_solutions();
            
            // Lift the vertex
            SETVAN(H, "Lifted", i, Lifted);
            // remove the edges before
            for (int j = 0; j < igraph_ecount(G); j++) { // G not H because we want to delete the edges in H and when doing so the ids will get rearranged so we can get segmentation falt. Also, when we add back the edges we are adding them back from G so we need to know their id in G.
                if (H->test_edge(G->get_edge(j))) { // If the edge is in H
                    int from, to;
                    igraph_edge(G,j,&from,&to);
                    if (from == i || to == i) {
                        deleted.push_back(j);
                        int eid;
                        igraph_get_eid(H, &eid, from, to, IGRAPH_DIRECTED, 1); // get id of the edge in H
                        igraph_delete_edges(H, igraph_ess_1(eid));
                    }
                }
            }
            
            // Compute new L1
            int level = L1();
            cout<<"index: "<<i<<" level: "<<level<<" max: "<<max_L1<<endl;
            // save result if > than the max_L1 acheived so far
            if (level >= max_L1 || level == -2) {
                max_L1 = level;
                index = i;
            }
            // Unlift the vertex
            SETVAN(H, "Lifted", i, NotLifted);
            // add back removed edges
            for (int j = 0; j < deleted.size(); j++)
                add_edge(deleted[j]);
        }
    }
    
    cout<<"max: "<<max_L1<<endl;
    
    if (index >= 0) {
        SETVAN(H, "Lifted", index, Lifted);
        LiftedVnE.vertexIDs.push_back(index);
        
        //        igraph_es_t es;
        //        igraph_integer_t vid = index;
        //        igraph_es_adj(&es, vid, IGRAPH_ALL);
        //        igraph_vector_t was;
        //        igraph_vector_init(&was, 0);
        //        while (!igraph_es_end(G, &es)) {
        //            igraph_vector_push_back(&was, igraph_es_adj_vertex(G, &es));
        //            igraph_es_next(G, &es);
        //        }
        //igraph_es_adj(H, &es, &vid, IGRAPH_ALL);
        //        igraph_integer_t size;
        //        igraph_es_size(G, &es, &size);
        //        cout<<VECTOR(size)[0]<<endl;
        //        for (int k = 0; k < size; k++)
        //            cout<<VECTOR(es)[k];
        //        cout<<endl;
        
        // delete edges from H and change value of lifted vertex in G
        for (int j = 0; j < igraph_ecount(G); j++) { // G not H because we want to delete the edges in H and when doing so the ids will get rearranged so we can get segmentation falt. Also, when we add back the edges we are adding them back from G so we need to know their id in G.
            int from, to;
            igraph_edge(G,j,&from,&to);
            
            if (from == index || to == index) {
                if (EAN(G,"Lifted",j) == Lifted)
                    notLifted++;
                
                SETEAN(G, "Lifted", j, Lifted);
                LiftedVnE.lifted.push_back(j);
                
                
                if (H->test_edge(G->get_edge(j))) { // If the edge is in H
                    int eid;
                    igraph_get_eid(H, &eid, from, to, IGRAPH_DIRECTED, 1); // get id of the edge in H
                    igraph_delete_edges(H, igraph_ess_1(eid));
                }
            }
        }
    }
    cout<<index<<" "<<igraph_ecount(H)<<endl;
    maxL1 = max_L1;
}

void Security::lift_vertex(int min_L1, int threads) {
    for (int i = 0; i < remove_vertices_max; i++) {
        // remove mappings that don't work
        vcount++;
        clean_solutions();
        lift_vertex();
        cout<<setfill('.')<<setw(250)<<notLifted<<endl;
        int lifted_edges = igraph_ecount(G) - igraph_ecount(H) - notLifted;
        
        if (optimalSolution.L1 != -1) {
            if (lifted_edges < optimalSolution.liftedEdges)
                updateOptimalSolution(maxL1, lifted_edges, vcount);
            else if (lifted_edges == optimalSolution.liftedEdges) {
                if (vcount < optimalSolution.liftedVertices)
                    updateOptimalSolution(maxL1, lifted_edges, vcount);
                else if (vcount == optimalSolution.liftedVertices)
                    if (maxL1 > optimalSolution.L1)
                        updateOptimalSolution(maxL1, lifted_edges, vcount);
            }
        } else updateOptimalSolution(maxL1, lifted_edges, vcount);
        
        cout<<setfill('/')<<setw(250)<<maxL1<<" "<<lifted_edges<<" "<<vcount<<endl;
        cout<<setfill('/')<<setw(250)<<optimalSolution.L1<<" "<<optimalSolution.liftedEdges<<" "<<optimalSolution.liftedVertices<<endl;
        
        // Add edges until we reach the target sec lvl
        clean_solutions();
        if (maxL1 >= min_L1)
            S1_greedy(false, threads, min_L1, maxL1);
    }
}

void Security::file(actions action, string outFileName) {
    switch (action) {
        case OPEN:
            koutfile.open(string(outFileName.substr(0,outFileName.rfind('.')) + "_raw.txt").c_str());
            koutfile<<"# security"<<"     "<<"# lifted e"<<endl;
            
            k2outfile.open(string(outFileName.substr(0,outFileName.rfind('.')) + "_no_lifting.txt").c_str());
            k2outfile<<"# security"<<"     "<<"# lifted e"<<endl;
            
            k3outfile.open(outFileName.c_str());
            k3outfile<<"# security"<<"     "<<"# lifted e"<<endl;
            
            k4outfile.open(string(outFileName.substr(0,outFileName.rfind('.')) + "_optimal.txt").c_str());
            k4outfile<<"# security"<<"     "<<"# lifted e"<<endl;
            
            k5outfile.open(string(outFileName.substr(0,outFileName.rfind('.')) + "_unlift.txt").c_str());
            k5outfile<<"# security"<<"     "<<"# lifted e"<<endl;
            break;
            
        case WRITE:
            koutfile<<setfill(' ')<<setw(6)<<maxL1<<setfill(' ')<<setw(15)<<igraph_ecount(G)-igraph_ecount(H) - notLifted<<setfill(' ')<<setw(15)<<vcount<<endl;
            k4outfile<<setfill(' ')<<setw(6)<<optimalSolution.L1<<setfill(' ')<<setw(15)<<optimalSolution.liftedEdges<<setfill(' ')<<setw(15)<<optimalSolution.liftedVertices<<endl;
            k5outfile<<setfill(' ')<<setw(6)<<maxL1<<setfill(' ')<<setw(15)<<igraph_ecount(G)-igraph_ecount(H)<<setfill(' ')<<setw(15)<<vcount<<endl;
            break;
            
        case CLOSE:
            koutfile.close();
            break;
            
        default:
            break;
    }
}

void Security::updateOptimalSolution(int maxL1, int lifted_Edges, int vcount) {
    optimalSolution.L1 = maxL1;
    optimalSolution.liftedEdges = lifted_Edges;
    optimalSolution.liftedVertices = vcount;
}
////////////////
