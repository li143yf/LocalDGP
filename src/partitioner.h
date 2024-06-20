

#ifndef ALP_C_PARTITIONER_ALP_H
#define ALP_C_PARTITIONER_ALP_H

#include "util.h"
#include "disk_graph.hpp"



class Partitioner {
public:
    disk_graph *graph;

    int P_id;
    
    vid_t Nv_num, Pv_num;
  
    std::vector<vid_t> N_v, P_v;
    
    std::unordered_map<vid_t, std::vector<vid_t>> local_graph_unpart;
    
    vid_t p_weight_sum;
    
    std::vector<size_t> level_weight_sum;
    
    std::unordered_set<vid_t> not_full_levels;
    
    bool full;

    int recent_free_v;
   
    int first_free_v;

public:

    
    /// \param Dgraph : disk_graph
    /// \param pID : ID of Part
    explicit Partitioner(disk_graph *Dgraph, const int *pID);


    ~Partitioner() = default;



    /// \param v: Vertex will be assign.
    void add_v_to_N(vid_t v);


    /// Randomly choose an free v.
    /// \return Return the chose v.
    vid_t choose_free_v();


    /// Choose one or more vertex from N.
    /// \return The vector of optimal vertices.
    std::tuple<double, vid_t, int> choose_optimal_vs_from_N();

    /// Assign the optimal vertex into P.
    /// \param v: the optimal vertex.
    void add_v_to_P(vid_t v, int opt_index_in_N);

    /// change the status of graph in the end of one part process.
    void change_g_status();


    /// judge P is full
    /// \return
    void is_full();

 
    /// One partition process start.
    void partitioning();
};


#endif //ALP_C_PARTITIONER_ALP_H
