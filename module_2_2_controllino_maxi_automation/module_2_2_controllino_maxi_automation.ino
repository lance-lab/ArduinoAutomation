//MODULE_2_MAXI_AUTOMATION
//updated 25_10_2020
//Dedicated to support shade and funs

/* Notes: Module provides 3 main modules. Light, Shade and Fun modules. 
 * All modules hear from upper level systems using MQTT protocol.
 * All statuses are sent to higher level system via MQTT protocol.
 * MQTT Publish topic Module2CMAOut, MQTT Subscribe topic Module2CMAIn.
 */


/* IMPORT LIBRARIES */

#include <SPI.h>
#include <avr/wdt.h>
#include <Controllino.h>
#include <Ethernet.h>
#include <PubSubClient.h>

/* DEFINE VARIABLES */

//THIS IS JUST ASSIGNMENT >> Btn0 >> "0", etc.
#define Btn0    0 
#define Btn1    1
#define Btn2    2
#define Btn3    3
#define BtnNone 4

//MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
#define MQTT_SOCKET_TIMEOUT 0.05
#endif

//DEFINE PROTOTYPE
void MQTTCallback(char* , byte* , unsigned int);

/* GLOBAL VARIABLE ASSIGNMENT */

//SET ETHERNET VARIABLES
byte Mac[]    = {  0xAE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress Ip(10, 10, 10, 22);
IPAddress Servers(10, 10, 10, 51);
EthernetClient EthClient;

//CREATE MQTT CLIENT (SERVER IP, PORT, SUBPROCESS, ETHCLIENT)
PubSubClient Clients(Servers, 1883, MQTTCallback, EthClient);

//SET TIMERS VARIABLES FOR MAIN PROGRAM
unsigned long CurrentTime = 0;
unsigned long PreviousTime1 = 0;
unsigned long PreviousTime2 = 0;
unsigned long PreviousTime3 = 0;
unsigned long PreviousTime4 = 0;
unsigned long PreviousTime5 = 0;
unsigned long PreviousTime6 = 0;

//ASSIGNMENT OF ANALOG INPUT EXAMPLE PORT AI0 = 54
String AnalogInputMapped [2][12] = {
  {"AI00",  "AI01", "AI02", "AI03", "AI04", "AI05", "AI06", "AI07", "AI08", "AI09", "AI10", "AI11"},
  {"54",    "55",   "56",   "57",   "58",   "59",   "60",   "61",   "62",   "63",   "64",   "65"}
};

//ASSIGNMENT OF DIGITAL INPUT PORT DO0 = 2
String DigitalOutputMapped [4][18] = {
  {"DO00",  "DO01", "DO02", "DO03", "DO04", "DO05", "DO06", "DO07", "RO00", "RO01", "RO02", "RO03", "RO04", "RO05", "RO06", "RO07", "RO08", "RO09"},
  {"2",     "3",    "4",    "5",    "6",    "7",    "8",    "9",    "22",   "23",   "24",   "25",   "26",   "27",   "28",   "29",   "30",   "31"},
  {"PORTE", "PORTE","PORTG","PORTE","PORTH","PORTH","PORTH","PORTH","PORTA","PORTA","PORTA","PORTA","PORTA","PORTA","PORTA","PORTA","PORTC","PORTC"},
  {"4",     "5",    "5",    "3",    "3",    "4",    "5",    "6",    "0",    "1",    "2",    "3",    "4",    "5",    "6",    "7",    "7",    "6"}
};

/* GENERAL ASSIGNMENT LIGHT SECTION */

//ASSIGNMENT OF ANALOG IMPUTS TO LIGHT MODULE
String ModuleLights = "enabled";
int ModuleLightTrigger = 1;
int InputGroupLightStandard [2] ={54,59};

/*ARRAY THAT MAPS ANALOG IMPUTS TO DIGITAL OUTPUTS
 *{{ANALOG INPUT FRAGMENT BTN ASSINGED TO DIGITAL OUTPUT PIN}, {VALUE INCRESES WHEN ANALOG INPUT FRAGMENT BTN PRESENTED AS PUSHED},{N/A} 
 * {{OUTPUT PIN DETAILS EXAMPLE: DO0},{AI BTN 0 - 3 BUTTON IS PRESSED = 1+, BTN 0 - 3 ARE RELEASED = 0},{N/A}
*/

int InputGroupLightStandardMapped [2][3][4] = {
  {{8,99,7,3},{0,0,0,0},{0,0,0,0}},      
  {{8,4,6,99},{0,0,0,0},{0,0,0,0}}  
};

/* GENERAL ASSIGNMENT SHADE SECTION */

//ASSIGNMENT OF ANALOG IMPUTS TO SHADE MODULE
String ModuleShades = "enabled";
int InputGroupShadeStandard [4]=  {55,56,55,56}; 
//ARRAY THAT MAPS ANALOG IMPUTS TO DIGITAL OUTPUTS
//{{ANALOG INPUT FRAGMENT BTN},{DIGITAL OUTPUT UP, DIGITAL OUTPUT DOWN},{VALUE INCRESES WHEN ANALOG INPUT FRAGMENT BTN PRESENTED AS PUSHED FOR UP/DOWN},{TIMER TIMESTAMPS UP/DOWN}}
unsigned long InputGroupShadeStandardMapped [4][4][2] = {
  {{2,3},{22,23},{0,0},{0,0}},
  {{2,3},{24,25},{0,0},{0,0}}, 
  {{0,1},{26,27},{0,0},{0,0}},
  {{0,1},{28,29},{0,0},{0,0}}  
};

//SHADE GENERAL TIMER UP/DOWN (IN THAT TIME SHADE MUST REACH MAXIMUM UP OR MAXIMUM DOWN)
unsigned long ShadeTime = 50000;

//WINDPROTECTION NotProtected:Protected:ProtectionReady
String WindProtection = "NotProtected";

//WINDCOUNTER TO MEASURE WIND INTENSITY
int WindProtectionCounter = -1;

//WINDCOUNTER ANALOG INPUT THAT SHOULD CONSUME WIND KIT DATA
int WindProtectionPin = 66;

/* GENERAL ASSIGNMENT FAN SECTION */

//ASSIGNMENT OF ANALOG IMPUTS TO FAN MODULE
String ModuleFan = "enabled";
int InputGroupFanStandard [2]=  {60,60};

/*ARRAY THAT MAPS ANALOG IMPUTS TO DIGITAL OUTPUTS
 *{{ANALOG INPUT FRAGMENT BTN},{DIGITAL OUTPUT},{VALUE INCRESES WHEN ANALOG INPUT FRAGMENT BTN PRESENTED AS PUSHED},{TIMER TIMESTAMP}}
*/

unsigned long InputGroupFanStandardMapped [2][4][1] = {
  {{1},{30},{0},{0}},
  {{0},{31},{0},{0}}
};

//FAN GENERAL TIMER
unsigned long FanTime = 300000;

/* INITIAL SETUP */

void setup() {
  
  //INITIATE SERIAL PORT
  Serial.begin(9600);
  Serial.setTimeout(50);

  Ethernet.begin(Mac, Ip);

  //ACTIVATE ALL ANALOG INPUTS FROM AnalogInputMapped ARRAY
  for (int i=0; i<sizeof(AnalogInputMapped[0])/sizeof(*AnalogInputMapped[0]);i++) { // Assign I/O to Analog PINs
    pinMode(AnalogInputMapped[1][i].toInt(), INPUT); //Set all PINs from AnalogInputMapped array as INPUT
  }

  //ACTIVATE ALL DIGITAL OUTPUTS FROM ANALOG INPUT MAPPED ARRAY
  for (int i=0; i<sizeof(DigitalOutputMapped[0])/sizeof(*DigitalOutputMapped[0]);i++) { // Assign I/O to Relay PINs (10 RELAYS AVAILABLE)
    pinMode(DigitalOutputMapped[1][i].toInt(), OUTPUT); // Set all PINs from DigitalOutputMapped array as OUTPUT
  }

  //ACTIVATE ANALOG INPUT FOR WIND KIT
  pinMode(WindProtectionPin, INPUT);

  //INITIATE CONNECTION TO MQTT SERVER IF NOT ESTABLISHED ALREADY. Clients.connected CAN BE "0" - NOT CONNECTED OR "1" - CONNECTED
  if (!Clients.connected()) {
    if (Clients.connect("ArduinoMega1","ArduinoUser","ArduinoUser12345")) {
      Clients.publish("Module1CMAOut","hello");
      Clients.subscribe("Module1CMAIn");
    }
  }
  // START WATCHDOG
  wdt_enable(WDTO_4S);

}


// THIS DEFINITION CHECKS IF BUTTON IS PRESSED OR NOT, IT RETURNS VALUE 0-3 IF BUTTON PRESSED AND 4 IF NOT.
int ReadAnalogButtons(int Pin) { // DEFINITION REQUIRES ANALOG PIN NUMBER AND RETURNS BTN NUMBER.
  int KeyIn = 0;
  KeyIn = analogRead(Pin);
  if (KeyIn <= 200) return BtnNone;  
  else if ((KeyIn > 200) && (KeyIn < 350)) return Btn3;  
  else if ((KeyIn > 350) && (KeyIn < 550)) return Btn2;  
  else if ((KeyIn > 550) && (KeyIn < 750)) return Btn1;  
  else if (KeyIn >= 750) return Btn0;  
  return BtnNone;
}

//IT SEARCHES FOR POSSITION IN ARRAY BASED ON PROVIDED KEY.
int FindElement(String Array[], int LastElement, String SearchKey) {//DEFINITION REQUIRES ARRAY NAME AND POINTER, AMOUNT OF VALUES IN THE ROW, SEARCHING KEY 
  int Index = -1;
  for(int i=0; i<LastElement; i++) {
    if(Array[i] == SearchKey) { Index = i;}
  }
  return Index;
}

/* LIGHT STANDARD DEFINITION */

void LightStandard(int MapPosition, int PinElement) { //DEFINITOIN REQUIRES InputGroupLightStandard VERTICAL POSSITIO AND PIN ELEMENT
  int ReadKey = PinElement;
  //IF READKEY IS -1 THEN SYSTEM IS CHECKING BUTTONS. IF READKEY IS 0 - 3 THEN REQUEST CAME FROM MQTT TOPIC.
  if ((ReadKey < 0) or (ReadKey > 3)) { ReadKey = ReadAnalogButtons(InputGroupLightStandard[MapPosition]); }
  switch (ReadKey) {
    case BtnNone: {
      for (int i=0; i<4; i++) {
        if ((InputGroupLightStandardMapped [MapPosition][0][i] != 99) && ((0 < InputGroupLightStandardMapped[MapPosition][1][i]) && (InputGroupLightStandardMapped[MapPosition][1][i] < ModuleLightTrigger))) {InputGroupLightStandardMapped[MapPosition][1][i] = 0;}
        else if ((InputGroupLightStandardMapped [MapPosition][0][i] != 99) && (InputGroupLightStandardMapped[MapPosition][1][i] > 0)) {
          int Index = FindElement(DigitalOutputMapped[1],sizeof(DigitalOutputMapped[1])/sizeof(*DigitalOutputMapped[1]), String(InputGroupLightStandardMapped [MapPosition][0][i]));
          int PinStatus = -1;
          //FIND CURRENT STATUS OF DIGITAL OUTPUT PINS
          if (DigitalOutputMapped[2][Index] == "PORTA") { PinStatus = bitRead(PORTA,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTB") { PinStatus = bitRead(PORTB,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTC") { PinStatus = bitRead(PORTC,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTD") { PinStatus = bitRead(PORTD,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTE") { PinStatus = bitRead(PORTE,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTF") { PinStatus = bitRead(PORTF,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTG") { PinStatus = bitRead(PORTG,DigitalOutputMapped[3][Index].toInt());}
          else if (DigitalOutputMapped[2][Index] == "PORTH") { PinStatus = bitRead(PORTH,DigitalOutputMapped[3][Index].toInt());}

          //BASED ON CURRENT STATUS OF DIGITAL OUTPUT PIN GO TO MATCH CONDITION. 
          if (PinStatus == 1) { 
            digitalWrite(InputGroupLightStandardMapped[MapPosition][0][i], LOW);
            InputGroupLightStandardMapped[MapPosition][2][i] = 0;
            String Port[4] = "";
            String Status[4] = ""; 
            Port[0] = (InputGroupLightStandardMapped[MapPosition][0][i]);
            Status[0] = "OFF";
            //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT, STATUS) EXAMPLE LHT_22_0
            MQTTEventHandlerOut("LHT", Port, Status);
          }  
          else if (PinStatus == 0) {
            digitalWrite(InputGroupLightStandardMapped[MapPosition][0][i], HIGH);
            InputGroupLightStandardMapped[MapPosition][2][i] = 1;
            String Port[4] = "";
            String Status[4] = ""; 
            Port[0] = (InputGroupLightStandardMapped[MapPosition][0][i]);
            Status[0] = "ON";
            //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT, STATUS) EXAMPLE LHT_22_1
            MQTTEventHandlerOut("LHT", Port, Status);
          }
          InputGroupLightStandardMapped[MapPosition][1][i] = 0;
        } 
      }
    break;
    }
    //IF BUTTON PRESSED INCREASE PARTICULAR COUNTER
    case Btn0: { InputGroupLightStandardMapped[MapPosition][1][0]++; break;}
    case Btn1: { InputGroupLightStandardMapped[MapPosition][1][1]++; break;}
    case Btn2: { InputGroupLightStandardMapped[MapPosition][1][2]++; break;}
    case Btn3: { InputGroupLightStandardMapped[MapPosition][1][3]++; break;}

  }
}

/* SHADE STANDARD DEFINITION */

void ShadeStandard(int MapPosition, int PinElement) {
  int ReadKey = PinElement;
  //IF READKEY IS -1 THEN SYSTEM IS CHECKING BUTTONS. IF READKEY IS 0 - 3 THEN REQUEST CAME FROM MQTT TOPIC.
  if ((ReadKey < 0) or (ReadKey > 4)) { ReadKey = ReadAnalogButtons(InputGroupShadeStandard[MapPosition]); }
  switch (ReadKey) {
    case BtnNone: {
      //IF SHADE IS GOING UP AND BUTTON DOWN IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE
      if ((InputGroupShadeStandardMapped[MapPosition][3][0] > 0 ) && (InputGroupShadeStandardMapped[MapPosition][2][1] > 0)) { 
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], LOW);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], LOW);
        InputGroupShadeStandardMapped[MapPosition][3][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][3][1] = 0; 
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Status[0] = "OFF";
        Status[1] = "OFF";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_0&23_0
        MQTTEventHandlerOut("SHD", Port, Status);
      }
      //IF SHADE IS GOING DOWN AND BUTTON UP IS PRESSED IT SET ALL TO ZERO THAT STOP SHADE
      else if ((InputGroupShadeStandardMapped[MapPosition][3][1] > 0 ) && (InputGroupShadeStandardMapped[MapPosition][2][0] > 0)) { 
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], LOW);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], LOW);
        InputGroupShadeStandardMapped[MapPosition][3][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][3][1] = 0; 
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Status[0] = "OFF";
        Status[1] = "OFF";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_0&23_0
        MQTTEventHandlerOut("SHD", Port, Status);
      }
      //IF UP AND DOWN WAS PRESSED AT THE SAME TIME IT WILL CLEAR STATUS AND WILL KEEP SHADE TO STAY
      else if ((InputGroupShadeStandardMapped[MapPosition][2][1] > 0 ) && (InputGroupShadeStandardMapped[MapPosition][2][0] > 0)) { 
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], LOW);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], LOW);
        InputGroupShadeStandardMapped[MapPosition][3][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][3][1] = 0; 
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;  
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Status[0] = "OFF";
        Status[1] = "OFF";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_0&23_0
        MQTTEventHandlerOut("SHD", Port, Status);

      }
      //IF UP WAS PRESSED THEN SET THE SHADE TO MOVE UP
      else if ((InputGroupShadeStandardMapped[MapPosition][2][0] > 0 ) && (InputGroupShadeStandardMapped[MapPosition][3][0] == 0)) { 
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], HIGH);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], LOW);
        InputGroupShadeStandardMapped[MapPosition][3][0] = CurrentTime; //SET TIMER INITIAL POSSITION
        InputGroupShadeStandardMapped[MapPosition][3][1] = 0; 
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;       
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Status[0] = "ON";
        Status[1] = "OFF";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_1&23_0
        MQTTEventHandlerOut("SHD", Port, Status);
        
      }
      //IF DOWN WAS PRESSED THEN SET THE SHADE TO MOVE DOWN
      else if ((InputGroupShadeStandardMapped[MapPosition][2][1] > 0 ) && (InputGroupShadeStandardMapped[MapPosition][3][1] == 0)) { 
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], LOW);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], HIGH);
        InputGroupShadeStandardMapped[MapPosition][3][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][3][1] = CurrentTime; //SET TIMER INITIAL POSSITION
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;    
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Status[0] = "OFF";
        Status[1] = "ON";  
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_0&23_1
        MQTTEventHandlerOut("SHD", Port, Status);
      }
      break;
    }
    //IF BUTTON PRESSED INCREASE PARTICULAR COUNTER
    case Btn0: { 
      if (InputGroupShadeStandardMapped[MapPosition][0][0] == 0) {InputGroupShadeStandardMapped[MapPosition][2][0]++;}
      break;
    }
    case Btn1: {
      if (InputGroupShadeStandardMapped[MapPosition][0][1] == 1) {InputGroupShadeStandardMapped[MapPosition][2][1]++;}
      break;
    }
    case Btn2: { 
      if (InputGroupShadeStandardMapped[MapPosition][0][0] == 2) {InputGroupShadeStandardMapped[MapPosition][2][0]++;}
      break;
    }
    case Btn3: {
      if (InputGroupShadeStandardMapped[MapPosition][0][1] == 3) {InputGroupShadeStandardMapped[MapPosition][2][1]++;}
      break;
    }
  }
  //CHECK TIMER FOR UP/DOWN PINS AND CLEAR THEM IF EXPIRED. PERFORM STOP IN BOTH DIRECTIONS.
  for (int i=0; i<2; i++) {
    if (((InputGroupShadeStandardMapped[MapPosition][3][i] + ShadeTime) < CurrentTime) && (InputGroupShadeStandardMapped[MapPosition][3][i] != 0)) {
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][0], LOW);
        digitalWrite(InputGroupShadeStandardMapped[MapPosition][1][1], LOW);
        InputGroupShadeStandardMapped[MapPosition][3][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][3][1] = 0; 
        InputGroupShadeStandardMapped[MapPosition][2][0] = 0;
        InputGroupShadeStandardMapped[MapPosition][2][1] = 0;    
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupShadeStandardMapped[MapPosition][1][0]);
        Port[1] = (InputGroupShadeStandardMapped[MapPosition][1][1]);
        Status[0] = "OFF";
        Status[1] = "OFF";  
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT&STATUS, PORT&STATUS) EXAMPLE SHD_22_0&23_0
        MQTTEventHandlerOut("SHD", Port, Status);

    }
  }
  
}
      
