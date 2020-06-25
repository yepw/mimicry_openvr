#include <openvr.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <map>

#include "mimicry_openvr/json.hpp"
#include "mimicry_openvr/mimicry_app.hpp"
#include "mimicry_openvr/updated_params.hpp"

#define REFRESH_RATE 120 // Polling rate (in Hz)
#define BUTTON_PRESS_TIME 750 // How long buttons must be pressed (in ms)

using json = nlohmann::json;

vr::ETrackedControllerRole roleToVREnum(VRDevice::DeviceRole role)
{
    vr::ETrackedControllerRole out_role;

    switch (role)
    {
        case VRDevice::DeviceRole::LEFT:
        {
            return vr::TrackedControllerRole_LeftHand;
        } break;

        case VRDevice::DeviceRole::RIGHT:
        {
            return vr::TrackedControllerRole_RightHand;
        } break;

        default:
        {
            return vr::TrackedControllerRole_Invalid;
        }
    }
}

std::string roleEnumToName(VRDevice::DeviceRole role)
{
    std::string out_role;

    switch (role)
    {
        case VRDevice::DeviceRole::LEFT:
        {
            out_role = "left";
        }   break;

        case VRDevice::DeviceRole::RIGHT:
        {
            out_role = "right";
        }   break;

        case VRDevice::DeviceRole::TRACKER:
        {
            out_role = "tracker";
        }   break;

        default:
        {
            out_role = "invalid";
        }   break;
    }

    return out_role;
}

std::string boolToString(bool val)
{
    if (val) {
        return "true";
    }
    else {
        return "false";
    }
}

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

void dots(unsigned freq) 
{
    static unsigned tick;
    unsigned max_width = 75;

    if (freq == 0) {
        tick = 0; // Reset
    }
    else {
        ++tick;
        if ((tick % freq) == 0) {
            printText(".", 0, true);
        }
        if ((tick / freq) == max_width) {
            printText();
        }
    }
}

bool loadingBar(double goal, double tot_elapsed=0.0)
{
    static int ticks = 0;
    static const int BAR_WIDTH = 55;
    
    if (goal == 0.0) {
        ticks = 0;
        return false;
    }

    double bar_period = goal / BAR_WIDTH;

    if ((tot_elapsed / bar_period) > ticks) {
        ticks++;
        printText("=", 0, true);
    }

    if (ticks == BAR_WIDTH) {
        tot_elapsed = 0.0;
        ticks = 0;
        return false;
    }

    return true;
}

void toLowercase(std::string &in)
{
    std::transform(in.begin(), in.end(), in.begin(), 
        [](unsigned char c) { return std::tolower(c); });
}

bool checkTrue(std::string in)
{
    std::string entry = in;
    toLowercase(entry);

    if (entry == "true" || entry == "yes" || entry == "y" || entry == "t") {
        return true;
    }

    return false;
}

bool checkFalse(std::string in)
{
    std::string entry = in;
    toLowercase(entry);

    if (entry == "false" || entry == "no" || entry == "n" || entry == "f") {
        return true;
    }

    return false;
}

bool checkHelp(std::string in, std::string help_text, EntryType type) 
{
    bool help_triggered = false;
    std::string entry = in;
    toLowercase(entry);
    
    if (entry == "?" || entry == "h") {
        help_triggered = true;
        printText("\n" + help_text + "\n");

        switch (type)
        {
            case EntryType::BOOLEAN:
            {
                printText("You may enter true/false, yes/no, t/f, or y/n.\n", 0);
            }   break;
        }

        printText();
    }
    else if (entry == "q") {
        printText("Exiting...");
        exit(0);
    }

    return help_triggered;
}

