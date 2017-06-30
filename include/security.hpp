
/*************************************************************************//**
                                                                            *****************************************************************************
                                                                            * @file        sat.hpp
                                                                            * @brief
                                                                            * @author      Frank Imeson
                                                                            * @date
                                                                            *****************************************************************************
                                                                            ****************************************************************************/

// http://www.parashift.com/c++-faq-lite/
// http://www.emacswiki.org/emacs/KeyboardMacrosTricks

#ifndef SAT_H		// guard
#define SAT_H

/*****************************************************************************
 * INCLUDE
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <queue>
#include <string>
#include <iostream>
#include <sstream>
#include <time.h>
#include <unistd.h>
#include <list>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <set>
#include <map>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <boost/regex.hpp>


#include "minisat/core/Solver.h"
#include "circuit.hpp"
#include "general.hpp"
#include <formula.hpp>
#include <subisosat.hpp>

// Added by Karl
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FILEPATH "/tmp/mmapped.bin"
////////////////

using namespace isosat;
using namespace Minisat;
using namespace std;
using namespace formula;

/*****************************************************************************
 * Defs
 ****************************************************************************/

typedef enum {
    OPEN,
    WRITE,
    CLOSE
} actions;


/*****************************************************************************
 * Prototypes
 ****************************************************************************/

class Security;
class LiftingInfo;
class OptimalSolution;
struct L1_Edge;
struct EdgeInfo;

string report (string prefix, Circuit *G, Circuit *H, int L1=-1, int L0=-1, Edge edge=Edge(-1,-1));
string report (igraph_vector_t *soln);
/* Make sure that the file is good and has all the info */
bool parse (string line, Circuit *G, int &L1, int &L0, Edge &edge);
bool parse (string line, igraph_vector_t *soln);
bool l1_edge_lt (const L1_Edge* rhs, const L1_Edge* lhs);
int  set_L1 (const Circuit *G, const vector<EdgeInfo> &edge_set);


/*****************************************************************************
 * Classes
 ****************************************************************************/
// Added by Karl
class OptimalSolution {
public:
    int L1;
    int liftedEdges;
    int liftedVertices;
};

class LiftingInfo {
public:
    vector<int> vertexIDs;
    vector<int> lifted;
    set<int> edgeIDsSet; // Id of edges in G
};

class EMBEDDINGS {
public:
    set<int> edges; // edges of that embedding
    set<int> vertices; // vertices of that embedding
    set<int> connected_embeddings; // embeddings that share one or more vertices with that embedding
    igraph_vector_t* map; // mappings of the embedding to the pag. Used for VM
    int size; // how many embeddings it has as not VD
    int max_degree; // degree of this embedding
};

class VDEMBEDDINGS {
public:
    map<int, set<int> > vd_embeddings; // edges of the vd embedding at this place in the map
    int max_degree; // max degree of all vd embeddings
    int max_count; // count of vd embeddings of this pag with this max degree
};

class PAG {
public:
    bool processed;
    set<int> pag; // edges in pag
    set<int> vertices; // vertices in pag
    map<int,int> mapPAGG; // map in which the corresponding vertices in G are stored
    int max_degree; // degree of this embedding (PAG)
    vector<EMBEDDINGS> embeddings; // embeddings of this pag
    VDEMBEDDINGS vd_embeddings; // vd embeddings of this pag
};
////////////////

struct EdgeInfo {
    int   eid;
    Edge  edge;
    int   max_degree;
    EdgeInfo (int _eid = -1, Edge _edge = Edge(-1,-1), int _max_degree = -1) { eid = _eid; edge = _edge; max_degree = _max_degree; };
};


struct EdgeStats {
private:
public:
    vector<EdgeInfo> placed;
    vector<EdgeInfo> unplaced;
    vector<bool> first;
    vector<bool> second;
};


struct L1_Edge {
private:
public:
    int  eid;
    Edge edge;
    int L1_prev;
    int L1_sat;
    int L1_vf2;
    L1_Edge (int _eid = -1, Edge _edge = Edge(-1,-1), int _L1 = -1) :L1_sat(-1), L1_vf2(-1), edge(-1,-1) { L1_prev = _L1; edge = _edge; eid = _eid; }
    int L1() { return L1_sat == -2 ? L1_sat : ::max(L1_sat, L1_vf2); } // Added by Karl: L1_sat == -2 ? L1_sat :
};


struct L1_struct {
    int                     vid2;
    vector< vector<bool> >  mapped;
    vector< vec<Lit>* >     reject;
    // Added by Karl
    vector<bool> infinite;
    int max_L1;
    ////////////////
};


class L1_Thread : public Thread {
public:
    L1_Edge *test_edge;
    bool    vf2;
    L1_Thread () : vf2(false), test_edge(NULL) {};
};

