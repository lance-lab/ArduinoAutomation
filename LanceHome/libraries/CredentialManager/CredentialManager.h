/*
  CredentialManager.h - Secure device configuration storage in EEPROM
  
  Manages MQTT and network configuration without hardcoding runtime values.
  Configuration is stored in Controllino Maxi's EEPROM and can be
  provisioned directly by each module sketch during setup.
*/

#ifndef CredentialManager_h
#define CredentialManager_h

#include <EEPROM.h>

// ===== SIZE LIMITS =====
#define CRED_USERNAME_MAX         32   // Max username length
#define CRED_PASSWORD_MAX         64   // Max password length
#define CRED_SERVER_MAX           16   // Max server IP string (e.g., "10.10.10.20\0")
#define CRED_CLIENT_ID_MAX        20   // Max client ID length
#define EEPROM_INIT_MAGIC         0xAB // Magic byte indicating valid data
#define EEPROM_LAYOUT_VERSION     2

// ===== EEPROM LAYOUT =====
#define EEPROM_MQTT_USER_ADDR      0
#define EEPROM_MQTT_PASS_ADDR      (EEPROM_MQTT_USER_ADDR + CRED_USERNAME_MAX)
#define EEPROM_MQTT_SERVER_ADDR    (EEPROM_MQTT_PASS_ADDR + CRED_PASSWORD_MAX)
#define EEPROM_MQTT_CLIENT_ID_ADDR (EEPROM_MQTT_SERVER_ADDR + CRED_SERVER_MAX)
#define EEPROM_MAC_ADDR            (EEPROM_MQTT_CLIENT_ID_ADDR + CRED_CLIENT_ID_MAX)
#define EEPROM_LOCAL_IP_ADDR       (EEPROM_MAC_ADDR + 6)
#define EEPROM_GATEWAY_IP_ADDR     (EEPROM_LOCAL_IP_ADDR + 4)
#define EEPROM_SUBNET_MASK_ADDR    (EEPROM_GATEWAY_IP_ADDR + 4)
#define EEPROM_MQTT_PORT_ADDR      (EEPROM_SUBNET_MASK_ADDR + 4)
#define EEPROM_INIT_FLAG_ADDR      (EEPROM_MQTT_PORT_ADDR + 2)
#define EEPROM_CHECKSUM_ADDR       (EEPROM_INIT_FLAG_ADDR + 1)
#define EEPROM_LAYOUT_VERSION_ADDR (EEPROM_CHECKSUM_ADDR + 2)

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

// ===== CREDENTIAL MANAGER CLASS =====
class CredentialManager {
  public:
    /*
      Load credentials from EEPROM.
      
      Returns: true if credentials were loaded successfully
               false if EEPROM not initialized or checksum failed
    */
    static bool loadCredentials(Credentials &cred, bool verbose = true) {
      // Check if EEPROM has been initialized with valid data
      if (EEPROM.read(EEPROM_INIT_FLAG_ADDR) != EEPROM_INIT_MAGIC) {
        if (verbose) {
          Serial.println("ERROR: Configuration not initialized in EEPROM!");
          Serial.println("SOLUTION: Module setup will provision configuration automatically");
        }
        return false;
      }

      if (EEPROM.read(EEPROM_LAYOUT_VERSION_ADDR) != EEPROM_LAYOUT_VERSION) {
        if (verbose) {
          Serial.println("ERROR: EEPROM configuration layout is outdated");
          Serial.println("SOLUTION: Module setup will rewrite configuration automatically");
        }
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
        if (verbose) {
          Serial.println("WARNING: Empty username or password in EEPROM");
        }
        return false;
      }

      if (cred.mqttPort == 0) {
        if (verbose) {
          Serial.println("WARNING: Invalid MQTT port in EEPROM");
        }
        return false;
      }
      
      if (verbose) {
        Serial.println("Configuration loaded from EEPROM");
        printConfiguredValues(cred);
      }
      
      return true;
    }
    
