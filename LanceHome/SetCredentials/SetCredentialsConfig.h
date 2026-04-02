#ifndef SET_CREDENTIALS_CONFIG_H
#define SET_CREDENTIALS_CONFIG_H

#define HOSTNAME "LanceControllino2"

namespace SetCredentialsConfig {
struct DeviceConfig {
  const char *hostname;
  const char *mqttUser;
  const char *mqttPass;
  const char *mqttServer;
  const char *mqttClientId;
  byte mac[6];
  byte localIp[4];
  byte gatewayIp[4];
  byte subnetMask[4];
  uint16_t mqttPort;
};

static const DeviceConfig DEVICES[] = {
  {
    "LanceControllino1",
    "lancehome",
    "611632787d524d3f154f88e1",
    "10.10.10.20",
    "LanceControllino1",
    {0x02, 0x10, 0x10, 0x00, 0x00, 0x21},
    {10, 10, 10, 28},
    {10, 10, 10, 1},
    {255, 255, 255, 0},
    1883
  },
  {
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
  }
};

static const unsigned int DEVICE_COUNT = sizeof(DEVICES) / sizeof(DEVICES[0]);
}

#endif