class Security {
private:
    Isosat *isosat;
    Circuit *G, *H, *F, *R;
    igraph_vector_int_t colour_G, colour_H;
    L1_struct L1_state;
    // Added by Karl
    int maxL1;
    int remove_vertices_max;;
    ofstream koutfile;
    ofstream k2outfile;
    ofstream k3outfile;
    ofstream k4outfile;
    ofstream k5outfile;
    ////////////////
    
    friend class C_SAT;
    
    vector<igraph_vector_t*> solutions;
    void clean_solutions ();
    
    void add_edge (int eid);
    void add_free_edges (int L1);
    void analyze_edge (int eid);
public:
    Security (Circuit *G, Circuit *H, Circuit *F, Circuit *R);
    // Added by Karl
    /* Compute the security level for the whole Graph. Called from main */
    void L1_main (string outFileName, int _remove_vertices_max, int threads=1, int min_L1=2, int max_L1=-1, bool quite = true);
    /* Lift the vertex that increases the level the most */
    void lift_vertex();
    /* Lift vertices until budget exhausted */
    void lift_vertex(int min_L1, int threads);
    /* Open, write and close file */
    void file(actions action, string outFileName = "out2.txt");
    /* Update optimal solution */
    void updateOptimalSolution(int maxL1, int lifted_Edges, int vcount);
    /* Fill the vector with the neighbors of every edge */
    void get_edge_neighbors();
    /* create all possible subgraphs of size maxPAGsize */
    void subgraphs(int v, set<int> current_subgraph, set<int> possible_edges, set<int> neighbors);
    /* create the graphs from the edges */
    void create_graph(igraph_t* g, set<int> edges, map<int,int>& map12, set<int>& vertices_set, int* max_degree, bool mapping = true, bool create = true);
    /* Find VD-embeddings */
    void find_VD_embeddings(int i);
    /* Find pags and embeddings. Take care of possible edges to add etc. */
    void find_subgraphs();
    /* Update pags, embeddings etc */
    void update_pags();
    /* Find VD embeddings */
    void VD_embeddings(int* max_degree, int* max_count, int* first_pag, int min_L1);
    /* Find new pags and embeddings */
    void isomorphic_test(set<int> current_subgraph);
    /* Save the neighbors of every vertex */
    void get_vertex_neighbors();
    ////////////////
    void setConfBudget(int budget) { isosat->setConfBudget(budget); };
    void setPropBudget(int budget) { isosat->setPropBudget(budget); };
    
    int  vf2_solve (igraph_bool_t *iso, igraph_vector_t *map12, igraph_vector_t *map21, L1_struct *mapped);
    bool L0 (int max_count, bool quite = true);
    bool L1 (int max_count, bool quite = true, L1_struct *L1_state = NULL, bool vf2 = false);
    int  L1 (bool quite = true, bool vf2 = false);
    void L1 (string label);
    
    void S1_rand    (int threads=1, int min_L1=2, bool quite = true);
    void S1_greedy  (bool save_state = true, int threads=1, int min_L1=2, int max_L1=-1, bool quite = true); // Added by Karl (int remove_vertex_max = 0)
    void kiso(int min_L1, int max_L1, int maxPsize, int tresh, bool baseline);
    void df(igraph_vector_t* v, igraph_t* vect, int vert1, int vert, int d);
    void p1(igraph_t* G, igraph_vector_t* ids, int min_L1);
    int count(igraph_t* G, igraph_vector_t* ids);
    void p2(igraph_t* G, igraph_vector_t* ids, int min_L1);
    
    int rSAT(int min_L1, int max_L1, int eta);
    int rSAT(int min_L1, int max_L1, int eta, int u, bool quite);
    int rSAT(int min_L1, int max_L1, int eta, int u);
    void S1_self ();
    
    void print_solutions();
    void eliminate();
    
};

class C_SAT {
    Security* self;
    Formula* sat;
    int marker;
    igraph_vector_ptr_t match_vert;
    Formula* geq( const vec<Formula*>& fvector, string n, int start);
    Formula* geq( const vec<Var>& fvector, string n, int start);
    Formula* leq( const vec<Formula*>& fvector, string n, int start);
    Formula* leq( const vec<Var>& fvector, string n, int start);
    int e(int i);
    int phi(int u, int v, int i, int j);
    string convertInt(int number);
public:
    C_SAT(Security* security, int min_L1, int max_L1, int eta);
    C_SAT(Security* security, int min_L1, int max_L1, int eta, int u, bool quite);
    C_SAT(Security* security, int min_L1, int max_L1, int eta, int u);
    operator int() { return 0; }
};

#endif
