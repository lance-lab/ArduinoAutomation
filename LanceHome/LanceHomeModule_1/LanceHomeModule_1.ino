// Last update: 8/29/2022
#include "LanceControllino.h"

#define MQTT_EVENTS true

#define ANALOG_INPUT_ASSIGNMENT_RW 29
int analogInputAssignment[ANALOG_INPUT_ASSIGNMENT_RW][3] = {
  {AI00,BTN0,DO07},
  {AI00,BTN1,RO08},
  {AI01,BTN0,RO04},
  {AI01,BTN1,DO06},
  {AI01,BTN2,DO07},
  {AI01,BTN3,RO07},
  {AI02,BTN0,RO03},
  {AI02,BTN1,DO05},
  {AI02,BTN2,RO01},
  {AI02,BTN3,RO07},
  {AI04,BTN0,RO07},
  {AI04,BTN1,RO05},
  {AI04,BTN2,RO01},
  {AI05,BTN1,DO03},
  {AI05,BTN2,DO07},
  {AI05,BTN3,RO02},
  {AI06,BTN0,DO07},
  {AI06,BTN1,RO09},
  {AI06,BTN2,DO02},
  {AI06,BTN3,RO01},
  {AI08,BTN0,DO04},
  {AI08,BTN1,DO07},
  {AI08,BTN2,RO06},
  {AI08,BTN3,DO02},
  {AI09,BTN0,RO09},
  {AI09,BTN1,RO00},
  {AI09,BTN2,DO00},
  {AI09,BTN3,DO01},
  {AI10,BTN1,RO02}
};

#define DIGITAL_OUTPUT_ASSIGNMENT_RW 18
DigitalOutputConfig digitalOutputAssignment[DIGITAL_OUTPUT_ASSIGNMENT_RW] = {
  {DO00,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       1}, //fixed
  {DO01,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       2}, //fixed
  {DO02,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           2}, //fixed
  {DO03,LLIGHT,      GROUPNONE, ZONE_BATH_ROOM,         1}, //fixed
  {DO04,LLIGHT,      GROUPNONE, ZONE_BATH_ROOM,         2}, //fixed
  {DO05,LLIGHT,      GROUPNONE, ZONE_KID_ROOM,          1}, //fixed
  {DO06,LLIGHT,      GROUPNONE, ZONE_WORKING_ROOM,      1}, //fixed
  {DO07,LLIGHT,      GROUPNONE, ZONE_COMMON,            2}, //fixed
  {RO00,LLIGHT,      GROUPNONE, ZONE_LIVING_ROOM,       3}, //fixed
  {RO01,LLIGHT,      GROUPNONE, ZONE_COMMON,            3}, //fixed
  {RO02,LLIGHT,      GROUPNONE, ZONE_MAINTENANCE_ROOM,  1}, //fixed
  {RO03,LLIGHT,      GROUPNONE, ZONE_KID_ROOM,          2}, //fixed
  {RO04,LLIGHT,      GROUPNONE, ZONE_WORKING_ROOM,      2}, //fixed
  {RO05,LLIGHT,      GROUPNONE, ZONE_BED_ROOM,          1}, //fixed 
  {RO06,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           1}, //fixed
  {RO07,LLIGHT,      GROUPNONE, ZONE_COMMON,            4}, //fixed
  {RO08,LLIGHT,      GROUPNONE, ZONE_COMMON,            5}, //fixed
  {RO09,LLIGHT,      GROUPNONE, ZONE_KITCHEN,           3}  //fixed
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
  lanceRuntime.setup();
}

void loop()
{
  lanceRuntime.loop();
}
