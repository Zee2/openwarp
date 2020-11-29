
#include "testrun.hpp"
#include <cmath>

using namespace Openwarp;
using namespace Eigen;

TestRun::TestRun() : outputDir("../output"), useRay(false) {
    std::cout << "Default constructor" << std::endl;
    // Empty. No test data requested.
}

TestRun::TestRun(double displacement, double stepSize, std::string outputDir, bool useRay, bool singleAxis, Eigen::Vector3f axis,
                Eigen::Vector3f startPos,
                Eigen::Quaternionf startOrientation)
                 : startPose(pose_t { startPos, Eigen::Vector3f(0,0,0), startOrientation}), outputDir(outputDir), useRay(useRay) {
                    
    if(stepSize == 0) {
        // First, the origin/non-displaced pose
        testPoses.push_back(pose_t {
            startPos + (startOrientation * (0 * axis)),
            0 * axis,
            Quaternionf::Identity() * startOrientation});

        // Single displacement
        testPoses.push_back(pose_t {
            startPos + (startOrientation * (displacement * axis)),
            displacement * axis,
            Quaternionf::Identity() * startOrientation});

        return;
    }

    int num_steps = round(displacement/stepSize);

    if(singleAxis) {
        
        for(int i = -num_steps; i <= num_steps; i++){
            testPoses.push_back(pose_t {
                startPos + (startOrientation * (i*stepSize * axis)),
                i*stepSize * axis,
                Quaternionf::Identity() * startOrientation});
        }
        return;
    }

    // Iterate through all displacements.
    for(int x = -num_steps; x <= num_steps; x++){
        for(int y = -num_steps; y <= num_steps; y++){
            for(int z = -num_steps; z <= num_steps; z++){
                testPoses.push_back(pose_t {
                    startPos + (startOrientation * (stepSize * Vector3f{x,y,z})),
                    stepSize * Vector3f{x,y,z},
                    Quaternionf::Identity() * startOrientation});
            }
        }
    }
}

