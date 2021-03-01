//DOME CALIBRATION- State Machine 
# include <math.h>
# define BUFFER_SIZE 8
# define MOTION_CW 0
# define MOTION_CCW 1

#define DEBUG_SERIAL_OUT 0  // Allows for printing during debugging if set to 1, no debug prints if set to 0

#if DEBUG_SERIAL_OUT
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

enum States
{
  IDLE_STATE,
  CALIBRATION_STATE,
  END_CALIBRATION_STATE,
  MOTOR_TURNING_STATE,
  END_MOTOR_TURNING_STATE 
};

typedef struct
{
  int sensorPin;
  int count; 
  bool lastState;
  char counterName;
}SensorInfo;

typedef struct
{
  int turningDirection;
  int gearCountsToTarget;
}MotorMovementInfo;

typedef struct
{
  char bufferData[BUFFER_SIZE];
  int index;
  bool hasData;
}CommandInfo;

typedef struct
{
  float absolutePosition;    //between 0-359 degrees
  float gearCountPerDomeDegree;
}DomeInfo;

SensorInfo sensorGear;
SensorInfo sensorDome;
CommandInfo commandInfo;
DomeInfo domeInfo;
MotorMovementInfo motorMovementInfo;

bool calculateCurrentState(int sensorStatus);
void updateCounter(SensorInfo* counter);
States currentState;
void readCommands();
bool updateMovement();
bool updateDeceleration();
void clearCommands();
void handleCalibrateCommand();
void handleMovementCommand();
void handleGetPositionCommand();
void handleParkCommand();

const int SENSITIVITY_THRESHOLD_MIN = 100; // The value that we'll consider to be high or low for the photoresistor
const int SENSITIVITY_THRESHOLD_MAX = 850;  //Dead zone is between threshold MIN and MAX

float countsToTurn;
bool motorDirection;

void setup() 
{
  Serial.begin(9600);   //speed in bits per second, talking to USB cable
  sensorGear.sensorPin = A0;
  sensorGear.count = 0;
  sensorGear.counterName = 'G'; //G for Gear
  sensorDome.sensorPin = A1;
  sensorDome.count = 0;
  sensorDome.counterName = 'D';  //D for Dome
  
  pinMode(sensorGear.sensorPin, INPUT); //initialize sensor on gear
  pinMode(sensorDome.sensorPin, INPUT);  //initialize sensor on dome

  domeInfo.absolutePosition = 0.0f; 

  currentState = IDLE_STATE;
  
}

void loop() 
{
 switch(currentState)
 {
  case IDLE_STATE:
  {
    //Serial.println("I'm in Idle");
    //commands from serial read from Serial Event function immediately when sent
    break;
  }
  case CALIBRATION_STATE:
    {
       updateCounter(&sensorDome);
       if(sensorDome.count > 1)
       {
        currentState = END_CALIBRATION_STATE;
       }
       
       if(sensorDome.count == 1)
       {
        updateCounter(&sensorGear);
       }
      break;
    }
  case END_CALIBRATION_STATE:
  {
    domeInfo.gearCountPerDomeDegree = float(sensorGear.count)/3.0f;  //number of 1/3 turns per one degree of dome turning 
#if DEBUG_SERIAL_OUT
    DEBUG_PRINT("Gear 1/3 Rotations per One Degree of Dome Rotation: ");
    DEBUG_PRINTLN(domeInfo.gearCountPerDomeDegree);
#endif
    currentState = IDLE_STATE;
    break; 
  }
  case MOTOR_TURNING_STATE:
  {
    break;
  }

   case END_MOTOR_TURNING_STATE:
   {
    //TODO: here we will send back a "finished moving command to driver";
    break;
   }
 }

}

bool calculateCurrentState(int sensorStatus)
{
  return (sensorStatus < SENSITIVITY_THRESHOLD_MIN);
}

void updateCounter(SensorInfo* counter)
{
   int sensorStatus = analogRead(counter->sensorPin);   //reads status of sensor value
#if DEBUG_SERIAL_OUT
   DEBUG_PRINT(counter->counterName);
   DEBUG_PRINT(" Sensor Status: ");
   DEBUG_PRINTLN(sensorStatus);
#endif

    if(sensorStatus < SENSITIVITY_THRESHOLD_MIN || sensorStatus > SENSITIVITY_THRESHOLD_MAX)
    {

        // True if photoresister has been covered (according to the average reading during our sample size)
        bool currentState = calculateCurrentState(sensorStatus);
#if DEBUG_SERIAL_OUT
        DEBUG_PRINT(counter->counterName);
        DEBUG_PRINT(" CurrentState: ");
        DEBUG_PRINTLN(currentState);
#endif
    
        if(currentState == true && counter->lastState == false)
        {

            DEBUG_PRINT(counter->counterName);
            DEBUG_PRINTLN(" currentState == true && lastState == false. Incrementing Counter");
            counter->lastState = true;
            counter->count++;
            DEBUG_PRINT(counter->counterName);
            DEBUG_PRINT(" Number of turns: ");
            DEBUG_PRINTLN(counter->count); 
        }
        else if(currentState == false && counter->lastState == true)
        {
            DEBUG_PRINT(counter->counterName);
            DEBUG_PRINTLN(" currentState == false && lastState == true");      
            counter->lastState = false;  
        }
    }


}

