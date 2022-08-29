/*
  LanceControllino.h Last update: 8/29/2022
*/

#ifndef LanceControllino_h
#define LanceControllino_h

#include <Controllino.h>
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

//Analog port availability
#define DISABLED 0
#define ENABLED 1

#define _ANALOG_INPUTS_SIZE 12
#define _DIGITAL_OUTPUTS_SIZE 18
#define _ANALOG_INPUT_ASSIGNMENT_SIZE 48
#define _DIGITAL_OUTPUT_ASSIGNMENT_SIZE 18
#define _DIGITAL_OUTPUT_ASSIGNMENT_CLOCK_SIZE 18
#define _TOPIC_MESSAGE_LENGTH 40

#define PRESSED_BTN_COUNT 2
#define RUN_FUN_COUNT 300000
#define RUN_SHADE_COUNT 50000

class LanceControllino
{
  public:
    LanceControllino(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize);
    LanceControllino(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize, bool events);
    void analogInputVerification();
    void statusTimeVerification();
    bool processExternalRequest(int port, int status);
    bool addToBuffer(int topic, String message);
    bool pullFromBuffer();

    struct _eventMessage
      {
        int _topic;
        char _topicMessage[_TOPIC_MESSAGE_LENGTH];
      };

    struct _eventMessage bufferAdd, bufferPull;

    RingBuf *_messageBuffer = RingBuf_new(sizeof(struct _eventMessage), 20);

  private:
    bool initialization(int analogAssignment[][3], int digitalAssignment[][3], int analogAssignmentSize, int digitalAssignmentSize);
    bool portActivation();
    bool lightOperation(int port, int status = -1);
    bool shadeOperation(int port, int status = -1);
    bool fanOperation(int port, int status = -1);
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

    /* _digitalOutputAssignmentClock
      Array extends _digitalOutputAssignment to capture millis (current time) if Fan or Shade operations are being processed.
      OFF - 0
    */
    unsigned long _digitalOutputAssignmentClock [_DIGITAL_OUTPUT_ASSIGNMENT_CLOCK_SIZE] = {OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF};


};
#endif