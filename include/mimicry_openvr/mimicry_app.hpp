#ifndef __MIMICRY_APP_HPP__
#define __MIMICRY_APP_HPP__

#include <iostream>
#include <map>
#include <chrono>
#include <sys/socket.h>

#include <openvr.h>


typedef vr::TrackedDeviceIndex_t DevIx;
typedef vr::VRControllerState_t DevState;
typedef vr::EVRButtonId ButtonId;

struct Pos2D
{
	float x;
	float y;

	Pos2D() { x = 0; y = 0; }
	Pos2D(float x_val, float y_val) : x(x_val), y(y_val) {}
};

struct VRButton
{
	ButtonId id;
	std::string name;
	bool pressed;
	float pressure;
	Pos2D touch_pos;
	std::map<std::string, bool> val_types;

	static std::map<std::string, ButtonId> KEY_TO_ID;
};

std::map<std::string, ButtonId> VRButton::KEY_TO_ID = {
	{"APP_MENU", vr::k_EButton_ApplicationMenu},
	{"GRIP", vr::k_EButton_Grip},
	{"AXIS0", vr::k_EButton_Axis0},
	{"AXIS1", vr::k_EButton_Axis1},
	{"AXIS2", vr::k_EButton_Axis2},
	{"AXIS3", vr::k_EButton_Axis3},
	{"AXIS4", vr::k_EButton_Axis4}
};

struct VRDevice
{
    enum DeviceRole
    {
        LEFT,
        RIGHT,
        TRACKER,
        INVALID
    };

	std::string name;
	bool track_pose;
	DeviceRole role;
	std::map<ButtonId, VRButton *> buttons;
};

struct VRParams
{
	int num_devices;
	bool bimanual;
	std::string out_addr;
	int out_port;
	int update_freq;
};

class MimicryApp
{
public:
	bool m_running;
	vr::IVRSystem *m_vrs;
	bool m_left_found;
	bool m_right_found;

	bool m_left_config;
	bool m_right_config;
	int m_socket;
	VRParams m_params;
	std::map<std::string, VRDevice *> m_inactive_dev;
	std::map<DevIx, VRDevice *> m_devices;
	
	MimicryApp() : m_running(false), m_left_config(false), m_right_config(false),
			m_left_found(false), m_right_found(false), m_socket(0) {
		m_params = {};
		};

	void runMainLoop(std::string params_file);

private:
	std::chrono::duration<double, std::milli> m_refresh_time;

	void addDeviceToIndex(VRDevice *dev, DevIx ix);
	VRDevice * findDevFromRole(VRDevice::DeviceRole role, bool from_active);
	VRDevice * activateDevice(DevIx ix);
	void deactivateDevice(DevIx ix);
	void handleButtonByProp(VRButton *button, vr::VRControllerAxis_t axis, int prop);

	bool appInit(std::string params_file);
	bool readParameters(std::string filename);
	void handleInput();
	bool processEvent(const vr::VREvent_t &event);
	
	void postOutputData();
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

#endif // __MIMICRY_APP_HPP__