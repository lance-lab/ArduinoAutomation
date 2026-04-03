// Last update: 8/29/2022
#include "LanceControllino.h"
#include "ModuleConfiguration.h"

// EEPROM provisioning mode:
// false = normal runtime startup
// true  = write DEVICE_CONFIG to EEPROM on boot, skip runtime, then switch back to false and upload again
#define PROVISION_EEPROM_ON_BOOT false

LanceControllino lanceControllino(
  ModuleConfiguration::ANALOG_INPUT_ASSIGNMENT,
  ModuleConfiguration::DIGITAL_OUTPUT_ASSIGNMENT,
  ModuleConfiguration::ANALOG_INPUT_ASSIGNMENT_RW,
  ModuleConfiguration::DIGITAL_OUTPUT_ASSIGNMENT_RW,
  ModuleConfiguration::DEVICE_CONFIG.mqttEvents
);

LanceControllinoRuntime lanceRuntime(
  lanceControllino,
  ModuleConfiguration::DEVICE_CONFIG.mqttEvents
);

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
#if PROVISION_EEPROM_ON_BOOT
  CredentialManager::provisionFromConfig(ModuleConfiguration::DEVICE_CONFIG);
#else
  lanceRuntime.setup();
#endif
}

void loop()
{
#if !PROVISION_EEPROM_ON_BOOT
  lanceRuntime.loop();
#endif
}
