/*
  LanceControllino.h Last update: 8/29/2022
*/

#ifndef LanceControllino_h
#define LanceControllino_h

#include <avr/wdt.h>
#include <Controllino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "CredentialManager.h"
#include "RingBuf.h"


//Analog port assignment
#define AI00 54
#define AI01 55
#define AI02 56
#define AI03 57
#define AI04 58
#define AI05 59
#define AI06 60
#define AI07 61
#define AI08 62
#define AI09 63
#define AI10 64
#define AI11 65

//Digital port assignment
#define DO00 2
#define DO01 3
#define DO02 4
#define DO03 5
#define DO04 6
#define DO05 7
#define DO06 8
#define DO07 9
#define DO08 10
#define DO09 11
#define RO00 22
#define RO01 23
#define RO02 24
#define RO03 25
#define RO04 26
#define RO05 27
#define RO06 28
#define RO07 29
#define RO08 30
#define RO09 31
#define DIGPORTNONE -1

//Register port assignment
#define _PORTA 101
#define _PORTB 102
#define _PORTC 103
#define _PORTD 104
#define _PORTE 105
#define _PORTF 106
#define _PORTG 107
#define _PORTH 108

//Digital port status assignment
#define OFF 0
#define ON 1

//Digital port type assignment
#define LLIGHT 0
#define LFAN 1
#define LSHADEUP 2
#define LSHADEDOWN 3
#define TYPENONE -1

//Analog port button assignment
#define BTN0 0 
#define BTN1 1
#define BTN2 2
#define BTN3 3
#define BTNNONE 4

//Analog button threshold values (ADC 0-1023 range)
//These thresholds are used to distinguish between 4 buttons on a single analog input
//Each button creates a different voltage divider ratio
#define ANALOG_BTN_NO_PRESS_THRESHOLD 200
#define ANALOG_BTN3_THRESHOLD 350
#define ANALOG_BTN2_THRESHOLD 550
#define ANALOG_BTN1_THRESHOLD 750

//Group assignment
#define GROUP101 101
#define GROUP102 102
#define GROUP103 103
#define GROUP104 104
#define GROUP105 105
#define GROUP106 106
#define GROUP107 107
#define GROUP108 108
#define GROUP109 109
#define GROUP110 110
#define GROUP111 111
#define GROUP112 112
#define GROUPNONE -1

// Zone assignment
#define ZONE_NONE "none"
#define ZONE_OUTSIDE "outside"
#define ZONE_LIVING_ROOM "living_room"
#define ZONE_KITCHEN "kitchen"
#define ZONE_BATH_ROOM "bath_room"
#define ZONE_KID_ROOM "kid_room"
#define ZONE_BED_ROOM "bed_room"
#define ZONE_WORKING_ROOM "working_room"
#define ZONE_MAINTENANCE_ROOM "maintenance_room"

//Analog port availability
#define DISABLED 0
#define ENABLED 1

// ARRAY SIZE DEFINITIONS
// _ANALOG_INPUT_ASSIGNMENT_SIZE: Theoretical maximum = _ANALOG_INPUTS_SIZE × 4 buttons
// (12 analog inputs × 4 possible buttons per input = 48 total slots)
// This is pre-allocated as a configuration matrix. The initialization() function
// populates only the used entries based on analogInputAssignment passed from the sketch.
// Actual usage specified by ANALOG_INPUT_ASSIGNMENT_RW in the sketch (typically 29 entries).
#define _ANALOG_INPUTS_SIZE 12
#define _DIGITAL_OUTPUTS_SIZE 18
#define _ANALOG_INPUT_ASSIGNMENT_SIZE 48
#define _DIGITAL_OUTPUT_ASSIGNMENT_SIZE 18
#define _DIGITAL_OUTPUT_ASSIGNMENT_CLOCK_SIZE 18
#define _TOPIC_MESSAGE_LENGTH 96
#define _MESSAGE_BUFFER_SIZE 20  // RingBuf capacity - if exceeded, messages are dropped
#define SERIAL_BAUD_RATE 115200

#ifndef ETHERNET_CHIP_SELECT_PIN
#define ETHERNET_CHIP_SELECT_PIN -1
#endif

