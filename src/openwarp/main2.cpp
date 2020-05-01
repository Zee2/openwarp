#include "openwarp.hpp"
#include "vulkan_util.hpp"
#include "OpenwarpApplication.hpp"

int main() {
    Openwarp::OpenwarpApplication* app = new Openwarp::OpenwarpApplication(true);
    delete(app);
}