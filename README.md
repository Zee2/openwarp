![logo](resources/logo.png)
[![NCSA licensed](https://img.shields.io/badge/license-NCSA-blue.svg)](LICENSE)
[![development build-gl](https://github.com/Zee2/openwarp/workflows/development%20build-gl/badge.svg)](https://github.com/Zee2/openwarp/actions)
[![master build-gl](https://github.com/Zee2/openwarp/workflows/master%20build-gl/badge.svg)](https://github.com/Zee2/openwarp/actions)


# Overview

Openwarp is the first public implementation of spatio-temporal reprojection available under a free and open source license. Intended for use in AR/VR runtimes or devices, spatial reprojection can reduce tracking latency, improve the quality and stability of AR objects, and compensate for dropped frames in a client application. The shaders are freely available under the NCSA license, with proper attribution. While several large proprietary runtimes already implement some form of spatial reprojection, Openwarp is the first public, free, and open-source implementation available to developers. 

## Mesh-based and raymarch-based

Openwarp includes two different algorithms for spatial reprojection. One is a mesh-based system, and another is a raymarching-based system. The mesh-based system is typically more performant than the raymarch system (for equivalent quality settings), but can fail to accurately reproject fine details due to the limitation of the mesh resolution. Both algorithms offer several customizable parameters that can be adjusted to the developers' liking, offering tradeoffs between performance, quality, and accuracy.

## Building from source

This project uses CMake, and is properly configured to build on both Linux and Windows. (Windows has an issue with the framebuffer objects not behaving properly; can cause issues when rendering is freezed in the demo application.) For Linux, install the xorg and OpenGL dependencies with
```
sudo apt-get install xorg-dev libgl-dev
```
Most Ubuntu distributions should include all other dependencies, and Windows should not require any other dependencies. GLEW, GLFW, Eigen, GLM, and IMGUI are all built from source in this project, and are included as git submodules. You'll need to run `git pull --recurse-submodules` to pull them down. `mkdir` a `./build/` directory, and then run `cmake ..` in that directory and compile with `make`.

## Demo application

Included is a demo application that visualizes the effects and benefits of spatial reprojection. You can switch between the two reprojection algorithms (mesh-based and raymarch-based), as well as adjust the parameters of each reprojection algorithm on the fly. In addition, you can adjust the rendering framerate of the "application", as well as freeze the rendering entirely.

```
usage: ./openwarp [-h] [-mesh integer] [-disp displacement] [-step stepSize] [-output outputDir]

Run the Openwarp demo application, with optional automation.

optional arguments:
  -h            Show this help message and exit
  -mesh         Specify the width of the reprojection mesh for openwarp-mesh.
                Defaults to 1024x1024.
  -disp         Specify the max reprojection displacement of the automated test
                run. If this is specified, you also need to specify -step.
  -step         Specify the step size of the automated test run. If this is
                specified, you also need to specify -disp.
  -output       Specify the output directory for the automated test run. If
                this is specified, you also need to specify -disp and -step.
```

## Analysis

The SSIM (structural similary index metric) of the reprojection algorithms can be analyzed in an automated fashion with the `ssim.py` script located in the `./analysis` folder. This will run Openwarp with an automated test configuration, and collect + dump reprojected frames. It performs SSIM analysis on each frame position and plots the SSIM in 3D space, with respect to the three-DoF displacement of the head pose with respect to the rendered frame's pose. The usage of the `ssim.py` script can be seen as follows:
```
usage: ssim.py [-h] [--norun] [--usecache] displacement stepSize

Perform automated SSIM analysis of Openwarp output.

positional arguments:
  displacement  Maximum displacement from the rendered pose to reproject
  stepSize      Distance between each analyzed reprojected frame in 3D space
                (i.e., the size of each step)

optional arguments:
  -h, --help    show this help message and exit
  --norun       Do not run Openwarp; instead, use the latest previous run
  --usecache    Do not run SSIM analysis; instead, use cached SSIM data from a
                previous analysis run
```

## Issues and contributing

Please file issues if you are having trouble running Openwarp, or if you have any issues with integrating the shaders themselves into your project.

I may not have time to accept or review large PRs right now (very busy!) but small bugfixes are always welcome.

## Paper

This is the reproduceable, open-source code component of an undergraduate thesis. The associated paper will be available upon publication.
