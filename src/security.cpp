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
#define USE_SOLNS

using namespace std;



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
Security::Security (Circuit *_G, Circuit *_H, Circuit *_F, Circuit *_R)
{
    G = _G;
    H = _H;
    F = _F;
    R = _R;
    
    nand_area = 0.0324;
    inv_area = 0.01944;
    nor_area = 0.0324;
    NAND = "nanf201";
    INV = "invf101";
    NOR = "norf201";
}

Security::~Security() {
    edge_neighbors.clear();
    vector<set<int> >().swap(edge_neighbors);
    
    vertex_neighbors_in.clear();
    vector<set<int> >().swap(vertex_neighbors_in);
    
    vertex_neighbors_out.clear();
    vector<set<int> >().swap(vertex_neighbors_out);
    
    top_tier_vertices.clear();
    set<int>().swap(top_tier_vertices); //
    
    top_tier_edges.clear();
    map<int, set<int> >().swap(top_tier_edges); //
    
    pags.clear();
    vector<PAG>().swap(pags);
    
    colors.clear();
    map<int,vector<int> >().swap(colors); //
}

// Added by Karl--
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
        igraph_vector_destroy(&nvids); // new addition
    }
}

void Security::create_graph(igraph_t* g, set<int> edges, set<int>& vertices_set, int* max_degree, vector<int>& verti, bool create) {
    map<int,int> vertices;
    map<int,int> vertices2;
    
    if (create)
        igraph_empty(g,0,IGRAPH_DIRECTED);
    
    set<int>::iterator it;
    for (it = edges.begin(); it != edges.end(); it++) {
        int from, to;
        int new_from, new_to;
        int F_from, F_to;
        
        igraph_edge(G,*it,&from,&to);
        //cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
        //cout<<from<<" "<<to<<endl;
        
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
            
            verti.push_back(VAN(G, "ID", from));
            
            if (start && !create) {
                H_v_dummy++;
                igraph_add_vertices(F, 1, 0);
                SETVAN(F, "Dummy", igraph_vcount(F)-1, kDummy);
                SETVAS(F, "type", igraph_vcount(F)-1, VAS(G, "type", from));
                SETVAN(F, "colour", igraph_vcount(F)-1, VAN(G, "colour", from));
                SETVAN(F, "ID", igraph_vcount(F)-1, igraph_vcount(F)-1);
                SETVAS(F, "Tier", igraph_vcount(F)-1, "Bottom");
                F_from = igraph_vcount(F)-1;
            } else if (!create) {
                SETVAS(F, "Tier", VAN(G, "ID", from), "Bottom");
                SETVAS(R, "Tier", VAN(G, "ID", from), "Bottom");
                F_from = VAN(G, "ID", from);
            }
            
            SETVAN(g, "colour", vid, VAN(G, "colour", from));
            SETVAN(g, "ID", vid, VAN(G, "ID", from));
            
            vertices.insert(pair<int,int>(from,vid));
            
            if (start && !create)
                vertices2.insert(pair<int,int>(from,F_from));
            
            new_from = vid;
            
            // add to vertices set
            set<int>::iterator has = vertices_set.find(from);
            if (has == vertices_set.end()) // not in set, security check
                vertices_set.insert(from);
            
            if (igraph_vertex_degree(G, from) > *max_degree)
                *max_degree = igraph_vertex_degree(G, from);
        } else {
            new_from = vertices[from];
            
            if (start)
                F_from = vertices2[from];
        }
        
        got = vertices.find(to);
        if (got == vertices.end()) { // not in set
            igraph_add_vertices(g, 1, 0);
            
            int vid = igraph_vcount(g) - 1;
            
            verti.push_back(VAN(G, "ID", to));
            
            if (start && !create) {
                H_v_dummy++;
                igraph_add_vertices(F, 1, 0);
                SETVAN(F, "Dummy", igraph_vcount(F)-1, kDummy);
                SETVAS(F, "type", igraph_vcount(F)-1, VAS(G, "type", to));
                SETVAN(F, "colour", igraph_vcount(F)-1, VAN(G, "colour", to));
                SETVAN(F, "ID", igraph_vcount(F)-1, igraph_vcount(F)-1);
                SETVAS(F, "Tier", igraph_vcount(F)-1, "Bottom");
                F_to = igraph_vcount(F)-1;
            } else if (!create) {
                SETVAS(F, "Tier", VAN(G, "ID", to), "Bottom");
                SETVAS(R, "Tier", VAN(G, "ID", to), "Bottom");
                F_to = VAN(G, "ID", to);
            }
            
            SETVAN(g, "colour", vid, VAN(G, "colour", to));
            SETVAN(g, "ID", vid, VAN(G, "ID", to));
            
            vertices.insert(pair<int,int>(to,vid));
            
            if (start)
                vertices2.insert(pair<int,int>(to,F_to));
            
            new_to = vid;
            
            // add to vertices set
            set<int>::iterator has = vertices_set.find(to);
            if (has == vertices_set.end()) // not in set, security check
                vertices_set.insert(to);
            
            if (igraph_vertex_degree(G, to) > *max_degree)
                *max_degree = igraph_vertex_degree(G, to);
        } else {
            new_to = vertices[to];
            
            if (start)
                F_to = vertices2[to];
        }
        
        igraph_add_edge(g, new_from, new_to);
        
        //cout<<F_from<<" "<<F_to<<endl;
        
        if (start && !create) {
            H_e_dummy++;
            igraph_add_edge(F, F_from, F_to);
            SETEAN(F, "Dummy", igraph_ecount(F)-1, kDummy);
            SETEAS(F, "Tier", igraph_ecount(F)-1, "Bottom");
        } else if (!create) {
            SETEAS(F, "Tier", EAN(G, "ID", *it), "Bottom");
            SETEAS(R, "Tier", EAN(G, "ID", *it), "Bottom");
        }
        
        SETEAN(g, "ID", igraph_ecount(g)-1, EAN(G, "ID", *it));
    }
}

