
#include "util.h"

/// 修正图数据，防止以"#"或"/"开头
/// \param filename
void uniform_graph_file(char *filename) {
    /*读入文件流*/
    std::ifstream infile;
    infile.open(filename);
    /*每行字符串*/
    char buf[1024];
    /*新文件名*/
    char attach_fname[] = ".uniform.txt";
    std::strcat(filename, attach_fname);
    /*读出文件流*/
    std::ofstream outfile;
    outfile.open(filename);
    int i = 0;
    /*循环读每一行*/
    while (!infile.eof()) {
        infile.getline(buf, 1024, '\n');
        /*判断并写入*/
        if (buf[0] != '#' and buf[0] != '/') {
            outfile << buf << std::endl;
        }
        std::cout << ++i << std::endl;
    }
    /*关闭文件*/
    infile.close();
    outfile.close();
}

//生成igraph图数据
void read_graph_edge(char *filename, igraph_t *graph) {
    /*如果需要修正图数据格式，取消下行注释*/
//    uniform_graph_file(filename);

    FILE *file;
    file = fopen(filename, "r");
    if (!file)
        DLOG(ERROR) << "文件路径不正确！";
    igraph_read_graph_edgelist(graph, file, 0, false);
    delete_isolated_v(graph);
}

//写入igraph图数据,边列表
void write_graph_edge(char *filename, igraph_t *graph) {
    FILE *file;
    file = fopen(filename, "w");
    if (!file)
        DLOG(ERROR) << "文件路径不正确！";
    igraph_write_graph_edgelist(graph, file);
}

//写入igraph图数据,邻接表
std::string write_graph_adj(const std::string &edge_filename, igraph_t *graph) {
    /*邻接表文件名，打开文件*/
    std::stringstream ss;
    ss << edge_filename << ".adjlist.txt";
    std::ofstream adj_file;
    adj_file.open(ss.str());
    LOG(WARNING) << "开始转换为邻接表文件:" << ss.str() << std::endl;
    /*写入点数、边数*/
    adj_file << igraph_vcount(graph) << "\t" << igraph_ecount(graph) << std::endl;
    /*所有点的迭代器*/
    igraph_vit_t v_iterator;
    igraph_vit_create(graph, igraph_vss_all(), &v_iterator);
    /*获取iterator中当前指向的第一条边*/
    int v_id = IGRAPH_VIT_GET(v_iterator);
    int v_num = 0;
    /*遍历iterator所有边*/
    while (!IGRAPH_VIT_END(v_iterator)) {
        /*写入度数*/
        adj_file << get_v_degree(graph, v_id);
        /*v的邻接点集*/
        igraph_vector_int_t  v_neis;
        igraph_vector_int_init(&v_neis, 0);
        igraph_neighbors(graph, &v_neis, v_id, IGRAPH_ALL);
        /*遍历邻接点集*/
        for (int k = 0; k < igraph_vector_int_size(&v_neis); ++k) {
            /*v的邻接点*/
            int adj_v = VECTOR(v_neis)[k];
            /*写入邻接点*/
            adj_file << "\t" << adj_v;
        }
        adj_file << std::endl;
        /*下一个点*/
        v_id = IGRAPH_VIT_NEXT(v_iterator);
        v_num++;
        LOG_IF(INFO, v_num % 50000 == 0) << "已转换节点数:" << v_num << std::endl;
    }
    /*销毁迭代器*/
    igraph_vit_destroy(&v_iterator);
    adj_file.close();
    LOG(WARNING) << "转换成功" << std::endl;
    return ss.str();
}

//简化igraph图数据
void simplify_graph(igraph_t *graph) {
    igraph_simplify(graph, true, true, 0);
}

/*获取v的度数*/
int get_v_degree(igraph_t *graph, int v_id) {
//    /*下面这个计算单个节点度数的代码不能用了，可能是和版本有关系，舍弃*/
//    /*设置存储度数的vector*/
//    igraph_vector_int_t v_degree;
//    igraph_vector_int_init(&v_degree, 0);
//    /*将vid对应的度数放到vector中*/
//    igraph_degree(graph, reinterpret_cast<igraph_vector_int_t *>(&v_degree), igraph_vss_1(v_id), IGRAPH_ALL, IGRAPH_NO_LOOPS);
//    /*获取度数，然后销毁向量*/
//    int degree = VECTOR(v_degree)[0];
//    igraph_vector_int_destroy(&v_degree);
//    return degree;

    /*这个是计算单个节点度数的代码，好使*/
    igraph_integer_t deg;
    igraph_degree_1(graph, &deg, v_id, IGRAPH_OUT, IGRAPH_NO_LOOPS);
    return deg;
}

