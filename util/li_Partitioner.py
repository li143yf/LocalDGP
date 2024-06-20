import random
from abc import abstractmethod
import torch
import math
import pickle
import numpy as np
import dgl
import dgl.backend as F
import time
# 基类
class Partitioner:
    def __init__(self, graph, k):
        self.graph = graph
        self.k = k
        self.partition_node_ids = None
        self.partition_offset = None
        self.blocks = None
        pass

    @abstractmethod
    def divide(self):
       
        pass

    def save(self, path):
        if self.partition_node_ids is None or self.partition_offset is None:
            self.divide()

        print("save to", path)
        with open(path, 'wb') as f:
            pickle.dump((self.partition_node_ids, self.partition_offset), f)
        pass

    def save_hdrf(self, path):
        if self.blocks is None:
            self.divide()

        print("save to", path)
        with open(path, 'wb') as f:
            pickle.dump((self.blocks), f)
        pass



class Metis(Partitioner):
    def __init__(self, graph, k):
        super().__init__(graph, k)
        self.g = graph
        self.k = k

    def divide(self):
        t0 = time.time()  
        partition_ids = dgl.metis_partition_assignment(
            self.g, self.k, mode="k-way")
        partition_ids = F.asnumpy(partition_ids)
        partition_node_ids = np.argsort(partition_ids)
        partition_size = F.zerocopy_from_numpy(np.bincount(partition_ids, minlength=self.k))
        partition_offset = F.zerocopy_from_numpy(np.insert(np.cumsum(partition_size), 0, 0))
        partition_node_ids = F.zerocopy_from_numpy(partition_node_ids)
        self.partition_node_ids = partition_node_ids
        self.partition_offset = partition_offset

        num_nodes_list = []
        num_edges_list = []
        for i in range(0,self.k):
            node_ids = self.partition_node_ids[self.partition_offset[i]:self.partition_offset[i + 1]]
            node_ids = torch.tensor(node_ids)
            minsg = self.g.subgraph(node_ids, relabel_nodes=True)  
            num_nodes = minsg.num_nodes()
            num_edges = minsg.num_edges()
            num_nodes_list.append(num_nodes)  
            num_edges_list.append(num_edges)

        return partition_node_ids, partition_offset
        pass

