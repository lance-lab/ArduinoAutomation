/*
  lancecontrollino.cpp - Library for enabling Controllino Maxi to smart home
  - Handle Lights, Fans and Shades
  - Posting messages to buffer. Messages are ready to be pull and publish on MQTT. Last update:
*/

#include <Controllino.h>
#include <Ethernet.h>
#include <stdio.h>
#include "RingBuf.h"
#include "LanceControllino.h"

LanceControllinoRuntime *LanceControllinoRuntime::_activeInstance = NULL;

namespace {
const __FlashStringHelper *hardwareStatusToText(EthernetHardwareStatus status)
{
  switch (status) {
    case EthernetNoHardware:
      return F("EthernetNoHardware");
    case EthernetW5100:
      return F("EthernetW5100");
    case EthernetW5200:
      return F("EthernetW5200");
    case EthernetW5500:
      return F("EthernetW5500");
    default:
      return F("Unknown");
  }
}

const __FlashStringHelper *linkStatusToText(EthernetLinkStatus status)
{
  switch (status) {
    case Unknown:
      return F("Unknown");
    case LinkON:
      return F("LinkON");
    case LinkOFF:
      return F("LinkOFF");
    default:
      return F("Unknown");
  }
}

const char *outputTypeToEntityComponent(int type)
{
  switch (type) {
    case LLIGHT:
      return "light";
    case LFAN:
      return "fan";
    case LSHADEUP:
      return "cover";
    default:
      return "unknown";
  }
}

const char *coverStateToPayload(int state)
{
  switch (state) {
    case COVER_STATE_OPENING:
      return "OPENING";
    case COVER_STATE_CLOSING:
      return "CLOSING";
    default:
      return "STOPPED";
  }
}
}

LanceControllino::LanceControllino(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize) 
{
  _initialized = initialization(analogAssignment, digitalAssignment, analogAssignmentSize, digitalAssignmentSize);
  _portActivated = portActivation();
  _eventActivated = false;
}  

LanceControllino::LanceControllino(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize, bool events) 
{
  _initialized = initialization(analogAssignment, digitalAssignment, analogAssignmentSize, digitalAssignmentSize);
  _portActivated = portActivation();
  _eventActivated = events;
}  

bool LanceControllino::parseMQTTMessage(const String &message, String &type, String &zone, int &index, String &status)
{
  int typePos = message.indexOf("TYPE=");
  int zonePos = message.indexOf("ZONE=");
  int indexPos = message.indexOf("INDEX=");
  int statusPos = message.indexOf("STATUS=");

  if (typePos == -1 || zonePos == -1 || indexPos == -1 || statusPos == -1) {
    Serial.println("ERROR: Missing required MQTT fields");
    return false;
  }

  int typeEnd = message.indexOf(":", typePos);
  int zoneEnd = message.indexOf(":", zonePos);
  int indexEnd = message.indexOf(":", indexPos);
  int statusEnd = message.indexOf(":", statusPos);

  if (typeEnd == -1) typeEnd = message.length();
  if (zoneEnd == -1) zoneEnd = message.length();
  if (indexEnd == -1) indexEnd = message.length();
  if (statusEnd == -1) statusEnd = message.length();

  type = message.substring(typePos + 5, typeEnd);
  zone = message.substring(zonePos + 5, zoneEnd);
  String indexText = message.substring(indexPos + 6, indexEnd);
  status = message.substring(statusPos + 7, statusEnd);

  if (status != "ON" && status != "OFF") {
    Serial.println("ERROR: Invalid STATUS value: " + status);
    return false;
  }

  if (zone.length() == 0) {
    Serial.println("ERROR: Invalid ZONE value");
    return false;
  }

  index = indexText.toInt();
  if (index <= 0) {
    Serial.println("ERROR: Invalid INDEX value: " + indexText);
    return false;
  }

  return true;
}

bool LanceControllino::parseLegacyMQTTMessage(const String &message, String &type, int &port, String &status)
{
  int typePos = message.indexOf("TYPE=");
  int portPos = message.indexOf("PORT=");
  int statusPos = message.indexOf("STATUS=");

  if (typePos == -1 || portPos == -1 || statusPos == -1) {
    return false;
  }

  int typeEnd = message.indexOf(":", typePos);
  int portEnd = message.indexOf(":", portPos);
  int statusEnd = message.indexOf(":", statusPos);

  if (typeEnd == -1) typeEnd = message.length();
  if (portEnd == -1) portEnd = message.length();
  if (statusEnd == -1) statusEnd = message.length();

  type = message.substring(typePos + 5, typeEnd);
  String portText = message.substring(portPos + 5, portEnd);
  status = message.substring(statusPos + 7, statusEnd);

  if (status != "ON" && status != "OFF") {
    Serial.println("ERROR: Invalid STATUS value: " + status);
    return false;
  }

  port = portText.toInt();
  if (port <= 0 && portText != "0") {
    Serial.println("ERROR: Invalid PORT value: " + portText);
    return false;
  }

  return true;
}

LanceControllinoRuntime::LanceControllinoRuntime(LanceControllino &controller,
                                                 bool mqttEvents)
  : _controller(controller),
    _mqttClient(_ethClient),
    _localIp(0, 0, 0, 0),
    _gatewayIp(0, 0, 0, 0),
    _subnetMask(0, 0, 0, 0),
    _mqttServerIp(0, 0, 0, 0),
    _mqttPort(0),
    _clientId(NULL),
    _mqttUserName(NULL),
    _mqttPassword(NULL),
    _mqttEvents(mqttEvents),
    _currentTime(0),
    _previousStatusTime(0),
    _previousAnalogTime(0),
    _previousMQTTReconnectTime(0),
    _previousMQTTLoopTime(0),
    _previousBufferStatsTime(0)
{
  memset(_mac, 0, sizeof(_mac));
  memset(&_credentials, 0, sizeof(_credentials));
  configureMqttTopics(NULL);
}

