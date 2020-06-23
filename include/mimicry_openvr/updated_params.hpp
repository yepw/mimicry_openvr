#ifndef __UPDATED_PARAMS_HPP__
#define __UPDATED_PARAMS_HPP__

#include <openvr.h>

#include "mimicry_openvr/mimicry_app.hpp"


struct ParamInfo
{
    vr::IVRSystem *vrs;
    bool auto_setup;
    bool bimanual;
    unsigned num_devices;
    double refresh_time; // in useconds

    vr::TrackedDeviceIndex_t contr_ix;
    VRDevice::DeviceRole cur_role;

    ParamInfo() : auto_setup(true) {}
};

enum class EntryType
{
    BOOLEAN,
    INTEGER,
    TEXT
};


vr::ETrackedControllerRole roleToVREnum(VRDevice::DeviceRole role);
void printText(std::string text="", int newlines, bool flush);
void dots(unsigned freq);
void toLowercase(std::string &in);
bool checkTrue(std::string in);
bool checkFalse(std::string in);
bool checkHelp(std::string in, std::string help_text, EntryType type);
bool validateEntry(std::string &entry, EntryType type, int max_val);

void consumeEvents(ParamInfo &params);
void checkController(ParamInfo &params, VRDevice::DeviceRole role);
std::string getEntry();
std::string promptUser(std::string query, std::string help_text, EntryType type, int max_val);
int promptInt(std::string query, std::string help_text, int max_val);
bool promptBool(std::string query, std::string help_text);
std::string promptText(std::string query, std::string help_text);


#endif // __UPDATED_PARAMS_HPP__