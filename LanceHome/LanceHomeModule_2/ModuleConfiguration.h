#ifndef LANCE_HOME_MODULE_2_CONFIGURATION_H
#define LANCE_HOME_MODULE_2_CONFIGURATION_H

#include "CredentialManager.h"

namespace ModuleConfiguration {
  static const DeviceConfig DEVICE_CONFIG = {
    "LanceControllino2",
    "lancehome",
    "611632787d524d3f154f88e1",
    "10.10.10.20",
    "LanceControllino2",
    {0x02, 0x10, 0x10, 0x00, 0x00, 0x2A},
    {10, 10, 10, 22},
    {10, 10, 10, 1},
    {255, 255, 255, 0},
    1883
  };
}

#endif
