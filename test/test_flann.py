#!/usr/bin/env python
import pyflann
import pickle
import numpy as np
from sklearn.preprocessing import normalize
 
train_n = 1
test_n = 1
feature_number = 192
 
train_data = np.random.rand(train_n, feature_number)
test_data = np.random.rand(test_n, feature_number)
print(train_data.shape)
print(test_data.shape)
print(train_data)
print(test_data)

train_data = normalize(train_data, norm='l2')
test_data = normalize(test_data, norm='l2')



mul = 0.0
mul1 = 0.0
for i in range(0,192):
    a = train_data[0][i] - test_data[0][i]
    mul1 += a*a
    mul += train_data[0][i] * test_data[0][i]

print("mul:") 
print(2-mul*2)
print(mul)
print(mul1)


pyflann.set_distance_type("euclidean")
     
flann = pyflann.FLANN()
     
branching = 10
params = flann.build_index(train_data, algorithm='kmeans',target_precision=0.9, branching = branching , log_level='info')
     
     
#params = flann.build_index(train_data, algorithm='kdtree', trees=4)

top_k_results = 1
sims, dists = flann.nn_index(test_data, top_k_results, checks = params['checks'])
print(sims.shape)
print(dists.shape)
print(sims)
print(dists)
     
exit(0)
pickle.dump(params,open('params.pk','wb'))
#flann.save_index(b'flann_index')
# Or 
# flann_filename = 'flann_index'
# flann.save_index(bytes(flann_filename, encoding='utf8'))
