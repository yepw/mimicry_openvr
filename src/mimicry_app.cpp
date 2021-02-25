#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <thread>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openvr.h>

#include "mimicry_openvr/json.hpp"
#include "mimicry_openvr/mimicry_app.hpp"


using json = nlohmann::json;
typedef vr::TrackedDeviceIndex_t DevIx;
typedef vr::VRControllerState_t DeviceState;
typedef vr::EVRButtonId ButtonId;


std::map<std::string, ButtonId> VRButton::KEY_TO_ID = {
	{"APP_MENU", vr::k_EButton_ApplicationMenu},
	{"GRIP", vr::k_EButton_Grip},
	{"AXIS0", vr::k_EButton_Axis0},
	{"AXIS1", vr::k_EButton_Axis1},
	{"AXIS2", vr::k_EButton_Axis2},
	{"AXIS3", vr::k_EButton_Axis3},
	{"AXIS4", vr::k_EButton_Axis4}
};

VRDevice::DeviceRole roleNameToEnum(std::string name)
{
    VRDevice::DeviceRole role;

    if (name == "left") {
        role = VRDevice::DeviceRole::LEFT;
    }
    else if (name == "right") {
        role = VRDevice::DeviceRole::RIGHT;
    }
    else if (name == "tracker") {
        role = VRDevice::DeviceRole::TRACKER;
    }
    else {
        role = VRDevice::DeviceRole::INVALID;
    }

    return role;
}

std::string roleEnumToName(VRDevice::DeviceRole role) {
	std::string name;

	switch (role)
	{
		case VRDevice::DeviceRole::LEFT:
		{
			name = "left";
		}	break;

		case VRDevice::DeviceRole::RIGHT:
		{
			name = "right";
		}	break;

		case VRDevice::DeviceRole::TRACKER:
		{
			name = "tracker";
		}	break;

		case VRDevice::DeviceRole::INVALID:
		{
			name = "invalid";
		}	break;
	}

	return name;
}


/**
 * Print a string to the screen.
 * 
 * Params:
 * 		text - text to print
 * 		newlines - number of newlines to print at the end of input string
 * 		flush - whether to flush text. Generally, only needed when
 * 			printing a large body of text with manual newlines
 */
void printText(std::string text="", int newlines=1, bool flush=false)
{
    // TODO: Consider adding param for width of text line
    std::cout << text;

    for (int i = 0; i < newlines; i++) {
        if (i > 1) {
            std::cout << "\n";
        }
        else {
            std::cout << std::endl;
        }
    }

    if (flush) {
        std::cout.flush();
    }

    return;
}

/**
 * Find a device within the list indicated that matches the given role.
 * It's expected that there is only one controller for each role of left
 * and right, though no validation is performed. For trackers, the first
 * one found will be returned.
 * 
 * Params:
 * 		role - role to search for
 * 		from_active - if true, will only search list of active devices.
 * 			Otherwise, only the list of inactive devices is searched
 * 
 * Returns: Pointer to device if found, NULL otherwise.
 **/
VRDevice * MimicryApp::findDevFromRole(VRDevice::DeviceRole role, bool from_active)
{
	if (from_active) {
		std::map<DevIx, VRDevice *>::iterator it(m_devices.begin());
		for ( ; it != m_devices.end(); ++it) {
			if (it->second->role == role) {
				return it->second;
			}
		}
	}
	else {
		std::map<std::string, VRDevice *>::iterator it(m_inactive_dev.begin());
		for ( ; it != m_inactive_dev.end(); ++it) {
			if (it->second->role == role) {
				return it->second;
			}
		}
	}

	return NULL;
}

/**
 * Move a device from the list of inactive devices to the list of active 
 * devices.
 * This is a helper function to perform the actual move and performs 
 * minimal validation--it should be called in the context of a broader
 * activation function. In particular, this function does not check
 * against the list of inactive devices.
 * 
 * Params:
 * 		dev - device to activate
 * 		ix - key to associate with the device
 **/
void MimicryApp::addDeviceToIndex(VRDevice *dev, DevIx ix) {
	if (dev != NULL && m_devices.find(ix) == m_devices.end()) {

		if (dev->role == VRDevice::DeviceRole::LEFT) {
			m_left_found = true;
		}
		else if (dev->role == VRDevice::DeviceRole::RIGHT) {
			m_right_found = true;
		}

		m_inactive_dev.erase(dev->name);
		m_devices[ix] = dev;
	}
}

