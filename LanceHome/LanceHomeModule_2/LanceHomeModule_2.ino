// Last update: 8/29/2022
#include "LanceControllino.h"
#include "ModuleConfiguration.h"

#define MQTT_EVENTS true

#define ANALOG_INPUT_ASSIGNMENT_RW 18
int analogInputAssignment[ANALOG_INPUT_ASSIGNMENT_RW][3] = {
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
  {AI06,BTN3,DO08}
};

#define DIGITAL_OUTPUT_ASSIGNMENT_RW 17
DigitalOutputConfig digitalOutputAssignment[DIGITAL_OUTPUT_ASSIGNMENT_RW] = {
  {DO06,LLIGHT,      GROUPNONE, ZONE_COMMON,            1},
  {DO05,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           1},
  {DO01,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           2},
  {DO02,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           3},
  {DO04,LLIGHT,      GROUPNONE, ZONE_OUTSIDE,           4},
  {RO00,LSHADEUP,    GROUP101,  ZONE_BED_ROOM,          1},
  {RO01,LSHADEDOWN,  GROUP101,  ZONE_BED_ROOM,          1},
  {RO02,LSHADEUP,    GROUP102,  ZONE_KID_ROOM,          1},
  {RO03,LSHADEDOWN,  GROUP102,  ZONE_KID_ROOM,          1},
  {RO04,LSHADEUP,    GROUP103,  ZONE_KID_ROOM,          2},
  {RO05,LSHADEDOWN,  GROUP103,  ZONE_KID_ROOM,          2},
  {RO06,LSHADEUP,    GROUP104,  ZONE_WORKING_ROOM,      1},
  {RO07,LSHADEDOWN,  GROUP104,  ZONE_WORKING_ROOM,      1},
  {RO08,LFAN,        GROUPNONE, ZONE_BATH_ROOM,         1},
  {RO09,LFAN,        GROUPNONE, ZONE_BATH_ROOM,         2},
  {DO07,LSHADEUP,    GROUP105,  ZONE_OUTSIDE,           1},
  {DO08,LSHADEDOWN,  GROUP105,  ZONE_OUTSIDE,           1}
};

LanceControllino lanceControllino(
  analogInputAssignment,
  digitalOutputAssignment,
  ANALOG_INPUT_ASSIGNMENT_RW,
  DIGITAL_OUTPUT_ASSIGNMENT_RW,
  MQTT_EVENTS
);

LanceControllinoRuntime lanceRuntime(
  lanceControllino,
  MQTT_EVENTS
);

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  CredentialManager::provisionFromConfig(ModuleConfiguration::DEVICE_CONFIG);
  lanceRuntime.setup();
}

void loop()
{
  lanceRuntime.loop();
}