#ifndef ENABLE_HOME_ASSISTANT_MQTT
#define ENABLE_HOME_ASSISTANT_MQTT 1
#endif

#define PRESSED_BTN_COUNT 2
#define RUN_FUN_COUNT 300000
#define RUN_SHADE_COUNT 50000
#define COVER_STATE_STOPPED 0
#define COVER_STATE_OPENING 1
#define COVER_STATE_CLOSING 2

/*
TIMING NOTES:
millis() returns an unsigned long that overflows every 49.7 days (2^32 milliseconds).
To handle overflow correctly, always use unsigned integer subtraction:
  CORRECT:   if ((unsigned long)(millis() - startTime) >= duration)
  WRONG:     if (millis() > (startTime + duration))

The subtraction method works because unsigned overflow is well-defined in C++.
*/

struct DigitalOutputConfig
{
  int port;
  int type;
  int group;
  const char *zone;
  int index;
};

class LanceControllino
{
  public:
    LanceControllino(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize);
    LanceControllino(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize, bool events);
    static bool parseMQTTMessage(const String &message, String &type, String &zone, int &index, String &status);
    static bool parseLegacyMQTTMessage(const String &message, String &type, int &port, String &status);
    void analogInputVerification();
    void statusTimeVerification();
    bool processExternalRequest(const String &type, const String &zone, int index, int status);
    bool processExternalRequest(int port, int status);
    bool processCoverCommand(const String &zone, int index, const String &command);
    bool processCoverCommand(const char *zone, int index, const char *command);
    bool addToBuffer(int topic, String message);
    bool pullFromBuffer();
    void getBufferStats(unsigned int &dropped, unsigned int &total);  // Get buffer overflow statistics
    int getOutputCount() const;
    bool isOutputConfigured(int outputIndex) const;
    int getOutputPort(int outputIndex) const;
    int getOutputType(int outputIndex) const;
    int getOutputGroup(int outputIndex) const;
    int getOutputStatus(int outputIndex) const;
    const char *getOutputZone(int outputIndex) const;
    int getOutputZoneIndex(int outputIndex) const;
    int getCoverState(const String &zone, int index) const;

    struct _eventMessage
      {
        int _topic;
        char _topicMessage[_TOPIC_MESSAGE_LENGTH];
      };

    struct _eventMessage bufferAdd, bufferPull;

    RingBuf *_messageBuffer = RingBuf_new(sizeof(struct _eventMessage), _MESSAGE_BUFFER_SIZE);
    
    // Buffer overflow tracking
    unsigned int _droppedMessageCount = 0;  // Count of messages dropped due to buffer overflow
    unsigned int _totalAddedCount = 0;      // Total messages attempted to add (for debugging)

  private:
    bool initialization(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize);
    bool portActivation();
    bool lightOperation(int port, int status = -1);
    bool shadeOperation(int port, int status = -1);
    bool fanOperation(int port, int status = -1);
    String buildMqttStatusMessage(int outputIndex, const char *typeName, int status);
    int findOutputIndexByIdentity(const String &type, const String &zone, int index);
    void debuger();

    int readKey(int analogInputPin);
    int findElementRow(int array[], int lastElement, int searchKey);
    int findElementColumn(int array[][4], int lastElement, int searchKey);
    bool _initialized;
    bool _portActivated;
    bool _eventActivated;

    // Analog inputs array
    int _analogInputs [2][_ANALOG_INPUTS_SIZE] = {
      {AI00,    AI01,    AI02,    AI03,    AI04,    AI05,    AI06,    AI07,    AI08,    AI09,    AI10,    AI11},
      {DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED,DISABLED}
      };

    // Digital outputs array
    const int _digitalOutputs [3][_DIGITAL_OUTPUTS_SIZE] = {
    { DO00,  DO01,  DO02,  DO03,  DO04,  DO05,  DO06,  DO07,  RO00,  RO01,  RO02,  RO03,  RO04,  RO05,  RO06,  RO07,  RO08,  RO09},
    {_PORTE,_PORTE,_PORTG,_PORTE,_PORTH,_PORTH,_PORTH,_PORTH,_PORTA,_PORTA,_PORTA,_PORTA,_PORTA,_PORTA,_PORTA,_PORTA,_PORTC,_PORTC},
    { 4,     5,     5,     3,     3,     4,     5,     6,     0,     1,     2,     3,     4,     5,     6,     7,     7,     6}};

