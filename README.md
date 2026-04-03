# ArduinoAutomation

This repository contains three main parts that work together:

1. `LanceHome`
2. `mqtt`
3. `HomeAssistant`

Recommended setup order:
1. Configure and upload the Arduino/Controllino modules
2. Install and start the MQTT broker
3. Configure Home Assistant to use the MQTT entities

## 1. LanceHome

This is the Arduino/Controllino firmware part of the project.

What it contains:
- module sketches
- module-specific configuration
- custom Arduino libraries
- EEPROM provisioning flow for network and MQTT settings

Main files:
- [LanceHome README](/home/lance/Repositories/ArduinoAutomation/LanceHome/README.md)
- [Module 1 Sketch](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_1/LanceHomeModule_1.ino)
- [Module 1 Config](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_1/ModuleConfiguration.h)
- [Module 2 Sketch](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_2/LanceHomeModule_2.ino)
- [Module 2 Config](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_2/ModuleConfiguration.h)

Use this part to:
- install Arduino IDE and Controllino board support
- install required Arduino libraries
- install the custom ZIP libraries from this repo
- configure button mappings, outputs, zones, MQTT, and Ethernet
- provision EEPROM if MQTT/network mode is enabled
- upload the final runtime firmware to each module

Detailed guide:
- [LanceHome Setup Guide](/home/lance/Repositories/ArduinoAutomation/LanceHome/README.md)

## 2. MQTT

This is the local Mosquitto broker setup used by the Arduino modules and Home Assistant.

What it contains:
- Mosquitto config
- local startup script
- optional `systemd` service file

Main files:
- [MQTT README](/home/lance/Repositories/ArduinoAutomation/mqtt/README.md)
- [mosquitto.conf](/home/lance/Repositories/ArduinoAutomation/mqtt/mosquitto.conf)
- [start-local.sh](/home/lance/Repositories/ArduinoAutomation/mqtt/start-local.sh)
- [systemd service](/home/lance/Repositories/ArduinoAutomation/mqtt/arduinoautomation-mosquitto.service)

Use this part to:
- install or use Mosquitto
- start the broker locally
- run it automatically on boot if wanted
- verify broker credentials and port

Current default broker settings in this repo:
- port: `1883`
- persistence stored outside the repo
- local broker credentials shared with the Arduino modules

Detailed guide:
- [MQTT Setup Guide](/home/lance/Repositories/ArduinoAutomation/mqtt/README.md)

## 3. Home Assistant

This is the Home Assistant side of the integration.

What it contains:
- Home Assistant MQTT entity YAML files
- Home Assistant main configuration snippet
- sample automation

Main files:
- [Home Assistant README](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/README.md)
- [configuration.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/configuration.yaml)
- [automations.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/automations.yaml)
- [controllino_1.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_1.yaml)
- [controllino_2.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_2.yaml)

Use this part to:
- install MQTT integration in Home Assistant
- copy the provided config files into your Home Assistant config directory
- load MQTT entity definitions for both Controllino modules
- enable automations
- verify that lights, fans, and shades appear and respond

Detailed guide:
- [Home Assistant Setup Guide](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/README.md)

## Quick Start

If you want the short version:

1. Follow [LanceHome Setup Guide](/home/lance/Repositories/ArduinoAutomation/LanceHome/README.md)
   Configure each module and upload it.
2. Follow [MQTT Setup Guide](/home/lance/Repositories/ArduinoAutomation/mqtt/README.md)
   Start Mosquitto and verify the broker works.
3. Follow [Home Assistant Setup Guide](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/README.md)
   Add MQTT integration, copy config files, and restart Home Assistant.

## Important dependency order

- Home Assistant depends on MQTT
- MQTT and Home Assistant both depend on the Arduino modules using matching broker credentials and topic naming
- If you change Arduino device names, MQTT client IDs, zones, or entity layout, you may also need to update the Home Assistant MQTT YAML files

## Repository structure

```text
ArduinoAutomation/
  LanceHome/
  mqtt/
  HomeAssistant/
```
