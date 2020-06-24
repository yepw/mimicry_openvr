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

struct Vec2D
{
	union 
	{
		struct 
		{
			float x, y;
		};
		float vec[2];
	};

	Vec2D() : x(0), y(0) {}
	Vec2D(float x_val, float y_val) : x(x_val), y(y_val) {}
};

struct Vec3D
{
	union 
	{
		struct 
		{
			float x, y, z;
		};
		float vec[3];
	};

	Vec3D() : x(0), y(0), z(0) {}
	Vec3D(float x_val, float y_val, float z_val) : x(x_val), y(y_val), z(z_val) {}
};

struct Quaternion
{
	union 
	{
		struct 
		{
			float x, y, z, w;
		};
		float vec[4];
	};

	Quaternion() : x(0), y(0), z(0), w(0) {}
	Quaternion(float x_val, float y_val, float z_val, float w_val) : x(x_val), y(y_val), 
		z(z_val), w(w_val) {}
};

struct VRPose
{
	Vec3D pos;
	Quaternion quat;
};

struct VRButton
{
	ButtonId id;
	std::string name;
	bool pressed;
	float pressure;
	Vec2D touch_pos;
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
	unsigned out_port;
	unsigned update_freq;
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

	bool appInit(std::string params_file);
	bool readParameters(std::string filename);
	void handleInput();
	bool processEvent(const vr::VREvent_t &event);
	
	void postOutputData();
};

#endif // __MIMICRY_APP_HPP__