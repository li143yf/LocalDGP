

int main(int argc, char *argv[]) {
    std::string ss=convert_edgelist_to_adjlist("/dataset_edge.txt");
    LOG(INFO) << "success: " << ss << std::endl;
}