void LanceControllinoRuntime::printStartupBanner()
{
  const char *deviceName = _clientId;

  if (deviceName == NULL || deviceName[0] == '\0') {
    deviceName = "LanceControllino";
  }

  Serial.println("\n====================================");
  Serial.println(String("    ") + deviceName + " Startup");
  Serial.println("====================================\n");
}

bool LanceControllinoRuntime::loadCredentialsFromEEPROM()
{
  if (!CredentialManager::loadCredentials(_credentials)) {
    Serial.println("\nFATAL ERROR: Cannot load device configuration from EEPROM!");
    Serial.println("Please upload SetCredentials.ino first to program your device details");
    return false;
  }

  memcpy(_mac, _credentials.mac, sizeof(_mac));
  _localIp = IPAddress(_credentials.localIp[0], _credentials.localIp[1], _credentials.localIp[2], _credentials.localIp[3]);
  _gatewayIp = IPAddress(_credentials.gatewayIp[0], _credentials.gatewayIp[1], _credentials.gatewayIp[2], _credentials.gatewayIp[3]);
  _subnetMask = IPAddress(_credentials.subnetMask[0], _credentials.subnetMask[1], _credentials.subnetMask[2], _credentials.subnetMask[3]);
  _mqttPort = _credentials.mqttPort;
  _clientId = _credentials.mqttClientId;
  _mqttUserName = _credentials.mqttUser;
  _mqttPassword = _credentials.mqttPass;
  _mqttServerIp = parseIPAddress(_credentials.mqttServer);
  configureMqttTopics(_clientId);

  Serial.print("Local IP: ");
  Serial.println(_localIp);
  Serial.println("MQTT Broker: " + String(_credentials.mqttServer));
  Serial.println("MQTT Client ID: " + String(_credentials.mqttClientId));
  return true;
}

void LanceControllinoRuntime::configureMqttTopics(const char *topicBaseName)
{
  const char *baseName = topicBaseName;

  if (baseName == NULL || baseName[0] == '\0') {
    _mqttMainTopicPublishNormal[0] = '\0';
    _mqttMainTopicPublishAdmin[0] = '\0';
    _mqttMainTopicSubscribe[0] = '\0';
    _mqttAvailabilityTopic[0] = '\0';
    _mqttCommandSubscribeTopic[0] = '\0';
    return;
  }

  _mqttMainTopicPublishNormal[0] = '\0';
  _mqttMainTopicPublishAdmin[0] = '\0';
  _mqttMainTopicSubscribe[0] = '\0';
  snprintf(_mqttAvailabilityTopic, sizeof(_mqttAvailabilityTopic), "controllino/%s/status", baseName);
  snprintf(_mqttCommandSubscribeTopic, sizeof(_mqttCommandSubscribeTopic), "controllino/%s/+/+/set", baseName);
}

