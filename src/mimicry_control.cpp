#include <iostream>
#include <unistd.h>

#include "mimicry_openvr/json.hpp"
#include "mimicry_openvr/mimicry_app.hpp"


using json = nlohmann::json;


// TODO: Get parameters for:
// - Verbose output?
// - Specifying refresh rate of 0 should indicate as fast as possible
// - Add tracker configuration
// - Switch to glm vectors

int main(int argc, char* argv[]) 
{
	MimicryApp app = MimicryApp();
	
	chdir("../../../src/mimicry_openvr");

	std::string config_file;
	for (int i(0); i < argc; ++i) {
		std::string cur_arg(argv[i]);
		if (cur_arg.find(".json") != std::string::npos) {
			config_file = "param_files/" + cur_arg;
			break;
		}
	}

	if (config_file.empty()) {
		printf("No config_file was selected.\n");
	}

	app.runMainLoop(config_file);
}