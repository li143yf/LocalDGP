

#include "partitioner.h"


Partitioner::Partitioner(disk_graph *Dgraph, const int *pID) {
    graph = Dgraph;
    P_id = *pID;
    Nv_num = 0;
    Pv_num = 0;

    N_v.reserve(graph->num_vertices);
    P_v.reserve(graph->num_vertices);
    p_weight_sum = 0;

    level_weight_sum.resize(FLAGS_weight_level);

    for (int k = 0; k < FLAGS_weight_level; ++k) {
        not_full_levels.insert(k);
    }
    full = false;
    recent_free_v = -1;
    first_free_v = -1;
}


void Partitioner::add_v_to_N(vid_t v) {

    if (FLAGS_debug){
        if (graph->v_2_p[v] != -2) {
            LOG(FATAL) << "!";
        }
    }


    graph->v_2_p[v] = -1;
    N_v.emplace_back(v);
    Nv_num++;

    std::vector<vid_t> v_neis = graph->read_neis(v);
    //test
    if (FLAGS_debug){
        if (v_neis.empty()) {
            LOG(FATAL) << "!";
        }
    }

    std::vector<vid_t> v_neis_unpart;
    for (auto &&adj_v:v_neis) {
        if (graph->v_2_p[adj_v] < 0) {
            v_neis_unpart.emplace_back(adj_v);
        }
    }
    if (FLAGS_debug) {
        if (v_neis_unpart.size() != graph->num_adj_not_parted[v]) {   
            LOG(FATAL) << "add_v_to_N: v_neis_unpart != graph->out_adj_nums[v]!";
        }
    }
   
    if (FLAGS_debug) {
        if ((graph->num_adj_not_parted[v] + graph->num_adj_in_p[v] + graph->num_adj_parted[v]) !=
        graph->v_2_d_origin[v]) {
            LOG(FATAL) << "!";
        }
    }
   
    local_graph_unpart.insert({v, v_neis_unpart});


}


vid_t Partitioner::choose_free_v() {
    
    graph->empty = first_free_v == recent_free_v;

    full = true;

    for (vid_t k = recent_free_v + 1; k < graph->v_2_p.size(); ++k) {
 
        if (graph->v_2_p[k] == -2) {
        
            if (first_free_v == recent_free_v) {
          
                graph->empty = false;
       
                first_free_v = k;
            }
        
            if (not_full_levels.find(graph->v_2_l[k]) != not_full_levels.end()) {
                if (FLAGS_debug){
                   
                    if (graph->num_adj_not_parted[k] == 0 || graph->num_adj_not_parted[k] != graph->v_2_d_tmp[k]){
                        LOG(FATAL) << "choose_free_v!";
                    }
                    if (graph->num_adj_in_p[k] > 0){    //测试，如果本分区邻接点数目不为0
                        LOG(FATAL) << "choose_free_v:!";
                    }
                  
                    if ((graph->num_adj_not_parted[k] + graph->num_adj_parted[k]) != graph->v_2_d_origin[k]) {
                        LOG(FATAL) << "choose_free_v: !";
                    }
                }
             
                full = false;
            
                recent_free_v = k;
           
                return k;
            }
          
            if (P_id == FLAGS_p-1){
             
                full = false;
              
                recent_free_v = k;
               
                return k;
            }
        }
    }
  
    return 0;
}