IPAddress LanceControllinoRuntime::parseIPAddress(const char *ipStr)
{
  int parts[4] = {0, 0, 0, 0};
  sscanf(ipStr, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
  return IPAddress((uint8_t)parts[0], (uint8_t)parts[1], (uint8_t)parts[2], (uint8_t)parts[3]);
}

void LanceControllinoRuntime::setup()
{
  _activeInstance = this;

  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(50);

  if (!_mqttEvents) {
    Serial.println("\n====================================");
    Serial.println("    LanceControllino Offline Startup");
    Serial.println("====================================\n");
    Serial.println("MQTT events disabled, skipping EEPROM, Ethernet, and MQTT setup");
    Serial.println("Starting watchdog timer (8 seconds)...");
    wdt_enable(WDTO_8S);
    Serial.println("Setup complete!\n");
    return;
  }

  if (!loadCredentialsFromEEPROM()) {
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }

  printStartupBanner();

  if (ETHERNET_CHIP_SELECT_PIN >= 0) {
    Ethernet.init(ETHERNET_CHIP_SELECT_PIN);
  }
  Ethernet.begin(_mac, _localIp, _gatewayIp, _gatewayIp, _subnetMask);
  Serial.println("Ethernet initialized");
  Serial.print("Hardware status: ");
  Serial.println(hardwareStatusToText(Ethernet.hardwareStatus()));
  Serial.print("Link status: ");
  Serial.println(linkStatusToText(Ethernet.linkStatus()));
  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway IP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("MQTT target: ");
  Serial.print(_mqttServerIp);
  Serial.print(":");
  Serial.println(_mqttPort);
  Serial.print("MQTT availability topic: ");
  Serial.println(_mqttAvailabilityTopic);
  Serial.print("MQTT command wildcard: ");
  Serial.println(_mqttCommandSubscribeTopic);

  _mqttClient.setClient(_ethClient);
  _mqttClient.setBufferSize(_maxMQTTPacketSize);
  _mqttClient.setServer(_mqttServerIp, _mqttPort);
  _mqttClient.setCallback(LanceControllinoRuntime::mqttCallbackRouter);

  connectMQTTIfNeeded();

  Serial.println("Starting watchdog timer (8 seconds)...");
  wdt_enable(WDTO_8S);
  Serial.println("Setup complete!\n");
}

void LanceControllinoRuntime::handleMQTTCallback(char* topic, byte* payload, unsigned int length)
{
  if (length == 0 || length > _maxMQTTMessageLength) {
    return;
  }

  char payloadText[_maxMQTTMessageLength + 1];
  memcpy(payloadText, payload, length);
  payloadText[length] = '\0';

  Serial.print("MQTT RX topic: ");
  Serial.println(topic);
  Serial.print("MQTT RX payload: ");
  Serial.println(payloadText);

  if (handleHomeAssistantCommandTopic(topic, payloadText)) {
    return;
  }
  Serial.print("Ignoring unknown MQTT topic: ");
  Serial.println(topic);
}

void LanceControllinoRuntime::mqttCallbackRouter(char* topic, byte* payload, unsigned int length)
{
  if (_activeInstance != NULL) {
    _activeInstance->handleMQTTCallback(topic, payload, length);
  }
}

void LanceControllinoRuntime::publishFromBuffer()
{
  if (!_mqttClient.connected()) {
    return;
  }

  if (_controller.pullFromBuffer()) {
    String bufferedMessage = String(_controller.bufferPull._topicMessage);

    if (!publishHomeAssistantStateFromMessage(bufferedMessage)) {
      Serial.println("ERROR: MQTT publish failed, re-queueing event");
      _controller.addToBuffer(_controller.bufferPull._topic, bufferedMessage);
    }
  }
}

void LanceControllinoRuntime::connectMQTTIfNeeded()
{
  if (!_mqttEvents || _mqttClient.connected()) {
    return;
  }

  Serial.println("Attempting MQTT connection...");
  if (_mqttClient.connect(_clientId, _mqttUserName, _mqttPassword, _mqttAvailabilityTopic, 0, true, "offline")) {
    Serial.println("MQTT connected");
    publishRetained(_mqttAvailabilityTopic, "online");
    if (_mqttClient.subscribe(_mqttCommandSubscribeTopic)) {
      Serial.println("Subscribed to: " + String(_mqttCommandSubscribeTopic));
    } else {
      Serial.println("ERROR: MQTT subscribe failed: " + String(_mqttCommandSubscribeTopic));
    }
  } else {
    Serial.println("Failed to connect to MQTT broker");
    Serial.print("MQTT state: ");
    Serial.println(_mqttClient.state());
  }
}

void LanceControllinoRuntime::printBufferStats()
{
  if (!_mqttEvents) {
    return;
  }

  unsigned int droppedMessages = 0;
  unsigned int totalMessages = 0;
  _controller.getBufferStats(droppedMessages, totalMessages);

  if (droppedMessages > 0) {
    Serial.println("WARNING: Buffer overflow detected!");
    Serial.println("Dropped messages: " + String(droppedMessages) + " / " + String(totalMessages));
  } else if (totalMessages > 0) {
    Serial.println("Buffer OK - Messages published: " + String(totalMessages));
  }
}

void LanceControllinoRuntime::loop()
{
  _currentTime = millis();

  if ((_currentTime - _previousStatusTime) >= _statusInterval) {
    _previousStatusTime = _currentTime;
    if (_mqttEvents) {
      publishFromBuffer();
    }
    _controller.statusTimeVerification();
  }

  if ((_currentTime - _previousAnalogTime) >= _analogInputInterval) {
    _previousAnalogTime = _currentTime;
    _controller.analogInputVerification();
  }

  if ((_currentTime - _previousMQTTReconnectTime) >= _mqttReconnectInterval) {
    _previousMQTTReconnectTime = _currentTime;
    connectMQTTIfNeeded();
  }

  if ((_currentTime - _previousMQTTLoopTime) >= _mqttLoopInterval) {
    _previousMQTTLoopTime = _currentTime;
    if (_mqttEvents) {
      _mqttClient.loop();
    }
  }

  if ((_currentTime - _previousBufferStatsTime) >= _bufferStatsInterval) {
    _previousBufferStatsTime = _currentTime;
    printBufferStats();
  }

  wdt_reset();
}

bool LanceControllino::initialization(int analogAssignment[][3], DigitalOutputConfig digitalAssignment[], int analogAssignmentSize, int digitalAssignmentSize) 
{
  /*
  Map input arrays to global arrays
  */
  for (int i=0; i<analogAssignmentSize;i++) {
    int indexAnalogInputs = -1;
    int indexDigitalAssignment = -1;
    int indexAnalogInputAssignment = -1;
    int indexDigitalOutputAssignment = -1;

    for (int j=0; j<_ANALOG_INPUTS_SIZE;j++) {
      if (_analogInputs[0][j] == analogAssignment[i][0]) {
        indexAnalogInputs = j;
        break;
      }
    }

    for (int j=0; j<digitalAssignmentSize;j++) {
      if (digitalAssignment[j].port == analogAssignment[i][2]) {
        indexDigitalAssignment = j;
        break;
      }
    }
    
    for (int j=0; j<_ANALOG_INPUT_ASSIGNMENT_SIZE;j++) {
      if (_analogInputAssignment[j][0] == analogAssignment[i][0] && _analogInputAssignment[j][1] == analogAssignment[i][1]) {
        indexAnalogInputAssignment = j;
        break;
      }
    }
    
    for (int j=0; j<_DIGITAL_OUTPUT_ASSIGNMENT_SIZE;j++) {
      if (_digitalOutputAssignment[j][0] == analogAssignment[i][2]) {
        indexDigitalOutputAssignment = j;
        break;
      }
    }

    if (indexDigitalAssignment != -1 && indexAnalogInputAssignment != -1 && indexDigitalOutputAssignment != -1 && indexAnalogInputs != -1) {
      _analogInputAssignment[indexAnalogInputAssignment][2] = analogAssignment[i][2];
      _digitalOutputAssignment[indexDigitalOutputAssignment][1] = digitalAssignment[indexDigitalAssignment].type;
      _digitalOutputAssignment[indexDigitalOutputAssignment][2] = digitalAssignment[indexDigitalAssignment].group;
      _digitalOutputZones[indexDigitalOutputAssignment] = digitalAssignment[indexDigitalAssignment].zone;
      _digitalOutputZoneIndexes[indexDigitalOutputAssignment] = digitalAssignment[indexDigitalAssignment].index;
      _analogInputs[1][indexAnalogInputs] = ENABLED;
    }
    else {
      return false;
    }
  }

  for (int i = 0; i < digitalAssignmentSize; i++) {
    int indexDigitalOutputAssignment = -1;

    for (int j = 0; j < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE; j++) {
      if (_digitalOutputAssignment[j][0] == digitalAssignment[i].port) {
        indexDigitalOutputAssignment = j;
        break;
      }
    }

    if (indexDigitalOutputAssignment == -1) {
      return false;
    }

    _digitalOutputAssignment[indexDigitalOutputAssignment][1] = digitalAssignment[i].type;
    _digitalOutputAssignment[indexDigitalOutputAssignment][2] = digitalAssignment[i].group;
    _digitalOutputZones[indexDigitalOutputAssignment] = digitalAssignment[i].zone;
    _digitalOutputZoneIndexes[indexDigitalOutputAssignment] = digitalAssignment[i].index;
  }

  return true;
}

bool LanceControllino::addToBuffer(int topic, String message)
{
  /*
  Add message to event buffer. Returns false if buffer is full (overflow).
  When buffer overflows, the message is dropped and overflow counter increments.
  This indicates that the MQTT publish loop is not reading messages fast enough.
  */
  if (_eventActivated == true) {
    _totalAddedCount++;
    memset(&bufferAdd, 0, sizeof(struct _eventMessage));

    bufferAdd._topic = topic;
    message.toCharArray(bufferAdd._topicMessage, message.length()+1);
    
    if(_messageBuffer->add(_messageBuffer, &bufferAdd) != -1){
      return true;
    }
    
    // Buffer overflow occurred
    _droppedMessageCount++;
    Serial.println("ERROR: Event buffer overflow! Dropped message. Total dropped: " + String(_droppedMessageCount));
    Serial.println("Message: " + message);
    Serial.println("Buffer size: " + String(_MESSAGE_BUFFER_SIZE) + " | Dropped: " + String(_droppedMessageCount) + " / " + String(_totalAddedCount));
    
    return false;
  }
  return true;
}

bool LanceControllino::pullFromBuffer()
{
  if (_eventActivated == true) {
    memset(&bufferPull, 0, sizeof(struct _eventMessage));
    
    if(_messageBuffer->pull(_messageBuffer, &bufferPull) != NULL){
      return true;
    }
    return false;
  }
  return false;
}

int LanceControllino::getOutputCount() const
{
  return _DIGITAL_OUTPUT_ASSIGNMENT_SIZE;
}

bool LanceControllino::isOutputConfigured(int outputIndex) const
{
  return outputIndex >= 0 &&
         outputIndex < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE &&
         _digitalOutputAssignment[outputIndex][1] != TYPENONE;
}

int LanceControllino::getOutputPort(int outputIndex) const
{
  return _digitalOutputAssignment[outputIndex][0];
}

int LanceControllino::getOutputType(int outputIndex) const
{
  return _digitalOutputAssignment[outputIndex][1];
}

int LanceControllino::getOutputGroup(int outputIndex) const
{
  return _digitalOutputAssignment[outputIndex][2];
}

int LanceControllino::getOutputStatus(int outputIndex) const
{
  return _digitalOutputAssignment[outputIndex][3];
}

const char *LanceControllino::getOutputZone(int outputIndex) const
{
  return _digitalOutputZones[outputIndex];
}

int LanceControllino::getOutputZoneIndex(int outputIndex) const
{
  return _digitalOutputZoneIndexes[outputIndex];
}

int LanceControllino::getCoverState(const String &zone, int index) const
{
  bool shadeUpOn = false;
  bool shadeDownOn = false;

  for (int i = 0; i < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (String(_digitalOutputZones[i]) != zone || _digitalOutputZoneIndexes[i] != index) {
      continue;
    }

    if (_digitalOutputAssignment[i][1] == LSHADEUP && _digitalOutputAssignment[i][3] == ON) {
      shadeUpOn = true;
    } else if (_digitalOutputAssignment[i][1] == LSHADEDOWN && _digitalOutputAssignment[i][3] == ON) {
      shadeDownOn = true;
    }
  }

  if (shadeUpOn) {
    return COVER_STATE_OPENING;
  }

  if (shadeDownOn) {
    return COVER_STATE_CLOSING;
  }

  return COVER_STATE_STOPPED;
}

void LanceControllino::getBufferStats(unsigned int &dropped, unsigned int &total)
{
  /*
  Get buffer overflow statistics for monitoring.
  - dropped: Number of messages dropped due to buffer overflow
  - total: Total number of messages attempted to add
  
  Useful for diagnosing if the MQTT publish loop is too slow to handle the event rate.
  */
  dropped = _droppedMessageCount;
  total = _totalAddedCount;
}

bool LanceControllino::portActivation()
{
 //ACTIVATE ALL ANALOG INPUTS FROM AnalogInputMapped ARRAY
  for (int i=0; i<_ANALOG_INPUTS_SIZE;i++) { // Assign I/O to Analog PINs
    if (_analogInputs[1][i] == ENABLED) {
      pinMode(_analogInputs[0][i], INPUT); //Set all PINs from AnalogInputMapped array as INPUT
    }
  }  

  //ACTIVATE ALL DIGITAL OUTPUTS FROM ANALOG INPUT MAPPED ARRAY
  for (int i=0; i<_DIGITAL_OUTPUTS_SIZE;i++) { // Assign I/O to Relay PINs (10 RELAYS AVAILABLE)
    pinMode(_digitalOutputs[0][i], OUTPUT); // Set all PINs from DigitalOutputMapped array as OUTPUT
  }
  return true;
}

void LanceControllino::analogInputVerification()
{
  for (int i=0; i<_ANALOG_INPUTS_SIZE;i++) { // Assign I/O to Analog PINs
    if (_analogInputs[1][i] == ENABLED) {
      int pressedStatus = readKey(_analogInputs[0][i]);
      if (pressedStatus == BTNNONE) {
        int counter = 0;
        for (int j=0; j<_ANALOG_INPUT_ASSIGNMENT_SIZE;j++) {
          if (_analogInputAssignment[j][0] == _analogInputs[0][i]) {
            if (_analogInputAssignment[j][3] == PRESSED_BTN_COUNT || _analogInputAssignment[j][3] > PRESSED_BTN_COUNT) {
              int digitalOutputAssignmentIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, _analogInputAssignment[j][2]);
              
              switch (_digitalOutputAssignment[digitalOutputAssignmentIndex][1]) {
                case LLIGHT: {lightOperation(_digitalOutputAssignment[digitalOutputAssignmentIndex][0]); break;}
                case LSHADEUP: {shadeOperation(_digitalOutputAssignment[digitalOutputAssignmentIndex][0]); break;}
                case LSHADEDOWN: {shadeOperation(_digitalOutputAssignment[digitalOutputAssignmentIndex][0]); break;}
                case LFAN: {fanOperation(_digitalOutputAssignment[digitalOutputAssignmentIndex][0]); break;}
              }
              _analogInputAssignment[j][3] = OFF;
            } else {
              _analogInputAssignment[j][3] = OFF;
            }
          }
        }
      }
      else {
        for (int k=0; k<_ANALOG_INPUT_ASSIGNMENT_SIZE;k++) {
          if (_analogInputAssignment[k][0] == _analogInputs[0][i] && _analogInputAssignment[k][1] == pressedStatus) {
            _analogInputAssignment[k][3]++;
            break;
          }
        }
      }  
    }
  }  
}

