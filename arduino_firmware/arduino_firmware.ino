#include <avr/io.h>
#include <Stepper.h>

// Declarations for pin usage
#define CARRIAGE_MOTOR_2 11
#define CARRIAGE_MOTOR_1 10
#define JOHNSON_RIGHT_1 5
#define JOHNSON_RIGHT_2 9
#define JOHNSON_LEFT_1 6
#define JOHNSON_LEFT_2 3
#define GANTRY_LIGHTS 7
#define AIR_PUMP_RELAY 4
#define SOLENOID_VALVE_RELAY 2
#define LEFT_ENCODER A0
#define RIGHT_ENCODER A1
#define CARRIAGE_ENCODER A2
// threshold values
#define LEFT_ENCODER_THRESH 512
#define RIGHT_ENCODER_THRESH 512
#define CARRIAGE_ENCODER_THRESH 512

// variables for storing encoder state
bool LEFT_ENCODER_PREV = false;
bool RIGHT_ENCODER_PREV = false;
bool CARRIAGE_ENCODER_PREV = false;
int LEFT_ENCODER_COUNT = 0;
int RIGHT_ENCODER_COUNT = 0;
int CARRIAGE_ENCODER_COUNT = 0;
int n=0;

// pwm parameters for motors
#define JOHNSON_LEFT_SPEED 140
#define JOHNSON_RIGHT_SPEED 140
#define CARRIAGE_MOTOR_SPEED 140

// Declarations for the z-axis stepper motor
#define stepsPerRevolution 48
#define STEPPER_PIN_1 A3
#define STEPPER_PIN_2 A4
#define STEPPER_PIN_3 A5
#define STEPPER_PIN_4 A6
#define STEPPER_RPM 120
Stepper z_stepper(stepsPerRevolution, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);

/*
 *  Instruction queue structure is as follows
 *  
 *   HEAD -> command_1 -> command_2 -> command_3 -> NULL
 *                                      ^~~TAIL
 *                                      
 *  The HEAD points to the instruction to be executed next
 *  TAIL points to the end of queue
 *  New instructions get added behind TAIL
 *  HEAD is shifted forward when the current instruction is 
 *  fully executed
 */

// Struct containing one command.
typedef struct node {
  char id;
  unsigned int amt;
  struct node *next;
} node;
/*
 * id field defines the direction of movement or part to move.
 * amt defines the amount the part should move to.
 * Various part ids are defines below
 */

/*
 * X is the direction along the rails
 * amt for X signifies the encoder marking to move to
 */
#define X_ID 'X'

/*
 * Y is direction along the gantry
 * id for Y signifies the encoder marking to move to
 */
#define Y_ID 'Y'

/*
 * Z is the direction along the rack
 * amt for Z signifies the encoder marking to move to
 */
#define Z_ID 'Z'

/*
 * GANTRYS_LIGHT_ID
 * if amt is non-zero the gantry lights are turned on
 */
#define GANTRY_LIGHTS_ID 'L'

/*
 * SOLENOID_VALVE_ID
 * amt specifies the time in ms the solenoid valve is 
 * turned on
 */
#define SOLENOID_VALVE_ID 'V'

/*
 * AIR_PUMP_ID
 * if amt is non-zero air pump is turned on
 */
 #define AIR_PUMP_ID 'A'

 /*
  * DEBUG_ID
  * prints debugging unfo to serial irrespective of amt
  */
#define DEBUG_ID 'D'

 /*
  * WAIT_ID
  * makes arduino wait for amt number of ms
  */
#define WAIT_ID 'W'

  
// HEAD pointer for the queue
node* HEAD = NULL;

// TAIL pointer for the queue
node* TAIL = NULL;

// temp variable to store incoming command
char id_tmp = "";
int amt_tmp = 0;

// indicator variable to check if the current
// task is complete
bool taskComplete = true;

// insert method for instruction queue
void q(char id, int amt) {
  Serial.print("inserting=");
  Serial.println(amt);
  node * temp = (node*) malloc(sizeof(node));
  temp->id = id;
  temp->amt = amt;
  temp->next = NULL;
  if (HEAD==NULL) {
    HEAD = temp;
    TAIL = HEAD;
  } else {
    TAIL -> next = temp;
    TAIL = temp;
  }
}