/**
 * Given the index for a device in OpenVR, performs validation steps and
 * moves the device from the inactive list to the active list.
 * 
 * Params:
 * 		ix - OpenVR index for device
 * 
 * Returns: Pointer to device if found, NULL otherwise.
 **/
VRDevice * MimicryApp::activateDevice(DevIx ix) 
{
	VRDevice *dev = NULL;
	vr::ETrackedDeviceClass dev_class(m_vrs->GetTrackedDeviceClass(ix));

	if (dev_class = vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
		vr::ETrackedControllerRole role(m_vrs->GetControllerRoleForTrackedDeviceIndex(ix));
		
		if (role == vr::TrackedControllerRole_LeftHand) {
			VRDevice *left_contr(findDevFromRole(VRDevice::DeviceRole::LEFT, false));
			addDeviceToIndex(left_contr, ix);
			dev = left_contr;
		}
		else if (role == vr::TrackedControllerRole_RightHand) {
			VRDevice *right_contr(findDevFromRole(VRDevice::DeviceRole::RIGHT, false));
			addDeviceToIndex(right_contr, ix);
			dev = right_contr;
		}
		else {
			// Controller was found but could not be identified, so it will remain inactive
		}
	}
	else if (dev_class == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) {
		VRDevice *tracker(findDevFromRole(VRDevice::DeviceRole::TRACKER, false));
		addDeviceToIndex(tracker, ix);
		dev = tracker;
	}

	return dev;
}

/**
 * Move device at given index from active list to inactive list.
 * 
 * Params:
 * 		ix - index of device to deactivate
 **/
void MimicryApp::deactivateDevice(DevIx ix)
{
	if (m_devices.find(ix) != m_devices.end()) {
		VRDevice *dev(m_devices[ix]);

		if (dev->role == VRDevice::DeviceRole::LEFT) {
			m_left_found = false;
		}
		else if (dev->role == VRDevice::DeviceRole::RIGHT) {
			m_right_found = false;
		}

		m_inactive_dev[dev->name] = dev;
		m_devices.erase(ix);
	}
}

/**
 * Validate relevant types of analog input based on the properties of the
 * button and store the appropriate values.
 * 
 * Params:
 * 		button - button to validate
 * 		axis - struct containing analog data
 * 		prop - type of button
 **/
void handleButtonByProp(VRButton *button, vr::VRControllerAxis_t axis, int prop)
{
	switch (prop)
	{
		case vr::k_eControllerAxis_TrackPad:
		case vr::k_eControllerAxis_Joystick:
		{
			button->touch_pos = Vec2D(axis.x, axis.y);
		}	break;

		case vr::k_eControllerAxis_Trigger:
		{
			button->pressure = axis.x;
		}	break;

		case vr::k_eControllerAxis_None:
		{
			
		}	break;
	}
}

/**
 * Get position vector from OpenVR device pose matrix.
 * 
 * Params:
 * 		matrix - pose matrix
 * 
 * Returns: position vector.
 **/
Vec3D getPositionFromPose(vr::HmdMatrix34_t matrix) 
{
	Vec3D pos;

	pos.vec[0] = matrix.m[0][3];
	pos.vec[1] = matrix.m[1][3];
	pos.vec[2] = matrix.m[2][3];

	return pos;
}

/**
 * Get orientation quaternion from OpenVR device pose matrix.
 * 
 * Params:
 * 		matrix - pose matrix
 * 
 * Returns: orientation quaternion.
 **/
Quaternion getOrientationFromPose(vr::HmdMatrix34_t matrix) 
{
	Quaternion quat;

	quat.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	quat.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	quat.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	quat.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
	quat.x = copysign(quat.x, matrix.m[2][1] - matrix.m[1][2]);
	quat.y = copysign(quat.y, matrix.m[0][2] - matrix.m[2][0]);
	quat.z = copysign(quat.z, matrix.m[1][0] - matrix.m[0][1]);

	return quat;
}

/**
 * Process configuration file and configure all parameters to the app,
 * including devices and buttons.
 * 
 * Params:
 * 		filename - path to the configuration file
 * 
 * Returns: true if all parameters read successfully, false otherwise.
 **/
