#include "CredentialManager.h"
#include "SetCredentialsConfig.h"
#include <string.h>

#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE 115200
#endif

Credentials cred;

const SetCredentialsConfig::DeviceConfig *findDeviceConfig(const char *hostname) {
  for (unsigned int i = 0; i < SetCredentialsConfig::DEVICE_COUNT; i++) {
    if (strcmp(SetCredentialsConfig::DEVICES[i].hostname, hostname) == 0) {
      return &SetCredentialsConfig::DEVICES[i];
    }
  }

  return NULL;
}

void printIPAddress(const byte *ipAddress) {
  Serial.print(ipAddress[0]);
  Serial.print(".");
  Serial.print(ipAddress[1]);
  Serial.print(".");
  Serial.print(ipAddress[2]);
  Serial.print(".");
  Serial.print(ipAddress[3]);
}

void printMacAddress(const byte *macAddress) {
  for (int i = 0; i < 6; i++) {
    if (macAddress[i] < 16) {
      Serial.print("0");
    }
    Serial.print(macAddress[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
}

void printBanner() {
  Serial.println();
  Serial.println("====================================");
  Serial.println("    MQTT Credentials EEPROM Setup");
  Serial.println("====================================");
  Serial.println();
}

void populateCredentials() {
  const SetCredentialsConfig::DeviceConfig *deviceConfig =
      findDeviceConfig(HOSTNAME);

  memset(&cred, 0, sizeof(cred));

  if (deviceConfig == NULL) {
    Serial.println("ERROR: No configuration found for target hostname");
    return;
  }

  // Set mqttServer to the LAN IP of the computer running Mosquitto.
  strncpy(cred.mqttUser, deviceConfig->mqttUser, CRED_USERNAME_MAX - 1);
  strncpy(cred.mqttPass, deviceConfig->mqttPass, CRED_PASSWORD_MAX - 1);
  strncpy(cred.mqttServer, deviceConfig->mqttServer, CRED_SERVER_MAX - 1);
  strncpy(cred.mqttClientId, deviceConfig->mqttClientId, CRED_CLIENT_ID_MAX - 1);
  memcpy(cred.mac, deviceConfig->mac, sizeof(cred.mac));
  memcpy(cred.localIp, deviceConfig->localIp, sizeof(cred.localIp));
  memcpy(cred.gatewayIp, deviceConfig->gatewayIp, sizeof(cred.gatewayIp));
  memcpy(cred.subnetMask, deviceConfig->subnetMask, sizeof(cred.subnetMask));
  cred.mqttPort = deviceConfig->mqttPort;
}

void printConfiguredValues() {
  Serial.println("Configured values:");
  Serial.println("  User: " + String(cred.mqttUser));
  Serial.println("  Server: " + String(cred.mqttServer));
  Serial.println("  Client ID: " + String(cred.mqttClientId));
  Serial.print("  MAC: ");
  printMacAddress(cred.mac);
  Serial.println();
  Serial.print("  Local IP: ");
  printIPAddress(cred.localIp);
  Serial.println();
  Serial.print("  Gateway: ");
  printIPAddress(cred.gatewayIp);
  Serial.println();
  Serial.print("  Subnet: ");
  printIPAddress(cred.subnetMask);
  Serial.println();
  Serial.println("  MQTT Port: " + String(cred.mqttPort));
  Serial.println();
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  printBanner();
  populateCredentials();
  printConfiguredValues();

  Serial.println("Setting credentials to EEPROM...");

  if (CredentialManager::saveCredentials(cred)) {
    Serial.println();
    Serial.println("SUCCESS");
    Serial.println("Credentials are now stored in EEPROM");
    CredentialManager::printStats();
    Serial.println();
    Serial.println("NEXT STEP:");
    Serial.println("1. Upload your main sketch");
    Serial.println("2. Watch Serial Monitor for credential load confirmation");
  } else {
    Serial.println();
    Serial.println("FAILED");
    Serial.println("Review the configured values above and try again");
  }
}

void loop() {
  // Nothing else to do after programming EEPROM once.
}
