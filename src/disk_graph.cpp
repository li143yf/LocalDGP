#include "disk_graph.hpp"


void disk_graph::read_adj_file() {
    FILE *inf = fopen(basefilename.c_str(), "r");
    if (inf == nullptr) {
        LOG(FATAL) << "无法加载邻接表文件：" << basefilename << std::endl;
    }
    LOG(INFO) << "开始加载邻接表文件：" << basefilename << std::endl;
    int maxlen = 1000000000;
    char *buff = (char *) malloc(maxlen);
    size_t bytesread = 0;
    size_t lastlog = 0;
    
    vid_t linenum = 0;
    
    size_t offset_now = 0;
    while (fgets(buff, maxlen, inf) != nullptr) {
        fix_line(buff);
        if (buff[0] == '#' or buff[0] == '%' or buff[0] == '/')
            continue; // Comment
       
        add_adj_line(buff, linenum++, &offset_now);
        
        if (FLAGS_debug){
            if (bytesread - lastlog >= 500000000) {
                LOG(INFO) << "已读" << linenum << "行，"
                          << (double) bytesread / 1024 / 1024. << " MB" << std::endl;
                lastlog = bytesread;
            }
        }
        bytesread += strlen(buff);
    }
    free(buff);
    fclose(inf);
    LOG(WARNING) << "Graph information.\tv_num: " << num_vertices
                 << "\te_num: " << num_edges
                 << "\tw_sum: " << g_weight_sum
                 << std::endl;
    
    divide_level();
    
    for (int k = 0; k < v_2_w.size(); ++k) {  //store the v_2_l of each v.
        v_2_l.emplace_back(count_level_of_v(k, v_2_w[k]));
    }
}


void disk_graph::add_adj_line(char *buffer, vid_t linenum, size_t *offset_now) {
    
    char delims[] = " \t,";
    
    char *t;
    
    if (linenum == 0) {
       
        t = strtok(buffer, delims);
        num_vertices = atoi(t);
        
        v_2_d_origin.reserve(num_vertices);
        v_2_d_tmp.reserve(num_vertices);
        num_adj_not_parted.reserve(num_vertices);
        num_adj_parted.reserve(num_vertices);
        num_adj_in_p.reserve(num_vertices);
        v_2_o.reserve(num_vertices);
        v_2_w.reserve(num_vertices);
       
        v_2_p.reserve(num_vertices);
        for (int k = 0; k < num_vertices; ++k) {
            v_2_p.emplace_back(-2);
        }
       
        t = strtok(nullptr, delims);
        num_edges = atoi(t);
    } else { 
        vid_t from = linenum - 1;
       
        v_2_o.emplace_back(*offset_now);
       
        t = strtok(buffer, delims);
        vid_t degree_v = atoi(t);
       
        v_2_d_origin.emplace_back(degree_v);
        v_2_d_tmp.emplace_back(degree_v);
       
        num_adj_not_parted.emplace_back(degree_v);
        num_adj_parted.emplace_back(0);
        num_adj_in_p.emplace_back(0);
       
        vid_t weight_v; 
        if (FLAGS_have_weight == "y") {
            weight_v = degree_v;   
        } else {
            weight_v = 1;  
        }
        
        v_2_w.emplace_back(weight_v);
       
        g_weight_sum = g_weight_sum + weight_v;

       
        (*offset_now) = (*offset_now) + degree_v * sizeof(vid_t);
      
        if (t != nullptr) {
           
            vid_t i = 0;
            while ((t = strtok(nullptr, delims)) != nullptr) {
                
                vid_t to = atoi(t);
               
                if (from != to) {
                    
                    fout_g.write((char *) &to, sizeof(vid_t));
                } else {
                    LOG(FATAL) << "不允许自环!\t当前行数：" << linenum << "\t当前邻接点：" << to << std::endl;
                }
                i++;
            }
            LOG_IF(ERROR, degree_v != i) << "度数与邻接点数目不一致！\t当前行数：" << linenum
                                         << "\t当前度数：" << degree_v << "\t当前邻接点数目：" << i << std::endl;
        }
    }
}


void disk_graph::divide_level() {
  
    std::vector<vid_t> weight_shuffle = v_2_w;
    
    std::sort(weight_shuffle.begin(), weight_shuffle.end());
    
    vid_t level_max_v_num = ceil((double) num_vertices / FLAGS_weight_level);
   
    weight_range_v.emplace_back(weight_shuffle[0]);
    
    vid_t flag_v_num_in_1_l = 1;
    
    vid_t flag_level_num = 1;
    
    size_t flag_weight_sum_in_1_l = weight_shuffle[0];
    
    for (vid_t k = 1; k < weight_shuffle.size(); ++k) {
        flag_v_num_in_1_l++;
        flag_weight_sum_in_1_l = flag_weight_sum_in_1_l + weight_shuffle[k];

       
        if (k == weight_shuffle.size() - 1) {
            
            if (flag_level_num != FLAGS_weight_level){
                LOG(FATAL) << "divide_level: 层数设置太多！" << std::endl;
            }
           
            max_weight_sum.emplace_back(ceil((double) FLAGS_balance_factor * flag_weight_sum_in_1_l / FLAGS_p));
            break;
        }

        
        if ((double)flag_v_num_in_1_l > (double)level_max_v_num * 0.8
        && weight_shuffle[k] != weight_shuffle[k + 1]
        && flag_level_num < FLAGS_weight_level) {
           
            weight_range_v.emplace_back(weight_shuffle[k+1]);
          
            max_weight_sum.emplace_back(ceil((double) FLAGS_balance_factor * flag_weight_sum_in_1_l / FLAGS_p));
           
            flag_v_num_in_1_l = 0;
            flag_level_num++;
            flag_weight_sum_in_1_l = 0;
            continue;
        }
    }
    
    if (weight_range_v.size() != FLAGS_weight_level || max_weight_sum.size() != FLAGS_weight_level){
        LOG(FATAL) << "divide_level: " << std::endl;
    }
}


