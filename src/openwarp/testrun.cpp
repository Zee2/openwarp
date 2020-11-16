
#include "testrun.hpp"

using namespace Openwarp;
using namespace Eigen;

TestRun::TestRun() : outputDir("../output"){
    std::cout << "Default constructor" << std::endl;
    // Empty. No test data requested.
}

TestRun::TestRun(float displacement, float stepSize, std::string outputDir,
                Eigen::Vector3f startPos,
                Eigen::Quaternionf startOrientation)
                 : startPose(pose_t { startPos, startOrientation}), outputDir(outputDir) {
    
    // Iterate through all displacements.
    for(float x = -displacement; x <= displacement; x += stepSize){
        for(float y = -displacement; y <= displacement; y += stepSize){
            for(float z = -displacement; z <= displacement; z += stepSize){
                testPoses.push_back(pose_t {startPos + (startOrientation * Vector3f{x,y,z}), Quaternionf::Identity() * startOrientation});
            }
        }
    }
}