bool validateEntry(std::string &entry, EntryType type, int max_val=0)
{
    bool entry_valid = false;

    if (entry.empty()) {
        printText("No response entered.");
        return false;
    }

    switch (type) 
    {
        case EntryType::BOOLEAN:
        {
            if (checkTrue(entry)) {
                entry = "true";
                entry_valid = true;
            }
            else if (checkFalse(entry)) {
                entry = "false";
                entry_valid = true;
            }
            else {
                printText("Value entered must be a boolean.", 2);
            }
        }   break;

        case EntryType::INTEGER:
        {
            std::string::const_iterator it = entry.begin();
            for ( ; it != entry.end(); it++) {
                if (!std::isdigit(*it)) {
                    printText("Value entered must be an integer.", 2);
                    return false;
                }
            }
            
            int value = std::stoi(entry);
            if (!value || value <= 0) {
                printText("Value entered is not valid.", 2);
                return false;
            }

            if (max_val != 0 && value > max_val) {
                printText("Value entered is too large.", 2);
                return false;
            }

            entry_valid = true;
            
        }   break;

        case EntryType::TEXT:
        {
            std::string::const_iterator it = entry.begin();
            for ( ; it != entry.end(); it++) {
                if (std::isspace(*it)) {
                    printText("Value entered should not contain spaces.", 2);
                    return false;
                }
            }

            entry_valid = true;
        }   break;
    }

    return entry_valid;
}

bool isControllerConnected(const ParamInfo &params)
{
    return params.vrs->IsTrackedDeviceConnected(params.contr_ix);
}


void checkController(ParamInfo &params)
{
    bool contr_found = false;
    vr::ETrackedDeviceClass dev_class;
    vr::ETrackedControllerRole dev_role;
    vr::ETrackedControllerRole in_role = roleToVREnum(params.cur_role);
    
    printText("Looking for controller...", 0, true);
    while (!contr_found) {
        for (unsigned ix = 0; ix < vr::k_unMaxTrackedDeviceCount; ix++) {
            dev_class = params.vrs->GetTrackedDeviceClass(ix);
            dev_role = params.vrs->GetControllerRoleForTrackedDeviceIndex(ix);

            if (dev_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
                if (params.vrs->IsTrackedDeviceConnected(ix) && dev_role == in_role) {
                    contr_found = true;
                    params.contr_ix = ix;
                }
            }
        }

        if (!contr_found) {
            dots(unsigned(REFRESH_RATE * 1.5));
            usleep(params.refresh_time);
        }
    }

    dots(0);
    printText();
    printText("Controller found.", 2);
}

void validateSingleButtonPress(ParamInfo &params)
{
    vr::VRControllerState_t contr_state;

    if (!isControllerConnected(params)) {
        checkController(params);
        params.button_pressed = false;
        return;
    }

    params.vrs->GetControllerState(params.contr_ix, &contr_state, sizeof(contr_state));

    std::vector<vr::EVRButtonId>::const_iterator it = params.button_map.begin();
    for ( ; it != params.button_map.end(); it++) {
        bool pressed = (vr::ButtonMaskFromId(*it) & contr_state.ulButtonPressed) != 0;

        if (pressed) {
            if (params.button_pressed && (*it != params.cur_button)) {
                params.button_pressed = false;
                return;
            }
            else {
                params.button_pressed = true;
                params.cur_button = *it;
            }
        }
        else {
            if ((*it == params.cur_button) && params.button_pressed) {
                params.button_pressed = false;
                return;
            }
        }
    }
}

void checkButton(ParamInfo &params, VRDevice &controller)
{
    params.button_pressed = false;
    params.button_selected = false;
    bool first_loop = true;
    bool counter_running = false;
    auto start = std::chrono::high_resolution_clock::now();

    while (!params.button_selected) {
        validateSingleButtonPress(params);

        if (params.button_pressed) {
            if (first_loop) {
                first_loop = false;
                counter_running = true;
                printText(params.button_names.at(params.cur_button) + " |", 0, true);
                start = std::chrono::high_resolution_clock::now();
            }

            auto current = std::chrono::high_resolution_clock::now();
            duration delta = current - start;
            if (!loadingBar(BUTTON_PRESS_TIME, delta.count())) {
                params.button_selected = true;
                printText("|", 2);

                if (controller.buttons.find(params.cur_button) != controller.buttons.end()) {
                    std::string query = 
                        "This button has already been configured. Would you like to\n"
                        "edit the entry for this button?";
                    std::string help_text =
                        "If true, you will be able to enter a new name for this button.";
                    bool edit_button = promptBool(query, help_text);

                    if (!edit_button) {

                        params.button_selected = false;
                        return;
                    }
                }

                promptButtonInfo(params, controller);
            }
        }
        else {
            first_loop = true;
            loadingBar(0.0);
            if (counter_running) {
                counter_running = false;
                printText("");
            }
        }
    }
}

