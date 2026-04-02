#ifndef LANCE_HOME_MODULE_1_CONFIGURATION_H
#define LANCE_HOME_MODULE_1_CONFIGURATION_H

#include "CredentialManager.h"

namespace ModuleConfiguration {
  static const DeviceConfig DEVICE_CONFIG = {
    "LanceControllino1",
    "lancehome",
    "611632787d524d3f154f88e1",
    "10.10.10.20",
    "LanceControllino1",
    {0x02, 0x10, 0x10, 0x00, 0x00, 0x29},
    {10, 10, 10, 21},
    {10, 10, 10, 1},
    {255, 255, 255, 0},
    1883
  };
}

#endif
