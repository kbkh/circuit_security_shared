
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


#include "circuit.hpp"
#include "general.hpp"
#include <subisosat.hpp>

using namespace std;

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

class Security;

string report (string prefix, Circuit *G, Circuit *H, int L1=-1, int L0=-1, Edge edge=Edge(-1,-1));

/*****************************************************************************
 * Classes
 ****************************************************************************/
// Added by Karl
class EMBEDDINGS {
public:
    set<int> edges; // edges of that embedding
    set<int> vertices; // vertices of that embedding
    set<int> connected_embeddings; // embeddings that share one or more vertices with that embedding
    int size; // how many embeddings it has as not VD
    int max_degree; // degree of this embedding
    vector<int> mapEMB; // for bonds
    igraph_vector_t mapp;
    map<int,int> mmap;
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
    //map<int,int> mapPAG; // map in which the corresponding vertices in G are stored
    vector<int> mapPAG; // for bonds
    int max_degree; // degree of this embedding (PAG)
    vector<EMBEDDINGS> embeddings; // embeddings of this pag
    VDEMBEDDINGS vd_embeddings; // vd embeddings of this pag
};
////////////////

class Security {
private:
    Circuit *G, *H, *F, *R;
    // Added by Karl
    int maxL1;
    ofstream koutfile;
    ofstream k2outfile;
    ofstream k3outfile;
    ofstream k4outfile;
    ofstream k5outfile;
    
    int maxPAGsize;
    vector<set<int> > edge_neighbors;
    vector<PAG> pags;
    map<int,vector<int> > colors;
    int H_v_dummy;
    int H_e_dummy;
    int G_v_lifted;
    int G_e_lifted;
    bool start;
    vector<set<int> > vertex_neighbors_out;
    vector<set<int> > vertex_neighbors_in;
    set<int> top_tier_vertices;
    map<int, set<int> >top_tier_edges;
    float nand_area;
    float inv_area;
    float nor_area;
    string NAND;
    string INV;
    string NOR ;
    
    ////////////////
public:
    Security (Circuit *G, Circuit *H, Circuit *F, Circuit *R);
    ~Security();
    
    /* Fill the vector with the neighbors of every edge */
    void get_edge_neighbors();
    
    /* create all possible subgraphs of size maxPAGsize */
    void subgraphs(int v, set<int> current_subgraph, set<int> possible_edges, set<int> neighbors);
    
    /* create the graphs from the edges */
    void create_graph(igraph_t* g, set<int> edges, set<int>& vertices_set, int* max_degree, vector<int>& verti, bool create = true/*, map<int,int>& map12, bool mapping*/);
    
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
    
    /* k-isomorphisim */
    void kiso(int min_L1, int max_L1, int maxPsize, int tresh, bool baseline);
};

#endif