void Security::isomorphic_test(set<int> current_subgraph) {
    set<int> vert;
    igraph_t new_pag; // deleted this
    int max_degree = 0;
    vector<int> verti;
    
    create_graph(&new_pag, current_subgraph, vert, &max_degree, verti);
    
    igraph_vector_t color11; // deleted this
    igraph_vector_init(&color11, 0);
    igraph_vector_int_t color1; // deleted this
    igraph_vector_int_init(&color1, 0);
    
    VANV(&new_pag, "colour", (igraph_vector_t*) &color11);
    for (int i = 0; i < igraph_vector_size(&color11); i++)
        igraph_vector_int_push_back(&color1, VECTOR(color11)[i]);
    
    // Check if this subgraph alrady has an isomorphic pag. If it does, it's an embedding of that pag
    igraph_bool_t iso = false;
    // debug
    int index = 0;
    
    igraph_vector_t map12;
    igraph_vector_init(&map12, 0);
    
    for (int i = 0; i < pags.size(); i++) {
        // debug
        index = i;
        
        // create the subgraphs
        set<int> dumy;
        int dum;
        igraph_t pag; // deleted this
        vector<int> d;
        create_graph(&pag, pags[i].pag, dumy, &dum, d);
        
        if (igraph_vcount(&pag) != igraph_vcount(&new_pag))
            continue;
        
        igraph_vector_t color22; // deleted this
        igraph_vector_init(&color22, 0);
        igraph_vector_int_t color2; // deleted this
        igraph_vector_int_init(&color2, 0);
        
        VANV(&pag, "colour", (igraph_vector_t*) &color22);
        for (int j = 0; j < igraph_vector_size(&color22); j++)
            igraph_vector_int_push_back(&color2, VECTOR(color22)[j]);
        igraph_isomorphic_vf2(&pag, &new_pag, &color2, &color1, NULL, NULL, &iso, &map12, NULL, NULL, NULL, NULL);
        
        igraph_destroy(&pag); // new addition
        igraph_vector_destroy(&color22); // new addition
        igraph_vector_int_destroy(&color2); // new addition
        
        if (iso)
            break;
    }
    
    igraph_destroy(&new_pag); // new addition
    igraph_vector_destroy(&color11); // new addition
    igraph_vector_int_destroy(&color1); // new addition
    
    if (!iso) {
        // add this subgraph to the pags
        PAG temp_pag;
        pags.push_back(temp_pag);
        pags[pags.size()-1].pag = current_subgraph;
        pags[pags.size()-1].vertices = vert;
        pags[pags.size()-1].max_degree = max_degree;
        pags[pags.size()-1].processed = false;
        pags[pags.size()-1].mapPAG = verti;
    } else {
        // add it to the list of embedding for that PAG
        EMBEDDINGS temp;
        pags[index].embeddings.push_back(temp);
        pags[index].embeddings[pags[index].embeddings.size()-1].edges = current_subgraph;
        pags[index].embeddings[pags[index].embeddings.size()-1].mapEMB = verti;
        pags[index].embeddings[pags[index].embeddings.size()-1].max_degree = 0;
        pags[index].embeddings[pags[index].embeddings.size()-1].vertices = vert;
        pags[index].embeddings[pags[index].embeddings.size()-1].max_degree = max_degree;
        pags[index].embeddings[pags[index].embeddings.size()-1].mapp = map12;
    }
}

