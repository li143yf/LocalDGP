import pickle
try:
    with open("cache", "rb") as f:
        cache = pickle.load(f)
except:
    cache = None
else:
    cache = None

print(cache)
