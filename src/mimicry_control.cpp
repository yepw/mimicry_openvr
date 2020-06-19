#include <iostream>

#include "mimicry_openvr/json.hpp"
#include "mimicry_openvr/mimicry_app.hpp"


using json = nlohmann::json;


// TODO: Get parameters for:
// - Parameter file location
// - Verbose output?
// - Demo mode (to test button config)
// - Flag that program is running sucessfully

int main(int argc, char* argv[]) 
{
	MimicryApp app = MimicryApp();

	std::cout << "Initializing mimicry..." << std::endl;
	app.runMainLoop("../param_files/params.json");
}