void buttonInfoQuery(ParamInfo &params, VRDevice &controller, std::map<std::string, bool> types)
{
    std::string query;
    std::string help_text;
    VRButton *button_entry;
    vr::EVRButtonId button = params.cur_button;

    if (controller.buttons.find(button) != controller.buttons.end()) {
        button_entry = controller.buttons.at(button);
    }
    else {
        button_entry = new VRButton();
    }

    query =
        "Default data type configuration for " + params.button_names.at(button) + ":\n"
        "  - boolean: " + boolToString(types["boolean"]) + "\n"
        "  - pressure: " + boolToString(types["pressure"]) + "\n"
        "  - 2d: " + boolToString(types["2d"]) + "\n\n"
        "Enter a name for the button.";
    help_text =
        "Enter a custom name to identify this button. The output for the mimicry\n"
        "program will use this label.\n"
        "If the data type settings are incorrect, you may edit them in the parameter\n"
        "file. Alternatively, you may run this program in manual mode.";
    button_entry->name = promptText(query, help_text);
    button_entry->id = button;
    button_entry->val_types["boolean"] = types["boolean"];
    button_entry->val_types["pressure"] = types["pressure"];
    button_entry->val_types["2d"] = types["2d"];

    controller.buttons[button] = button_entry;
}

void promptButtonInfo(ParamInfo &params, VRDevice &controller)
{
    vr::EVRButtonId button = params.cur_button;
    int32_t axis_prop;
    std::map<std::string, bool> types;

    switch (button)
    {
        case vr::k_EButton_ApplicationMenu:
        case vr::k_EButton_Grip:
        {
            types["boolean"] = true;
            types["pressure"] = false;
            types["2d"] = false;
            buttonInfoQuery(params, controller, types);
        }	break;

        case vr::k_EButton_Axis0:
        case vr::k_EButton_Axis1:
        case vr::k_EButton_Axis2:
        case vr::k_EButton_Axis3:
        case vr::k_EButton_Axis4:
        {
            int axis_offset = button - vr::k_EButton_Axis0;
            axis_prop = params.vrs->GetInt32TrackedDeviceProperty(params.contr_ix, 
                (vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32 + axis_offset));

            switch (axis_prop)
            {
                case vr::k_eControllerAxis_TrackPad:
                {
                    types["boolean"] = true;
                    types["pressure"] = false;
                    types["2d"] = true;
                    buttonInfoQuery(params, controller, types);
                }   break;

                case vr::k_eControllerAxis_Joystick:
                {
                    types["boolean"] = false;
                    types["pressure"] = false;
                    types["2d"] = true;
                    buttonInfoQuery(params, controller, types);
                }   break;

                case vr::k_eControllerAxis_Trigger:
                {
                    types["boolean"] = true;
                    types["pressure"] = true;
                    types["2d"] = false;
                    buttonInfoQuery(params, controller, types);
                }   break;

                case vr::k_eControllerAxis_None:
                {
                    printText("Button types could not be assigned.");
                }   break;
            }
        }	break;

        default:
        {
            printText("Button could not be recognized.");
        }   break;
    }
}

std::string getEntry()
{
    char entry_line[256];
    std::string cur_entry;

    std::cin.getline(entry_line, 256);
    cur_entry = entry_line;

    return cur_entry;
}  

std::string promptUser(std::string query, std::string help_text, EntryType type, int max_val=0)
{
    bool valid_entry = false;
    std::string cur_entry;

    while (!valid_entry) {
        printText(query);
        printText("Entry: ", 0);
        cur_entry = getEntry();

        if (!checkHelp(cur_entry, help_text, type)) {
            valid_entry = validateEntry(cur_entry, type, max_val);
        }

        printText();
    }

    return cur_entry;
}

int promptInt(std::string query, std::string help_text, int max_val=0)
{
    int entry;
    entry = std::stoi(promptUser(query, help_text, EntryType::INTEGER, max_val));
    return entry;
}

bool promptBool(std::string query, std::string help_text)
{
    bool entry;
    entry = checkTrue(promptUser(query, help_text, EntryType::BOOLEAN));
    return entry;
}

std::string promptText(std::string query, std::string help_text)
{
    std::string entry;
    entry = promptUser(query, help_text, EntryType::TEXT);
    return entry;
}

