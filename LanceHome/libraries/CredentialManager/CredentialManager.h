/*
  CredentialManager.h - Secure credential storage in EEPROM
  
  Manages MQTT credentials securely without hardcoding in source.
  Credentials are stored in Controllino Maxi's 4KB EEPROM.
  
  Usage:
    1. Upload SetCredentials sketch to program EEPROM once
    2. Upload main sketch that loads from EEPROM
    3. Credentials persist across power loss and code updates
*/

#ifndef CredentialManager_h
#define CredentialManager_h

#include <EEPROM.h>

// ===== EEPROM LAYOUT =====
#define EEPROM_MQTT_USER_ADDR      0   // MQTT username (32 bytes max)
#define EEPROM_MQTT_PASS_ADDR      32  // MQTT password (64 bytes max)
#define EEPROM_MQTT_SERVER_ADDR    96  // MQTT server IP string (16 bytes max)
#define EEPROM_MQTT_CLIENT_ID_ADDR 112 // MQTT client ID (16 bytes max)
#define EEPROM_MAC_ADDR            128 // MAC address (6 bytes)
#define EEPROM_LOCAL_IP_ADDR       134 // Local IP address (4 bytes)
#define EEPROM_GATEWAY_IP_ADDR     138 // Gateway IP address (4 bytes)
#define EEPROM_SUBNET_MASK_ADDR    142 // Subnet mask (4 bytes)
#define EEPROM_MQTT_PORT_ADDR      146 // MQTT port (2 bytes)
#define EEPROM_INIT_FLAG_ADDR      148 // Initialization flag (magic byte 0xAB)
#define EEPROM_CHECKSUM_ADDR       149 // CRC16 checksum (optional, 2 bytes)

// ===== SIZE LIMITS =====
#define CRED_USERNAME_MAX         32   // Max username length
#define CRED_PASSWORD_MAX         64   // Max password length
#define CRED_SERVER_MAX           16   // Max server IP string (e.g., "10.10.10.20\0")
#define CRED_CLIENT_ID_MAX        16   // Max client ID length
#define EEPROM_INIT_MAGIC         0xAB // Magic byte indicating valid data

// ===== CREDENTIAL STRUCTURE =====
struct Credentials {
  char mqttUser[CRED_USERNAME_MAX];      // MQTT broker username
  char mqttPass[CRED_PASSWORD_MAX];      // MQTT broker password
  char mqttServer[CRED_SERVER_MAX];      // MQTT broker IP address (dotted decimal)
  char mqttClientId[CRED_CLIENT_ID_MAX]; // MQTT client identifier
  byte mac[6];                           // Ethernet MAC address
  byte localIp[4];                       // Device LAN IP address
  byte gatewayIp[4];                     // Gateway IP address
  byte subnetMask[4];                    // Subnet mask
  uint16_t mqttPort;                     // MQTT broker port
};

// ===== CREDENTIAL MANAGER CLASS =====
class CredentialManager {
  public:
    /*
      Load credentials from EEPROM.
      
      Returns: true if credentials were loaded successfully
               false if EEPROM not initialized or checksum failed
    */
    static bool loadCredentials(Credentials &cred) {
      // Check if EEPROM has been initialized with valid data
      if (EEPROM.read(EEPROM_INIT_FLAG_ADDR) != EEPROM_INIT_MAGIC) {
        Serial.println("ERROR: Credentials not initialized in EEPROM!");
        Serial.println("SOLUTION: Upload SetCredentials.ino to program EEPROM first");
        return false;
      }
      
      // Read all credential strings from EEPROM
      readEEPROMString(cred.mqttUser, EEPROM_MQTT_USER_ADDR, CRED_USERNAME_MAX);
      readEEPROMString(cred.mqttPass, EEPROM_MQTT_PASS_ADDR, CRED_PASSWORD_MAX);
      readEEPROMString(cred.mqttServer, EEPROM_MQTT_SERVER_ADDR, CRED_SERVER_MAX);
      readEEPROMString(cred.mqttClientId, EEPROM_MQTT_CLIENT_ID_ADDR, CRED_CLIENT_ID_MAX);
      
      readEEPROMBytes(cred.mac, EEPROM_MAC_ADDR, sizeof(cred.mac));
      readEEPROMBytes(cred.localIp, EEPROM_LOCAL_IP_ADDR, sizeof(cred.localIp));
      readEEPROMBytes(cred.gatewayIp, EEPROM_GATEWAY_IP_ADDR, sizeof(cred.gatewayIp));
      readEEPROMBytes(cred.subnetMask, EEPROM_SUBNET_MASK_ADDR, sizeof(cred.subnetMask));
      cred.mqttPort = readEEPROMUInt16(EEPROM_MQTT_PORT_ADDR);

      // Verify data looks valid (basic sanity check)
      if (strlen(cred.mqttUser) == 0 || strlen(cred.mqttPass) == 0) {
        Serial.println("WARNING: Empty username or password in EEPROM");
        return false;
      }

      if (cred.mqttPort == 0) {
        Serial.println("WARNING: Invalid MQTT port in EEPROM");
        return false;
      }
      
      Serial.println("✓ Credentials loaded from EEPROM");
      Serial.println("  User: " + String(cred.mqttUser));
      Serial.println("  Server: " + String(cred.mqttServer));
      Serial.print("  Local IP: ");
      printIPAddress(cred.localIp);
      Serial.println();
      
      return true;
    }
    
