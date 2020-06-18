#include <iostream>
#include <fstream>

#include <openvr.h>
#include "mimicry_openvr/json.hpp"
#include "mimicry_openvr/param_info.hpp"

using json = nlohmann::json;
using ButtonInfo = mimicry::ButtonInfo;
using DeviceInfo = mimicry::DeviceInfo;
using ParamList = mimicry::ParamList;


// -- Helper functions --

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

DeviceInfo::DeviceRole checkRole(std::string in) {
    DeviceInfo::DeviceRole role;
    int val = std::stoi(in);

    switch(val)
    {
        case 1:
        {
            role = DeviceInfo::DeviceRole::LEFT;
        }   break;
        
        case 2:
        {
            role = DeviceInfo::DeviceRole::RIGHT;
        }   break;

        case 3:
        {
            role = DeviceInfo::DeviceRole::TRACKER;
        }   break;
    }

    return role;
}

std::string roleEnumToName(DeviceInfo::DeviceRole role)
{
    std::string role_name;

    switch(role)
    {
        case DeviceInfo::DeviceRole::LEFT:
        {
            return "left";
        }   break;

        case DeviceInfo::DeviceRole::RIGHT:
        {
            return "right";
        }   break;

        case DeviceInfo::DeviceRole::TRACKER:
        {
            return "tracker";
        }   break;
    }

    return role_name;
}

bool checkHelp(std::string in, std::string help_text, ParamList::EntryType type) 
{
    bool help_triggered = false;
    std::string entry = in;
    toLowercase(entry);
    
    if (entry == "?" || entry == "h") {
        help_triggered = true;
        std::cout << "\n" << help_text << "\n";

        switch (type)
        {
            case ParamList::BOOLEAN:
            {
                std::cout << "You may enter true/false, yes/no, t/f, or y/n.\n";
            }   break;
        
            default:
            {
            }   break;
        }

        std::cout << std::endl;
    }

    if (entry == "q") {
        std::cout << "Exiting..." << std::endl;
        exit(0);
    }

    return help_triggered;
}

bool validateEntry(std::string &entry, ParamList::EntryType type, int max_val=0)
{
    bool entry_valid = false;

    switch (type) 
    {
        case ParamList::BOOLEAN:
        {
            if (entry.empty()) {
                std::cout << "No response entered." << std::endl;
                return false;
            }

            if (checkTrue(entry)) {
                entry = "true";
                entry_valid = true;
            }
            else if (checkFalse(entry)) {
                entry = "false";
                entry_valid = true;
            }
            else {
                std::cout << "Value entered must be a boolean." << std::endl;
            }
        }   break;

        case ParamList::INTEGER:
        {
            if (entry.empty()) {
                std::cout << "No value entered." << std::endl;
                return false;
            }

            std::string::const_iterator it = entry.begin();
            for ( ; it != entry.end(); it++) {
                if (!std::isdigit(*it)) {
                    std::cout << "Value entered must be an integer." << std::endl << std::endl;
                    return false;
                }
            }
            
            int value = std::stoi(entry);
            if (!value || value <= 0) {
                std::cout << "Value entered is not valid." << std::endl << std::endl;
                return false;
            }

            if (max_val != 0 && value > max_val) {
                std::cout << "Value entered is too large." << std::endl << std::endl;
                return false;
            }

            entry_valid = true;
            
        }   break;

        case ParamList::TEXT:
        {
            if (entry.empty()) {
                std::cout << "No text entered." << std::endl;
                return false;
            }

            std::string::const_iterator it = entry.begin();
            for ( ; it != entry.end(); it++) {
                if (std::isspace(*it)) {
                    std::cout << "Value entered should not contain spaces." << std::endl << std::endl;
                    return false;
                }
            }

            // TODO: Check that name hasn't been entered previously

            entry_valid = true;
        }   break;
    }

    return entry_valid;
}


// -- Action functions --

std::string getEntry(bool lowercase=true)
{
    char entry_line[256];
    std::string cur_entry;

    std::cin.getline(entry_line, 256);
    cur_entry = entry_line;

    if (lowercase) {
        toLowercase(cur_entry);
    }
    return cur_entry;
}   