/* FAN STANDARD DEFINITION */

void FanStandard(int MapPosition, int PinElement) {
  int ReadKey = PinElement;
  //IF READKEY IS -1 THEN SYSTEM IS CHECKING BUTTONS. IF READKEY IS 0 - 3 THEN REQUEST CAME FROM MQTT TOPIC.
  if ((ReadKey < 0) or (ReadKey > 3)) { ReadKey = ReadAnalogButtons(InputGroupFanStandard[MapPosition]); }
  switch (ReadKey) {
    case BtnNone: {
      //IF FAN IS IN MOVEMENT AND BUTTON WAS PRESSED, THE FAN WILL BE DISABLED
      if ((InputGroupFanStandardMapped[MapPosition][2][0] > 0 ) && (InputGroupFanStandardMapped[MapPosition][3][0] > 0)) { 
        digitalWrite(InputGroupFanStandardMapped[MapPosition][1][0], LOW);
        InputGroupFanStandardMapped[MapPosition][2][0] = 0;
        InputGroupFanStandardMapped[MapPosition][3][0] = 0;
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupFanStandardMapped[MapPosition][1][0]);
        Status[0] = "OFF";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT, STATUS) EXAMPLE FAN_30_0
        MQTTEventHandlerOut("FAN", Port, Status);

        
      }
      //IF BUTTON WAS PRESSED AND FAN IS NOT IN MOVEMENT IT KICKS OFF THE FAN
      else if ((InputGroupFanStandardMapped[MapPosition][2][0] > 0 ) && (InputGroupFanStandardMapped[MapPosition][3][0] == 0)) { 
        digitalWrite(InputGroupFanStandardMapped[MapPosition][1][0], HIGH);
        InputGroupFanStandardMapped[MapPosition][2][0] = 0;
        InputGroupFanStandardMapped[MapPosition][3][0] = CurrentTime;
        String Port[4] = "";
        String Status[4] = ""; 
        Port[0] = (InputGroupFanStandardMapped[MapPosition][1][0]);
        Status[0] = "ON";
        //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT, STATUS) EXAMPLE FAN_30_1
        MQTTEventHandlerOut("FAN", Port, Status);
      }
      break;
    }
    //IF BUTTON PRESSED INCREASE PARTICULAR COUNTER
    case Btn0: { 
      if (InputGroupFanStandardMapped[MapPosition][0][0] == 0) {InputGroupFanStandardMapped[MapPosition][2][0]++;}
      break;
    }
    case Btn1: {
      if (InputGroupFanStandardMapped[MapPosition][0][0] == 1) {InputGroupFanStandardMapped[MapPosition][2][0]++;}
      break;
    }
    case Btn2: { 
      if (InputGroupFanStandardMapped[MapPosition][0][0] == 2) {InputGroupFanStandardMapped[MapPosition][2][0]++;}
      break;
    }
    case Btn3: {
      if (InputGroupFanStandardMapped[MapPosition][0][0] == 3) {InputGroupFanStandardMapped[MapPosition][2][0]++;}
      break;
    }
  }
  //CHECK TIMER FOR FAN PIN AND CLEAR IT IF EXPIRED. PERFORM FAN STOP.
  if (((InputGroupFanStandardMapped[MapPosition][3][0] + FanTime) < CurrentTime) && (InputGroupFanStandardMapped[MapPosition][3][0] != 0)) {
    digitalWrite(InputGroupFanStandardMapped[MapPosition][1][0], LOW);
    InputGroupFanStandardMapped[MapPosition][3][0] = 0;  
    InputGroupFanStandardMapped[MapPosition][2][0] = 0;
    String Port[4] = "";
    String Status[4] = ""; 
    Port[0] = (InputGroupFanStandardMapped[MapPosition][1][0]);
    Status[0] = "ON";
    //SEND STATUS VIA MQTT USING MQTTEventHandlerOut(MODULE, PORT, STATUS) EXAMPLE FAN_30_0
    MQTTEventHandlerOut("FAN", Port, Status);
  }
}