    /*_analogInputAssignment
      0 - analog pin
      1 - analog pin button 0-1
      2 - digital pin assigned
      3 - button pressed status 0 = NOT PRESSED, 1+ PRESSED
    */
    int _analogInputAssignment [_ANALOG_INPUT_ASSIGNMENT_SIZE][4] = {
    {AI00,BTN0,DIGPORTNONE,0},{AI00,BTN1,DIGPORTNONE,0},{AI00,BTN2,DIGPORTNONE,0},{AI00,BTN3,DIGPORTNONE,0},
    {AI01,BTN0,DIGPORTNONE,0},{AI01,BTN1,DIGPORTNONE,0},{AI01,BTN2,DIGPORTNONE,0},{AI01,BTN3,DIGPORTNONE,0},    
    {AI02,BTN0,DIGPORTNONE,0},{AI02,BTN1,DIGPORTNONE,0},{AI02,BTN2,DIGPORTNONE,0},{AI02,BTN3,DIGPORTNONE,0},
    {AI03,BTN0,DIGPORTNONE,0},{AI03,BTN1,DIGPORTNONE,0},{AI03,BTN2,DIGPORTNONE,0},{AI03,BTN3,DIGPORTNONE,0},
    {AI04,BTN0,DIGPORTNONE,0},{AI04,BTN1,DIGPORTNONE,0},{AI04,BTN2,DIGPORTNONE,0},{AI04,BTN3,DIGPORTNONE,0},
    {AI05,BTN0,DIGPORTNONE,0},{AI05,BTN1,DIGPORTNONE,0},{AI05,BTN2,DIGPORTNONE,0},{AI05,BTN3,DIGPORTNONE,0},
    {AI06,BTN0,DIGPORTNONE,0},{AI06,BTN1,DIGPORTNONE,0},{AI06,BTN2,DIGPORTNONE,0},{AI06,BTN3,DIGPORTNONE,0},
    {AI07,BTN0,DIGPORTNONE,0},{AI07,BTN1,DIGPORTNONE,0},{AI07,BTN2,DIGPORTNONE,0},{AI07,BTN3,DIGPORTNONE,0},
    {AI08,BTN0,DIGPORTNONE,0},{AI08,BTN1,DIGPORTNONE,0},{AI08,BTN2,DIGPORTNONE,0},{AI08,BTN3,DIGPORTNONE,0},
    {AI09,BTN0,DIGPORTNONE,0},{AI09,BTN1,DIGPORTNONE,0},{AI09,BTN2,DIGPORTNONE,0},{AI09,BTN3,DIGPORTNONE,0},
    {AI10,BTN0,DIGPORTNONE,0},{AI10,BTN1,DIGPORTNONE,0},{AI10,BTN2,DIGPORTNONE,0},{AI10,BTN3,DIGPORTNONE,0},
    {AI11,BTN0,DIGPORTNONE,0},{AI11,BTN1,DIGPORTNONE,0},{AI11,BTN2,DIGPORTNONE,0},{AI11,BTN3,DIGPORTNONE,0}};

    /*_digitalOutputAssignment
      0 - digital pin
      1 - type  0 = LLIGHT, 1 = LSHADEUP, 2 = LSHADEDOWN, 3 = LFAN 
      2 - group number
      3 - digital output status 0 = OFF, 1 = ON
    */
    int _digitalOutputAssignment [_DIGITAL_OUTPUT_ASSIGNMENT_SIZE][4] = {
    {DO00,TYPENONE,GROUPNONE,OFF},{DO01,TYPENONE,GROUPNONE,OFF},{DO02,TYPENONE,GROUPNONE,OFF},{DO03,TYPENONE,GROUPNONE,OFF},
    {DO04,TYPENONE,GROUPNONE,OFF},{DO05,TYPENONE,GROUPNONE,OFF},{DO06,TYPENONE,GROUPNONE,OFF},{DO07,TYPENONE,GROUPNONE,OFF},
    {RO00,TYPENONE,GROUPNONE,OFF},{RO01,TYPENONE,GROUPNONE,OFF},{RO02,TYPENONE,GROUPNONE,OFF},{RO03,TYPENONE,GROUPNONE,OFF},
    {RO04,TYPENONE,GROUPNONE,OFF},{RO05,TYPENONE,GROUPNONE,OFF},{RO06,TYPENONE,GROUPNONE,OFF},{RO07,TYPENONE,GROUPNONE,OFF},
    {RO08,TYPENONE,GROUPNONE,OFF},{RO09,TYPENONE,GROUPNONE,OFF}};

