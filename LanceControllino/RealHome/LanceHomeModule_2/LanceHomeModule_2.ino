

//INCLUDE LIBRARIES
#include <SPI.h>
#include <avr/wdt.h>
#include <Controllino.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <LanceControllino.h>

//MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#ifndef MQTT_SOCKET_TIMEOUT
  #define MQTT_SOCKET_TIMEOUT 0.05
#endif

#define MQTT_EVENTS false

// DEFINE MQTT BASIC ATTRIBUTES
const char *MQTT_CLIENT_ID = "LanceControllino2";
const char *MQTT_MAIN_TOPIC_SUBSCRIBE = "LanceControllino2/out";
const char *MQTT_MAIN_TOPIC_PUBLISH_ADMIN = "LanceControllino2/in/admin";
const char *MQTT_MAIN_TOPIC_PUBLISH_NORMAL = "LanceControllino2/in/normal";
const char *MQTT_USER_NAME = "ArduinoUser";
const char *MQTT_PASSWORD = "ArduinoUser12345";

//DEFINE PROTOTYPE
void MQTTCallback(char* , byte* , unsigned int);


//SET ETHERNET VARIABLES
byte Mac[] = { 0xAE, 0xED, 0xBA, 0xFE, 0xFF, 0xFD };
IPAddress Ip( 10, 10, 10, 22 );
IPAddress Servers( 10, 10, 10, 20 );
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

/*
ANALOG INPUT ASSIGNMENT defines mapping between Analog Inputs and digital Outputs
{AI00,BTN0,DO00} > ANALOG PORT, BUTTON, DIGITAL OUTPUT

Available Analog Ports:
#define AI00 54         #define AI01 55       #define AI02 56       #define AI03 57         #define AI04 58       #define AI05 59
#define AI06 60         #define AI07 61       #define AI08 62       #define AI09 63         #define AI10 64       #define AI11 65

Availabe Analog Buttons:
#define BTN0 0          #define BTN1 1        #define BTN2 2        #define BTN3 3          #define BTNNONE 4

Available Digital Ports:
#define DO00 2          #define DO01 3        #define DO02 4        #define DO03 5          #define DO04 6         #define DO05 7
#define DO06 8          #define DO07 9        #define RO00 22       #define RO01 23         #define RO02 24        #define RO03 25
#define RO04 26         #define RO05 27       #define RO06 28       #define RO07 29         #define RO08 30        #define RO09 31
#define DIGPORTNONE -1
*/

#define ANALOG_INPUT_ASSIGNMENT_RW 18
int analogInputAssignment [ANALOG_INPUT_ASSIGNMENT_RW][3] = {
  {AI00,BTN0,DO06},
  {AI00,BTN2,DO05},
  {AI00,BTN3,DO01},
  {AI05,BTN0,DO06},
  {AI05,BTN1,DO02},
  {AI05,BTN2,DO04},
  {AI01,BTN0,RO04},
  {AI01,BTN1,RO05},
  {AI01,BTN2,RO00},
  {AI01,BTN3,RO01},
  {AI02,BTN0,RO06},
  {AI02,BTN1,RO07}, 
  {AI02,BTN2,RO02},  
  {AI02,BTN3,RO03},
  {AI06,BTN0,RO09},  
  {AI06,BTN1,RO08},
  {AI06,BTN2,DO07},
  {AI06,BTN3,DO08}};

/*
DIGITAL OUTPUT ASSIGNMENT extends ANALOG INPUT ASSIGNMENT with digital 
{DO00,LLIGHT,GROUPNONE} > DIGITAL PORT, PORT TYPE, GROUP NUMBER

Available Digital Ports:
#define DO00 2          #define DO01 3        #define DO02 4        #define DO03 5          #define DO04 6         #define DO05 7
#define DO06 8          #define DO07 9        #define RO00 22       #define RO01 23         #define RO02 24        #define RO03 25
#define RO04 26         #define RO05 27       #define RO06 28       #define RO07 29         #define RO08 30        #define RO09 31
#define DIGPORTNONE -1

Available Digital Port Types:
#define LLIGHT 0        #define LFAN 1        #define LSHADEUP 2    #define LSHADEDOWN 3    #define TYPENONE -1

Avalibale Groups:
#define GROUP101 101    #define GROUP102 102  #define GROUP103 103  #define GROUP104 104    #define GROUP105 105    #define GROUP106 106
#define GROUP107 107    #define GROUP108 108  #define GROUP109 109  #define GROUP110 110    #define GROUP111 111    #define GROUP112 112
#define GROUPNONE -1

NOTE: Group must to be defined only for shade (SHADEUP/SHADEDOWN) related ports.
*/

