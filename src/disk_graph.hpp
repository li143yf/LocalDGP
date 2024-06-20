#ifndef ALP_C_DISK_GRAPH_HPP
#define ALP_C_DISK_GRAPH_HPP

#include "util.h"


class disk_graph {
public:
    
    
    std::string basefilename;
    
    vid_t num_vertices;
    
    vid_t num_par_vertices;
    
    size_t num_edges;
    
    vid_t g_weight_sum;
    
    std::fstream fout_g;
    
    bool empty;

    
    std::vector<vid_t> v_2_d_origin;
    
    std::vector<vid_t> v_2_d_tmp;
    
    std::vector<vid_t> num_adj_not_parted;
    
    std::vector<vid_t> num_adj_parted;
    
    std::vector<vid_t> num_adj_in_p;
    
    std::vector<vid_t> v_2_w;
   
    std::vector<vid_t> v_2_l;
    
    std::vector<size_t> v_2_o;
   
    std::vector<int> v_2_p;

    
    std::vector<vid_t> weight_range_v;
    
    std::vector<size_t> max_weight_sum;

private:


    void add_adj_line(char *buffer, vid_t linenum, size_t *offset_now);

public:

    explicit disk_graph(const std::string &filename) {
        basefilename = filename;
        num_vertices = 0;
        num_edges = 0;
        empty = false;
        g_weight_sum = 0;
        num_par_vertices = 0;
        weight_range_v.reserve(FLAGS_weight_level);


        if (!is_exists(binedgelist_name(basefilename))) {
      
            fout_g.open(binedgelist_name(basefilename), std::ios::out);
            fout_g.close();
        }


        fout_g.open(binedgelist_name(basefilename), std::ios::out | std::ios::in | std::ios::binary);
    }

    virtual ~disk_graph() {
        fout_g.close();
    }


    void read_adj_file();


    void divide_level();

    void write_neis(vid_t v, const std::vector<vid_t> &neis);


    std::vector<vid_t> read_neis(vid_t v);


    vid_t count_level_of_v(vid_t v, vid_t weight);


    size_t count_cut_e();


    void write_part_results();

    void l_2_w();
};

#endif //ALP_C_DISK_GRAPH_HPP