bool LanceControllino::lightOperation(int port, int status)
{
  int digitalOutputsIndex = findElementRow(_digitalOutputs[0], _DIGITAL_OUTPUTS_SIZE, port);
  int digitalOutputAssignmentIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);
  int pinStatus = -1;
  String message = "";

  //FIND CURRENT STATUS OF DIGITAL OUTPUT PINS
  if (digitalOutputsIndex != -1 && digitalOutputAssignmentIndex != -1) {
    switch (_digitalOutputs[1][digitalOutputsIndex]) {
      case _PORTA: { pinStatus = bitRead(PORTA,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTB: { pinStatus = bitRead(PORTB,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTC: { pinStatus = bitRead(PORTC,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTD: { pinStatus = bitRead(PORTD,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTE: { pinStatus = bitRead(PORTE,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTF: { pinStatus = bitRead(PORTF,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTG: { pinStatus = bitRead(PORTG,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTH: { pinStatus = bitRead(PORTH,_digitalOutputs[2][digitalOutputsIndex]); break;}
    }


    if (pinStatus == ON) {
      if (status == ON) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "LIGHT", ON);
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, LOW);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "LIGHT", OFF);
        addToBuffer(1, message);
      }
    }
    else if (pinStatus == OFF) {
      if (status == OFF) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "LIGHT", OFF);
        addToBuffer(1, message);
        Serial.println("no_action");
      }
      else {
        //digitalWrite(port, HIGH);
        digitalWrite(port, HIGH);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "LIGHT", ON);
        addToBuffer(1, message);
      }
    } else {
      return false; // Could not find port
    }
  }
  return true;
}

bool LanceControllino::shadeOperation(int port, int status) {
  //IF SHADE IS GOING UP AND BUTTON DOWN IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE

  int shadeActionPortIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);
  
  // Return false if port not found
  if (shadeActionPortIndex == -1) {
    return false;
  }
  
  int shadeGroup = _digitalOutputAssignment[shadeActionPortIndex][2];

  int shadeUpPortIndex = -1;
  int shadeDownPortIndex = -1;

  String messageUp = "";
  String messageDown = "";

  for(int i=0; i<_DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (_digitalOutputAssignment[i][2] == shadeGroup && _digitalOutputAssignment[i][1] == LSHADEUP) {shadeUpPortIndex = i;}
    else if (_digitalOutputAssignment[i][2] == shadeGroup && _digitalOutputAssignment[i][1] == LSHADEDOWN) {shadeDownPortIndex = i;}
  }

  if (shadeUpPortIndex != -1 && shadeDownPortIndex != -1) {

    //IF SHADE IS GOING UP AND BUTTON DOWN IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE
    if (_digitalOutputAssignment[shadeActionPortIndex][1] == LSHADEDOWN && _digitalOutputAssignment[shadeUpPortIndex][3] == ON) {
      
      digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
      digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
      _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
      _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
      _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
      _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
      messageUp = buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", OFF);
      messageDown = buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", OFF);
      addToBuffer(1, messageUp);
      addToBuffer(1, messageDown);
    }

    //IF SHADE IS GOING DOWN AND BUTTON UP IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE
    else if (_digitalOutputAssignment[shadeActionPortIndex][1] == LSHADEUP && _digitalOutputAssignment[shadeDownPortIndex][3] == ON) {
      digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
      digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
      _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
      _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
      _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
      _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
      messageUp = buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", OFF);
      messageDown = buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", OFF);
      addToBuffer(1, messageUp);
      addToBuffer(1, messageDown);    
    }

    //IF UP WAS PRESSED THEN SET THE SHADE TO MOVE UP
    else if (_digitalOutputAssignment[shadeActionPortIndex][1] == LSHADEUP && _digitalOutputAssignment[shadeUpPortIndex][3] == OFF) {

      digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
      digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], HIGH);
      _digitalOutputAssignment[shadeUpPortIndex][3] = ON;
      _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
      _digitalOutputAssignmentClock[shadeUpPortIndex] = millis();
      _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
      messageUp = buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", ON);
      messageDown = buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", OFF);
      addToBuffer(1, messageUp);
      addToBuffer(1, messageDown);
    }
    //IF DOWN WAS PRESSED THEN SET THE SHADE TO MOVE DOWN
    else if (_digitalOutputAssignment[shadeActionPortIndex][1] == LSHADEDOWN && _digitalOutputAssignment[shadeDownPortIndex][3] == OFF) {

      digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
      digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], HIGH);
      _digitalOutputAssignment[shadeDownPortIndex][3] = ON;
      _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
      _digitalOutputAssignmentClock[shadeDownPortIndex] = millis();
      _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
      messageUp = buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", OFF);
      messageDown = buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", ON);
      addToBuffer(1, messageUp);
      addToBuffer(1, messageDown);
    }
  }
  return true;
}

bool LanceControllino::fanOperation(int port, int status)
{
  int digitalOutputsIndex = findElementRow(_digitalOutputs[0], _DIGITAL_OUTPUTS_SIZE, port);
  int digitalOutputAssignmentIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);
  int pinStatus = -1;

  String message = "";

  //FIND CURRENT STATUS OF DIGITAL OUTPUT PINS
  if (digitalOutputsIndex != -1 && digitalOutputAssignmentIndex != -1) {
    switch (_digitalOutputs[1][digitalOutputsIndex]) {
      case _PORTA: { pinStatus = bitRead(PORTA,_digitalOutputs[2][digitalOutputsIndex]); break;} 
      case _PORTB: { pinStatus = bitRead(PORTB,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTC: { pinStatus = bitRead(PORTC,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTD: { pinStatus = bitRead(PORTD,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTE: { pinStatus = bitRead(PORTE,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTF: { pinStatus = bitRead(PORTF,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTG: { pinStatus = bitRead(PORTG,_digitalOutputs[2][digitalOutputsIndex]); break;}
      case _PORTH: { pinStatus = bitRead(PORTH,_digitalOutputs[2][digitalOutputsIndex]); break;}
    }
  
    if (pinStatus == ON) {
      if (status == ON) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = millis();
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "FAN", ON);
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, LOW);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = OFF;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "FAN", OFF);
        addToBuffer(1, message);
      }
    }
    else if (pinStatus == OFF) {
      if (status == OFF) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = OFF;
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "FAN", OFF);
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, HIGH);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = millis();
        message = buildMqttStatusMessage(digitalOutputAssignmentIndex, "FAN", ON);
        addToBuffer(1, message);
      }
    } else {
      return false; // Could not find port
    }
  }
  return true;
}

void LanceControllino::statusTimeVerification() {
  /*
  Verify timeout conditions for shades and fans.
  Uses safe unsigned integer subtraction to handle millis() overflow (every ~49 days).
  */
  for(int i=0; i<_DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (_digitalOutputAssignment[i][1] == LSHADEUP || _digitalOutputAssignment[i][1] == LSHADEDOWN) {
      // Safe timeout check that works correctly with millis() overflow
      if (_digitalOutputAssignment[i][3] == ON &&
          (unsigned long)(millis() - _digitalOutputAssignmentClock[i]) >= RUN_SHADE_COUNT) {
        digitalWrite(_digitalOutputAssignment[i][0], LOW);
        _digitalOutputAssignment[i][3] = OFF;
        _digitalOutputAssignmentClock[i] = OFF;
        addToBuffer(1, buildMqttStatusMessage(i, _digitalOutputAssignment[i][1] == LSHADEUP ? "SHADEUP" : "SHADEDOWN", OFF));
      }
    }
    else if (_digitalOutputAssignment[i][1] == LFAN) {
      // Safe timeout check that works correctly with millis() overflow
      if (_digitalOutputAssignment[i][3] == ON &&
          (unsigned long)(millis() - _digitalOutputAssignmentClock[i]) >= RUN_FUN_COUNT) {
        digitalWrite(_digitalOutputAssignment[i][0], LOW);
        _digitalOutputAssignment[i][3] = OFF;
        _digitalOutputAssignmentClock[i] = OFF;
        addToBuffer(1, buildMqttStatusMessage(i, "FAN", OFF));
      }
    }
  }
}

String LanceControllino::buildMqttStatusMessage(int outputIndex, const char *typeName, int status)
{
  String message = "TYPE=";
  message += typeName;
  message += ":ZONE=";
  message += _digitalOutputZones[outputIndex];
  message += ":INDEX=";
  message += String(_digitalOutputZoneIndexes[outputIndex]);
  message += ":STATUS=";
  message += (status == ON) ? "ON" : "OFF";
  return message;
}

int LanceControllino::findOutputIndexByIdentity(const String &type, const String &zone, int index)
{
  for (int i = 0; i < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (String(_digitalOutputZones[i]) != zone || _digitalOutputZoneIndexes[i] != index) {
      continue;
    }

    switch (_digitalOutputAssignment[i][1]) {
      case LLIGHT:
        if (type == "LIGHT") return i;
        break;
      case LFAN:
        if (type == "FAN") return i;
        break;
      case LSHADEUP:
        if (type == "SHADEUP") return i;
        break;
      case LSHADEDOWN:
        if (type == "SHADEDOWN") return i;
        break;
    }
  }

  return -1;
}

bool LanceControllino::processExternalRequest(const String &type, const String &zone, int index, int status) {

  int digitalOutputAssignmentIndex = findOutputIndexByIdentity(type, zone, index);

  // Return false if port not found
  if (digitalOutputAssignmentIndex == -1) {
    return false;
  }

  int port = _digitalOutputAssignment[digitalOutputAssignmentIndex][0];

  switch (_digitalOutputAssignment[digitalOutputAssignmentIndex][1]) {
    case LLIGHT: {lightOperation(port, status); break;}
    case LSHADEUP: {shadeOperation(port, status); break;}
    case LSHADEDOWN: {shadeOperation(port, status); break;}
    case LFAN: {fanOperation(port, status); break;}
  }
  return true;
}

bool LanceControllino::processExternalRequest(int port, int status) {

  int digitalOutputAssignmentIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);

  if (digitalOutputAssignmentIndex == -1) {
    return false;
  }

  switch (_digitalOutputAssignment[digitalOutputAssignmentIndex][1]) {
    case LLIGHT: {lightOperation(port, status); break;}
    case LSHADEUP: {shadeOperation(port, status); break;}
    case LSHADEDOWN: {shadeOperation(port, status); break;}
    case LFAN: {fanOperation(port, status); break;}
  }
  return true;
}

bool LanceControllino::processCoverCommand(const String &zone, int index, const String &command)
{
  int shadeUpPortIndex = -1;
  int shadeDownPortIndex = -1;

  for (int i = 0; i < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (String(_digitalOutputZones[i]) != zone || _digitalOutputZoneIndexes[i] != index) {
      continue;
    }

    if (_digitalOutputAssignment[i][1] == LSHADEUP) {
      shadeUpPortIndex = i;
    } else if (_digitalOutputAssignment[i][1] == LSHADEDOWN) {
      shadeDownPortIndex = i;
    }
  }

  if (shadeUpPortIndex == -1 || shadeDownPortIndex == -1) {
    return false;
  }

  if (command == "OPEN") {
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], HIGH);
    _digitalOutputAssignment[shadeUpPortIndex][3] = ON;
    _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = millis();
    _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
  } else if (command == "CLOSE") {
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], HIGH);
    _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
    _digitalOutputAssignment[shadeDownPortIndex][3] = ON;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
    _digitalOutputAssignmentClock[shadeDownPortIndex] = millis();
  } else if (command == "STOP") {
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
    _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
    _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
    _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
  } else {
    return false;
  }

  addToBuffer(1, buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", _digitalOutputAssignment[shadeUpPortIndex][3]));
  addToBuffer(1, buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", _digitalOutputAssignment[shadeDownPortIndex][3]));
  return true;
}

bool LanceControllino::processCoverCommand(const char *zone, int index, const char *command)
{
  int shadeUpPortIndex = -1;
  int shadeDownPortIndex = -1;

  for (int i = 0; i < _DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (strcmp(_digitalOutputZones[i], zone) != 0 || _digitalOutputZoneIndexes[i] != index) {
      continue;
    }

    if (_digitalOutputAssignment[i][1] == LSHADEUP) {
      shadeUpPortIndex = i;
    } else if (_digitalOutputAssignment[i][1] == LSHADEDOWN) {
      shadeDownPortIndex = i;
    }
  }

  if (shadeUpPortIndex == -1 || shadeDownPortIndex == -1) {
    return false;
  }

  if (strcmp(command, "OPEN") == 0) {
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], HIGH);
    _digitalOutputAssignment[shadeUpPortIndex][3] = ON;
    _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = millis();
    _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
  } else if (strcmp(command, "CLOSE") == 0) {
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], HIGH);
    _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
    _digitalOutputAssignment[shadeDownPortIndex][3] = ON;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
    _digitalOutputAssignmentClock[shadeDownPortIndex] = millis();
  } else if (strcmp(command, "STOP") == 0) {
    digitalWrite(_digitalOutputAssignment[shadeUpPortIndex][0], LOW);
    digitalWrite(_digitalOutputAssignment[shadeDownPortIndex][0], LOW);
    _digitalOutputAssignment[shadeUpPortIndex][3] = OFF;
    _digitalOutputAssignment[shadeDownPortIndex][3] = OFF;
    _digitalOutputAssignmentClock[shadeUpPortIndex] = OFF;
    _digitalOutputAssignmentClock[shadeDownPortIndex] = OFF;
  } else {
    return false;
  }

  addToBuffer(1, buildMqttStatusMessage(shadeUpPortIndex, "SHADEUP", _digitalOutputAssignment[shadeUpPortIndex][3]));
  addToBuffer(1, buildMqttStatusMessage(shadeDownPortIndex, "SHADEDOWN", _digitalOutputAssignment[shadeDownPortIndex][3]));
  return true;
}