void Security::subgraphs(int v, set<int> current_subgraph, set<int> possible_edges, set<int> neighbors) {
    if (maxPAGsize == 1)
        isomorphic_test(current_subgraph);
    else if (current_subgraph.size() == maxPAGsize-1) {
        set<int>::iterator it;
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
    // add the pag itself to the embeddings to be studied because it is a part of the graph
    EMBEDDINGS temp;
    pags[i].embeddings.push_back(temp);
    pags[i].embeddings[pags[i].embeddings.size()-1].edges = pags[i].pag;
    pags[i].embeddings[pags[i].embeddings.size()-1].max_degree = pags[i].max_degree;
    pags[i].embeddings[pags[i].embeddings.size()-1].vertices = pags[i].vertices;
    pags[i].embeddings[pags[i].embeddings.size()-1].mapEMB = pags[i].mapPAG;
    igraph_vector_init(&pags[i].embeddings[pags[i].embeddings.size()-1].mapp, 0);
    for (int j = 0; j < pags[i].vertices.size(); j++)
        igraph_vector_push_back(&pags[i].embeddings[pags[i].embeddings.size()-1].mapp, j);
    
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
        
        // start constructing subgraphs of size maxPAGsize
        subgraphs(i, current_subgraph, permitted_neighbors, neighbors);
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
            
            // if it was, remove it from the embeddings and from the vd-embeddings // delete as well
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
                
                // delete the pag
                pags[i].embeddings.erase(pags[i].embeddings.begin()); // delete as well
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
            cout<<*iter<<" ";
        cout<<endl;
        
        cout<<"vertices: ";
        for (iter = pags[i].vertices.begin(); iter != pags[i].vertices.end(); iter++)
            cout<<*iter<<" ";
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
        
        igraph_vector_t nvids; // deleted this
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
        igraph_vector_destroy(&nvids); // new addition
    }
}

