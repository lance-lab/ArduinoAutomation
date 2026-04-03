LanceHome Setup Guide

This guide explains how to install the required Arduino tools, configure a module, write its network settings to EEPROM, and then run it normally.

Available sketches:
- `LanceHomeModule_1/LanceHomeModule_1.ino`
- `LanceHomeModule_2/LanceHomeModule_2.ino`

1. Install Arduino IDE

Install Arduino IDE from:
- https://www.arduino.cc/en/software

You can also use VS Code with the Arduino extension, but Arduino IDE is the simplest way to install boards and libraries the first time.

2. Install the Controllino board package

In Arduino IDE:
1. Open `Tools > Board > Boards Manager`
2. Search for `Controllino`
3. Install Controllino board support

After installation, select the correct board for your hardware.

Example used in this project:
- `CONTROLLINO MAXI`

3. Install required Arduino libraries

In Arduino IDE:
1. Open `Tools > Manage Libraries`
2. Install these libraries:

- `RingBuf` version `2.0.0`
- `PubSubClient` version `2.8.0`

Usually already available with Arduino:
- `SPI`
- `Ethernet`
- `EEPROM`

4. Install the custom project libraries

This project uses local custom libraries stored under:
- `LanceHome/libraries/CredentialManager`
- `LanceHome/libraries/LanceControllino`

You can install them through Arduino IDE as ZIP libraries:
1. Open `Sketch > Include Library > Add .ZIP Library...`
2. Select:
   - `LanceHome/libraries/CredentialManager.zip`
   - `LanceHome/libraries/LanceControllino.zip`

If you change the local library source later, rebuild the ZIP files before importing them again.

5. Open the module you want to upload

Choose one sketch:
- `LanceHomeModule_1/LanceHomeModule_1.ino`
- `LanceHomeModule_2/LanceHomeModule_2.ino`

Each module has its own settings file:
- `LanceHomeModule_1/ModuleConfiguration.h`
- `LanceHomeModule_2/ModuleConfiguration.h`

6. Configure the module

Open the matching `ModuleConfiguration.h` file and update the settings for your installation.

Important sections:
- `ANALOG_INPUT_ASSIGNMENT`
  - maps wall buttons to output ports
- `DIGITAL_OUTPUT_ASSIGNMENT`
  - maps each output to type, zone, and index
- `DEVICE_CONFIG`
  - contains device name, MQTT enable flag, MQTT settings, and Ethernet settings

Important `DEVICE_CONFIG` fields:
- `hostname`
  - friendly device name used for logs and identity
- `mqttEvents`
  - `true`: enable EEPROM, Ethernet, and MQTT startup
  - `false`: offline mode, network config is not required
- MQTT config:
  - MQTT username
  - MQTT password
  - broker IP
  - MQTT client ID
  - MQTT port
- Ethernet config:
  - MAC address
  - local IP
  - gateway IP
  - subnet mask

Offline-only example:

```cpp
static const DeviceConfig DEVICE_CONFIG = {
  "LanceControllino1",
  false,
  {},
  {}
};
```

7. Select board and serial port

In Arduino IDE:
1. Select the correct Controllino board
2. Select the correct serial port for the device

In this repo, VS Code is currently configured with:
- board: `CONTROLLINO_Boards:avr:controllino_maxi`
- port: `/dev/ttyACM0`

Those values may need to be changed on your machine.

8. Provision EEPROM when MQTT/network mode is enabled

If `DEVICE_CONFIG.mqttEvents` is `true`, the module needs its network and MQTT settings written to EEPROM once.

Open the module `.ino` file and find:

```cpp
#define PROVISION_EEPROM_ON_BOOT false
```

Provisioning procedure:
1. Change it to `true`
2. Upload the sketch once
3. This writes `DEVICE_CONFIG` to EEPROM
4. The runtime setup is skipped in this mode
5. Change it back to `false`
6. Upload the sketch again

Normal meaning of the flag:
- `false` = normal runtime startup
- `true` = write config to EEPROM and skip normal runtime

If `DEVICE_CONFIG.mqttEvents` is `false`, EEPROM/network provisioning is not needed.

9. Upload the final runtime sketch

After provisioning is complete:
1. Make sure `PROVISION_EEPROM_ON_BOOT` is `false`
2. Upload the sketch again
3. Open Serial Monitor if you want to verify startup logs

Expected normal runtime behavior:
- configuration is loaded from EEPROM
- Ethernet is initialized
- MQTT connects using the configured credentials

10. Repeat for the second module if needed

Each module has its own:
- button mapping
- output mapping
- device name
- MAC address
- IP address
- MQTT client ID

Do not reuse the same MAC address, IP address, or MQTT client ID across multiple devices.

11. Troubleshooting

If the module does not start correctly:
- confirm the correct Controllino board is selected
- confirm the correct serial port is selected
- confirm custom ZIP libraries are installed
- confirm `PROVISION_EEPROM_ON_BOOT` was set back to `false` after provisioning
- confirm MQTT broker IP, username, password, and port are correct
- confirm local IP and gateway are correct for your network
- if EEPROM contents may be outdated, provision again by setting `PROVISION_EEPROM_ON_BOOT` to `true` and uploading once

Project paths reference:
- `LanceHome/LanceHomeModule_1/LanceHomeModule_1.ino`
- `LanceHome/LanceHomeModule_1/ModuleConfiguration.h`
- `LanceHome/LanceHomeModule_2/LanceHomeModule_2.ino`
- `LanceHome/LanceHomeModule_2/ModuleConfiguration.h`
- `LanceHome/libraries/CredentialManager`
- `LanceHome/libraries/LanceControllino`
