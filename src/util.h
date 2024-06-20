
#ifndef ALP_C_UTIL_H
#define ALP_C_UTIL_H

#include <igraph.h>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <cmath>
#include <set>
#include <map>
#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <tuple>
#include <numeric>
#include <ctime>
#include <cstdlib>

DECLARE_int32(p);
DECLARE_int32(tnum);
DECLARE_string(filename);
DECLARE_string(have_weight);
DECLARE_int32(weight_level);
DECLARE_double(balance_factor);
DECLARE_int32(method);
DECLARE_string(stream_method);
DECLARE_int32(debug);

/*sizeof(vid_t)=4*/
typedef uint32_t vid_t;

void uniform_graph_file(char *filename);

void read_graph_edge(char *filename, igraph_t *graph);

void write_graph_edge(char *filename, igraph_t *graph);

std::string write_graph_adj(const std::string &edge_filename, igraph_t *graph);

void simplify_graph(igraph_t *graph);

int get_v_degree(igraph_t *graph, int v_id);


class Timer {
private:
    std::chrono::system_clock::time_point t1, t2;
    double total;

public:
    Timer() : total(0) {}

    void reset() { total = 0; }

    void start() { t1 = std::chrono::system_clock::now(); }

    void stop() {
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = t2 - t1;
        total += diff.count();
    }

    double get_time() { return total; }
};


/// \param item 
/// \param vec 
void delete_vec_by_value(vid_t item, std::vector<vid_t> *vec);


/// \param index 被
/// \param vec
void delete_vec_by_index(vid_t index, std::vector<vid_t> *vec);


/// \param graph
void delete_isolated_v(igraph_t *graph);


/// \param i_vector
void print_igraph_vector(igraph_vector_t *i_vector);


/// \param basefilename 
/// \return
inline std::string binedgelist_name(const std::string &basefilename) {
   
    std::stringstream ss;
    
    ss << basefilename << ".binedgelist";
   
    return ss.str();
}


/// \param name 
/// \return
inline bool is_exists(const std::string &name) {
   
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}


/// \param s
void fix_line(char *s);


/// \param edge_filename 
std::string convert_edgelist_to_adjlist(const std::string &edge_filename);


/// \param done_num 
/// \param all_num 
/// \param tag 
void process_bar(size_t done_num, size_t all_num, const std::string &tag);

#endif //ALP_C_UTIL_H