void Security::kiso(int min_L1, int max_L1, int maxPsize, int tresh, bool baseline) {
    // Initialization
    
    maxPAGsize = 0;
    
    edge_neighbors.clear();
    vector<set<int> >().swap(edge_neighbors);
    
    vertex_neighbors_in.clear();
    vector<set<int> >().swap(vertex_neighbors_in);
    
    vertex_neighbors_out.clear();
    vector<set<int> >().swap(vertex_neighbors_out);
    
    top_tier_vertices.clear();
    set<int>().swap(top_tier_vertices); //
    
    top_tier_edges.clear();
    map<int, set<int> >().swap(top_tier_edges); //
    
    pags.clear();
    vector<PAG>().swap(pags);
    
    colors.clear();
    map<int,vector<int> >().swap(colors); //
    
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
    
    int bond_points = 0;
    
    while (igraph_vcount(G) != 0) {
        start = false;
        
        pags.clear();
        pags = vector<PAG>();
        
        edge_neighbors.clear();
        edge_neighbors = vector<set<int> >();
        
        cout<<"PAG: "<<maxPAGsize<<endl;
        cout<<"G: "<<igraph_vcount(G)<<endl;
        cout<<"H: "<<igraph_vcount(H)<<endl;
        
        if (maxPAGsize > 1)
            get_edge_neighbors();
        
        find_subgraphs();
        
        if (maxPAGsize == 0) {
            map<int, vector<int> >::iterator it;
            for (it = colors.begin(); it != colors.end(); it++) {
                vector<int> temp = it->second;

                int loop = temp.size()-1;
                cout<<it->first<<" "<<loop<<endl;
                
                if (temp.size() >= min_L1) {
                    for (int i = loop; i >= 0; i--) {
                        // add v to H
                        igraph_add_vertices(H, 1, 0);
                        int vid = igraph_vcount(H) - 1;
                        SETVAN(H, "colour", vid, VAN(G, "colour", temp[i]));
                        SETVAN(H, "ID", vid, VAN(G, "ID", temp[i]));
                        
                        SETVAN(G, "Removed", temp[i], Removed);
                        
                        SETVAS(F, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                        SETVAS(R, "Tier", VAN(G, "ID", temp[i]), "Bottom");
                        // delete v from vector
                        temp.erase(temp.begin()+i);
                    }
                } else {
                    if (temp.size() >= treshold || baseline) {
                        // add
                        for (int i = min_L1-1; i >= 0; i--) {
                            // add v to H
                            igraph_add_vertices(H, 1, 0);
                            
                            int vid = igraph_vcount(H) - 1;
                            if (i < temp.size()) {
                                SETVAN(H, "colour", vid, VAN(G, "colour", temp[i]));
                                SETVAN(H, "ID", vid, VAN(G, "ID", temp[i]));
                                
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
                                SETVAS(F, "type", igraph_vcount(F) - 1, VAS(G, "type", temp[0]));
                                SETVAN(F, "colour", igraph_vcount(F) - 1, VAN(G, "colour", temp[0]));
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
                    igraph_vs_destroy(&vid);
                }
            }
            
            continue;
        }
        
        // find the VD embeddings of every PAG
        int max_degree = 0;
        int max_count = 0;
        int first_pag = -1;
        
        do {
            max_degree = 0;
            max_count = 0;
            first_pag = -1;
            
            VD_embeddings(&max_degree, &max_count, &first_pag, min_L1);
            
            cout<<endl<<setfill('-')<<setw(100)<<"Add embeddings to H"<<setfill('-')<<setw(99)<<"-"<<endl;
            cout<<"first: "<<first_pag<<endl;
            
            if (first_pag == -1)
                continue;
            
            if (pags[first_pag].vd_embeddings.vd_embeddings.size() >= min_L1) {
                float lifting_cost = 0;
                float replicating_cost = 0;
    
                // cost of lifting
                int lifting_bonds = bond_points;
                float lifting_top_area = 0.0;
                float lifting_bottom_area = 0.0;
                
                set<int> top_vertices;
                map<int, set<int> >::iterator itr;
                
                for (itr = pags[first_pag].vd_embeddings.vd_embeddings.begin(); itr != pags[first_pag].vd_embeddings.vd_embeddings.end(); itr++) {
                    set<int>::iterator its;
    
                    for (its = pags[first_pag].embeddings[itr->first].vertices.begin(); its != pags[first_pag].embeddings[itr->first].vertices.end(); its++)
                        top_vertices.insert(*its);
                }
                
                for (itr = pags[first_pag].vd_embeddings.vd_embeddings.begin(); itr != pags[first_pag].vd_embeddings.vd_embeddings.end(); itr++) {
                    set<int>::iterator its;
                    
                    //debug
                    for (its = itr->second.begin(); its != itr->second.end(); its++) {
                        int from, to;
                        igraph_edge(G,*its,&from,&to);
                        cout<<from<<" "<<to<<endl;
                    }
                    cout<<"break"<<endl;
                    for (its = pags[first_pag].embeddings[itr->first].vertices.begin(); its != pags[first_pag].embeddings[itr->first].vertices.end(); its++) {
                        cout<<*its<<" ";
                    }
                    cout<<endl;
                    //
                    
                    for (its = pags[first_pag].embeddings[itr->first].vertices.begin(); its != pags[first_pag].embeddings[itr->first].vertices.end(); its++) {
                        //top_vertices.insert(*its);
                        set<int>::iterator itj;
                        for (itj = vertex_neighbors_in[*its].begin(); itj != vertex_neighbors_in[*its].end(); itj++) {
                            set<int>::iterator got = pags[first_pag].embeddings[itr->first].vertices.find(*itj);
                            if (got == pags[first_pag].embeddings[itr->first].vertices.end()) { // not in set
                                int eid = -1;
                                igraph_get_eid(G, &eid, *itj, *its, IGRAPH_UNDIRECTED, 1);
                                if ((string)EAS(G, "Tier", eid) != "Bottom")
                                    lifting_bonds--;
                                else {
                                    set<int>::iterator goti = top_vertices.find(*itj);
                                    if (goti == top_vertices.end()) // not in set
                                        lifting_bonds++;
                                }
                            }
                        }
                        for (itj = vertex_neighbors_out[*its].begin(); itj != vertex_neighbors_out[*its].end(); itj++) {
                            set<int>::iterator got = pags[first_pag].embeddings[itr->first].vertices.find(*itj);
                            if (got == pags[first_pag].embeddings[itr->first].vertices.end()) { // not in set
                                int eid = -1;
                                igraph_get_eid(G, &eid, *itj, *its, IGRAPH_UNDIRECTED, 1);
                                if ((string)EAS(G, "Tier", eid) != "Bottom")
                                    lifting_bonds--;
                                else {
                                    set<int>::iterator goti = top_vertices.find(*itj);
                                    if (goti == top_vertices.end()) // not in set
                                        lifting_bonds++;
                                }
                            }
                        }
                    }
                    cout<<"bond: "<<lifting_bonds<<endl;
                    cout<<"next"<<endl;
                }
                
                for (int v = 0; v < igraph_vcount(G); v++) {
                    set<int>::iterator got = top_vertices.find(v);
                    
                    if ((string)VAS(G, "type", v) == NAND) {
                        if (got == top_vertices.end()) // not in set
                            lifting_bottom_area += nand_area;
                        else lifting_top_area += nand_area;
                    }
                    else if ((string)VAS(G, "type", v) == NOR) {
                        if (got == top_vertices.end()) // not in set
                            lifting_bottom_area += nor_area;
                        else lifting_top_area += nor_area;
                    }
                    else if ((string)VAS(G, "type", v) == INV) {
                        if (got == top_vertices.end()) // not in set
                            lifting_bottom_area += inv_area;
                        else lifting_top_area += inv_area;
                    }
                }
                
                lifting_top_area *= 4; // *2 for older technology; *2 for scaling to "real" area
                lifting_bottom_area *= 2; // *2 for scaling to "real" area
                float lifting_bonds_area = (float)lifting_bonds * 0.0784;
                
                float lifting_temp = max(lifting_top_area, lifting_bottom_area);
                lifting_cost = max(lifting_temp, lifting_bonds_area);
                cout<<"top: "<<lifting_top_area<<" bottom: "<<lifting_bottom_area<<" bonds: "<<lifting_bonds_area<<endl;
                cout<<"cost: "<<lifting_cost<<endl;
                cout<<endl;
                
                // cost of keeping everything in bottom tier
                int rep_bonds = bond_points;
                float rep_top_area = 0.0;
                float rep_bottom_area = 0.0;
                
                itr = pags[first_pag].vd_embeddings.vd_embeddings.begin();
                
                map<int,set<int> >::iterator b;
                map<int,set<int> >::iterator tmp = pags[first_pag].vd_embeddings.vd_embeddings.begin();
                tmp++;
                for (b = tmp; b != pags[first_pag].vd_embeddings.vd_embeddings.end(); b++) {
                    cout<<b->first<<endl;
                    cout<<pags[first_pag].embeddings.size()<<endl;
                    cout<<igraph_vector_size(&pags[first_pag].embeddings[b->first].mapp)<<endl;
                    for (int x = 0; x < igraph_vector_size(&pags[first_pag].embeddings[b->first].mapp); x++) {
                        int tp = pags[first_pag].embeddings[b->first].mapEMB[VECTOR(pags[first_pag].embeddings[b->first].mapp)[x]];
                        int tp2 = pags[first_pag].embeddings[itr->first].mapEMB[VECTOR(pags[first_pag].embeddings[itr->first].mapp)[x]];

                        pags[first_pag].embeddings[b->first].mmap.insert(pair<int,int>(tp2,tp));
                    }
                    
                    //debug
                    map<int,int>::iterator z;
                    for (z = pags[first_pag].embeddings[b->first].mmap.begin(); z != pags[first_pag].embeddings[b->first].mmap.end(); z++)
                        cout<<z->first<<" ";
                    cout<<endl;
                    for (z = pags[first_pag].embeddings[b->first].mmap.begin(); z != pags[first_pag].embeddings[b->first].mmap.end(); z++)
                        cout<<z->second<<" ";
                    cout<<endl;
                    //
                }
                
                set<int>::iterator its;
                
                for (its = pags[first_pag].embeddings[itr->first].vertices.begin(); its != pags[first_pag].embeddings[itr->first].vertices.end(); its++) {
                    int bond = 0;
                    
                    bool stop = false;
                    
                    set<int>::iterator ita;
                    for (ita = vertex_neighbors_in[*its].begin(); ita != vertex_neighbors_in[*its].end(); ita++) {
                        set<int>::iterator gt = pags[first_pag].embeddings[itr->first].vertices.find(*ita);
                        
                        if (gt == pags[first_pag].embeddings[itr->first].vertices.end()) // not in set
                            bond++;
                    }
                            
                    for (ita = vertex_neighbors_out[*its].begin(); ita != vertex_neighbors_out[*its].end(); ita++) {
                        set<int>::iterator gt = pags[first_pag].embeddings[itr->first].vertices.find(*ita);
                        
                        if (gt == pags[first_pag].embeddings[itr->first].vertices.end()) { // not in set
                            bond++;
                            stop = true;
                            break;
                        }
                    }
                    
                    if (stop) {
                        int num = pags[first_pag].vd_embeddings.vd_embeddings.size();
                        rep_bonds += (bond*max(num, max_L1)); //k
                        
                        continue;
                    }
                    
                    map<int,set<int> >::iterator b;
                    map<int,set<int> >::iterator tmp = pags[first_pag].vd_embeddings.vd_embeddings.begin();
                    tmp++;
                    
                    for (b = tmp; b != pags[first_pag].vd_embeddings.vd_embeddings.end(); b++) {
                        int v = pags[first_pag].embeddings[b->first].mmap[*its];
                    }
                }

                
                // choose
                pags[first_pag].processed = true;
                
               // map<int, set<int> >::iterator itr;
                int counter = 0;
                // for every vd-embedding
                for (itr = pags[first_pag].vd_embeddings.vd_embeddings.begin(); itr != pags[first_pag].vd_embeddings.vd_embeddings.end(); itr++) {
                    cout<<"embedding added to H: "<<itr->first<<endl;
                    
                    set<int> dummy;
                    int dum;
                    set<int> edges = itr->second;
                    vector<int> d;
                    
                    // add embedding to H
                    create_graph(H, edges, dummy, &dum, d, false);
                    counter++;
                }
                
                // Update PAGs
                update_pags();
            }
        } while (first_pag != -1);
        
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
        
                        set<int> dummy;
                        int dum;
                        set<int> edges = it->second;
                        vector<int> d;
                        // add embedding to H
                        create_graph(H, edges, dummy, &dum, d, false);
                        
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
                pags.erase(pags.begin() + pag); // delete as well
                update_pags();
                
                max_degree = 0;
                max_count = 0;
                first_pag = -1;
                
                VD_embeddings(&max_degree, &max_count, &first_pag, min_L1);
            }
        }
        
        cout<<endl;
        
        int vcount = igraph_vcount(G) - 1;
        for (int i = vcount; i >= 0; i--) {
            if (VAN(G, "Removed", i) == Removed) {
                igraph_vs_t vid;
                igraph_vs_1(&vid, i);
                igraph_delete_vertices(G,vid);
                igraph_vs_destroy(&vid);
            }
        }
        
        int ecount = igraph_ecount(G) - 1;
        for (int i = ecount; i >= 0; i--) {
            if (EAN(G, "Removed", i) == Removed) {
                igraph_es_t eid;
                igraph_es_1(&eid, i);
                igraph_delete_edges(G,eid);
                igraph_es_destroy(&eid);
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
    //--
    cout<<"top tier: "<<top_tier_vertices.size()<<endl;
}
///////////////