    const char *_digitalOutputZones[_DIGITAL_OUTPUT_ASSIGNMENT_SIZE] = {
      ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE,
      ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE, ZONE_NONE
    };
    int _digitalOutputZoneIndexes[_DIGITAL_OUTPUT_ASSIGNMENT_SIZE] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /* _digitalOutputAssignmentClock
      Array extends _digitalOutputAssignment to capture millis (current time) if Fan or Shade operations are being processed.
      OFF - 0
    */
    unsigned long _digitalOutputAssignmentClock [_DIGITAL_OUTPUT_ASSIGNMENT_CLOCK_SIZE] = {OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF};


};

class LanceControllinoRuntime
{
  public:
    LanceControllinoRuntime(LanceControllino &controller, bool mqttEvents);
    void setup();
    void loop();
    void handleMQTTCallback(char* topic, byte* payload, unsigned int length);
    static void mqttCallbackRouter(char* topic, byte* payload, unsigned int length);

  private:
    static LanceControllinoRuntime *_activeInstance;

    static const unsigned int _maxMQTTMessageLength = 128;
#if ENABLE_HOME_ASSISTANT_MQTT
    static const unsigned int _maxTopicLength = 96;
    static const unsigned int _maxMQTTPacketSize = 192;
#else
    static const unsigned int _maxTopicLength = 64;
    static const unsigned int _maxMQTTPacketSize = 256;
#endif
    static const unsigned long _statusInterval = 200;
    static const unsigned long _analogInputInterval = 40;
    static const unsigned long _mqttReconnectInterval = 5000;
    static const unsigned long _mqttLoopInterval = 200;
    static const unsigned long _bufferStatsInterval = 30000;

    LanceControllino &_controller;
    EthernetClient _ethClient;
    PubSubClient _mqttClient;
    Credentials _credentials;

    byte _mac[6];
    IPAddress _localIp;
    IPAddress _gatewayIp;
    IPAddress _subnetMask;
    IPAddress _mqttServerIp;
    uint16_t _mqttPort;

    const char *_clientId;
    const char *_mqttUserName;
    const char *_mqttPassword;
    char _mqttMainTopicSubscribe[_maxTopicLength];
    char _mqttMainTopicPublishAdmin[_maxTopicLength];
    char _mqttMainTopicPublishNormal[_maxTopicLength];
    char _mqttAvailabilityTopic[_maxTopicLength];
    char _mqttCommandSubscribeTopic[_maxTopicLength];
    bool _mqttEvents;
    unsigned long _currentTime;
    unsigned long _previousStatusTime;
    unsigned long _previousAnalogTime;
    unsigned long _previousMQTTReconnectTime;
    unsigned long _previousMQTTLoopTime;
    unsigned long _previousBufferStatsTime;

    void printStartupBanner();
    bool loadCredentialsFromEEPROM();
    void configureMqttTopics(const char *topicBaseName);
    static IPAddress parseIPAddress(const char *ipStr);
    void publishFromBuffer();
    bool publishRetained(const char *topic, const char *payload);
    bool publishHomeAssistantStateFromMessage(const String &message);
    void publishHomeAssistantStateForOutput(int outputIndex);
    void publishAllHomeAssistantStates();
    bool handleHomeAssistantCommandTopic(const char *topic, const char *payload);
    bool isPrimaryEntityOutput(int outputIndex) const;
    void buildEntityObjectId(int outputIndex, char *buffer, size_t bufferSize) const;
    void buildEntityTopics(int outputIndex, char *stateTopic, size_t stateTopicSize, char *commandTopic, size_t commandTopicSize) const;
    void connectMQTTIfNeeded();
    void printBufferStats();
};
#endif