bool LanceControllinoRuntime::publishRetained(const char *topic, const char *payload)
{
  return _mqttClient.publish(topic, payload, true);
}

bool LanceControllinoRuntime::isPrimaryEntityOutput(int outputIndex) const
{
  return _controller.isOutputConfigured(outputIndex) && _controller.getOutputType(outputIndex) != LSHADEDOWN;
}

void LanceControllinoRuntime::buildEntityObjectId(int outputIndex, char *buffer, size_t bufferSize) const
{
  snprintf(
    buffer,
    bufferSize,
    "%s_%s_%d",
    outputTypeToEntityComponent(_controller.getOutputType(outputIndex)),
    _controller.getOutputZone(outputIndex),
    _controller.getOutputZoneIndex(outputIndex)
  );
}

void LanceControllinoRuntime::buildEntityTopics(int outputIndex, char *stateTopic, size_t stateTopicSize, char *commandTopic, size_t commandTopicSize) const
{
  char objectId[_maxTopicLength];
  buildEntityObjectId(outputIndex, objectId, sizeof(objectId));

  snprintf(stateTopic, stateTopicSize, "controllino/%s/%s/%s/state", _clientId, outputTypeToEntityComponent(_controller.getOutputType(outputIndex)), objectId);
  snprintf(commandTopic, commandTopicSize, "controllino/%s/%s/%s/set", _clientId, outputTypeToEntityComponent(_controller.getOutputType(outputIndex)), objectId);
}

