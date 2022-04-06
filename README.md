# mimicry_openvr

This package contains the mimicry_control functionality using the OpenVR API.
As of June 2020, only Linux is supported (tested under Ubuntu 18.04).


## Install

### Prerequisites:
* Install Steam
* Install SteamVR in Steam
* Install glm
 ```
 sudo apt install libglm-dev
 ```
* Install the [OpenVR package](https://github.com/ValveSoftware/openvr)
   1. Rollback to `v1.11.11`
   2. `cd openvr; mkdir build && cd build;`
   3. `cmake ..`
   4. `make install`

### mimicry_control package:
* Clone this repository into a local directory
* Create a `build` folder at the root of this package: `mkdir build && cd build`
* From the `build` folder, call cmake: `cmake ..`
* Make the package: `make`

## Running
* Currently, the program reads the `params.json` file from the `param_files` folder, though this will be changed to an argument to the program later. If you need to make any changes to the parameters file, make sure to edit the one in that folder
* SteamVR must be running in order for the program to run
* If you get 'Headset not detected (108)' when launching SteamVR, try
```
sudo chmod a+rw /dev/hidraw*
```
## Parameter File
The parameter file is a JSON-formatted set of configuration values for the program. This file allows the user to specify program-wide settings, as well as to apply custom names to the controllers and buttons (in order to make parsing output easier).

### Program-wide Settings
**_bimanual** (bool): Require exactly two controllers to be connected and active. If both controllers are not connected/active, no data will be published (including for tracking devices).
**_num_devices** (int): Number of devices configured in the parameter file. Only configured devices will return data.
**_out_addr** (string): Address to which to bind output data socket. An empty string indicates INADDR_ANY.
**_out_port** (int): Port to which to bind output data socket.
**_update_freq** (int): Rate at which to publish device data (in Hz).

### Device Settings
All devices (controllers and trackers) are configured as objects under a `dev#` attribute (where `#` represents a unique number for each device). 

**_name** (string): Custom name by which to identify device. The output JSON string will use this name to group the device data.
**_track_pose** (bool): Whether to publish the device's pose as part of the output data.
**_role** (string): The device's role. Valid values are `"left"`, `"right"`, and `"tracker"`.

### Button Settings
Button configuration is only valid for controllers (i.e., devices specified with a role of `left` or `right`). The set of buttons is configured as an object under a `buttons` attribute within the device object and the settings for each individual button are located under an attribute matching the name of the button in OpenVR. The valid names for buttons are: `APP_MENU`, `GRIP`, `AXIS0`, `AXIS1`, `AXIS2`, `AXIS3`, `AXIS4`.

**name** (string): Custom name by which to identify the button. The output JSON string will use this name to group data for the button.
**types** (object): Identifies what input types are valid for the button. The valid attributes are: `boolean` (is the button pressed/unpressed), `pressure` (how far is the button pressed, between 0.0 and 1.0), `2d`. Each attribute has a boolean value indicating whether the input type is valid.


## Output
The output of the program is a JSON-formatted string published through a socket using the address and port specified in the parameter file. An example of the output format is as follows:

```
{
   "[device0_name]": {
      "pose": {
         "orientation": {
            "w": 0.4908093512058258,
            "x": -0.015055283904075623,
            "y": 0.8651738166809082,
            "z": -0.10175364464521408
         },
         "position": {
            "x": -0.688750147819519,
            "y": 1.0423650741577148,
            "z": 0.1396269053220749
         }
      },
      "role": "left",
      "[device0_gripper]": {
         "boolean": false
      },
      "[device0_menu]": {
         "boolean": true
      },
      "[device0_trackpad]": {
         "2d": {
            "x": 0.0,
            "y": 0.0
         },
         "boolean": true
      },
      "[device0_trigger]": {
         "boolean": false,
         "pressure": 0.9254902601242065
      }
   },
   "[device1_name]": {
      ...
   }
}
```