void writeToFile(const ParamInfo &params)
{
    json j;

    j["_bimanual"] = params.system.bimanual;
    j["_num_devices"] = params.system.num_devices;
    j["_out_addr"] = params.system.out_addr;
    j["_out_port"] = params.system.out_port;
    j["_update_freq"] = params.system.update_freq;

    for (int i = 0; i < params.devices.size(); i++) {
        std::string dev_ix = "dev" + std::to_string(i);
        VRDevice cur_dev = params.devices[i];

        j[dev_ix]["_name"] = cur_dev.name;
        j[dev_ix]["_role"] = roleEnumToName(cur_dev.role);

        if (cur_dev.role == VRDevice::DeviceRole::TRACKER) {
            continue;
        }

        j[dev_ix]["_track_pose"] = cur_dev.track_pose;

        std::map<vr::EVRButtonId, VRButton*>::iterator it = cur_dev.buttons.begin();
        for ( ; it != cur_dev.buttons.end(); it++) {
            VRButton *cur_but = it->second;
            std::string but_id = params.button_names.at(cur_but->id);

            j[dev_ix]["buttons"][but_id]["name"] = cur_but->name;

            std::map<std::string, bool>::iterator t_it = cur_but->val_types.begin();
            for ( ; t_it != cur_but->val_types.end(); t_it++) {
                std::string but_name = t_it->first;
                bool type_valid = t_it->second;

                j[dev_ix]["buttons"][but_id]["types"][but_name] = type_valid;
            }
        }
    }

    std::ofstream out_file;
    out_file.open(params.out_file);
    out_file << j.dump(4) << std::endl;
    out_file.close();
    printText(" * Data written to " + params.out_file + " *");
}

