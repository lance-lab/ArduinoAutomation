# Home Assistant Setup Guide

This guide explains how to connect Home Assistant to the Controllino modules in this repository using MQTT.

Project files used by Home Assistant:
- [configuration.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/configuration.yaml)
- [automations.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/automations.yaml)
- [controllino_1.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_1.yaml)
- [controllino_2.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_2.yaml)

## 1. Install Home Assistant

Install Home Assistant using your preferred method:
- Home Assistant OS
- Home Assistant Container
- Home Assistant Supervised
- Home Assistant Core

Official installation docs:
- https://www.home-assistant.io/installation/

## 2. Install and configure an MQTT broker

This project expects Home Assistant and the Controllino modules to communicate through MQTT.

Common option:
- Mosquitto broker

If you use Home Assistant Add-ons:
1. Install the `Mosquitto broker` add-on
2. Start it
3. Create or reuse MQTT credentials

You will need:
- broker IP address
- MQTT username
- MQTT password
- port, usually `1883`

These values must match the module settings in:
- [ModuleConfiguration.h](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_1/ModuleConfiguration.h)
- [ModuleConfiguration.h](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_2/ModuleConfiguration.h)

## 3. Add MQTT integration in Home Assistant

In Home Assistant:
1. Open `Settings > Devices & Services`
2. Click `Add Integration`
3. Search for `MQTT`
4. Enter your broker connection details

Make sure Home Assistant can connect successfully before continuing.

## 4. Copy the Home Assistant config files

Copy this repo's Home Assistant files into your Home Assistant config directory.

Required files:
- `HomeAssistant/config/configuration.yaml`
- `HomeAssistant/config/automations.yaml`
- `HomeAssistant/config/mqtt/controllino_1.yaml`
- `HomeAssistant/config/mqtt/controllino_2.yaml`

Expected final structure in Home Assistant:

```text
config/
  configuration.yaml
  automations.yaml
  mqtt/
    controllino_1.yaml
    controllino_2.yaml
```

## 5. Configure `configuration.yaml`

Your Home Assistant `configuration.yaml` should include:

```yaml
mqtt: !include_dir_merge_list mqtt/
automation: !include automations.yaml
```

In this repo that is already set in:
- [configuration.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/configuration.yaml)

What it does:
- loads all MQTT entity files from the `mqtt/` directory
- loads automations from `automations.yaml`

## 6. Understand what each MQTT file provides

### Module 1

File:
- [controllino_1.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_1.yaml)

Provides MQTT `light` entities for:
- living room
- kitchen
- bath room
- kid room
- working room
- common area
- maintenance room
- bed room

### Module 2

File:
- [controllino_2.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_2.yaml)

Provides MQTT entities for:
- lights
- fans
- covers/shades

Examples:
- `light.common_1`
- `cover.bed_room_shade_1`
- `fan.bath_room_fan_1`

## 7. Make sure MQTT topics match the Arduino modules

The YAML files in Home Assistant must match the MQTT client IDs and topic structure used by the modules.

This repo currently uses:
- `LanceControllino1`
- `LanceControllino2`

Examples:
- `controllino/LanceControllino1/status`
- `controllino/LanceControllino2/light/light_common_1/state`
- `controllino/LanceControllino2/cover/cover_bed_room_1/set`

If you change any of these in Arduino configuration:
- device hostname
- MQTT client ID
- zone names
- output types
- output indexes

then you must also update the Home Assistant MQTT YAML files.

## 8. Restart Home Assistant

After copying the files:
1. Check configuration in Home Assistant
2. Restart Home Assistant

If everything is correct, the MQTT entities should appear automatically.

## 9. Verify the entities

In Home Assistant:
1. Open `Settings > Devices & Services > Entities`
2. Search for:
   - `Living Room`
   - `Common`
   - `Shade`
   - `Fan`

You should see entities from both modules.

You can also verify availability:
- when the module is online, availability topic publishes `online`
- when offline, availability topic publishes `offline`

## 10. Update the sample automation

This repo includes one example automation in:
- [automations.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/automations.yaml)

Current example:
- turns on selected lights when `person.wife` arrives home after sunset

Before using it, update:
- the presence entity
- the target lights if needed

The sample currently expects:
- `person.wife`

If your entity is different, replace it with something like:
- `person.your_name`
- `device_tracker.your_phone`

## 11. If you want to add more entities

When you change output mappings in Arduino:
- [ModuleConfiguration.h](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_1/ModuleConfiguration.h)
- [ModuleConfiguration.h](/home/lance/Repositories/ArduinoAutomation/LanceHome/LanceHomeModule_2/ModuleConfiguration.h)

you may need to add or update matching Home Assistant MQTT entities in:
- [controllino_1.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_1.yaml)
- [controllino_2.yaml](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/config/mqtt/controllino_2.yaml)

Typical rule:
- `LLIGHT` -> `light`
- `LFAN` -> `fan`
- `LSHADEUP` / `LSHADEDOWN` pair -> one `cover`

## 12. Troubleshooting

If entities do not appear:
- confirm MQTT integration is installed and connected
- confirm Home Assistant loaded `mqtt: !include_dir_merge_list mqtt/`
- confirm files are inside the real Home Assistant config directory
- restart Home Assistant after copying changes

If entities appear but do not respond:
- confirm the Arduino module is online
- confirm the broker IP, username, password, and port match on both sides
- confirm the module was provisioned to EEPROM and uploaded in normal runtime mode
- confirm the MQTT topics in Home Assistant exactly match the module client ID and zone/index names

If entities show unavailable:
- check the device publishes to:
  - `controllino/LanceControllino1/status`
  - `controllino/LanceControllino2/status`
- confirm network connectivity from the Controllino device to the broker

## 13. Recommended setup order

Best order for first-time setup:
1. Set up MQTT broker
2. Configure Arduino module MQTT and Ethernet settings
3. Provision EEPROM on the module
4. Upload the module in normal runtime mode
5. Add MQTT integration in Home Assistant
6. Copy the Home Assistant config files
7. Restart Home Assistant
8. Test lights, fans, and shades