/* MQTT Callback listens for incoming MQTT messages. 
 * As an input it takes, topic name, payload and length. The message is received as char object. 
 * Simple message:MODULE_PORT_STATUS.
 * Enhanced message: MODULE_PORT_STATUS&PORT_STATUS&PORT_STATUS (maximum 1 moduel, 4 ports and statuses)
 * Light standard input message: LHT_AI00_0 (Module_AnalogInput_AnalogInputPossition)
 * Shade standard input message: SHD_AI03_0 (Module_AnalogInput_AnalogInputPossition)
 * Fan standard input message: FAN_AI04_0 (Module_AnalogInput_AnalogInputPossition)
 * Get status: STS_LHT&SHD&FAN or simple STS_LHT ...
 */

void MQTTCallback(char* topic, byte* payload, unsigned int length) {
  String Module = "";
  String Port[4] = "";
  String Status[4] = "";
  int UnderscoreCounter = 0;
  int AndCounter1 = 0;
  int AndCounter2 = 0;


  // SPLIT RECEIVED MESSAGE VIA MQTT TO MUDULE, PORT, STATUS
  for (int i=0;i<length;i++) {
    if ((char)payload[i] == '_') { UnderscoreCounter = UnderscoreCounter + 1; }
    if ((UnderscoreCounter == 0) && ((char)payload[i] != '_')) { Module = Module + (char)payload[i]; }
    if ((UnderscoreCounter == 1) && ((char)payload[i] != '_')){
      if ((char)payload[i] == '&') { AndCounter1 = AndCounter1 + 1; }
      else { Port[AndCounter1] = Port[AndCounter1] + (char)payload[i]; }
    }
    if ((UnderscoreCounter == 2) && ((char)payload[i] != '_')) { 
      if ((char)payload[i] == '&') { AndCounter2 = AndCounter2 + 1; }
      else { Status[AndCounter2] = Status[AndCounter2] + (char)payload[i]; }
    }

  }
  // CALL MQTTEventHandlerIn PROCESS SENDING MODULE, PORT, STATUS
  Serial.println(String(Module));
  Serial.println(String(Port[0]));
  Serial.println(String(Status[0]));
  
  MQTTEventHandlerIn(Module, Port, Status);

}


