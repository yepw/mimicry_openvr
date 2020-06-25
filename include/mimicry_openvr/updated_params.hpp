#ifndef __UPDATED_PARAMS_HPP__
#define __UPDATED_PARAMS_HPP__

#include <openvr.h>
#include <map>
#include <vector>

#include "mimicry_openvr/mimicry_app.hpp"

typedef std::chrono::duration<double, std::milli> duration;


struct ParamInfo
{
    vr::IVRSystem *vrs;
    std::string out_file;
    bool auto_setup;
    double refresh_time; // in useconds
    VRParams system;
    std::vector<VRDevice> devices;

    vr::TrackedDeviceIndex_t contr_ix;
    VRDevice::DeviceRole cur_role;
    vr::EVRButtonId cur_button;
    bool button_pressed;
    bool button_selected;
    double elapsed_time; // Time button has been pressed

    static const std::vector<vr::EVRButtonId> button_map;
    static const std::map<vr::EVRButtonId, std::string> button_names;
    static const unsigned NUM_BUTTONS = 7;
    bool config_buttons[NUM_BUTTONS];

    ParamInfo() : auto_setup(true) {}
};

const std::vector<vr::EVRButtonId> ParamInfo::button_map = 
{
    vr::k_EButton_ApplicationMenu,
    vr::k_EButton_Grip,
    vr::k_EButton_Axis0,
    vr::k_EButton_Axis1,
    vr::k_EButton_Axis2,
    vr::k_EButton_Axis3,
    vr::k_EButton_Axis4        
};

const std::map<vr::EVRButtonId, std::string> ParamInfo::button_names =
{
    {vr::k_EButton_ApplicationMenu, "APP_MENU"},
    {vr::k_EButton_Grip, "GRIP"},
    {vr::k_EButton_Axis0, "AXIS0"},
    {vr::k_EButton_Axis1, "AXIS1"},
    {vr::k_EButton_Axis2, "AXIS2"},
    {vr::k_EButton_Axis3, "AXIS3"},
    {vr::k_EButton_Axis4, "AXIS4"}
};


enum class EntryType
{
    BOOLEAN,
    INTEGER,
    TEXT
};


vr::ETrackedControllerRole roleToVREnum(VRDevice::DeviceRole role);
std::string boolToString(bool val);
void printText(std::string text, int newlines, bool flush);
void dots(unsigned freq);
void toLowercase(std::string &in);
bool checkTrue(std::string in);
bool checkFalse(std::string in);
bool checkHelp(std::string in, std::string help_text, EntryType type);
bool validateEntry(std::string &entry, EntryType type, int max_val);
bool validateSingleButtonPress(vr::EVRButtonId &cur_button);
bool isControllerConnected(const ParamInfo &params);
void buttonInfoQuery(const ParamInfo &params, VRDevice &controller, std::map<std::string, bool> values);


void checkController(ParamInfo &params);
void checkButton(ParamInfo &params, VRDevice &controller);
void promptButtonInfo(ParamInfo &params, VRDevice &controller);
std::string getEntry();
std::string promptUser(std::string query, std::string help_text, EntryType type, int max_val);
int promptInt(std::string query, std::string help_text, int max_val);
bool promptBool(std::string query, std::string help_text);
std::string promptText(std::string query, std::string help_text);
void writeToFile(const ParamInfo &params);


#endif // __UPDATED_PARAMS_HPP__