// Stores value of dequeued id
char id = '\0';

// Stores value of dequeued amt
int amt = 0;

void dq() {
  node * temp = HEAD;
  HEAD = temp->next;
  id = temp->id;
  amt = temp->amt;
  Serial.print("dq=");
  Serial.println(amt);
  free(temp);
}

// method to print contents of the instruction queue
// only use for debugging
void printq() {
  node * temp = HEAD;
  Serial.println("Instruction Queue:");
  Serial.println("==================");
  while(temp=temp->next) {
    Serial.println(temp->id+" "+temp->amt);
  }
  Serial.println("==================");
}

// method to check if instruction queue is empty
bool qnotempty() {
  if (HEAD==NULL) {
    return false;
  }
  return true;
}

void setup() {
  // put your setup code here, to run once:
  // setting GPIO 2 to 7 as outputs
  DDRD = 0xfc;
  // setting GPIO 9 to 11 as outputs
  DDRB = 0x0e;
  
  // Initialiser sequence
  // Set every used GPIO to HIGH because negative logic is used
  PORTD = 0xfc;
  PORTB = 0x0e;

  // Initialisers for z_stepper
  z_stepper.setSpeed(STEPPER_RPM);
  
  // Initialising serial for communication and debugging
  Serial.begin(9600);

  // Blink gantry lights to indicate setup completion
  Serial.println("everything K");
  digitalWrite(GANTRY_LIGHTS, LOW);
  delay(500);
  digitalWrite(GANTRY_LIGHTS, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

  // check if command is received
  // if yes, queue it
//  if (commandReceived) {
//    Serial.print("amt_tmp=");
//    Serial.println(amt_tmp);
//    q(id_tmp,amt_tmp);
//    commandReceived = false;
//    id_tmp = 0;
//    amt_tmp = 0;
//  }

  // pull tasks from instruction queue
  if (qnotempty()) {
    dq();

    // check ids
    Serial.print("amt=");
    Serial.println(amt);
    if (id==DEBUG_ID) {
      Serial.println("Printing Debug info...");
      Serial.println("Current id="+id);
      Serial.println("Current amt="+amt);
      printq();
    } else if (id==GANTRY_LIGHTS_ID) {
      if (amt!=0) {
        Serial.println("Gantry lights on");
        digitalWrite(GANTRY_LIGHTS, LOW);
        //delay(3000);
      } else {
        digitalWrite(GANTRY_LIGHTS, HIGH);
        Serial.println("Gantry lights off");
      }
    } else if (id==SOLENOID_VALVE_ID) {
      Serial.println("Solenoid valve on");
      digitalWrite(SOLENOID_VALVE_RELAY, LOW);
      Serial.println(amt);
      delay(amt);
      Serial.println("Solenoid off");
      digitalWrite(SOLENOID_VALVE_RELAY, HIGH);
    } else if (id==AIR_PUMP_ID) {
      if (amt!=0) {
        digitalWrite(AIR_PUMP_RELAY, LOW);
        Serial.println("air pump on");
      } else {
        digitalWrite(AIR_PUMP_RELAY, HIGH);
        Serial.println("air pump off");
      }
    } else if (id==Z_ID) {
      z_stepper.step(stepsPerRevolution*(amt-25)/10);
    } else if (id==WAIT_ID) {
      Serial.println("waiting for");
      Serial.println(amt);
      delay(amt);
      Serial.println("wait over");
    } else if (id==X_ID) {
      // x-direction is driven by johnson motors
      if (amt > count) {
        //move forward till count reaches amt
        while (count<=amt) {
          
        }
      }
    } else if (id==Y_ID) {
      
    } else {
      Serial.println("Invalid keyword");
    }
    Serial.println("===");
    Serial.println(++n);
    Serial.println("===");
  }
  
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    Serial.print("-");
    Serial.println(inChar);
    if (inChar == '.') {
        q(id_tmp,amt_tmp);
        id_tmp = 0;
        amt_tmp = 0;
    } else if (inChar>='0' && inChar<='9') {
      amt_tmp = 10*amt_tmp+(int)(inChar-'0');
    } else {
      id_tmp = inChar;
    }
  }
}