/* MQTT MQTTEventHandlerOut sends outgoing messages to MQTT server using Module2CMAOut topic
 * All statuses are prepared in this stage and are sent out. 
 * STS provide status of all interfaces based on received request. LHD, SHD and FAN pins status is provided.
 */

void MQTTEventHandlerOut(String Module, String Port[], String Status[] ) {
  if (Clients.connected()) {
    String Message = "";
    if (Module == "STS") {
      for (int i=0; i<4; i++) {
        if (Port[i] == "LHT") { 
          //RETRIEVE STATUSES FROM ALL LIGHTS PINS
          for (int j=0; j<sizeof(InputGroupLightStandard)/sizeof(*InputGroupLightStandard); j++) {
            for (int k=0; k<4; k++) {
              if (InputGroupLightStandardMapped [j][0][k] != 99) { 
                Message = Message + ("LHT_" + String(InputGroupLightStandardMapped [j][0][k]) + "_" + String(InputGroupLightStandardMapped [j][2][k]) + "|");
              }        
            }
          }
        }

        if (Port[i] == "SHD") { 
          //RETRIEVE STATUSES FROM ALL SHADES PINS
          for (int j=0; j<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard); j++) {
            if ((InputGroupShadeStandardMapped [j][1][0] != 99) && (InputGroupShadeStandardMapped [j][1][0] != 99)) { 
              Message = Message + ("SHD_" + String(InputGroupShadeStandardMapped [j][1][0]) + "_" + String(InputGroupShadeStandardMapped [j][3][0]) + "&" + String(InputGroupShadeStandardMapped [j][1][1]) + "_" + String(InputGroupShadeStandardMapped [j][3][1]) + "|");
            }        
          }
        }
     
      
        if (Port[i] == "FAN") { 
          //RETRIEVE STATUSES FROM ALL FAN PINS
          for (int j=0; j<sizeof(InputGroupFanStandard)/sizeof(*InputGroupFanStandard); j++) {
            if ((InputGroupFanStandardMapped [j][1][0] != 99)) { 
              Message = Message + ("FAN_" + String(InputGroupFanStandardMapped [j][1][0]) + "_" + String(InputGroupFanStandardMapped [j][2][0]) + "|");
            }        
          }
        }     
      }
    }
    //PREPARE STRING THAT NEEDS TO BE SENT
    if ((Module == "LHT") or (Module == "SHD") or (Module == "FAN")) {
      Message = Module + "_";
      for (int i=0; i<4; i++) {
        if (Port[i] != "") { Message = Message + Port[i] + "_" + Status[i] + "&"; }
      }
    }

    //IF MESSAGE IS NOT EMPTY IT IS TRANSLATED TO CHAR ARRAY AND PUBLISHED ON TOPIC.
    if (Message != "") {
        char Buffer [Message.length()];
        Message.toCharArray(Buffer,Message.length());
        Clients.publish("Module1CMAOut", Buffer, Message.length());
    }
  }
}