int main(int argc, char *argv[])
{
    int err_val = 0;
    vr::EVRInitError vr_err = vr::VRInitError_None;
    std::string err_str;
    std::string query;
    std::string help_text;
    ParamInfo params;
    bool default_out_addr;
    bool default_out_port;
    bool add_trackers;
    bool customize_trackers;
    unsigned num_trackers;

    auto refresh_in_milli = std::chrono::duration<double, std::milli>(1000 / REFRESH_RATE);
    params.refresh_time = refresh_in_milli.count() * 1000;

    std::string welcome_text =
        "\n=============================================\n"
        "== mimicry_control Configuration File Tool ==\n"
        "=============================================\n\n"
        "Enter \"?\" or \"h\" for help.";
    printText(welcome_text, 2);

    // Load the SteamVR Runtime
	params.vrs = vr::VR_Init(&vr_err, vr::VRApplication_Background);

    if (vr_err != vr::VRInitError_None){
		printText("Unable to init VR runtime: ", 0);
        err_str = vr::VR_GetVRInitErrorAsEnglishDescription(vr_err);
		printText(err_str, 1);
        goto shutdown;
	}

    // Check arguments to program
    if (argc > 3) {
        printText("usage: ./updated_params [manual] [params_file.json]");
        goto shutdown;
    }

    params.out_file = "params.json";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "manual") == 0) {
            params.auto_setup = false;
        }
        else if (strstr(argv[i], ".json")) {
            params.out_file = argv[i];
        }
        else {
            printText("Invalid argument to program.");
            goto shutdown;
        }
    }

    if (params.auto_setup) {
        // -- param bimanual --
        query = "Configure bimanual control?";
        help_text =
            "If true, this tool will verify that there is both a left controller and a\n"
            "right controller connected. Otherwise, only the right controller will be\n"
            "configured.";
        params.system.bimanual = promptBool(query, help_text);
        
        // -- param num_devices --
        if (params.system.bimanual) {
            params.system.num_devices = 2;
        }
        else {
            params.system.num_devices = 1;
        }

        // NOTE: The Right controller role appears to be the default
        printText();
        printText("-- Right Controller Configuration --", 2);
        params.cur_role = VRDevice::RIGHT;
        checkController(params);

        VRDevice controller;
        controller.role = params.cur_role;

        printText("Configuring buttons...\n", 0);
        printText("Please press the button you would like to configure first.", 2);        
        checkButton(params, controller);

        query = "Configure another button?";
        help_text =
            "Specify whether more buttons should be configured.";
        while (promptBool(query, help_text)) {
            checkButton(params, controller);
        }

        // -- device name --
        query = "Enter a name to identify the right controller.";
        help_text =
            "Each controller should have a unique name. This name is intended to\n"
            "make it easier for users to identify each controller.";
        controller.name = promptText(query, help_text);

        // -- device track_pose --
        query = "Should pose be tracked for " + controller.name + "?";
        help_text =
            "Indicates whether to return pose data for this device.";
        controller.track_pose = promptBool(query, help_text);

        params.devices.push_back(controller);


        if (params.system.bimanual) {
            printText();
            printText("-- Left Controller Configuration --", 2);
            params.cur_role = VRDevice::LEFT;
            checkController(params);

            VRDevice controller;
            controller.role = params.cur_role;

            printText("Configuring buttons...\n", 0);
            printText("Please press the button you would like to configure first.", 2);        
            checkButton(params, controller);

            query = "Configure another button?";
            help_text =
                "Specify whether more buttons should be configured.";
            while (promptBool(query, help_text)) {
                checkButton(params, controller);
            }

            // -- device name --
            query = "Enter a name to identify the left controller.";
            help_text =
                "Each controller should have a unique name. This name is intended to\n"
                "make it easier for users to identify each controller.";
            controller.name = promptText(query, help_text);

            // -- device track_pose --
            query = "Should pose be tracked for " + controller.name + "?";
            help_text =
                "Indicates whether to return pose data for this device.";
            controller.track_pose = promptBool(query, help_text);

            params.devices.push_back(controller);
        }    
    }
    else {
        printText("Using manual setup...");
    }

    printText();
    printText("-- General Settings Configuration --", 2);

    // -- update param num_devices --
    query = "Would you like to configure tracker devices?";
    help_text =
        "If true, you will be able to configure tracker devices in addition to the\n"
        "controllers already configured. Tracker devices only return pose data.";
    add_trackers = promptBool(query, help_text);

    if (add_trackers) {
        query = "How many trackers should be configured?";
        help_text =
            "Enter the number of trackers to configure. Controllers should not be included\n"
            "in this number.";
        num_trackers = promptInt(query, help_text);

        query = "Customize tracker names?";
        help_text =
            "If true, you will enter a custom name for each tracker. Otherwise, each tracker\n"
            "will be automatically assigned a name in the format 'tracker#'.";
        customize_trackers = promptBool(query, help_text);

        for (int i = 0; i < num_trackers; i++) {
            VRDevice tracker;
            std::string tracker_name;

            tracker.role = VRDevice::DeviceRole::TRACKER;
            tracker_name = "tracker" + std::to_string(i);
            if (customize_trackers) {
                query = "Enter a name to identify " + tracker_name + ".";
                help_text =
                    "The name entered will be used to identify this tracker in the output.";
                tracker_name = promptText(query, help_text);
            }
            tracker.name = tracker_name;

            params.devices.push_back(tracker);
        }

        params.system.num_devices += num_trackers;
    }

    // -- param update_freq --
    query = "How often should data be updated (in Hz)?";
    help_text =
        "Suggested: 60\n"
        "This parameter indicates the refresh rate for acquiring and publishing\n"
        "input from the configured devices. A value of '0' indicates that the refresh\n"
        "should happen as fast as possible.";
    params.system.update_freq = promptInt(query, help_text);

    // -- param out_addr --
    query = "Use default address for data output socket?";
    help_text =
        "Suggested: yes\n"
        "The default address is a blank value, which indicates a value of INADDR_ANY\n"
        "for binding. If you need a different address for binding to the output socket,\n"
        "you will enter it next.";
    default_out_addr = promptBool(query, help_text);

    if (!default_out_addr) {
        query = "Enter the address to which to bind output socket.";
        help_text =
            "Custom address for binding output socket.";
        params.system.out_addr = promptText(query, help_text);
    }
    else {
        params.system.out_addr = "";
    }

    // -- param out_port --
    query = "Use default port for data output socket?";
    help_text =
        "Suggested: yes\n"
        "The default port for binding is set to the arbitrary value of 8080. If you\n"
        "would like a different port for binding to the output socket, you will enter\n"
        "it next.";
    default_out_port = promptBool(query, help_text);

    if (!default_out_port) {
        query = "Enter the port to which to bind output socket.";
        help_text =
            "Custom port for binding output socket.";
        params.system.out_port = promptInt(query, help_text);
    }
    else {
        params.system.out_port = 8080;
    }


    writeToFile(params);

shutdown:
    if (params.vrs != NULL) {
        vr::VR_Shutdown();
        params.vrs = NULL;
    }
	printText("Exiting configuration...");
	return err_val;
}