#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"

#include <cstring>

using namespace Openwarp;

int main(int argc, char *argv[]) {

    std::vector<std::string> args(argv + 1, argv + argc);

    bool debug = false;
    bool doTestRun = false;
    float displacement = 0;
    float stepSize = 0;
    size_t meshSize = 1024;
    std::string outputDir = "../output";

    for(size_t i = 0; i < args.size(); i++){
        if(args[i].compare("-debug") == 0){
            debug = true;
        }

        if(args[i].rfind("-mesh") == 0){
            if(i == args.size() - 1) {
                throw std::invalid_argument("Usage: -mesh [reprojection mesh dimension]");
            }
            std::stringstream stream(args[i+1]);
            if(!(stream >> meshSize)){
                throw std::invalid_argument("Usage: -mesh must be followed by a valid reprojection mesh size (integer)");
            }
        }

        if(args[i].rfind("-disp", 0) == 0){

            if(i == args.size() - 1) {
                throw std::invalid_argument("Usage: -disp [max displacement]");
            }

            std::stringstream stream(args[i+1]);
            if(!(stream >> displacement)){
                throw std::invalid_argument("Usage: -disp must be followed by a valid displacement value for test run.");
            }
            doTestRun = true;
        }

        if(args[i].rfind("-step", 0) == 0){

            if(i == args.size() - 1) {
                throw std::invalid_argument("Usage: -step [stepSize value]");
            }

            std::stringstream stream(args[i+1]);
            if(!(stream >> stepSize)){
                throw std::invalid_argument("Usage: -step must be followed by a valid stepSize value for test run.");
            }
            doTestRun = true;
        }

        if(args[i].rfind("-output", 0) == 0){

            if(i == args.size() - 1) {
                throw std::invalid_argument("Usage: -output [output directory, default ../output]");
            }

            outputDir = args[i+1];
            doTestRun = true;
        }
    }

    if((displacement == 0 || stepSize == 0) && doTestRun)
        throw std::runtime_error("Usage: Neither stepSize nor displacement can be zero, if provided.");

    OpenwarpApplication app = OpenwarpApplication(debug, meshSize);

    if(doTestRun) {
        TestRun test = TestRun(displacement, stepSize, outputDir);
        std::cout << "Running automated test. " << test.GetNumPoints() << " poses to run." << std::endl;
        app.DoFullTestRun(test);
    } else {
        app.Run();
    }
}