void serialEvent()  //reads data from the serial connection when data sent
{    
  while(Serial.available())
  { 
   char currentChar = (char)Serial.read();
   commandInfo.bufferData[commandInfo.index] = currentChar;
   if(currentChar == ';')
   {
    commandInfo.hasData = true;
    commandInfo.bufferData[commandInfo.index] = '\0';
   }
   commandInfo.index++;
  }

  if(commandInfo.hasData == true)
  {
    readCommands();
  
    for(int i=0; i<commandInfo.index ; i++)
    {
      DEBUG_PRINT(commandInfo.bufferData[i]);
    }

    clearCommands();
    }
}

void readCommands()
{
  DEBUG_PRINT("commandInfo.index: ");
  DEBUG_PRINTLN(commandInfo.index);
  DEBUG_PRINT("commandInfo.bufferData[0]: ");
  DEBUG_PRINTLN(commandInfo.bufferData[0]);
  
  if(commandInfo.index >= 3 && commandInfo.bufferData[0] == '+')
  {
    DEBUG_PRINT("commandInfo.bufferData[1]: ");    
    DEBUG_PRINTLN(commandInfo.bufferData[1]);
    switch(commandInfo.bufferData[1])
    {
      case 'C':
      {
        handleCalibrateCommand();
        break;
      }
      case 'M':
      {
        handleMovementCommand(commandInfo.bufferData); 
        break;
      }
      case 'G':
      {
        handleGetPositionCommand();
        break;
      }
      case 'P':
      {
        handleParkCommand();
        break;
      }
    }
  }
}

void clearCommands()
{
  commandInfo.hasData = false;
  memset(commandInfo.bufferData,0,BUFFER_SIZE); //sets buffer array to zero
  commandInfo.index = 0;
  DEBUG_PRINTLN();
}

bool updateMovement()
{
  return false;
}

bool updateDeceleration()
{
  return false;
}

void handleCalibrateCommand()
{
  domeInfo.absolutePosition = 0.0f;
  domeInfo.gearCountPerDomeDegree = 0.0f;
  sensorGear.count = 0;
  sensorDome.count = 0;
  sensorGear.lastState = calculateCurrentState(analogRead(sensorGear.sensorPin));
  sensorDome.lastState = calculateCurrentState(analogRead(sensorDome.sensorPin));
  currentState = CALIBRATION_STATE;
}

void handleMovementCommand(char* commandBuffer)
{
  bool isRelativeMovement = false;
  int moveDegrees; 
    
  switch(commandBuffer[2])
  {
    case 'A':
    {
     isRelativeMovement = false;
     DEBUG_PRINTLN("I am in case A ");
     break;
    }
    case 'R':
    {
      isRelativeMovement = true;
      DEBUG_PRINTLN("I am in case R");
      break;
    }
  }

  moveDegrees = atoi(&commandInfo.bufferData[3]);
  DEBUG_PRINT("Degrees entered to move: ");
  DEBUG_PRINTLN(moveDegrees);
 DEBUG_PRINT("Current Absolute Dome Position: ");
  DEBUG_PRINTLN(domeInfo.absolutePosition);

  if(isRelativeMovement == false) //converting to relative movement from absolute movement
  {
    int targetPosition = moveDegrees;
    float relativeMovement = fmod(targetPosition - domeInfo.absolutePosition + 180.0f, 360.0f) - 180;
    relativeMovement += (relativeMovement < -180) ? 360 : 0;
    moveDegrees = relativeMovement;
    DEBUG_PRINT("Absolute Degrees entered to move converted to Relative: ");
    DEBUG_PRINTLN(moveDegrees);
   //FOR DEBUG CODE ONLY, REMOVE BEFORE USE!!
   domeInfo.absolutePosition = targetPosition;
  }
 
 motorMovementInfo.gearCountsToTarget = abs(moveDegrees)*domeInfo.gearCountPerDomeDegree; //TODO: round function
 motorMovementInfo.turningDirection = moveDegrees > 0 ? MOTION_CW : MOTION_CCW;

 currentState = IDLE_STATE;
 sensorGear.count = 0;
}

void handleGetPositionCommand() //Retrieves Absolute position 
{
  Serial.print("#");
  Serial.print(domeInfo.absolutePosition);
  Serial.print(";");
}
void handleParkCommand()
{
  
}

