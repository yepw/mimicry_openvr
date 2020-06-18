#ifndef __PARAM_INFO_HPP__
#define __PARAM_INFO_HPP__

#include <iostream>
#include <vector>

namespace mimicry
{

struct ButtonInfo
{
    static const int BUT_NUM = 7;
    static const std::string BUT_NAMES[];

    enum ButtonID
    {
        APP_MENU = 0,
        GRIP,
        AXIS0,
        AXIS1,
        AXIS2,
        AXIS3,
        AXIS4
    };

    static const int BUT_TYPE_NUM = 3;
    static const std::string BUT_TYPE_NAMES[];

    enum ButtonType
    {
        V_BOOLEAN = 0,
        V_PRESSURE = 1,
        V_2D = 2,
    };

    std::string name;
    ButtonID id;
    bool configured;
    bool val_types[BUT_TYPE_NUM] = {};
};

const std::string ButtonInfo::BUT_NAMES[] = { "APP_MENU", "GRIP", "AXIS0", "AXIS1", 
    "AXIS2", "AXIS3", "AXIS4" };

const std::string ButtonInfo::BUT_TYPE_NAMES[] = { "boolean", "pressure", "2d" };

struct DeviceInfo
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
    std::vector<ButtonInfo> buttons;
};

struct ParamList
{
    enum EntryType
    {
        BOOLEAN,
        INTEGER,
        TEXT
    };

    int num_devices;
    bool all_required;
    int update_freq;
    std::vector<DeviceInfo> devices;
};


} // namespace mimicry

#endif // __PARAM_INFO_HPP__