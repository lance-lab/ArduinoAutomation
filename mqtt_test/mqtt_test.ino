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
byte Mac[]    = {  0xAE, 0xED, 0xBA, 0xFE, 0xFE, 0xFD };
IPAddress Ip(10, 10, 10, 199);
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



/* INITIAL SETUP */

void setup() {
  
  //INITIATE SERIAL PORT
  Serial.begin(9600);
  Serial.setTimeout(50);

  Ethernet.begin(Mac, Ip);



  //INITIATE CONNECTION TO MQTT SERVER IF NOT ESTABLISHED ALREADY. Clients.connected CAN BE "0" - NOT CONNECTED OR "1" - CONNECTED
  if (!Clients.connected()) {
    if (Clients.connect("ArduinoClient1","ArduinoUser","ArduinoUser12345")) {
      Clients.publish("Module1CMAOut","hello");
      Clients.subscribe("Module1CMAIn");
    }
  }
  // START WATCHDOG
  wdt_enable(WDTO_4S);

}


void MQTTCallback(char* topic, byte* payload, unsigned int length) {
  String Module = "";
  String Port[4] = "";
  String Status[4] = "";
  int UnderscoreCounter = 0;
  int AndCounter1 = 0;
  int AndCounter2 = 0;

}




/* MQTT MQTTEventHandlerOut sends outgoing messages to MQTT server using Module2CMAOut topic
 * All statuses are prepared in this stage and are sent out. 
 * STS provide status of all interfaces based on received request. LHD, SHD and FAN pins status is provided.
 */


void loop() {

  //INITIATE MAJOR TIMER
  CurrentTime = millis();



  //CONNECTIVITY TO MQTT SERVER IS CHECKED AT CERTAIN TIMES. IT IS ATTEMPTED THE CONNECTION TO BE REESTABLISHED IF IT'S DOWN.
  if ((CurrentTime - PreviousTime4) >= 60000) { 
    PreviousTime4 = CurrentTime;
    Serial.println(String(Clients.connected()));
    if (!Clients.connected()) {
      if (Clients.connect("ArduinoClient","ArduinoUser","ArduinoUser12345")) {
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