bool MimicryApp::readParameters(std::string filename) {
	json j;
	std::ifstream in_file;

	in_file.open(filename);
	if (!in_file.is_open()) {
		printText("Unable to open parameter file.");
		goto param_exit;
	}

	in_file >> j;

	m_params.bimanual = j["_bimanual"];
	m_params.num_devices = j["_num_devices"];
	m_params.out_addr = j["_out_addr"];
	m_params.out_port = j["_out_port"];
	m_params.vibration_port = j["_vibr_port"];
	m_params.update_freq = j["_update_freq"];

	if (m_params.num_devices <= 0 || m_params.num_devices > vr::k_unMaxTrackedDeviceCount) {
		printText("Invalid number of devices specified.");
		goto param_exit;
	}
	if (m_params.num_devices != (j.size() - m_params.NUM_PARAMS)) {
		printText("The number of devices configured does not match '_num_devices'.");
		goto param_exit;
	}
	if (m_params.bimanual && m_params.num_devices < 2) {
		printText("At least 2 devices must be specified for bimanual control.");
		goto param_exit;
	}

	for (int i = 0; i < m_params.num_devices; ++i) {
		VRDevice *dev(new VRDevice());
		std::string cur_dev("dev" + std::to_string(i));

		dev->name = j[cur_dev]["_name"];
		dev->role = roleNameToEnum(j[cur_dev]["_role"]);

		if (dev->role == VRDevice::DeviceRole::TRACKER) {
			continue;
		}

		dev->track_pose = j[cur_dev]["_track_pose"];

		if (m_inactive_dev.find(dev->name) != m_inactive_dev.end()) {
			printText("Duplicate device name specified: " + dev->name);
			goto param_exit;
		}

		switch (dev->role)
		{
			case VRDevice::DeviceRole::LEFT:
			{
				if (m_left_config) {
					printText("Multiple left controllers specified.");
					goto param_exit;
				}
				m_left_config = true;
			}	break;

			case VRDevice::DeviceRole::RIGHT:
			{
				if (m_right_config) {
					printText("Multiple right controllers specified.");
					goto param_exit;
				}
				m_right_config = true;
			}	break;

			case VRDevice::DeviceRole::TRACKER:
			{

			}	break;

			default:
			{
				printText("Invalid device role for " + dev->name);
				goto param_exit;
			}
		}

		json::iterator it(j[cur_dev]["buttons"].begin());
		for ( ; it != j[cur_dev]["buttons"].end(); ++it) {
			std::string but_key(it.key());

			if (VRButton::KEY_TO_ID.find(but_key) == VRButton::KEY_TO_ID.end()) {
				printText("Invalid button ID: " + but_key);
				goto param_exit;
			}

			vr::EVRButtonId cur_but = VRButton::KEY_TO_ID.at(but_key);
			VRButton *but(new VRButton());
			but->id = cur_but;

			but->name = j[cur_dev]["buttons"][but_key]["name"];
			
			std::map<ButtonId, VRButton *>::iterator but_it = dev->buttons.begin();
			for ( ; but_it != dev->buttons.end(); ++but_it) {
				if (but_it->second->name == but->name) {
					printText("Duplicate button name: " + but->name + " for device " + dev->name);
					goto param_exit;
				}
			}

			json::iterator t_it(j[cur_dev]["buttons"][but_key]["types"].begin());
			json::iterator t_it_end(j[cur_dev]["buttons"][but_key]["types"].end());
			for ( ; t_it != t_it_end; ++t_it) {
				std::string cur_type(t_it.key());

				if (cur_type != "boolean" && cur_type != "pressure" && cur_type != "2d") {
					printText("Invalid button data input type: " + cur_type);
					goto param_exit;
				}

				but->val_types[cur_type] = j[cur_dev]["buttons"][but_key]["types"][cur_type];
			}
			
			dev->buttons[cur_but] = but;
		}

		m_inactive_dev[dev->name] = dev;
	}

	if (m_params.bimanual && (!m_left_config || !m_right_config)) {
		printText("Bimanual mode was specified, but left and right controllers are not\n", 0);
		printText("both configured.");
		goto param_exit;
	}

	m_configured = true;

param_exit:
	if (in_file.is_open()) {
		in_file.close();
	}
	return m_configured;
}



/**
 * Perform initialization steps for the mimicry_control app.
 * 
 * Params:
 * 		filename - name of parameter file
 * 
 * Returns: true if initialization successful, false otherwise.
 **/
