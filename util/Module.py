import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.nn.init as init

import dgl
import dgl.nn as dglnn


class SAGE2(nn.Module): 
    def __init__(self, in_feats, n_hidden, n_classes,dropout):
        super().__init__()
        self.layers = nn.ModuleList()
        self.layers.append(dglnn.SAGEConv(in_feats, n_hidden, "mean"))
        self.layers.append(dglnn.SAGEConv(n_hidden, n_classes, "mean"))
        self.dropout = nn.Dropout(dropout)

    def forward(self, sg, x):
        h = x
        for l, layer in enumerate(self.layers):         
            h = layer(sg, h)              
            if l != len(self.layers) - 1:
                h = F.relu(h)
                h = self.dropout(h)
        return h 

    def inference(self, sg, x):
        h = x
        for l, layer in enumerate(self.layers):
            h = layer(sg, h)
            if l != len(self.layers) - 1:
                h = F.relu(h)
        return h





class Graph_Conv(nn.Module):
    def __init__(self, in_feats, n_hidden, n_classes,dropout):
        super().__init__()
        self.layers = nn.ModuleList()
        self.layers.append(dglnn.GraphConv(in_feats, n_hidden))
        self.layers.append(dglnn.GraphConv(n_hidden, n_classes))
        self.dropout = nn.Dropout(dropout)


    def forward(self, sg, x):
        h = x
        for l, layer in enumerate(self.layers):
            h = layer(sg, h)
            if l != len(self.layers) - 1:
                h = F.relu(h)
                h = self.dropout(h)
        return h

    def inference(self, sg, x):
        h = x
        for l, layer in enumerate(self.layers):
            h = layer(sg, h)
            if l != len(self.layers) - 1:
                h = F.relu(h)
        return h

class GAT(nn.Module):
    def __init__(self, in_feats, n_hidden, n_classes,dropout,num_heads,activation):
        super().__init__()
        self.layers = nn.ModuleList()
        self.layers.append(dglnn.GATConv(
                in_feats,
                n_hidden,
                num_heads=num_heads,
                feat_drop=dropout,
                attn_drop=dropout,
                activation=activation,
                negative_slope=0.2,
            ))
       
        self.layers.append(dglnn.GATConv(
                n_hidden * num_heads,
                n_classes,
                num_heads=num_heads,
                feat_drop=dropout,
                attn_drop=dropout,
                activation=None,
                negative_slope=0.2,
            ))
        

   
    def forward(self,sg, x):
        h = x
        for l, conv in enumerate(self.layers):
            h = conv(sg, h)
            if l < len(self.layers) - 1:
                h = h.flatten(1)
        return h.mean(1)

    def inference(self, sg, x,dropout):
        h = x
        for l, conv in enumerate(self.layers):
            h = conv(sg, h)
            if l < len(self.layers) - 1:
                h = h.flatten(1)
        return h.mean(1)
