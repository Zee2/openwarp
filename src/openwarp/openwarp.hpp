#pragma once
#include <vector>
#include <optional>
#include <iostream>
#include <Eigen/Dense>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Openwarp{
    class OpenwarpApplication;
    class OpenwarpUtils;

    typedef struct pose_t {
        Eigen::Vector3f position;
        Eigen::Vector3f relative_pos;
        Eigen::Quaternionf orientation;
    } pose_t;
}