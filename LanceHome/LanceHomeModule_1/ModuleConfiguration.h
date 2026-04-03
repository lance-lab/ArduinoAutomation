#ifndef LANCE_HOME_MODULE_1_CONFIGURATION_H
#define LANCE_HOME_MODULE_1_CONFIGURATION_H

#include "LanceControllino.h"

// Select the CONTROLLINO MAXI AUTOMATION board in Arduino IDE or VS Code before uploading this module.

namespace ModuleConfiguration {
  // Analog button mapping entries are:
  // { analog input pin, button on that analog input, output port to control }
  static const int ANALOG_INPUT_ASSIGNMENT_RW = 29;
  static int ANALOG_INPUT_ASSIGNMENT[ANALOG_INPUT_ASSIGNMENT_RW][3] = {
    {AI00,BTN0,DO07},
    {AI00,BTN1,RO08},
    {AI01,BTN0,RO04},
    {AI01,BTN1,DO06},
    {AI01,BTN2,DO07},
    {AI01,BTN3,RO07},
    {AI02,BTN0,RO03},
    {AI02,BTN1,DO05},
    {AI02,BTN2,RO01},
    {AI02,BTN3,RO07},
    {AI04,BTN0,RO07},
    {AI04,BTN1,RO05},
    {AI04,BTN2,RO01},
    {AI05,BTN1,DO03},
    {AI05,BTN2,DO07},
    {AI05,BTN3,RO02},
    {AI06,BTN0,DO07},
    {AI06,BTN1,RO09},
    {AI06,BTN2,DO02},
    {AI06,BTN3,RO01},
    {AI08,BTN0,DO04},
    {AI08,BTN1,DO07},
    {AI08,BTN2,RO06},
    {AI08,BTN3,DO02},
    {AI09,BTN0,RO09},
    {AI09,BTN1,RO00},
    {AI09,BTN2,DO00},
    {AI09,BTN3,DO01},
    {AI10,BTN1,RO02}
  };

  // Digital output entries are:
  // { output port, output type, group id, Home Assistant zone name, zone index }
  static const int DIGITAL_OUTPUT_ASSIGNMENT_RW = 18;
  static DigitalOutputConfig DIGITAL_OUTPUT_ASSIGNMENT[DIGITAL_OUTPUT_ASSIGNMENT_RW] = {
    {DO00,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       1},
    {DO01,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       2},
    {DO02,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           2},
    {DO03,LLIGHT,      GROUPNONE, ZONE_BATH_ROOM,         1},
    {DO04,LLIGHT,      GROUPNONE, ZONE_BATH_ROOM,         2},
    {DO05,LLIGHT,      GROUPNONE, ZONE_KID_ROOM,          1},
    {DO06,LLIGHT,      GROUPNONE, ZONE_WORKING_ROOM,      1},
    {DO07,LLIGHT,      GROUPNONE, ZONE_COMMON,            2},
    {RO00,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       3},
    {RO01,LLIGHT,      GROUPNONE, ZONE_COMMON,            3},
    {RO02,LLIGHT,      GROUPNONE, ZONE_MAINTENANCE_ROOM,  1},
    {RO03,LLIGHT,      GROUPNONE, ZONE_KID_ROOM,          2},
    {RO04,LLIGHT,      GROUPNONE, ZONE_WORKING_ROOM,      2},
    {RO05,LLIGHT,      GROUPNONE, ZONE_BED_ROOM,          1},
    {RO06,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           1},
    {RO07,LLIGHT,      GROUPNONE, ZONE_COMMON,            4},
    {RO08,LLIGHT,      GROUPNONE, ZONE_COMMON,            5},
    {RO09,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           3}
  };

  // Device setup:
  // hostname: friendly device name for logs and MQTT identity
  // mqttEvents:
  //   true  = enable EEPROM/network/MQTT startup and use mqtt + ethernet settings below
  //   false = offline mode; runtime skips EEPROM/network/MQTT, so mqtt/ethernet can stay empty
  //
  // MQTT config order:
  // { mqtt user, mqtt password, broker IP, MQTT client id, broker port }
  //
  // Ethernet config order:
  // { MAC address, local IP, gateway IP, subnet mask }
  static const DeviceConfig DEVICE_CONFIG = {
    "LanceControllino1",
    true,
    {
      "lancehome",
      "611632787d524d3f154f88e1",
      "10.10.10.20",
      "LanceControllino1",
      1883
    },
    {
      {0x02, 0x10, 0x10, 0x00, 0x00, 0x29},
      {10, 10, 10, 21},
      {10, 10, 10, 1},
      {255, 255, 255, 0}
    }
  };

  // Example offline-only configuration:
  // static const DeviceConfig DEVICE_CONFIG = {
  //   "LanceControllino1",
  //   false,
  //   {},
  //   {}
  // };
}

#endif