/// \param weight 
/// \return 
vid_t disk_graph::count_level_of_v(vid_t v, vid_t weight) {
   
    for (int l = (int) weight_range_v.size() - 1; l >= 0; --l) {
      
        if (weight >= weight_range_v[l]) {
            return l;
        }
    }
   
    LOG(FATAL) << "count_level_of_v: ！ v is: " << v
               << ", weight is: " << weight << "." << std::endl;
    return 0;
}


/// \param v 
/// \param neis 
void disk_graph::write_neis(vid_t v, const std::vector<vid_t> &neis) {
   
    fout_g.seekp(v_2_o[v]);
    
    for (vid_t nei : neis) fout_g.write((char *) &nei, sizeof(vid_t));
}


/// \param v 
/// \return 
std::vector<vid_t> disk_graph::read_neis(vid_t v) {
   
    vid_t deg = v_2_d_tmp[v];
    
    std::vector<vid_t> neis;
    neis.reserve(deg);
   
    fout_g.seekg(v_2_o[v]);
    for (int k = 0; k < deg; ++k) {
        vid_t v_nei;
        fout_g.read((char *) &v_nei, sizeof(vid_t));
        neis.emplace_back(v_nei);
    }
    return neis;
}

/// Write the part result into file.
void disk_graph::write_part_results() {
    /*file name*/
    std::stringstream ss;
    ss << FLAGS_filename << "-LocalWGVP-" << FLAGS_p << "-part-result.txt";
    std::ofstream result_file;
    result_file.open(ss.str(), std::ios::app);
    for (auto result: v_2_p) {    //write the part result of each vertex.
        result_file << result << std::endl;
    }
    result_file.close();
}


size_t disk_graph::count_cut_e() {
    FILE *inf = fopen(basefilename.c_str(), "r");
    LOG_IF(FATAL, inf == nullptr) << "：" << basefilename << std::endl;
    int maxlen = 1000000000;
    char *buff = (char *) malloc(maxlen);
    
    vid_t linenum = 0;
    size_t cut_edge_num = 0;
    
    while (fgets(buff, maxlen, inf) != nullptr) {
       
        char *t;
        fix_line(buff);
        if (buff[0] == '#' or buff[0] == '%' or buff[0] == '/')
            continue;
        
        char delims[] = " \t,";
       
        if (linenum == 0) {
            strtok(buff, delims);
            strtok(nullptr, delims);
        } else {
            
            vid_t from = linenum - 1;
           
            t = strtok(buff, delims);
            vid_t degree_v = atoi(t);
           
            if (t != nullptr) {
                
                vid_t num_adj = 0;
               
                while ((t = strtok(nullptr, delims)) != nullptr) {
                   
                    vid_t to = atoi(t);
                   
                    if (from < to && v_2_p[from] != v_2_p[to]){
                        cut_edge_num++;
                    }
                    LOG_IF(FATAL, from == to) << "from = to!\tlinenum：" << linenum << "\tadj：" << to << std::endl;
                    num_adj++;
                }
                LOG_IF(FATAL, degree_v != num_adj) << "：" << linenum
                                                   << "：" << degree_v << "：" << num_adj << std::endl;
            }
        }
        linenum++;
    }
    fclose(inf);
    return cut_edge_num;
}


void disk_graph::l_2_w(){

    std::vector<vid_t> G_l_2_w;
    G_l_2_w.resize(FLAGS_weight_level);
    
    std::vector<std::vector<vid_t>> p_l_2_w;
    p_l_2_w.resize(FLAGS_p);
    for (int p = 0; p < FLAGS_p; ++p) {
        p_l_2_w[p].resize(FLAGS_weight_level);
    }

    for (int v = 0; v < num_vertices; ++v) {
        vid_t l = v_2_l[v];
        vid_t w = v_2_w[v];
        vid_t p = v_2_p[v];
        G_l_2_w[l] = G_l_2_w[l] + w;
        p_l_2_w[p][l] = p_l_2_w[p][l] + w;
    }

 
    std::stringstream ss;
    ss << FLAGS_filename << "-LocalWGVP-" << FLAGS_p << "-cal_l_2_w.csv";
    std::ofstream result_file;
    result_file.open(ss.str(), std::ios::app);
    std::map<vid_t, vid_t>::iterator iter;
    for (auto w : G_l_2_w) {
        result_file << w << ",";
    }
    result_file << std::endl;
    for (auto p : p_l_2_w) {
        for (auto w : p) {
            result_file << w << ",";
        }
        result_file << std::endl;
    }

    result_file.close();

}
