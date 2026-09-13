import numpy as np
from PIL import Image
a = np.asarray(Image.open("out/startroom_masterstart.png").convert("RGB")).astype(np.int16)
b = np.asarray(Image.open("out/startroom_branchstart.png").convert("RGB")).astype(np.int16)
d = np.abs(a-b).max(axis=2)
ys, xs = np.nonzero(d > 6)
print("pixels differing >6:", len(ys), "of", d.size)
if len(ys):
    print("bounding box of the differences: x", xs.min(), "-", xs.max(), " y", ys.min(), "-", ys.max())
    for name, arr, n in (("rows", ys, a.shape[0]), ("cols", xs, a.shape[1])):
        h, _ = np.histogram(arr, bins=8, range=(0, n))
        print(name, "spread over 8 bands:", h.tolist())