/// Choose one or more vertex from N.
/// \return The vector of optimal vertices.
std::tuple<double, vid_t, int> Partitioner::choose_optimal_vs_from_N() {
  
    if (FLAGS_debug){
        if (Nv_num != N_v.size()) {
            LOG(FATAL) << "!";
        }
    }
    struct Compare {    //Compare struct in OpenMP.
    
        double score;
   
        vid_t vb_id;

        int vb_index_in_N;
    };

  
#pragma omp declare reduction(maxVal : struct Compare : omp_out = omp_in.score > omp_out.score ? omp_in : omp_out)
   
    struct Compare max_v = {0, 0, 0};
  
    int num_threads = ((int) Nv_num / 500) + 1;
    if (num_threads > FLAGS_tnum) {
        num_threads = FLAGS_tnum;
    }
    omp_set_num_threads(num_threads);
   
#pragma omp parallel for reduction(maxVal: max_v)
  
    for (int i = 0; i < N_v.size(); i++) {
       
        vid_t v = N_v[i];
      
        vid_t weight_v = graph->v_2_w[v];
     
        vid_t level_v = graph->v_2_l[v];
      
        double alpha = (double) level_weight_sum[level_v] / (double) graph->max_weight_sum[level_v];
       
        double score_v;
      
        if (alpha >= 1) {
            score_v = 1e-10;
        } else {
            double mu_1, mu_2;
           
            mu_1 = weight_v / (double) (weight_v + graph->max_weight_sum[level_v] - level_weight_sum[level_v]);
           
            mu_2 = 1 - (double) 1 / (1 + graph->num_adj_not_parted[v] + graph->num_adj_parted[v]);
           
//            mu_2 = (double) 1 / (1 + graph->num_adj_in_p[v]);
          
//            mu_2 = 1 - (double) graph->num_adj_in_p[v] / graph->v_2_d_origin[v];

           
            score_v = 1 / (alpha * mu_1 + (1 - alpha) * mu_2);
        
//            score_v =1 / mu_1;
           
          
//            score_v =1 / mu_2;
        }
       
        if (score_v > max_v.score) {
            max_v.score = score_v;
            max_v.vb_id = v;
            max_v.vb_index_in_N = i;
        }
    }
   
    return std::make_tuple(max_v.score, max_v.vb_id, max_v.vb_index_in_N);
}
#pragma clang diagnostic pop


void Partitioner::add_v_to_P(vid_t v, int opt_index_in_N) {

    
    if (FLAGS_debug){
        if (local_graph_unpart[v].size() != graph->num_adj_not_parted[v]) { //test
            LOG(FATAL) << "add_v_to_P: local_graph_unpart[v].size() != graph->num_adj_not_parted[v]!!";
        }
       
        if ((graph->num_adj_not_parted[v] + graph->num_adj_in_p[v] + graph->num_adj_parted[v]) !=
            graph->v_2_d_origin[v]) {
            LOG(FATAL) << "add_v_to_P!";
        }
        if (graph->v_2_p[v] >= 0){
            LOG(FATAL) << "！";
        }
    }
  
    graph->v_2_p[v] = P_id;
  

    if (opt_index_in_N == -1){
        delete_vec_by_value(v, &N_v);
    }
   
    else{
        delete_vec_by_index(opt_index_in_N, &N_v);
    }
    Nv_num--;
    P_v.emplace_back(v);
    Pv_num++;
    graph->num_par_vertices++;
    p_weight_sum = p_weight_sum + graph->v_2_w[v];
    vid_t level_v = graph->v_2_l[v];
  
    level_weight_sum[level_v] = level_weight_sum[level_v] + graph->v_2_w[v];
   
    if (level_weight_sum[level_v] >= graph->max_weight_sum[level_v]) {
       
        not_full_levels.erase(level_v);
    }
   
    std::vector<vid_t> adj_vs = local_graph_unpart[v];
    local_graph_unpart.erase(v);

    
    for (vid_t adj_v:adj_vs) {
        graph->num_adj_not_parted[adj_v]--; 
        graph->num_adj_parted[adj_v]++; 
        if (graph->v_2_p[adj_v] == -1) {  
            delete_vec_by_value(v, &local_graph_unpart[adj_v]);  
        }
        if (graph->v_2_p[adj_v] == -2)  
            add_v_to_N(adj_v); 
        if (FLAGS_debug){
            if (graph->v_2_p[adj_v] >= 0){  
                LOG(FATAL) << "!";
            }
        }
        
        if (graph->num_adj_not_parted[adj_v] == 0)  
            add_v_to_P(adj_v, -1);
       
        if (FLAGS_debug){
            if ((graph->num_adj_not_parted[adj_v] + graph->num_adj_in_p[adj_v] + graph->num_adj_parted[adj_v]) !=
                graph->v_2_d_origin[adj_v]) {
                LOG(FATAL) << "add_v_to_P: !";
            }
        }
    }
}