bool MimicryApp::appInit(std::string filename)
{
	if (!readParameters(filename)) {
		return false;
	}

	int opt = 1;
	sockaddr_in address;

	if ((m_socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
		printText("Could not initialize socket.");
		return false;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(m_params.out_port);

	if (m_params.out_addr == "") {
		address.sin_addr.s_addr = INADDR_ANY; 
	}
	else {
		if(inet_pton(AF_INET, m_params.out_addr.c_str(), &address.sin_addr) <= 0)  
    	{ 
			printText("Invalid address specified."); 
			return false; 
    	} 
	}
   
    if (connect(m_socket, (struct sockaddr *)&address, sizeof(address)) < 0) 
    { 
		std::string err_str = strerror(errno);
        printText("Socket connection failed.");
		printText("Error: " + err_str);
        return false; 
    } 

	// Convert refresh frequency from Hz to actual time for each loop
	this->m_refresh_time = std::chrono::duration<double, std::milli>(1000 / m_params.update_freq);

	return true;
}

/**
 * Process state data for all configured devices, along with any events
 * or other input.
 **/
void MimicryApp::handleInput()
{
	// Mark all devices as deactivated and reactivate them if still connected
	std::map<DevIx, VRDevice *>::iterator it(m_devices.begin());
	for ( ; it != m_devices.end(); ++it) {
		deactivateDevice(it->first);
	}

	for (unsigned ix = 0; ix < vr::k_unMaxTrackedDeviceCount; ++ix) {
		if (!m_vrs->IsTrackedDeviceConnected(ix)) {
			continue;
		}

		vr::TrackedDevicePose_t dev_pose;
		DeviceState dev_state;
		m_vrs->GetControllerStateWithPose(vr::TrackingUniverseStanding, ix, &dev_state, 
			sizeof(dev_state), &dev_pose);

		if (!dev_pose.bPoseIsValid) {
			continue;
		}

		VRDevice *dev = activateDevice(ix);
		if (dev == NULL) {
			continue;
		}

		// Exclude trackers from button handling
		if (dev->role == VRDevice::DeviceRole::LEFT || dev->role == VRDevice::DeviceRole::RIGHT) {
			std::map<ButtonId, VRButton *>::iterator it(dev->buttons.begin());
			for ( ; it != dev->buttons.end(); ++it) {
				VRButton *button(it->second);
					
				ButtonId id(it->first);
				// NOTE: OpenVR has built-in pressure thresholds to identify a button as pressed
				bool pressed((vr::ButtonMaskFromId(id) & dev_state.ulButtonPressed) != 0);

				switch (id)
				{
					case vr::k_EButton_ApplicationMenu:
					{
						button->pressed = pressed;
					}	break;

					case vr::k_EButton_Grip:
					{
						button->pressed = pressed;
					}	break;

					case vr::k_EButton_Axis0:
					{
						button->pressed = pressed;
						int prop(m_vrs->GetInt32TrackedDeviceProperty(ix, 
							(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + 0)));
						handleButtonByProp(button, dev_state.rAxis[0], prop);
					}	break;

					case vr::k_EButton_Axis1:
					{
						button->pressed = pressed;
						int prop(m_vrs->GetInt32TrackedDeviceProperty(ix, 
							(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + 1)));
						handleButtonByProp(button, dev_state.rAxis[1], prop);
					}	break;

					case vr::k_EButton_Axis2:
					{
						button->pressed = pressed;
						int prop(m_vrs->GetInt32TrackedDeviceProperty(ix, 
							(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + 2)));
						handleButtonByProp(button, dev_state.rAxis[2], prop);
					}	break;

					case vr::k_EButton_Axis3:
					{
						button->pressed = pressed;
						int prop(m_vrs->GetInt32TrackedDeviceProperty(ix, 
							(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + 3)));
						handleButtonByProp(button, dev_state.rAxis[3], prop);
					}	break;

					case vr::k_EButton_Axis4:
					{
						button->pressed = pressed;
						int prop(m_vrs->GetInt32TrackedDeviceProperty(ix, 
							(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + 4)));
						handleButtonByProp(button, dev_state.rAxis[4], prop);
					}	break;

				}
			}
		}
		
		dev->pose.pos = getPositionFromPose(dev_pose.mDeviceToAbsoluteTracking);
		dev->pose.quat = getOrientationFromPose(dev_pose.mDeviceToAbsoluteTracking);
	}	
}

/**
 * Publish state data for all configured devices in JSON format.
 **/
void MimicryApp::postOutputData()
{
	json j;

	if (m_params.bimanual && (!m_left_found || !m_right_found)) {
		printText("No data published due to missing devices.");
		return;
	}

	if (m_devices.size() == 0) {
		printText("No devices are currently active.");
		return;
	}

	std::map<DevIx, VRDevice *>::iterator it(m_devices.begin());
	for ( ; it != m_devices.end(); ++it) {
		VRDevice *dev(it->second);
		
		j[dev->name]["_role"] = roleEnumToName(dev->role);

		VRPose dev_pose = dev->pose;
		j[dev->name]["pose"]["position"]["x"] = dev_pose.pos.x;
		j[dev->name]["pose"]["position"]["y"] = dev_pose.pos.y;
		j[dev->name]["pose"]["position"]["z"] = dev_pose.pos.z;

		j[dev->name]["pose"]["orientation"]["x"] = dev_pose.quat.x;
		j[dev->name]["pose"]["orientation"]["y"] = dev_pose.quat.y;
		j[dev->name]["pose"]["orientation"]["z"] = dev_pose.quat.z;
		j[dev->name]["pose"]["orientation"]["w"] = dev_pose.quat.w;

		if (dev->role == VRDevice::DeviceRole::TRACKER) {
			continue;
		}

		// if (dev->role == VRDevice::DeviceRole::RIGHT) {
		// 	DevIx ix(it->first);
		// 	m_vrs->TriggerHapticPulse(ix, ButtonId::k_EButton_SteamVR_Touchpad, 1000);
		// }

		std::map<ButtonId, VRButton *>::iterator b_it(dev->buttons.begin());
		for ( ; b_it != dev->buttons.end(); ++b_it) {
			VRButton *button(b_it->second);

			std::map<std::string, bool>::iterator t_it(button->val_types.begin());
			for ( ; t_it != button->val_types.end(); ++t_it) {
				std::string type(t_it->first);
				bool enabled(t_it->second);

				if (enabled) {
					if (type == "boolean") {
						j[dev->name][button->name]["boolean"] = button->pressed;
					}
					else if (type == "pressure") {
						j[dev->name][button->name]["pressure"] = button->pressure;
					}
					else if (type == "2d") {
						j[dev->name][button->name]["2d"]["x"] = button->touch_pos.x;
						j[dev->name][button->name]["2d"]["y"] = button->touch_pos.y;						
					}
				}				
			}
		}
	}
	
	std::string output(j.dump(3));
	send(m_socket, output.c_str(), output.size(), 0);
	printText(output);
}

void MimicryApp::handleVibration()
{
	while (!m_configured && m_running) {} // Wait for configuration
	
}

void MimicryApp::handleSigint(int sig)
{
	MimicryApp::m_running = false;
}

bool MimicryApp::m_running = false;

/**
 * Entry point for the mimicry_control application.
 * 
 * Params:
 * 		params_file - name of the parameter file
 **/
void MimicryApp::runMainLoop(std::string params_file)
{
	vr::EVRInitError vr_err(vr::VRInitError_None);
	auto start(std::chrono::high_resolution_clock::now());
	auto end(std::chrono::high_resolution_clock::now());

	printText("Initializing mimicry_control...");

	// Load the SteamVR Runtime
	m_vrs = vr::VR_Init(&vr_err, vr::VRApplication_Background);
	// NOTE: VRApplication_Background requires an SteamVR instance to already be running.
	// In theory, the other modes will automatically launch SteamVR, but that was not the
	// case for me.

	MimicryApp::m_running = true;
	std::thread handle_vibration(&MimicryApp::handleVibration, this);

    if (vr_err != vr::VRInitError_None) {
		printText("Unable to init VR runtime: ", 0);
		printText(vr::VR_GetVRInitErrorAsEnglishDescription(vr_err));
        goto shutdown;
	}

	if (!appInit(params_file)) {
		goto shutdown;
	}

	signal(SIGINT, MimicryApp::handleSigint);

	while (MimicryApp::m_running) {
		std::chrono::duration<double, std::milli> time_elapsed = end - start;
		std::chrono::duration<double, std::milli> delta = this->m_refresh_time - time_elapsed;
		// TODO: Do something to deal with negative values?
		std::this_thread::sleep_for(delta);

		start = std::chrono::high_resolution_clock::now();
		handleInput();
		postOutputData();
		end = std::chrono::high_resolution_clock::now();
	}

shutdown:
	MimicryApp::m_running = false;
	handle_vibration.join();

    if (m_vrs != NULL) {
        vr::VR_Shutdown();
        m_vrs = NULL;
    }
	printText("Exiting VR system...");
	return;
}
