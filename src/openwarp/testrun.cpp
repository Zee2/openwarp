
#include "testrun.hpp"
#include <cmath>

using namespace Openwarp;
using namespace Eigen;

TestRun::TestRun() : outputDir("../output"), useRay(false) {
    std::cout << "Default constructor" << std::endl;
    // Empty. No test data requested.
}

TestRun::TestRun(double displacement, double stepSize, std::string outputDir, bool useRay, bool axisMode,
                Eigen::Vector3f startPos,
                Eigen::Quaternionf startOrientation)
                 : startPose(pose_t { startPos, Eigen::Vector3f(0,0,0), startOrientation}), outputDir(outputDir), useRay(useRay) {

    int num_steps = round(displacement/stepSize);

    if(axisMode) {

        testPoses.push_back(pose_t {
            startPos + (startOrientation * Vector3f::Zero()),
            Vector3f::Zero(),
            Quaternionf::Identity() * startOrientation});

        Vector3f axis = Vector3f::UnitX();

        for(int i = -num_steps; i <= num_steps; i++){
            if(i == 0) continue;
            testPoses.push_back(pose_t {
                startPos + (startOrientation * (i*stepSize * Vector3f::UnitX())),
                i*stepSize * Vector3f::UnitX(),
                Quaternionf::Identity() * startOrientation});

            testPoses.push_back(pose_t {
                startPos + (startOrientation * (i*stepSize * Vector3f::UnitY())),
                i*stepSize * Vector3f::UnitY(),
                Quaternionf::Identity() * startOrientation});
            
            testPoses.push_back(pose_t {
                startPos + (startOrientation * (i*stepSize * Vector3f::UnitZ())),
                i*stepSize * Vector3f::UnitZ(),
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

