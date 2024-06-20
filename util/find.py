import pickle
import numpy as np
import torch
import random
import dgl


def find_top(val_acc, test_acc):
    top_1_indices = sorted(range(len(val_acc)), key=lambda i: val_acc[i], reverse=True)[:1] 
    top_1_val_acc = [val_acc[i] for i in top_1_indices]
    print("val_acc：", top_1_val_acc)
    top_1_test_acc = [test_acc[i] for i in top_1_indices]
    print("test_acc：", top_1_test_acc)


  
