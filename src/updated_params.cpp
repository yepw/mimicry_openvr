#include <openvr.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>

#include "mimicry_openvr/mimicry_app.hpp"
#include "mimicry_openvr/updated_params.hpp"

#define REFRESH_RATE 120 // Polling rate (in Hz)


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

void printText(std::string text="", int newlines=1, bool flush=false)
{
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

            // TODO: Check that name hasn't been entered previously

            entry_valid = true;
        }   break;
    }

    return entry_valid;
}


void consumeEvents(ParamInfo &params)
{
    vr::VREvent_t event;

    while (params.vrs->PollNextEvent(&event, sizeof(event))) {
        switch (event.eventType)
        {
            case vr::VREvent_TrackedDeviceDeactivated:
            {
                if (event.trackedDeviceIndex == params.contr_ix) {
                    printText("Controller deactivated.");
                    checkController(params, params.cur_role);
                }
            }   break;
        }
	}
}

void checkController(ParamInfo &params, VRDevice::DeviceRole role)
{
    bool contr_found = false;
    vr::ETrackedDeviceClass dev_class;
    vr::ETrackedControllerRole dev_role;
    vr::ETrackedControllerRole in_role = roleToVREnum(role);
    
    printText("Looking for controller...", 0, true);
    while (!contr_found) {
        for (unsigned ix = 0; ix < vr::k_unMaxTrackedDeviceCount; ix++) {
            dev_class = params.vrs->GetTrackedDeviceClass(ix);
            dev_role = params.vrs->GetControllerRoleForTrackedDeviceIndex(ix);

            if (dev_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
                if (params.vrs->IsTrackedDeviceConnected(ix) && dev_role == in_role) {
                    contr_found = true;
                    params.contr_ix = ix;
                    params.cur_role = role;
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

int main(int argc, char *argv[])
{
    int err_val = 0;
    vr::EVRInitError vr_err = vr::VRInitError_None;
    std::string err_str;
    std::string query;
    std::string help_text;
    ParamInfo params;

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

    // Auto or Manual
    query = "Use auto configuration?";
    help_text =
        "If true, the tool will automatically check connection to the controllers and\n"
        "walk you through button configuration. Otherwise, you will be prompted for all\n"
        "configuration information.";
    params.auto_setup = promptBool(query, help_text); 

    if (params.auto_setup) {
        // Controllers Check
        query = "Configure bimanual control?";
        help_text =
            "If true, this tool will verify that there is both a left controller and a\n"
            "right controller connected. Otherwise, only the right controller will be\n"
            "configured.";
        params.bimanual = promptBool(query, help_text);
        
        if (params.bimanual) {
            params.num_devices = 2;
        }
        else {
            params.num_devices = 1;
        }

        // NOTE: The Right controller role appears to be the default
        printText("Configuring right controller...");
        checkController(params, VRDevice::RIGHT);

        // TODO: Prompt user to press each button in turn, detecting that it's been
        // held for a certain amount of time. Once detected, allow user to enter a
        // custom name and automatically detect types of input.

    }
    else {
        printText("Using manual setup...");
    }

shutdown:
    if (params.vrs != NULL) {
        vr::VR_Shutdown();
        params.vrs = NULL;
    }
	printText("Exiting VR system...");
	return err_val;
}