    /*
      Save credentials to EEPROM.
      
      Call this function once using the SetCredentials.ino sketch to 
      program the device with your MQTT broker details.
      
      Args:
        cred - Credentials struct with all fields populated
      
      Returns: true if save successful, false if validation failed
    */
    static bool saveCredentials(const Credentials &cred) {
      // Validate inputs before writing to EEPROM (prevent junk data)
      if (strlen(cred.mqttUser) == 0 || strlen(cred.mqttUser) > CRED_USERNAME_MAX - 1) {
        Serial.println("ERROR: Invalid username length");
        return false;
      }
      if (strlen(cred.mqttPass) == 0 || strlen(cred.mqttPass) > CRED_PASSWORD_MAX - 1) {
        Serial.println("ERROR: Invalid password length");
        return false;
      }
      if (strlen(cred.mqttServer) == 0 || strlen(cred.mqttServer) > CRED_SERVER_MAX - 1) {
        Serial.println("ERROR: Invalid server address");
        return false;
      }
      if (strlen(cred.mqttClientId) == 0 || strlen(cred.mqttClientId) > CRED_CLIENT_ID_MAX - 1) {
        Serial.println("ERROR: Invalid client ID");
        return false;
      }
      if (cred.mqttPort == 0) {
        Serial.println("ERROR: Invalid MQTT port");
        return false;
      }
      
      // Write all credential strings to EEPROM
      writeEEPROMString(cred.mqttUser, EEPROM_MQTT_USER_ADDR, CRED_USERNAME_MAX);
      writeEEPROMString(cred.mqttPass, EEPROM_MQTT_PASS_ADDR, CRED_PASSWORD_MAX);
      writeEEPROMString(cred.mqttServer, EEPROM_MQTT_SERVER_ADDR, CRED_SERVER_MAX);
      writeEEPROMString(cred.mqttClientId, EEPROM_MQTT_CLIENT_ID_ADDR, CRED_CLIENT_ID_MAX);
      writeEEPROMBytes(cred.mac, EEPROM_MAC_ADDR, sizeof(cred.mac));
      writeEEPROMBytes(cred.localIp, EEPROM_LOCAL_IP_ADDR, sizeof(cred.localIp));
      writeEEPROMBytes(cred.gatewayIp, EEPROM_GATEWAY_IP_ADDR, sizeof(cred.gatewayIp));
      writeEEPROMBytes(cred.subnetMask, EEPROM_SUBNET_MASK_ADDR, sizeof(cred.subnetMask));
      writeEEPROMUInt16(cred.mqttPort, EEPROM_MQTT_PORT_ADDR);
      
      // Write initialization flag to mark EEPROM as valid
      EEPROM.write(EEPROM_INIT_FLAG_ADDR, EEPROM_INIT_MAGIC);
      
      Serial.println("✓ Credentials programmed to EEPROM successfully!");
      Serial.println("  You can now upload your main sketch");
      
      return true;
    }
    
    /*
      Erase all credentials from EEPROM.
      Use this to clear credentials (for security or reset purposes).
    */
    static void eraseCredentials() {
      for (int i = 0; i < 150; i++) {
        EEPROM.write(i, 0xFF);  // 0xFF is the EEPROM empty value
      }
      Serial.println("Credentials erased from EEPROM");
    }
    
    /*
      Get EEPROM usage statistics (for debugging)
    */
    static void printStats() {
      int used = 150; // EEPROM address used up to init flag
      int available = 4096 - used;
      
      Serial.println("=== EEPROM Statistics ===");
      Serial.println("Used: " + String(used) + " bytes");
      Serial.println("Available: " + String(available) + " bytes");
      Serial.println("Init Flag: " + String(EEPROM.read(EEPROM_INIT_FLAG_ADDR) == EEPROM_INIT_MAGIC ? "VALID" : "INVALID"));
    }
    
  private:
    /*
      Read a null-terminated string from EEPROM.
      
      Args:
        dest - buffer to store the string
        addr - EEPROM start address
        maxLen - maximum length to read
    */
    static void readEEPROMString(char *dest, int addr, int maxLen) {
      int i = 0;
      while (i < maxLen - 1) {  // Leave room for null terminator
        dest[i] = EEPROM.read(addr + i);
        if (dest[i] == '\0') break;  // Stop at null terminator
        i++;
      }
      dest[i] = '\0';  // Ensure null termination
    }
    
    /*
      Write a null-terminated string to EEPROM.
      
      Args:
        src - string to write
        addr - EEPROM start address
        maxLen - maximum length to write
    */
    static void writeEEPROMString(const char *src, int addr, int maxLen) {
      int i = 0;
      while (i < maxLen - 1 && src[i] != '\0') {
        EEPROM.write(addr + i, src[i]);
        i++;
      }
      // Always write null terminator
      EEPROM.write(addr + i, '\0');
    }

    static void readEEPROMBytes(byte *dest, int addr, int len) {
      for (int i = 0; i < len; i++) {
        dest[i] = EEPROM.read(addr + i);
      }
    }

    static void writeEEPROMBytes(const byte *src, int addr, int len) {
      for (int i = 0; i < len; i++) {
        EEPROM.write(addr + i, src[i]);
      }
    }

    static uint16_t readEEPROMUInt16(int addr) {
      uint16_t value = EEPROM.read(addr);
      value |= (uint16_t)EEPROM.read(addr + 1) << 8;
      return value;
    }

    static void writeEEPROMUInt16(uint16_t value, int addr) {
      EEPROM.write(addr, value & 0xFF);
      EEPROM.write(addr + 1, (value >> 8) & 0xFF);
    }

    static void printIPAddress(const byte *ip) {
      Serial.print(ip[0]);
      Serial.print(".");
      Serial.print(ip[1]);
      Serial.print(".");
      Serial.print(ip[2]);
      Serial.print(".");
      Serial.print(ip[3]);
    }
};

#endif
