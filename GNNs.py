import time
import numpy as np
import torch
import torch.nn.functional as F
import torchmetrics.functional as MF
import dgl
import util
import util.Sampler
import util.Module
import util.find
from sklearn.metrics import f1_score
import csv
from ogb.nodeproppred import DglNodePropPredDataset


methods = ["LDG", "Fennel", "Metis", "localDGP"]
method = methods[3] 
num_partitions = 10
batch_size = 1
num_epochs = 100
num_hidden = 256
lr = 0.01
weight_decay = 5e-4
dropout = 0.2



dataset = dgl.data.RedditDataset(raw_dir='./RedditDataset') 
graph = dataset[0]
      

graph = dgl.add_self_loop(graph) 
num_edges = graph.num_edges()
num_nodes = graph.num_nodes()
print("节点数", num_nodes)
print("边数", num_edges)



for j in range(5):
    print(j,"#########################################")
    model = util.Module.Graph_Conv(graph.ndata["feat"].shape[1], num_hidden, dataset.num_classes,dropout).cuda()
    #model = util.Module.SAGE2(graph.ndata["feat"].shape[1], num_hidden, dataset.num_classes).cuda()
    #model = util.Module.GAT(in_feats = graph.ndata["feat"].shape[1],num_heads=5,n_hidden=num_hidden,n_classes= dataset.num_classes,activation= F.relu,dropout=dropout).cuda()
    opt = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=weight_decay)

   
    sampler = util.Sampler.Node_partition_sampler(
        graph,
        num_partitions,
        cache_path=f'./partition_data/data/LOCAL_10',
        prefetch_ndata=["feat", "label", "train_mask", "val_mask", "test_mask"],
    ) 
    dataloader = dgl.dataloading.DataLoader(
        graph,
        torch.arange(num_partitions).to("cuda"),
        sampler,
        device="cuda",
        batch_size=batch_size,
        shuffle=True,
        drop_last=False,
        num_workers=0,
        use_uva=True,
    )

    eval_dataloader = dgl.dataloading.DataLoader(
        graph,
        torch.arange(num_partitions).to("cuda"),
        util.Sampler.Node_partition_sampler(
            graph,
            num_partitions,
            cache_path=f'./partition_data/data/LOCAL_10',
            prefetch_ndata=["feat", "label", "train_mask", "val_mask", "test_mask"],
        ),
        device="cuda",
        batch_size=batch_size,
        shuffle=False,
        drop_last=False,
        num_workers=0,
        use_uva=True,
    )  
    durations = []
    aveLOSS = []
    aveTrain_acc = []
    Val_acc = []
    Test_acc = []
    node_num = []
    edge_num = []

    for e in range(num_epochs):
       
        t0 = time.time()
        model.train()
      
        i = 0
        Loss = []
        Acc = []

        for it, sg in enumerate(dataloader):
            i += 1

            x = sg.ndata["feat"]
            y = sg.ndata["label"]
            m = sg.ndata["train_mask"].bool()  
        

            loss = F.cross_entropy(y_hat[m], y[m])

            opt.zero_grad()
            loss.backward()
            opt.step()

            acc = MF.accuracy(y_hat[m], y[m], task="multiclass", num_classes=dataset.num_classes)

            Loss.append(loss.item())
            Acc.append(acc.item())
            mem = torch.cuda.max_memory_allocated() / 1000000
        tt = time.time()
        durations.append(tt - t0)
          

        aveLOSS.append(f"{sum(Loss) / i:.8f}") 
        aveTrain_acc.append(f"{sum(Acc) / i:.8f}")
       

       
        model.eval()
        with torch.no_grad():
           
            val_preds, test_preds = [], []
           
            val_labels, test_labels = [], []
            for it, sg in enumerate(eval_dataloader):
                x = sg.ndata["feat"]  
                y = sg.ndata["label"] 
                m_val = sg.ndata["val_mask"].bool()
                m_test = sg.ndata["test_mask"].bool()
                y_hat = model.inference(sg, x)#,dropout=0)
           
               
                val_preds.append(y_hat[m_val])
                val_labels.append(y[m_val])
                test_preds.append(y_hat[m_test])
                test_labels.append(y[m_test])

            
            val_preds = torch.cat(val_preds, 0)
            val_labels = torch.cat(val_labels, 0)
            test_preds = torch.cat(test_preds, 0)
            test_labels = torch.cat(test_labels, 0)

            val_acc = MF.accuracy(val_preds, val_labels, task="multiclass", num_classes=dataset.num_classes)
            test_acc = MF.accuracy(test_preds, test_labels, task="multiclass", num_classes=dataset.num_classes)
            Val_acc.append(val_acc.item())
            Test_acc.append(test_acc.item())

    util.find.find_top(Val_acc, Test_acc)


    print(f"Average time: {np.mean(durations):.2f}s, std: {np.std(durations):.2f}s")
    print(j, "#########################################")

