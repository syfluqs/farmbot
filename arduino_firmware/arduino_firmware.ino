#include <avr/io.h>

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
// thresholding values
#define LEFT_ENCODER_THRESH 512
#define RIGHT_ENCODER_THRESH 512
#define CARRIAGE_ENCODER_THRESH 512

//variables for storing encoder state
bool LEFT_ENCODER_PREV = false;
bool RIGHT_ENCODER_PREV = false;
bool CARRIAGE_ENCODER_PREV = false;
int LEFT_ENCODER_COUNT = 0;
int RIGHT_ENCODER_COUNT = 0;
int CARRIAGE_ENCODER_COUNT = 0;

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
  String command;
  struct node *next;
} node;

// HEAD pointer for the queue
volatile node* HEAD = NULL;

// TAIL pointer for the queue
volatile node* TAIL = NULL;

// temp variable to store incoming command
String command_tmp = "";

// indicator variable to check when incoming 
// command is fully received
bool commandReceived = false;


// insert method for instruction queue
void q(String command) {
  node * temp = (node*) malloc(sizeof(node));
  temp->command = command;
  temp->next = NULL;
  if (HEAD==NULL) {
    HEAD = temp;
    TAIL = HEAD;
  } else {
    TAIL -> next = temp;
    TAIL = temp;
  }
}

String dq() {
  node * temp = HEAD;
  HEAD = temp->next;
  String ret = temp->command;
  free(temp);
  return ret;
}

// method to print contents of the instruction queue
// only use for debugging
void printq() {
  node * temp = HEAD;
  Serial.println("Instruction Queue:");
  Serial.println("==================");
  while(temp=temp->next) {
    Serial.println(temp->command);
  }
  Serial.println("==================");
}

// method to check if instruction queue is empty
bool isempty() {
  if (HEAD==NULL) {
    return true;
  }
  return false;
}

void setup() {
  // reserve 16 bytes for command_tmp
  command_tmp.reserve(16);
  
  // put your setup code here, to run once:
  // setting GPIO 2 to 7 as outputs
  DDRD = 0xfc;
  // setting GPIO 9 to 11 as outputs
  DDRB = 0x0e;
  
  // Initialiser sequence
  // Set every used GPIO to HIGH because negative logic is used
  PORTD = 0xfc;
  PORTB = 0x0e;
  
  // Initialising serial for communication and debugging
  Serial.begin(9600);

  // Blink gantry lights to indicate setup completion
  digitalWrite(GANTRY_LIGHTS, LOW);
  delay(500);
  digitalWrite(GANTRY_LIGHTS, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:

  // check if command is received
  // if yes, queue it
  if (commandReceived) {
    q(command_tmp);
    command_tmp = "";
    commandReceived = false;
  }

  //check input from encoders
  
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    if (inChar == '\n') {
      commandReceived = true;
    } else {
      command_tmp += inChar;
    }
  }
}

