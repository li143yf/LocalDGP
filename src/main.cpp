
#include "partitioner.h"
#include "memory_monitor.h"

DECLARE_bool(help);
DECLARE_bool(helpshort);
DEFINE_int32(p, 25,);
DEFINE_int32(weight_level, 3);
DEFINE_int32(tnum, omp_get_num_procs());
DEFINE_string(filename, "/DATASET.adjlist.txt");
DEFINE_string(have_weight, "y");
DEFINE_double(balance_factor, 1.1);

DEFINE_int32(method, 2);
DEFINE_int32(debug, 1);

void calculate_result() {
   
    Timer time_all;
    time_all.start();
  
    disk_graph Dgraph(FLAGS_filename);
    Dgraph.read_adj_file();
   
    vid_t v_num = Dgraph.num_vertices;
    
    size_t e_num = Dgraph.num_edges;
    /*当前分区ID*/
    int p_num = 0;
    vid_t now_v_num = 0;

    Timer time_p;
    time_p.start();

    LOG(WARNING) << "Partition start.\t" << FLAGS_filename << "\t" << FLAGS_p;
   
    while (!Dgraph.empty) {
       
        Partitioner partition(&Dgraph, &p_num);
      
        partition.partitioning();
        p_num = p_num + 1;
        now_v_num = now_v_num + partition.Pv_num;
    }
    time_p.stop();
    time_all.stop();

    LOG(WARNING) << "Partition end." << std::endl;

//    Dgraph.write_part_results(); //Write the part result.
    Dgraph.l_2_w();

   
    std::ofstream num_result_file;
    std::stringstream ss;
    ss << FLAGS_filename << "-LocalDGP-p" << FLAGS_p << "-l" << FLAGS_weight_level <<"-";

    std::string mem_name = ss.str() + "mem-result.txt";
    num_result_file.open(mem_name, std::ios::app);
    double peakSize = (double) getPeakRSS() / (1024 * 1024); 
    LOG(WARNING) << "Peak size: " << peakSize << std::endl;
    num_result_file << peakSize << ",";
    num_result_file.close();

    std::string cut_name = ss.str() + "cut-result.txt";
    num_result_file.open(cut_name, std::ios::app);
    size_t cut_edge_num = Dgraph.count_cut_e();
    LOG(WARNING) << "：" << FLAGS_method;
    LOG(WARNING) << "：" << cut_edge_num << "\t：" << (double) cut_edge_num/e_num << std::endl;
    num_result_file << (double) cut_edge_num/e_num << ",";
    num_result_file.close();

    if (FLAGS_debug){
        LOG(WARNING) << "Num of vertices now：" << now_v_num << std::endl;
    }

    std::string time_name = ss.str() + "time-result.txt";
    num_result_file.open(time_name, std::ios::app);
    double time = time_all.get_time();
    LOG(WARNING) << "：" << time_all.get_time() << " " << "partition time: " << time_p.get_time()
                 << "" << std::endl;
    num_result_file << time << ",";
    num_result_file.close();

    std::string v2p_name = ss.str() + "v2p.txt";
    num_result_file.open(v2p_name, std::ios::app);
    for (int j : Dgraph.v_2_p) {
        num_result_file << j << std::endl;
    }
    num_result_file.close();
}

int main(int argc, char *argv[]) {
   
    google::InitGoogleLogging(argv[0]);
//    FLAGS_log_dir = "../logs/"; 
    google::SetStderrLogging(google::GLOG_WARNING);  
    FLAGS_max_log_size = 5000; 

    
    std::string usage = "-filename <图文件路径> "
                        "[-filetype <edgelist|adjlist>] "
                        "[-p <数目>] ";
    google::SetUsageMessage(usage);
    google::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_help) {
        FLAGS_help = false;
        FLAGS_helpshort = true;
    }
    google::HandleCommandLineHelpFlags();

   
    double balance_factor = 1;

    calculate_result();

    google::ShutdownGoogleLogging();    
}
