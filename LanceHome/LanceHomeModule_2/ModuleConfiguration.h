#ifndef LANCE_HOME_MODULE_2_CONFIGURATION_H
#define LANCE_HOME_MODULE_2_CONFIGURATION_H

#include "LanceControllino.h"

// Select the CONTROLLINO MAXI board in Arduino IDE or VS Code before uploading this module.

namespace ModuleConfiguration {
  // Analog button mapping entries are:
  // { analog input pin, button on that analog input, output port to control }
  static const int ANALOG_INPUT_ASSIGNMENT_RW = 19;
  static int ANALOG_INPUT_ASSIGNMENT[ANALOG_INPUT_ASSIGNMENT_RW][3] = {
    {AI00,BTN0,DO06},
    {AI00,BTN2,DO05},
    {AI00,BTN3,DO01},
    {AI05,BTN0,DO06},
    {AI05,BTN1,DO02},
    {AI05,BTN2,DO04},
    {AI01,BTN0,RO04},
    {AI01,BTN1,RO05},
    {AI01,BTN2,RO00},
    {AI01,BTN3,RO01},
    {AI02,BTN0,RO06},
    {AI02,BTN1,RO07},
    {AI02,BTN2,RO02},
    {AI02,BTN3,RO03},
    {AI06,BTN0,RO09},
    {AI06,BTN1,RO08},
    {AI06,BTN2,DO07},
    {AI06,BTN3,DO08},
    {AI07,BTN0,DO09}
  };

  // Digital output entries are:
  // { output port, output type, group id, Home Assistant zone name, zone index }
  static const int DIGITAL_OUTPUT_ASSIGNMENT_RW = 18;
  static DigitalOutputConfig DIGITAL_OUTPUT_ASSIGNMENT[DIGITAL_OUTPUT_ASSIGNMENT_RW] = {
    {DO06,LLIGHT,      GROUPNONE, ZONE_COMMON,            1},
    {DO05,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           1},
    {DO01,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           2},
    {DO02,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           3},
    {DO04,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           4},
    {RO00,LSHADEUP,    GROUP101,  ZONE_BED_ROOM,          1},
    {RO01,LSHADEDOWN,  GROUP101,  ZONE_BED_ROOM,          1},
    {RO02,LSHADEUP,    GROUP102,  ZONE_KID_ROOM,          1},
    {RO03,LSHADEDOWN,  GROUP102,  ZONE_KID_ROOM,          1},
    {RO04,LSHADEUP,    GROUP103,  ZONE_KID_ROOM,          2},
    {RO05,LSHADEDOWN,  GROUP103,  ZONE_KID_ROOM,          2},
    {RO06,LSHADEUP,    GROUP104,  ZONE_WORKING_ROOM,      1},
    {RO07,LSHADEDOWN,  GROUP104,  ZONE_WORKING_ROOM,      1},
    {RO08,LFAN,        GROUPNONE, ZONE_BATH_ROOM,         1},
    {RO09,LFAN,        GROUPNONE, ZONE_BATH_ROOM,         2},
    {DO07,LSHADEUP,    GROUP105,  ZONE_OUTSIDE,           1},
    {DO08,LSHADEDOWN,  GROUP105,  ZONE_OUTSIDE,           1},
    {DO09,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           5}
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
    "LanceControllino2",
    true,
    {
      "lancehome",
      "611632787d524d3f154f88e1",
      "10.10.10.20",
      "LanceControllino2",
      1883
    },
    {
      {0x02, 0x10, 0x10, 0x00, 0x00, 0x2A},
      {10, 10, 10, 22},
      {10, 10, 10, 1},
      {255, 255, 255, 0}
    }
  };

  // Example offline-only configuration:
  // static const DeviceConfig DEVICE_CONFIG = {
  //   "LanceControllino2",
  //   false,
  //   {},
  //   {}
  // };
}

#endif
