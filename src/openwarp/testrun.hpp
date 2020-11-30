#pragma once
#include "openwarp.hpp"
#include <vector>
#include <Eigen/Dense>
#include <string>
#include "util/lib/iterator_tpl.h"

namespace Openwarp {
    class TestRun {

        public:
            TestRun();
            TestRun(double displacement, double stepSize, std::string outputDir = "../output", bool useRay=false, bool axisMode = false,
                Eigen::Vector3f startPos = Eigen::Vector3f{1.3,1.5,2.2},
                Eigen::Quaternionf startOrientation = Eigen::AngleAxisf(M_PI/8, Eigen::Vector3f::UnitY())
                                                    * Eigen::AngleAxisf(-M_PI/10, Eigen::Vector3f::UnitX())
                                                    
                                                    * Eigen::Quaternionf::Identity());
            
            // AR
            // Eigen::Vector3f startPos = Eigen::Vector3f{-0.601,0.138,0.912},
            // Eigen::AngleAxisf(-M_PI * 0.23, Eigen::Vector3f::UnitY())
            // * Eigen::AngleAxisf(-M_PI * 0.03, Eigen::Vector3f::UnitX())
            // * Eigen::Quaternionf::Identity());
            
            // vangogh
            // Eigen::Vector3f startPos = Eigen::Vector3f{-0.798,1.377,3.1},
            // Eigen::AngleAxisf(0, Eigen::Vector3f::UnitY())
            // * Eigen::AngleAxisf(-M_PI/16, Eigen::Vector3f::UnitX())
            // * Eigen::Quaternionf::Identity());

            // couch_2
            // Eigen::Vector3f startPos = Eigen::Vector3f{-2.2,0.8,-0.7},
            // Eigen::AngleAxisf(-M_PI/5, Eigen::Vector3f::UnitY())
            // * Eigen::AngleAxisf(-M_PI/15, Eigen::Vector3f::UnitX())
            
            // * Eigen::Quaternionf::Identity());

            // couch_good
            // Eigen::Vector3f startPos = Eigen::Vector3f{1.3,1.5,2.2},
            //     Eigen::Quaternionf startOrientation =   
            //     Eigen::AngleAxisf(M_PI/8, Eigen::Vector3f::UnitY())
            //     * Eigen::AngleAxisf(-M_PI/10, Eigen::Vector3f::UnitX())
                
            //     * Eigen::Quaternionf::Identity());

            const pose_t startPose;
            const bool useRay;
            const std::string outputDir;
            const double displacement, stepSize;

            size_t GetNumPoints() { return testPoses.size(); }

            struct it_state {
                int pos;
                inline void next(const TestRun* ref) { ++pos; };
                inline void begin(const TestRun* ref) { pos = 0; };
                inline void end(const TestRun* ref) { pos = ref->testPoses.size(); };
                inline const pose_t& get(const TestRun* ref) { return ref->testPoses[pos]; };
                inline bool cmp(const it_state& s) const { return pos != s.pos; };
            };
            SETUP_ITERATORS(TestRun, pose_t&, it_state);

        private:
            void generatePoints(float displacement, float stepSize);
            int pos;
            std::vector<pose_t> testPoses;
    };
}