void LanceControllinoRuntime::publishHomeAssistantStateForOutput(int outputIndex)
{
  if (!isPrimaryEntityOutput(outputIndex)) {
    return;
  }

  char stateTopic[_maxTopicLength];
  char commandTopic[_maxTopicLength];
  buildEntityTopics(outputIndex, stateTopic, sizeof(stateTopic), commandTopic, sizeof(commandTopic));

  switch (_controller.getOutputType(outputIndex)) {
    case LLIGHT:
    case LFAN:
      publishRetained(stateTopic, _controller.getOutputStatus(outputIndex) == ON ? "ON" : "OFF");
      break;
    case LSHADEUP:
      publishRetained(stateTopic, coverStateToPayload(_controller.getCoverState(String(_controller.getOutputZone(outputIndex)), _controller.getOutputZoneIndex(outputIndex))));
      break;
  }
}

void LanceControllinoRuntime::publishAllHomeAssistantStates()
{
  for (int i = 0; i < _controller.getOutputCount(); i++) {
    publishHomeAssistantStateForOutput(i);
  }
}

bool LanceControllinoRuntime::publishHomeAssistantStateFromMessage(const String &message)
{
  String type;
  String zone;
  String status;
  int index = 0;
  (void)status;

  if (!LanceControllino::parseMQTTMessage(message, type, zone, index, status)) {
    return true;
  }

  for (int i = 0; i < _controller.getOutputCount(); i++) {
    if (!isPrimaryEntityOutput(i)) {
      continue;
    }

    if (String(_controller.getOutputZone(i)) != zone || _controller.getOutputZoneIndex(i) != index) {
      continue;
    }

    if ((type == "LIGHT" && _controller.getOutputType(i) == LLIGHT) ||
        (type == "FAN" && _controller.getOutputType(i) == LFAN) ||
        ((type == "SHADEUP" || type == "SHADEDOWN") && _controller.getOutputType(i) == LSHADEUP)) {
      publishHomeAssistantStateForOutput(i);
      return true;
    }
  }

  return true;
}

