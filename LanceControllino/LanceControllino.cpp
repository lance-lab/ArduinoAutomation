/*
  lancecontrollino.cpp - Library for enabling Controllino Maxi to smart home
  - Handle Lights, Fans and Shades
  - Posting messages to buffer. Messages are ready to be pull and publish on MQTT.
*/

#include <Controllino.h>
#include "RingBuf.h"
#include "LanceControllino.h"

LanceControllino::LanceControllino(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize) 
{
  _initialized = initialization(analogAssignment, digitalAssignment, analogAssignmentSize, digitalAssignmentSize);
  _portActivated = portActivation();
  _eventActivated = false;
}  

LanceControllino::LanceControllino(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize, bool events) 
{
  _initialized = initialization(analogAssignment, digitalAssignment, analogAssignmentSize, digitalAssignmentSize);
  _portActivated = portActivation();
  _eventActivated = events;
}  

bool LanceControllino::initialization(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize) 
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
      if (digitalAssignment[j][0] == analogAssignment[i][2]) {
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
      _digitalOutputAssignment[indexDigitalOutputAssignment][1] = digitalAssignment[indexDigitalAssignment][1];
      _digitalOutputAssignment[indexDigitalOutputAssignment][2] = digitalAssignment[indexDigitalAssignment][2];
      _analogInputs[1][indexAnalogInputs] = ENABLED;
    }
    else {
      return false;
    }
  }
  return true;
}

bool LanceControllino::addToBuffer(int topic, String message)
{
  if (_eventActivated == true) {
    memset(&bufferAdd, 0, sizeof(struct _eventMessage));

    bufferAdd._topic = topic;
    message.toCharArray(bufferAdd._topicMessage, message.length()+1);
    
    if(_messageBuffer->add(_messageBuffer, &bufferAdd) != -1){
      return true;
    }
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
            _analogInputAssignment[k][3] == _analogInputAssignment[k][3]++;
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
        message = "TYPE=LIGHT:PORT=" + String(port) + ":STATUS=ON";
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, LOW);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        message = "TYPE=LIGHT:PORT=" + String(port) + ":STATUS=OFF";
        addToBuffer(1, message);
      }
    }
    else if (pinStatus == OFF) {
      if (status == OFF) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        message = "TYPE=LIGHT:PORT=" + String(port) + ":STATUS=OFF";
        addToBuffer(1, message);
        Serial.println("no_action");
      }
      else {
        //digitalWrite(port, HIGH);
        digitalWrite(port, HIGH);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        message = "TYPE=LIGHT:PORT=" + String(port) + ":STATUS=ON";
        addToBuffer(1, message);
      }
    }
  }
}

bool LanceControllino::shadeOperation(int port, int status) {
  //IF SHADE IS GOING UP AND BUTTON DOWN IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE

  int shadeActionPortIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);
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
      messageUp = "TYPE=SHADEUP:PORT=" + String(_digitalOutputAssignment[shadeUpPortIndex][0]) + ":STATUS=OFF";
      messageDown = "TYPE=SHADEDOWN:PORT=" + String(_digitalOutputAssignment[shadeDownPortIndex][0]) + ":STATUS=OFF";
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
      messageUp = "TYPE=SHADEUP:PORT=" + String(_digitalOutputAssignment[shadeUpPortIndex][0]) + ":STATUS=OFF";
      messageDown = "TYPE=SHADEDOWN:PORT=" + String(_digitalOutputAssignment[shadeDownPortIndex][0]) + ":STATUS=OFF";
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
      messageUp = "TYPE=SHADEUP:PORT=" + String(_digitalOutputAssignment[shadeUpPortIndex][0]) + ":STATUS=ON";
      messageDown = "TYPE=SHADEDOWN:PORT=" + String(_digitalOutputAssignment[shadeDownPortIndex][0]) + ":STATUS=OFF";
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
      messageUp = "TYPE=SHADEUP:PORT=" + String(_digitalOutputAssignment[shadeUpPortIndex][0]) + ":STATUS=OFF";
      messageDown = "TYPE=SHADEDOWN:PORT=" + String(_digitalOutputAssignment[shadeDownPortIndex][0]) + ":STATUS=ON";
      addToBuffer(1, messageUp);
      addToBuffer(1, messageDown);
    }
  }
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
        message = "TYPE=FAN:PORT=" + String(port) + ":STATUS=ON";
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, LOW);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = OFF;
        message = "TYPE=FAN:PORT=" + String(port) + ":STATUS=OFF";
        addToBuffer(1, message);
      }
    }
    else if (pinStatus == OFF) {
      if (status == OFF) {
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = OFF;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = OFF;
        message = "TYPE=FAN:PORT=" + String(port) + ":STATUS=OFF";
        addToBuffer(1, message);
      }
      else {
        digitalWrite(port, HIGH);
        _digitalOutputAssignment[digitalOutputAssignmentIndex][3] = ON;
        _digitalOutputAssignmentClock[digitalOutputAssignmentIndex] = millis();
        message = "TYPE=FAN:PORT=" + String(port) + ":STATUS=ON";
        addToBuffer(1, message);
      }
    }
  }
}

void LanceControllino::statusTimeVerification() {

  for(int i=0; i<_DIGITAL_OUTPUT_ASSIGNMENT_SIZE; i++) {
    if (_digitalOutputAssignment[i][1] == LSHADEUP || _digitalOutputAssignment[i][1] == LSHADEDOWN) {
      if (millis() > (_digitalOutputAssignmentClock[i] + RUN_SHADE_COUNT)) {
        digitalWrite(_digitalOutputAssignment[i][0], LOW);
        _digitalOutputAssignment[i][3] = OFF;
        _digitalOutputAssignmentClock[i] = OFF;
      }
    }
    else if (_digitalOutputAssignment[i][1] == LFAN) {
      if (millis() > (_digitalOutputAssignmentClock[i] + RUN_FUN_COUNT)) {
        digitalWrite(_digitalOutputAssignment[i][0], LOW);
        _digitalOutputAssignment[i][3] = OFF;
        _digitalOutputAssignmentClock[i] = OFF;
      }
    }
  }
}

bool LanceControllino::processExternalRequest(int port, int status) {

  int digitalOutputAssignmentIndex = findElementColumn(_digitalOutputAssignment, _DIGITAL_OUTPUT_ASSIGNMENT_SIZE, port);

  switch (_digitalOutputAssignment[digitalOutputAssignmentIndex][1]) {
    case LLIGHT: {lightOperation(port, status); break;}
    case LSHADEUP: {shadeOperation(port, status); break;}
    case LSHADEDOWN: {shadeOperation(port, status); break;}
    case LFAN: {fanOperation(port, status); break;}
  }
}

int LanceControllino::readKey(int analogInputPin)
{
  int KeyIn = 0;
  KeyIn = analogRead(analogInputPin);
  if (KeyIn <= 200) return BTNNONE;  
  else if ((KeyIn > 200) && (KeyIn < 350)) return BTN3;  
  else if ((KeyIn > 350) && (KeyIn < 550)) return BTN2;  
  else if ((KeyIn > 550) && (KeyIn < 750)) return BTN1;  
  else if (KeyIn >= 750) return BTN0;  
  return BTNNONE;
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