std::string promptUser(std::string query, std::string help_text, ParamList::EntryType type,
        bool lower=true, int max_val=0)
{
    bool valid_entry = false;
    std::string cur_entry;

    while (!valid_entry) {
        std::cout << query << std::endl;
        std::cout << "Entry: ";
        cur_entry = getEntry(lower);

        if (!checkHelp(cur_entry, help_text, type)) {
            valid_entry = validateEntry(cur_entry, type, max_val);
        }

        std::cout << std::endl;
    }

    return cur_entry;
}

void promptButtonTypes(ButtonInfo &button)
{
    std::string cur_entry;
    bool valid_entry = false;

    // Display options
    std::string query = 
        "Enter a comma- or space-separated list of types of values to return\n"
        "for this button (e.g., \"1, 2, 3\" or \"2 4\").";
    std::string help_text =
        "This prompt only accepts numeric values from those listed.\n"
        "These types specify the kind of data that will be returned for each\n"
        "button. Note that not all buttons support all types of return value.\n"
        "This tool does not perform validation on the types permitted.\n\n"
        "Types:\n"
        " - Boolean: button is pressed [true], not pressed [false].\n"
        " - Pressure: float indicating how far button is pressed.\n"
        " - 2D: float pair in the format \"x:[x_val],y:[y_val]\".\n"
        " - 3D: float triple in the format \"x:[x_val],y:[y_val],z:[z_val]\".";
    
    while (!valid_entry) {
        std::cout << query << "\n";
    
        // List all valid types
        for (int i = 0; i < ButtonInfo::BUT_TYPE_NUM; i++) {
            std::cout << i + 1 << ". " << ButtonInfo::BUT_TYPE_NAMES[i] << "\n";
        }

        std::cout << std::endl << "Entry: ";
        cur_entry = getEntry();

        if (cur_entry == "q") {
            std::cout << "Exiting..." << std::endl;
            exit(0);
        }

        if (checkHelp(cur_entry, help_text, ParamList::TEXT)) {
            std::cout << std::endl;
            continue;
        }

        int val;
        std::string val_str;
        std::string::const_iterator it = cur_entry.begin();
        for ( ; it != cur_entry.end(); it++) {
            if (std::isdigit(*it)) {
                char next = *(it + 1);
                if ((it + 1) != cur_entry.end() && !std::isspace(next) && next != ',') {
                    std::cout << "* Invalid entry *\n" << std::endl;
                    valid_entry = false;
                    break;
                }

                val_str = *it;
                val = std::stoi(val_str);
                if (val == 0 || val > ButtonInfo::BUT_TYPE_NUM) {
                    std::cout << val << " is not a valid value.\n" << std::endl;
                    valid_entry = false;
                    break;
                }

                button.val_types[val - 1] = true;
                valid_entry = true;
            }
            else if (std::isspace(*it) || *it == ',') {
                continue;
            }
            else {
                std::cout << "* Invalid entry *\n" << std::endl;
                valid_entry = false;
                break;
            }
        }
    }
}

void promptButtons(DeviceInfo &dev)
{
    std::string cur_entry;

    std::string query = "Set up button ";
    std::string help = 
        "If true, you will be able to assign a custom name to this button\n"
        "as well as the type of data you expect to receive from it. Skip\n"
        "configuration for this button otherwise.";
    for (int i = 0; i < ButtonInfo::BUT_NUM; i++) {
        ButtonInfo button;
        std::string cur_but = ButtonInfo::BUT_NAMES[i];

        std::cout << query << cur_but << " (for " << dev.name << ")?" << std::endl;
        std::cout << "Entry: ";
        cur_entry = getEntry();

        if (checkTrue(cur_entry)) {
            std::string but_query;
            std::string but_help;
            but_query = "Custom name for " + cur_but;
            but_help =
                "The name by which you would like to identify this button in the\n"
                "output for mimicry_control.";
            std::cout << std::endl;
            button.name = promptUser(but_query, but_help, ParamList::TEXT, false);

            promptButtonTypes(button);

            button.id = ButtonInfo::ButtonID(i);
            button.configured = true;
            dev.buttons.push_back(button);
        }
        else if (checkFalse(cur_entry)) {
            button.configured = false;
            dev.buttons.push_back(button);
        }
        else {
            if (!checkHelp(cur_entry, help, ParamList::BOOLEAN)) {
                std::cout << "Value entered must be a boolean." << std::endl;
            }

            i--; // Keep repeating this entry until input is valid
        }

        std::cout << std::endl;
    }
}