bool LanceControllinoRuntime::handleHomeAssistantCommandTopic(const char *topic, const char *payload)
{
  for (int i = 0; i < _controller.getOutputCount(); i++) {
    if (!isPrimaryEntityOutput(i)) {
      continue;
    }

    char stateTopic[_maxTopicLength];
    char commandTopic[_maxTopicLength];
    buildEntityTopics(i, stateTopic, sizeof(stateTopic), commandTopic, sizeof(commandTopic));

    if (strcmp(topic, commandTopic) != 0) {
      continue;
    }

    Serial.print("Matched command topic: ");
    Serial.println(topic);

    switch (_controller.getOutputType(i)) {
      case LLIGHT:
        if (strcmp(payload, "ON") == 0 || strcmp(payload, "OFF") == 0) {
          return _controller.processExternalRequest(_controller.getOutputPort(i), strcmp(payload, "ON") == 0 ? ON : OFF);
        }
        return false;
      case LFAN:
        if (strcmp(payload, "ON") == 0 || strcmp(payload, "OFF") == 0) {
          return _controller.processExternalRequest(_controller.getOutputPort(i), strcmp(payload, "ON") == 0 ? ON : OFF);
        }
        return false;
      case LSHADEUP:
        return _controller.processCoverCommand(_controller.getOutputZone(i), _controller.getOutputZoneIndex(i), payload);
    }
  }

  return false;
}

