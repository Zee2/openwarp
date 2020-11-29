#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"

#include <cstring>

using namespace Openwarp;

int main(int argc, char *argv[]) {

    std::vector<std::string> args(argv + 1, argv + argc);

    std::string usageMessage =
    "usage: ./openwarp [-h] [-mesh integer] [-disp displacement] [-step stepSize] [-output outputDir] [-ray] [-singleaxis Axis]\n\n"
    "Run the Openwarp demo application, with optional automation.\n\n"
    "optional arguments:\n"
    "  -h            Show this help message and exit\n"
    "  -mesh         Specify the width of the reprojection mesh for openwarp-mesh.\n"
    "                Defaults to 1024x1024.\n"
    "  -disp         Specify the max reprojection displacement of the automated test\n"
    "                run. If this is specified, you also need to specify -step.\n"
    "  -step         Specify the step size of the automated test run. If this is\n"
    "                specified, you also need to specify -disp.\n"
    "  -output       Specify the output directory for the automated test run. If\n"
    "                this is specified, you also need to specify -disp and -step.\n"
    "  -ray          Use raymarching instead of mesh-based reprojection.\n"
    "  -axis         Generate test points only along view-aligned axes\n";

    bool doTestRun = false;
    bool useRay = false;
    bool axisMode = false;
    double displacement = 0;
    double stepSize = 0;
    size_t meshSize = 512;
    bool showGUI = true;
    std::string outputDir = "../output";

    for(size_t i = 0; i < args.size(); i++){

        if(args[i].rfind("-h") == 0){
            std::cout << usageMessage;
            return 0;
        }

        if(args[i].rfind("-nogui") == 0){
            showGUI = false;
        }

        if(args[i].rfind("-ray") == 0){
            doTestRun = true;
            useRay = true;
        }

        if(args[i].rfind("-axis") == 0){
            doTestRun = true;
            axisMode = true;
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

    OpenwarpApplication app = OpenwarpApplication(meshSize);

    if(doTestRun) {
        TestRun test = TestRun(displacement, stepSize, outputDir, useRay, axisMode);
        std::cout << "Running automated test. " << test.GetNumPoints() << " poses to run." << std::endl;
        app.DoFullTestRun(test);
    } else {
        app.Run(showGUI);
    }
}