    /*
      Save configuration to EEPROM.
      
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
      EEPROM.write(EEPROM_LAYOUT_VERSION_ADDR, EEPROM_LAYOUT_VERSION);
      
      Serial.println("Configuration programmed to EEPROM successfully");
      
      return true;
    }

    static void populateCredentials(Credentials &cred, const DeviceConfig &deviceConfig) {
      memset(&cred, 0, sizeof(cred));

      strncpy(cred.mqttUser, deviceConfig.mqttUser, CRED_USERNAME_MAX - 1);
      strncpy(cred.mqttPass, deviceConfig.mqttPass, CRED_PASSWORD_MAX - 1);
      strncpy(cred.mqttServer, deviceConfig.mqttServer, CRED_SERVER_MAX - 1);
      strncpy(cred.mqttClientId, deviceConfig.mqttClientId, CRED_CLIENT_ID_MAX - 1);
      memcpy(cred.mac, deviceConfig.mac, sizeof(cred.mac));
      memcpy(cred.localIp, deviceConfig.localIp, sizeof(cred.localIp));
      memcpy(cred.gatewayIp, deviceConfig.gatewayIp, sizeof(cred.gatewayIp));
      memcpy(cred.subnetMask, deviceConfig.subnetMask, sizeof(cred.subnetMask));
      cred.mqttPort = deviceConfig.mqttPort;
    }

    static bool provisionFromConfig(const DeviceConfig &deviceConfig, bool force = false) {
      Credentials desired;
      Credentials current;

      populateCredentials(desired, deviceConfig);

      if (!force && loadCredentials(current, false) && credentialsEqual(current, desired)) {
        Serial.println("EEPROM configuration already matches module configuration");
        return true;
      }

      if (force) {
        Serial.println("Forcing EEPROM configuration rewrite from module configuration");
      } else {
        Serial.println("Provisioning EEPROM from module configuration");
      }

      printConfiguredValues(desired);
      return saveCredentials(desired);
    }
    
    /*
      Erase all credentials from EEPROM.
      Use this to clear credentials (for security or reset purposes).
    */
    static void eraseCredentials() {
      for (int i = 0; i <= EEPROM_LAYOUT_VERSION_ADDR; i++) {
        EEPROM.write(i, 0xFF);  // 0xFF is the EEPROM empty value
      }
      Serial.println("Configuration erased from EEPROM");
    }
    
    /*
      Get EEPROM usage statistics (for debugging)
    */
    static void printStats() {
      int used = EEPROM_LAYOUT_VERSION_ADDR + 1;
      int available = 4096 - used;
      
      Serial.println("=== EEPROM Statistics ===");
      Serial.println("Used: " + String(used) + " bytes");
      Serial.println("Available: " + String(available) + " bytes");
      Serial.println("Init Flag: " + String(EEPROM.read(EEPROM_INIT_FLAG_ADDR) == EEPROM_INIT_MAGIC ? "VALID" : "INVALID"));
      Serial.println("Layout Version: " + String(EEPROM.read(EEPROM_LAYOUT_VERSION_ADDR)));
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

    static bool credentialsEqual(const Credentials &a, const Credentials &b) {
      return strcmp(a.mqttUser, b.mqttUser) == 0 &&
             strcmp(a.mqttPass, b.mqttPass) == 0 &&
             strcmp(a.mqttServer, b.mqttServer) == 0 &&
             strcmp(a.mqttClientId, b.mqttClientId) == 0 &&
             memcmp(a.mac, b.mac, sizeof(a.mac)) == 0 &&
             memcmp(a.localIp, b.localIp, sizeof(a.localIp)) == 0 &&
             memcmp(a.gatewayIp, b.gatewayIp, sizeof(a.gatewayIp)) == 0 &&
             memcmp(a.subnetMask, b.subnetMask, sizeof(a.subnetMask)) == 0 &&
             a.mqttPort == b.mqttPort;
    }

    static void printMacAddress(const byte *mac) {
      for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) {
          Serial.print("0");
        }
        Serial.print(mac[i], HEX);
        if (i < 5) {
          Serial.print(":");
        }
      }
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

    static void printConfiguredValues(const Credentials &cred) {
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
    }
};

#endif