/* MQTT MQTTEventHandlerIn is processing MQTT messages (requests). 
 *  Light procedure is called if MQTT request contains LHT
 *  Shade procedure is called if MQTT request contains SHD
 *  Fan procedure is called if MQTT request contains FAN
 *  Get status request is just passing through STS
 */

void MQTTEventHandlerIn(String Module, String Port[], String Parameter[]) {
  //PROCESSING LIGHTS REQUESTS
  if (Module == "LHT") {
    int Index = FindElement(AnalogInputMapped[0],sizeof(AnalogInputMapped[0])/sizeof(*AnalogInputMapped[0]), Port[0]);
    if (Index >= 0) {
      for(int i=0; i<sizeof(InputGroupLightStandard)/sizeof(*InputGroupLightStandard); i++) {
        if (AnalogInputMapped[1][Index] == String(InputGroupLightStandard[i])) { 
          LightStandard(i,Parameter[0].toInt()); 
            Serial.println(String(i));
            Serial.println(String(Parameter[0]));
          break;
         }
      }   
    }
  }
  //PROCESSING SHADES REQUESTS
  if (Module == "SHD") {
    int Index = FindElement(AnalogInputMapped[0],sizeof(AnalogInputMapped[0])/sizeof(*AnalogInputMapped[0]), Port[0]);
    if (Index >= 0) {
      for(int i=0; i<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard); i++) {
        if (AnalogInputMapped[1][Index] == String(InputGroupShadeStandard[i])) { ShadeStandard(i,Parameter[0].toInt()); break;}
      }
    }
  }
  //PROCESSING FAN REQUESTS
  if (Module == "FAN") {
    int Index = FindElement(AnalogInputMapped[0],sizeof(AnalogInputMapped[0])/sizeof(*AnalogInputMapped[0]), Port[0]);
    if (Index >= 0) {
      for(int i=0; i<sizeof(InputGroupFanStandard)/sizeof(*InputGroupFanStandard); i++) {
        if (AnalogInputMapped[1][Index] == String(InputGroupFanStandard[i])) { FanStandard(i,Parameter[0].toInt()); break;}
      }
    }
  }
  //PROCESSING GET STATUS REQUESTS
  if (Module == "STS") {
    MQTTEventHandlerOut(Module, Port, Parameter);
  }
}

