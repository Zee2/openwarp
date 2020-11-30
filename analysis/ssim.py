import argparse
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from mpl_toolkits.mplot3d import Axes3D
import plotly.graph_objects as go
from skimage import data, img_as_float
from skimage.metrics import structural_similarity as ssim
from skimage.metrics import mean_squared_error
import imageio
from mayavi import mlab
import _pickle
import concurrent.futures as fut
import sys
import math

import scipy.stats as stats

from flip import compute_flip
from utils import *

import wquantiles

import ipyvolume as ipv

from scipy.sparse import csr_matrix

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

    return ssim_warp, ssim_image

def ssim_thread(filename):
    vector = filename.replace('.png', '').split('_')
    truth_img = imageio.imread("ground_truth/" + filename)
    warp_img = imageio.imread("warped/" + filename)
    ssim_result,_ = run_ssim(truth_img,warp_img)
    vector = [float(i) for i in vector]
    print("SSIM for " + str(vector) + ": " + str(ssim_result));

    return ([vector, ssim_result])

# https://stackoverflow.com/a/26888164/3234070
def binned_statistic(x, values, func, nbins, range):
    '''The usage is nearly the same as scipy.stats.binned_statistic''' 

    N = len(values)
    r0, r1 = range

    digitized = (float(nbins)/(r1 - r0)*(x - r0)).astype(int)
    S = csr_matrix((values, [digitized, np.arange(N)]), shape=(nbins, N))

    return [func(group) for group in np.split(S.data, S.indptr[1:-1])]

def FLIP_thread(filename):
    result,_ = FLIP(filename, filename)
    return result

def FLIP(truth,warp):
    vector = truth.replace('.png', '').split('_')
    vector = [float(i) for i in vector]
    truth_img = load_image_array("ground_truth/" + truth)
    warp_img = load_image_array("warped/" + warp)

    pixels_wide = len(truth_img[0])
    fov = 90
    pixels_per_degree = pixels_wide / fov

    deltaE = compute_flip(truth_img, warp_img, pixels_per_degree)

    # fig=plt.figure()
    # plt.imshow(deltaE.squeeze(0))
    # plt.show()

    flatDeltaE = deltaE.flatten()

    print("Computing histogram for " + str(vector))
    histogram, edges = np.histogram(deltaE, bins=100, range=(0,0.9))
    print("Computing median of each bin for " + str(vector))
    medians_of_bins,_,_ = stats.binned_statistic(flatDeltaE, flatDeltaE, bins=100, statistic='median', range=(0,0.9))
    print("Weighting histogram bins for " + str(vector))

    weightedHistogram = np.empty(histogram.shape[0], dtype=float)
    megapixels = (pixels_wide ** 2) / 1000000

    for i in range(len(histogram)):

        # Empty bin
        if(math.isnan(medians_of_bins[i])):
            medians_of_bins[i] = 0

        weightedHistogram[i] = float(histogram[i]) * float(medians_of_bins[i])
    

    median = wquantiles.median(medians_of_bins, weightedHistogram)
    trueMedian = np.median(flatDeltaE)

    print("Width: " + str(pixels_wide) + ", PPD: " + str(pixels_per_degree) + ", megapixels: " + str(megapixels) + ", deltaE histogram median for " + str(vector) + ": " + str(median) + ", true median: " + str(trueMedian));

    # plt.plot(medians_of_bins, weightedHistogram)
    # plt.axvline(x=median)
    # plt.show()
    return [vector,median], deltaE.squeeze(0)



def crop_center(img, edge):
       y, x, _ = img.shape
       startx = edge
       starty = edge
       endx = x - edge
       endy = y - edge

       print("Bounds: " + str(startx) + ', ' + str(starty) + ', ' + str(endx) + ', ' + str(endy))
       return img[starty:endy, startx:endx, :]

parser = argparse.ArgumentParser(description='Perform automated image similarity analysis of Openwarp output. Uses Nvidia FLIP pooled deltaE, unless --ssim is specified.')
parser.add_argument('--disp', nargs='?', default=None, help='Maximum displacement from the rendered pose to reproject')
parser.add_argument('--stepSize', nargs='?', default=None, help='Distance between each analyzed reprojected frame in 3D space (i.e., the size of each step)')
parser.add_argument('--ssim', action="store_true", help="Use SSIM instead of Nvidia FLIP pooled DeltaE")
parser.add_argument('--norun', action="store_true", help='Do not run Openwarp; instead, use the latest previous run')
parser.add_argument('--usecache', action="store_true", help='Do not run SSIM analysis; instead, use cached SSIM data from a previous analysis run')
parser.add_argument('--single', action="store_true", help='Run on single frame, return SSIM image')
parser.add_argument('--graph', action="store_true", help="Graph SSIM on a flat chart instead of 3D volume")
parser.add_argument('--dir', nargs='?', default=None, help="Use a specific output directory")

args = parser.parse_args()

assert(not (args.dir is not None and args.norun == False))

orig_dir = os.getcwd()

if not args.norun:
    os.chdir('../build')
    os.system('make && ./openwarp -disp ' + displacement + ' -step ' + stepSize);
    os.chdir('../analysis')

os.chdir('../output')

if args.dir is None:
    run_dirs = [d for d in os.listdir('.') if os.path.isdir(d)]
    latest_run = max(run_dirs, key=os.path.getmtime);