/// 删除vector中指定元素（如果有多个值一样的元素，会删除第一个）
/// \param item 指定元素
/// \param vec vector指针
void delete_vec_by_value(vid_t item, std::vector<vid_t> *vec) {
    /*遍历vec,找到对应元素的位置*/
    for (int k = 0; k < vec->size(); ++k) {
        if ((*vec)[k] == item){
            /*根据位置删除元素，结束*/
            delete_vec_by_index(k, vec);
            return;
        }
    }
//    auto iter = std::remove(vec->begin(), vec->end(), item);
//    vec->erase(iter, vec->end());
}

/// 删除vector中指定位置元素
/// \param index 被删除元素的位置
/// \param vec
void delete_vec_by_index(vid_t index, std::vector<vid_t> *vec) {
    /*交换被删除元素与最后一个元素*/
    std::swap((*vec)[index], (*vec)[vec->size()-1]);
    /*删除最后一个元素*/
    vec->pop_back();
}

/// 删除图中孤立节点
/// \param graph
void delete_isolated_v(igraph_t *graph) {
    /*将selector(选取全部节点)实施在graph上，并将选择的点放入iterator中*/
    igraph_vit_t v_iterator;
    igraph_vit_create(graph, igraph_vss_all(), &v_iterator);
    /*度数为0的节点向量*/
    igraph_vector_t zero_v;
    igraph_vector_init(&zero_v, 0);
    /*获取iterator中当前指向的点（第一个点）*/
    int v_id = IGRAPH_EIT_GET(v_iterator);
    /*遍历iterator所有点*/
    while (!IGRAPH_VIT_END(v_iterator)) {
        /*如果度数等于0*/
        if (get_v_degree(graph, v_id) == 0) {
            /*存入*/
            igraph_vector_push_back(&zero_v, v_id);
        }
        /*下一个节点*/
        v_id = IGRAPH_VIT_NEXT(v_iterator);
    }
    igraph_delete_vertices(graph, igraph_vss_vector(reinterpret_cast<const igraph_vector_int_t *>(&zero_v)));
    /*销毁*/
    igraph_vit_destroy(&v_iterator);
    igraph_vector_destroy(&zero_v);
}

/// 打印igraph向量
/// \param i_vector
void print_igraph_vector(igraph_vector_t *i_vector) {
    for (int i = 0; i < igraph_vector_size(i_vector); ++i) {
        std::cout << "v_id: " << i << "\titem: " << VECTOR(*i_vector)[i] << std::endl;
    }
}

/// 删除每行末尾的\n
/// \param s
void fix_line(char *s) {
    int len = (int) strlen(s) - 1;
    if (s[len] == '\n') {
        s[len] = 0;
    }
    if (s[len - 1] == '\r') {  //在windows中换行符为\r\n
        s[len - 1] = 0;
    }
}

/// 将边列表文件转换为邻接表文件
/// \param edge_filename 边列表文件名
std::string convert_edgelist_to_adjlist(const std::string &edge_filename) {
    LOG(WARNING) << "开始读入边列表文件:" << edge_filename << std::endl;
    /*导入igraph图*/
    char *file_path = (char *) edge_filename.data();
    igraph_t graph;
    read_graph_edge(file_path, &graph);
    simplify_graph(&graph);

    /*基于igraph将图转换为adjlist格式并存储*/
    return write_graph_adj(edge_filename, &graph);
}

/// 进度条
/// \param done_num 已完成数目
/// \param all_num 总数目
/// \param tag 箭头内容
void process_bar(size_t done_num, size_t all_num, const std::string &tag) {
    // 整体放大
    int percent = (int) ((double) done_num / all_num * 100);
    std::cout << "\33[1A"; // 终端光标向上移动1行
    std::cout << "[" + std::string(percent, '=') + tag + std::string(100 - percent, ' ') << "]  "
              << std::to_string(done_num) << " / " << std::to_string(all_num) << std::endl;
    fflush(stdout); // 刷新缓冲区
}