/* Execute all shades UP or DOWN request. Procedure takes Direction that can be Up, Down or None */

void ShadesAllUpDown(String Direction) {
  //ALL SHADES UP
  if (Direction == "Up") {
    for (int i=0; i<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard);i++) {
      ShadeStandard(i, InputGroupShadeStandardMapped[i][0][0]);
      ShadeStandard(i, 4);
    }
  }
  //ALL SHADES DOWN
  else if (Direction == "Down") {
    for (int i=0; i<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard);i++) {
      ShadeStandard(i, InputGroupShadeStandardMapped[i][0][1]);
      ShadeStandard(i, 4);
    }
  }
  //THIS ONE PRETENDS THAT NOTHING IS PRESSED. IT STOPS TIMERS AFTER UP/DOWN DIRECTION 
  else if (Direction == "None") {
    for (int i=0; i<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard);i++) {
      ShadeStandard(i, 4);
    }
  } 
}

/* MAIN PROGRAM TRUNK */

void loop() {

  //INITIATE MAJOR TIMER
  CurrentTime = millis();

  if (ModuleLights == "enabled") {
  //PERFORM EVERY 50MS USING MAJOR TIMER, LIGHT STANDARD
    if ((CurrentTime - PreviousTime1) >= 50) {    
      PreviousTime1 = CurrentTime;
      for (int i=0; i<sizeof(InputGroupLightStandard)/sizeof(*InputGroupLightStandard);i++) {
        LightStandard(i, -1);
      }
    }
  }

  if (ModuleShades == "enabled") {
    //PERFORM EVERY 50MS USING MAJOR TIMER, SHADE STANDARD AND PROTECTION CONTROL
    if ((CurrentTime - PreviousTime2) >= 50) { 
      PreviousTime2 = CurrentTime;
      if (WindProtection == "NotProtected") {
        for (int i=0; i<sizeof(InputGroupShadeStandard)/sizeof(*InputGroupShadeStandard);i++) {
          ShadeStandard(i, -1);
        }
      }
      else if (WindProtection == "ProtectionReady") {
        ShadesAllUpDown("Up");
        WindProtection = "Protected";
      }   
      else if (WindProtection == "Protected") {
        ShadesAllUpDown("None");
      }   
    }
    //PERFORM EVERY 20S USING MAJOR TIMER, WIND PROTECTION COUNT
    if ((CurrentTime - PreviousTime6) >= 20000) { 
      PreviousTime6 = CurrentTime;
      if (analogRead(WindProtectionPin) > 200) { if (WindProtectionCounter < 10) { WindProtectionCounter = WindProtectionCounter + 1;}}
      else if (WindProtectionCounter > -1) { WindProtectionCounter = WindProtectionCounter - 1; }

      //SET WINDPROTECTION STATUS
      if (WindProtectionCounter ==10) {WindProtection = "ProtectionReady";}
      else if (WindProtectionCounter == -1) {WindProtection = "NotProtected";}
    }
  }

  if (ModuleFan == "enabled") {
    //PERFORM EVERY 50MS USING MAJOR TIMER, FAN STANDARD
    if ((CurrentTime - PreviousTime3) >= 50) { 
      PreviousTime3 = CurrentTime;
      for (int i=0; i<sizeof(InputGroupFanStandard)/sizeof(*InputGroupFanStandard);i++) {
        FanStandard(i, -1);
      }
    }
  }


  //CONNECTIVITY TO MQTT SERVER IS CHECKED AT CERTAIN TIMES. IT IS ATTEMPTED THE CONNECTION TO BE REESTABLISHED IF IT'S DOWN.
  if ((CurrentTime - PreviousTime4) >= 1800000) { 
    PreviousTime4 = CurrentTime;
    Serial.println(String(Clients.connected()));
    if (!Clients.connected()) {
      if (Clients.connect("ArduinoMega1","ArduinoUser","ArduinoUser12345")) {
        Clients.publish("Module1CMAOut","hello");
        Clients.subscribe("Module1CMAIn");
      }
    }
  }
  //PERFORM EVERY 200MS USING MAJOR TIMER, CHECK FOR INCOMING MQTT MESSAGES
  if ((CurrentTime - PreviousTime5) >= 200) { 
    PreviousTime5 = CurrentTime;
    Clients.loop();
  } 

  //Watchdog
  wdt_reset();

}