else:
    latest_run = args.dir

print("Using run: " + latest_run)

os.chdir(latest_run)

run_info = open("run_info.txt")
origin = np.array(run_info.readline().split("_")).astype(np.float)
dispData = run_info.readline().split(" ")
disp = dispData[1].rstrip()
step = dispData[2].rstrip()
run_info.readline()
reprojection_type = run_info.readline().split(" ")[2].rstrip()
mesh_res = run_info.readline().split(" ")[2].rstrip()
run_info.close()
print("Run origin: " + str(origin))
print("Mesh res: " + str(mesh_res))
print("Repro type: " + str(reprojection_type))

ground_truth_files = os.listdir("./ground_truth/")
warp_files = os.listdir("./warped/")


# print("Ground truth files: " + str(ground_truth_files))


if args.single:

    fig=plt.figure(figsize=(20, 20))

    target = [0.1,0,0]

    for gt in ground_truth_files:
        vector = gt.replace('.png', '').split('_')
        if [float(i) for i in vector] == [0,0,0]:
            reference = gt

    for w in warp_files:
        vector = w.replace('.png', '').split('_')
        if [float(i) for i in vector] == target:
            targetWarp = w
    
    # targetWarp = warp_files[0]

    truth_img = imageio.imread("ground_truth/" + targetWarp)
    ref_image = imageio.imread("ground_truth/" + gt)
    warp_img = imageio.imread("warped/" + targetWarp)

    # truth_img = crop_center(truth_img, 100)
    # ref_image = crop_center(ref_image, 100)
    # warp_img = crop_center(warp_img, 100)

    flip_result, flip_image = FLIP(targetWarp, targetWarp)
    ssim_result, ssim_image = run_ssim(truth_img,warp_img)

    nowarp_ssim_result, nowarp_ssim_image = run_ssim(truth_img,ref_image)
    nowarp_flip_result, nowarp_flip_image = FLIP(targetWarp,gt)


    ax1 = fig.add_subplot(2,2,1)
    ax1.title.set_text("SSIM comparison, without reprojection")
    ax1.set(xlabel="SSIM: " + str(nowarp_ssim_result))
    plt.imshow(nowarp_ssim_image)
    ax2 = fig.add_subplot(2,2,2)
    ax2.set(xlabel="SSIM: " + str(ssim_result))
    ax2.title.set_text("SSIM comparison, reprojection enabled")
    plt.imshow(ssim_image)

    ax1 = fig.add_subplot(2,2,3)
    ax1.title.set_text("Nvidia FLIP comparison, without reprojection")
    ax1.set(xlabel="FLIP pooled deltaE: " + str(nowarp_flip_result))
    plt.imshow(nowarp_flip_image)
    ax2 = fig.add_subplot(2,2,4)
    ax2.set(xlabel="FLIP pooled deltaE: " + str(flip_result))
    ax2.title.set_text("Nvidia FLIP comparison, reprojection enabled")
    plt.imshow(flip_image)
    
    plt.show()
    sys.exit()

run_data = []

if args.usecache:
    cache = open('cache.p', 'rb')
    run_data = _pickle.load(cache)
    cache.close()
else:

    with fut.ThreadPoolExecutor(max_workers=8) as executor:
        results = executor.map(FLIP_thread, ground_truth_files)
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

disp = float(disp)
step = float(step)
num_steps = ((2.0 * disp) / step + 1.0)

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
    x = int((d[0][0] + disp)/step)
    y = int((d[0][1] + disp)/step)
    z = int((d[0][2] + disp)/step)
    # print("Putting " + str(d[0]) + " into " + str([x,y,z]))
    V[x,y,z] = d[1]

if args.graph:

    flattened = [d[1] for d in run_data]

    total_mean = np.mean(flattened)
    total_median = np.median(flattened)
    total_stdev = np.std(flattened)

    print("Mean: " + str(total_mean))
    print("Median: " + str(total_median))
    print("stdev: " + str(total_stdev))
    
    midpoint = len(X)//2

    fig, ax=plt.subplots(1,1)
    if(reprojection_type == "Mesh"):
        ax.set_title('Mesh-based, ' + str(mesh_res) + 'x' + str(mesh_res) + " mesh, max displacement " + str(disp) + 'm')
    else:
        ax.set_title('Raymarch-based, max displacement ' + str(disp) + 'm')
    ax.plot(np.arange(len(X)) * step - disp, V[:,midpoint,midpoint], 'r', label=r"$\Delta E$ along x-axis")
    ax.plot(np.arange(len(X)) * step - disp, V[midpoint,:,midpoint], 'g', label=r"$\Delta E$ along y-axis")
    ax.plot(np.arange(len(X)) * step - disp, V[midpoint,midpoint,:], 'b', label=r"$\Delta E$ along z-axis")
    ax.yaxis.set_major_locator(ticker.MultipleLocator(0.02))

    plt.legend()
    # plt.plot(range(len(Y)), V[0,:,0])
    # plt.plot(range(len(Z)), V[0,0,:])
    plt.xlabel('Per-axis displacement of reprojection (m)')
    plt.ylabel(r'Nvidia FLIP pooled $\Delta E$')
    plt.show()
    sys.exit()

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