void outputToFile(ParamList params) 
{
    json j;
    j["_num_devices"] = params.num_devices;
    j["_update_freq"] = params.update_freq;
    j["_all_required"] = params.all_required;

    for (int i = 0; i < params.devices.size(); i++) {
        std::string dev_ix = "dev" + std::to_string(i);
        DeviceInfo cur_dev = params.devices[i];

        j[dev_ix]["_name"] = cur_dev.name;
        j[dev_ix]["_pose"] = cur_dev.track_pose;
        j[dev_ix]["_role"] = roleEnumToName(cur_dev.role);

        std::vector<ButtonInfo>::iterator it = cur_dev.buttons.begin();
        for ( ; it != cur_dev.buttons.end(); it++) {
            ButtonInfo cur_but = *it;
            std::string but_id = cur_but.BUT_NAMES[cur_but.id];

            // Skip buttons that are not configured
            if (!cur_but.configured) {
                continue;
            }

            j[dev_ix]["buttons"][but_id]["name"] = cur_but.name;

            for (int v = 0; v < cur_but.BUT_TYPE_NUM; v++) {
                std::string but_name = cur_but.BUT_TYPE_NAMES[v];
                bool type_valid = cur_but.val_types[v];

                j[dev_ix]["buttons"][but_id]["types"][but_name] = type_valid;
            }
        }
    }

    std::string filename = "params.json";
    std::ofstream out_file;
    out_file.open(filename);
    out_file << j.dump(4);
    out_file << std::endl;
    out_file.close();
    std::cout << " * Data written to " << filename << " *" << std::endl;
}


int main(int argc, char* argv[])
{
    ParamList params;
    ParamList::EntryType type;
    std::string query;
    std::string help_text;

    std::string welcome_text =
        "=============================================\n"
        "== mimicry_control Configuration File Tool ==\n"
        "=============================================";
    std::cout << std::endl << welcome_text << std::endl << std::endl;
    std::cout << "Enter \"?\" or \"h\" for help." << std::endl << std::endl;

    // -- all_required --
    query = "Require all devices to be active?";
    help_text =
        "Suggested: true\n"
        "If this is set to true, there will be no output unless the number\n"
        "of devices specified next are active. This is particularly useful\n"
        "for bimanual robot control.";
    type = ParamList::BOOLEAN;
    params.all_required = checkTrue(promptUser(query, help_text, type));

    // -- update_freq --
    query = "How often should data be updated (in Hz)?";
    help_text =
        "Suggested: 30\n"
        "This parameter indicates the refresh rate for acquiring input from\n"
        "the devices specified next and publishing them.";
    type = ParamList::INTEGER;
    params.update_freq = std::stoi(promptUser(query, help_text, type, true, 1000));

    // -- num_devices --
    query = "How many devices will you track?";
    help_text =
        "Suggested: 2\n" 
        "This value will generally be 1 or 2, for the left and right\n"
        "controllers, but other devices may be tracked if SteamVR is able\n"
        "to identify them.";
    type = ParamList::INTEGER;
    params.num_devices = std::stoi(promptUser(query, help_text, type, true, vr::k_unMaxTrackedDeviceCount));

    // -- Devices configuration --
    for (uint i = 0; i < params.num_devices; i++) {
        std::cout << "\n" << "Device " << i << " configuration" << std::endl << std::endl;
        DeviceInfo cur_dev;

        // -- device name --
        query = "Enter a name to identify this device.";
        help_text =
            "Each controller should have a unique name. This name is intended to\n"
            "make it easier for users to identify each controller.";
        type = ParamList::TEXT;
        cur_dev.name = promptUser(query, help_text, type, false);

        // -- device role --
        query = 
            "Select role for this device:\n"
            " 1. Left\n"
            " 2. Right\n"
            " 3. Tracker";
        help_text =
            "Controllers assigned to Left and Right will be matched to the roles\n"
            "assigned by OpenVR. Devices assigned as Tracker will only return pose\n"
            "data. Make sure to assign Left or Right to only one controller each.";
        type = ParamList::INTEGER;
        cur_dev.role = checkRole(promptUser(query, help_text, type, true, 3));

        // -- device track_pose --
        query = "Should pose be tracked for " + cur_dev.name + "?";
        help_text =
            "Indicates whether to return pose data for this device.";
        type = ParamList::BOOLEAN;
        cur_dev.track_pose = checkTrue(promptUser(query, help_text, type));

        // -- Buttons configuration --
        promptButtons(cur_dev);

        params.devices.push_back(cur_dev);
    }

    outputToFile(params);
}