int LanceControllino::readKey(int analogInputPin)
{
  /*
  Read analog button input and return which button is pressed.
  Uses voltage divider thresholds to distinguish 4 buttons on a single analog input.
  */
  int KeyIn = 0;
  KeyIn = analogRead(analogInputPin);
  
  if (KeyIn <= ANALOG_BTN_NO_PRESS_THRESHOLD) {
    return BTNNONE;  
  }
  else if (KeyIn < ANALOG_BTN3_THRESHOLD) {
    return BTN3;  
  }
  else if (KeyIn < ANALOG_BTN2_THRESHOLD) {
    return BTN2;  
  }
  else if (KeyIn < ANALOG_BTN1_THRESHOLD) {
    return BTN1;  
  }
  else {
    return BTN0;  
  }
}

int LanceControllino::findElementRow(int array[], int lastElement, int searchKey) {//DEFINITION REQUIRES ARRAY NAME AND POINTER, AMOUNT OF VALUES IN THE ROW, SEARCHING KEY 
  int index = -1;
  for(int i=0; i<lastElement; i++) {
    if(array[i] == searchKey) {
      index = i;
      return index;
    }
  }
  return index;
}

int LanceControllino::findElementColumn(int array[][4], int lastElement, int searchKey) {//DEFINITION REQUIRES ARRAY NAME AND POINTER, AMOUNT OF VALUES IN THE ROW, SEARCHING KEY 
  int index = -1;
  for(int i=0; i<lastElement; i++) {
    if(array[i][0] == searchKey) {
      index = i;
      return index;
    }
  }
  return index;
}

void LanceControllino::debuger()
{
  Serial.println("Initialization: " + String(_initialized));
  Serial.println("Ports activated: " + String(_portActivated));
  Serial.println("Buffer activated: " + String(_eventActivated));
  Serial.println();
  
  for (int i=0; i<_ANALOG_INPUTS_SIZE;i++) { // Assign I/O to Analog PINs
    Serial.println(String(_analogInputs[0][i]) + ">>>" + String(_analogInputs[1][i]));
  }
  
  for (int k=0; k<_ANALOG_INPUT_ASSIGNMENT_SIZE;k++) {
    Serial.println("APORT: " + String(_analogInputAssignment[k][0]) + " BTN: " +  String(_analogInputAssignment[k][1]) + " DPORT: " + String(_analogInputAssignment[k][2]) + " STATUS: " + String(_analogInputAssignment[k][3]));
  } 
}