void Partitioner::change_g_status() {
    
    for (auto &iter : local_graph_unpart) {
        if (!iter.second.empty()) {  
            if (FLAGS_debug){
                if(graph->v_2_p[iter.first] != -1){  
                    LOG(FATAL) << "change_g_status: !";
                }
              
                if (iter.second.size() != graph->num_adj_not_parted[iter.first]){
                    LOG(FATAL) << "change_g_status: !";
                }
                
                if ((graph->num_adj_not_parted[iter.first] + graph->num_adj_in_p[iter.first] + graph->num_adj_parted[iter.first]) !=
                    graph->v_2_d_origin[iter.first]) {
                    LOG(FATAL) << "change_g_status:!";
                }
            }
            graph->write_neis(iter.first, iter.second);   
            graph->v_2_p[iter.first] = -2;  
           
            graph->v_2_d_tmp[iter.first] = graph->num_adj_not_parted[iter.first];
        }
    }

   
    for (int k = 0; k < graph->num_adj_in_p.size(); ++k) {
        if (graph->num_adj_in_p[k] > 0){
            graph->num_adj_parted[k] = graph->num_adj_parted[k] + graph->num_adj_in_p[k];
            graph->num_adj_in_p[k] = 0;
           
            if (FLAGS_debug){
                if ((graph->num_adj_not_parted[k] + graph->num_adj_in_p[k] + graph->num_adj_parted[k]) !=
                    graph->v_2_d_origin[k]) {
                    LOG(FATAL) << "change_g_status: !";
                }
            }
        }
    }

 
    if (FLAGS_debug){
        for (int k = 0; k < graph->v_2_p.size(); ++k) {
            LOG_IF(FATAL, graph->v_2_p[k] == -1)
            << "change_g_status " << k << ". adj num is: "
            << local_graph_unpart[k].size();
        }
    }
}


void Partitioner::is_full() {
//    if (p_weight_sum >= FLAGS_balance_factor * graph->g_weight_sum / FLAGS_p)
//        full = true;
    full = not_full_levels.empty(); //if not_full_levels is empty, the part is full.

    
    if (std::accumulate(level_weight_sum.begin(), level_weight_sum.end(), 0) >= std::accumulate(graph->max_weight_sum.begin(), graph->max_weight_sum.end(), 0))
        full = true;

  
    if (P_id == FLAGS_p - 1)
        full = false;
}


void Partitioner::partitioning() {
    
    while (!full && !graph->empty) {
        
        vid_t optimal_v;
       
        int opt_index_in_N;
      
        if (Nv_num == 0) {
          
            optimal_v = choose_free_v();
          
            if (full || graph->empty) {
                break;
            }
          
            add_v_to_N(optimal_v);
           
            opt_index_in_N = (int) N_v.size()-1;
        }
      
        else {
           
            double max_sore;
            vid_t max_v;
            int v_index_in_N;
          
            std::tie(max_sore, max_v, v_index_in_N) = choose_optimal_vs_from_N();

           
            if (max_sore == 0) {
               
                optimal_v = choose_free_v();
              
                if (full || graph->empty) {
                    break;
                }
               
                add_v_to_N(optimal_v);
               
                opt_index_in_N = (int) N_v.size()-1;
            }
               
            else {
             
                optimal_v = max_v;
              
                opt_index_in_N = v_index_in_N;
            }
        }
        
        add_v_to_P(optimal_v, opt_index_in_N);
        
        is_full();
    }
   
    change_g_status();
    LOG(WARNING) << "Part " << P_id << " is end.\tNum v in P: " << Pv_num
                 << ".\tNum v parted: " << graph->num_par_vertices
                 << ".\tSum w in P: " << p_weight_sum
                 << std::endl;
}