#define DIGITAL_OUTPUT_ASSIGNMENT_RW 17
int digitalOutputAssignment [DIGITAL_OUTPUT_ASSIGNMENT_RW][3] = {
  {DO06,LLIGHT,      GROUPNONE},
  {DO05,LLIGHT,      GROUPNONE},
  {DO01,LLIGHT,      GROUPNONE},
  {DO02,LLIGHT,      GROUPNONE},
  {DO04,LLIGHT,      GROUPNONE},
  {RO00,LSHADEUP,    GROUP101},
  {RO01,LSHADEDOWN,  GROUP101},
  {RO02,LSHADEUP,    GROUP102},
  {RO03,LSHADEDOWN,  GROUP102},
  {RO04,LSHADEUP,    GROUP103},
  {RO05,LSHADEDOWN,  GROUP103},
  {RO06,LSHADEUP,    GROUP104},
  {RO07,LSHADEDOWN,  GROUP104},
  {RO08,LFAN,        GROUPNONE},
  {RO09,LFAN,        GROUPNONE},
  {DO07,LSHADEUP,    GROUP105},
  {DO08,LSHADEDOWN,  GROUP105}};
    
LanceControllino LanceControllino(analogInputAssignment,digitalOutputAssignment,ANALOG_INPUT_ASSIGNMENT_RW,DIGITAL_OUTPUT_ASSIGNMENT_RW,MQTT_EVENTS);

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(50);

  Serial.println("marker");

  Ethernet.begin(Mac, Ip);

  //INITIATE CONNECTION TO MQTT SERVER IF NOT ESTABLISHED ALREADY. Clients.connected CAN BE "0" - NOT CONNECTED OR "1" - CONNECTED
  if (!Clients.connected() && MQTT_EVENTS == true) {
    if (Clients.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD)) {
        Clients.publish(MQTT_MAIN_TOPIC_PUBLISH_ADMIN,"HALLO:LanceControllino2:INIT");
        Clients.subscribe(MQTT_MAIN_TOPIC_SUBSCRIBE);
    }
  }
  // START WATCHDOG
  wdt_enable(WDTO_4S);
}

void MQTTCallback(char* topic, byte* payload, unsigned int length) 
{
  /*
  MQTTCallback is defined to process payload coming from MQTT server. The system subscribes messages from topic defined in MQTT_MAIN_TOPIC_SUBSCRIBE variable.
  Message syntax:
  EX1: TYPE=LIGHT:PORT=8:STATUS=OFF
  EX2: TYPE=SHADEUP:PORT=8:STATUS=OFF
  EX3: TYPE=FAN:PORT=8:STATUS=ON

  Acceptable Types: LIGHT, SHADEUP, SHADEDOWN, FAN
  Acceptable Ports: See digitalOutputAssignment array
  Acceptable Statuses: ON, OFF
  */
  if(length > 0) {

    String message;
    String type;
    String port;
    String status;
    int statusInt;
    int portInt;

    for(int i=0;i<length;i++){
    message = message + (char)payload[i];
    }

    if (message.indexOf("TYPE=") != -1 && message.indexOf("PORT=") != -1 && message.indexOf("STATUS=") != -1) {
      type = message.substring(message.indexOf("TYPE=") + 5,message.indexOf(":",message.indexOf("TYPE=")));
      port = message.substring(message.indexOf("PORT=") + 5,message.indexOf(":",message.indexOf("PORT=")));
      status = message.substring(message.indexOf("STATUS=") + 7,message.indexOf(":",message.indexOf("STATUS=")));

      if (status == "ON") {
        statusInt = ON;
        portInt = port.toInt();
        }
      else if (status == "OFF") {
        statusInt = OFF;
        portInt = port.toInt();
        }

      Serial.println("type=" + type + " port=" + port + " status=" + status);
      LanceControllino.processExternalRequest(portInt, statusInt);
    }
  }  
}


void mqttPublishToTopicFromBuffer() {
  if (LanceControllino.pullFromBuffer()) {
    Clients.publish(MQTT_MAIN_TOPIC_PUBLISH_NORMAL,LanceControllino.bufferPull._topicMessage);
  }
}

void loop()
{ 
  //INITIATE MAJOR TIMER
  CurrentTime = millis();

  //PERFORM EVERY 200MS USING MAJOR TIMER, LIGHT STANDARD
  if ((CurrentTime - PreviousTime1) >= 200) {    
    PreviousTime1 = CurrentTime;
      if(MQTT_EVENTS == true) {
        mqttPublishToTopicFromBuffer();
      }
      LanceControllino.statusTimeVerification();
  }

  if ((CurrentTime - PreviousTime2) >= 40) { 
    PreviousTime2 = CurrentTime;
    LanceControllino.analogInputVerification();
  } 

 //CONNECTIVITY TO MQTT SERVER IS CHECKED AT CERTAIN TIMES. IT IS ATTEMPTED THE CONNECTION TO BE REESTABLISHED IF IT'S DOWN.
  if ((CurrentTime - PreviousTime4) >= 60000) { 
    PreviousTime4 = CurrentTime;
    if (!Clients.connected() && MQTT_EVENTS == true) {
      if (Clients.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD)) {
        Clients.publish(MQTT_MAIN_TOPIC_PUBLISH_ADMIN,"HALLO:LanceControllino2:INIT");
        Clients.subscribe(MQTT_MAIN_TOPIC_SUBSCRIBE);
      }
    }
  }

  if ((CurrentTime - PreviousTime5) >= 200) { 
    PreviousTime5 = CurrentTime;
    if(MQTT_EVENTS == true) {
      Clients.loop();
    }
  } 
  
  //Watchdog
  wdt_reset();
}
