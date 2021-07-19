#ifndef __MIMICRY_APP_HPP__
#define __MIMICRY_APP_HPP__

#include <iostream>
#include <map>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <openvr.h>


typedef vr::TrackedDeviceIndex_t DevIx;
typedef vr::VRControllerState_t DevState;
typedef vr::EVRButtonId ButtonId;

struct VRPose
{
	glm::vec3 pos;
	glm::vec4 quat;
};

struct VRButton
{
	ButtonId id;
	std::string name;
	bool pressed;
	float pressure;
	glm::vec2 touch_pos;
	std::map<std::string, bool> val_types;

	static std::map<std::string, ButtonId> KEY_TO_ID;
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
	VRPose pose;
	DeviceRole role;
	std::map<ButtonId, VRButton *> buttons;
};

struct VRParams
{
	unsigned num_devices;
	bool bimanual;
	std::string out_addr;
	unsigned out_port, vibration_port;
	unsigned update_freq;

	const unsigned NUM_PARAMS = 6;
};

class MimicryApp
{
public:
	static bool m_running;
	
	MimicryApp() : m_configured(false), m_left_config(false), m_right_config(false),
			m_left_found(false), m_right_found(false), m_socket(0) {};

	void runMainLoop(std::string params_file);

private:
	vr::IVRSystem *m_vrs;
	bool m_configured;
	bool m_left_found;
	bool m_right_found;

	bool m_left_config;
	bool m_right_config;
	int m_socket, m_vibration_socket;
	VRParams m_params;
	std::map<std::string, VRDevice *> m_inactive_dev;
	std::map<DevIx, VRDevice *> m_devices;

	std::chrono::duration<double, std::milli> m_refresh_time;

	static void handleSigint(int sig);

	void addDeviceToIndex(VRDevice *dev, DevIx ix);
	VRDevice * findDevFromRole(VRDevice::DeviceRole role, bool from_active);
	DevIx findDevIndexFromRole(VRDevice::DeviceRole role);
	VRDevice * activateDevice(DevIx ix);
	void deactivateDevice(DevIx ix);

	bool appInit(std::string params_file);
	bool readParameters(std::string filename);
	void handleInput();
	bool processEvent(const vr::VREvent_t &event);
	void handleVibration();
	
	void postOutputData();
};

void printText(std::string text, int newlines, bool flush);
void handleButtonByProp(VRButton *button, vr::VRControllerAxis_t axis, int prop);
glm::vec3 getPositionFromPose(vr::HmdMatrix34_t matrix);
glm::vec4 getOrientationFromPose(vr::HmdMatrix34_t matrix);
std::string getSocketData(int socket, sockaddr_in &address);

#endif // __MIMICRY_APP_HPP__