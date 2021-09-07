

#include <string>
#include <iostream>

#include "control_application.h"

int main(int argc, char** argv){
    std::string config_str;
    std::string arg_config("--config");

    if (argc > 1) {
        std::string arg_val = argv[1];
        size_t start_pos = arg_val.find(arg_config);
        if (start_pos != std::string::npos) {
            start_pos += arg_config.size();
            if (arg_val[start_pos] == '=') {
                config_str = arg_val.substr(start_pos + 1);
            } else {
                std::cout << "The only correct argument syntax is --config=" << std::endl;
                return 0;
            }
        } else {
            std::cout << "The only acceptable argument is --config=" << std::endl;
            return 0;
        }
    } else {
        std::cout << "Please provide --config argument" << std::endl;
    }

    ControlApplication ca;
    ca.run(config_str);
}