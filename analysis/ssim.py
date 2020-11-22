import argparse
import os
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import plotly.graph_objects as go
from skimage import data, img_as_float
from skimage.metrics import structural_similarity as ssim
from skimage.metrics import mean_squared_error
import imageio
from mayavi import mlab
import _pickle
import concurrent.futures as fut

import ipyvolume as ipv

def getDisplacement(filename, origin):
    nums = np.array(filename.replace('.png', '').split('_')).astype(np.float)
    
    return nums-origin

def run_ssim(truth, warp):
    img = img_as_float(truth)
    rows, cols, channels = img.shape

    warp_img = img_as_float(warp)

    mse_warp = mean_squared_error(img, warp_img)
    ssim_warp,ssim_image = ssim(img, warp_img,
                    data_range=(warp_img.max() - warp_img.min()), multichannel=True, full=True)

    return ssim_warp

def ssim_thread(filename):
    vector = filename.replace('.png', '').split('_')
    truth_img = imageio.imread("ground_truth/" + filename)
    warp_img = imageio.imread("warped/" + filename)
    ssim_result = run_ssim(truth_img,warp_img)

    return ([float(i) for i in vector], ssim_result)

parser = argparse.ArgumentParser(description='Perform automated SSIM analysis of Openwarp output.')
parser.add_argument('displacement', help='Maximum displacement from the rendered pose to reproject')
parser.add_argument('stepSize', help='Distance between each analyzed reprojected frame in 3D space (i.e., the size of each step)')
parser.add_argument('--norun', action="store_true", help='Do not run Openwarp; instead, use the latest previous run')
parser.add_argument('--usecache', action="store_true", help='Do not run SSIM analysis; instead, use cached SSIM data from a previous analysis run')

args = parser.parse_args()

orig_dir = os.getcwd()

if not args.norun:
    os.chdir('../build')
    os.system('make && ./openwarp -disp ' + isplacement + ' -step ' + stepSize);
    os.chdir('../analysis')

os.chdir('../output')

run_dirs = [d for d in os.listdir('.') if os.path.isdir(d)]
latest_run = max(run_dirs, key=os.path.getmtime);

print("Using run: " + latest_run)

os.chdir(latest_run)

run_info = open("run_info.txt")
origin = np.array(run_info.readline().split("_")).astype(np.float)
run_info.close()
print("Run origin: " + str(origin))

ground_truth_files = os.listdir("./ground_truth/")
warp_files = os.listdir("./warped/")

# print("Ground truth files: " + str(ground_truth_files))

run_data = []

if args.usecache:
    cache = open('cache.p', 'rb')
    run_data = _pickle.load(cache)
    cache.close()
else:

    with fut.ThreadPoolExecutor(max_workers=8) as executor:
        results = executor.map(ssim_thread, ground_truth_files)
        for result in results:
            run_data.append(result)
        # run_data[0].result()
    # i = 0
    # for truthFile in ground_truth_files:
    #     print("Reading: " + truthFile)
    #     vector = truthFile.replace('.png', '').split('_')
    #     truth_img = imageio.imread("ground_truth/" + truthFile)
    #     warp_img = imageio.imread("warped/" + truthFile)
    #     ssim_result = run_ssim(truth_img,warp_img)
        
    #     run_data.append(([float(i) for i in vector], ssim_result))

cache = open('cache.p', 'wb')
_pickle.dump(run_data, open('cache.p', 'wb'))
cache.close()

# print(vectors)

disp = float(args.displacement)
num_steps = ((2.0 * disp) / float(args.stepSize) + 1.0)
step = float(args.stepSize)

assert(int(num_steps) == num_steps)
print("Num steps: " + str(num_steps))
num_steps = int(num_steps)

# X, Y, Z = np.mgrid[-8:8:40j, -8:8:40j, -8:8:40j]
# values = np.sin(X*Y*Z) / (X*Y*Z)

# fig = go.Figure(data=go.Volume(
#     x=X.flatten(),
#     y=Y.flatten(),
#     z=Z.flatten(),
#     value=values.flatten(),
#     isomin=0.1,
#     isomax=0.8,
#     opacity=0.1, # needs to be small to see through all surfaces
#     surface_count=17, # needs to be a large number for good volume rendering
#     ))
# fig.show()

X, Y, Z = np.mgrid[-disp:disp:1j * num_steps, -disp:disp:1j * num_steps, -disp:disp:1j * num_steps]
values = np.ndarray((num_steps, num_steps, num_steps), dtype=float)

V = np.zeros((num_steps, num_steps, num_steps))

for d in run_data:
    x = int((d[0][0] + disp)/float(args.stepSize))
    y = int((d[0][1] + disp)/float(args.stepSize))
    z = int((d[0][2] + disp)/float(args.stepSize))
    V[x,y,z] = d[1]

ipv.figure()
ipv.volshow(V, level=[0.25, 0.75], opacity=0.03, level_width=0.1, data_min=0.7, data_max=1)
ipv.view(-30, 40)
ipv.show()

# print(run_data)
# print(len(run_data))
sorted_data = sorted(run_data, key=lambda d:d[0])
# print([ d[0][2] for d in sorted_data])

sorted_ssims = [ d[1] for d in sorted_data ]

# X = np.array([v[0] for v in vectors])
# Y = np.array([v[1] for v in vectors])
# Z = np.array([v[2] for v in vectors])

print(Z.flatten())

fig = go.Figure(data=go.Volume(
    x=X.flatten(),
    y=Y.flatten(),
    z=Z.flatten(),
    value=np.array(sorted_ssims),
    isomin=0.9,
    isomax=1.0,
    opacity=0.1, # needs to be small to see through all surfaces
    surface_count=32, # needs to be a large number for good volume rendering
    ))

fig.show()
