#include <Stepper.h>

// number of steps in one revolution of your motor
#define STEPS 2038

// set the speed in rpm
#define MOTOR_RPM 6

Stepper stepper(STEPS, 8, 10, 9, 11);

String cmd;
String dir = "R";
int currentPos = 0;
int minPos = 0;
int maxPos = 4200;
int calSteps = 100;

// AZIMUT ROTOR (horizontal)
#define ROT_AZI_STEP 2
#define ROT_AZI_DIR 3
#define ROT_AZI_ENABLED 4
#define ROT_AZI_LIMIT_SWITCH 5
#define ROT_ELE_LIMIT_SWITCH 6

#define ROT_STEPS_AFTER_LIMIT 1

const int steps_rotate_360 = 200;


void setup()
{
  pinMode(ROT_AZI_STEP, OUTPUT);
  pinMode(ROT_AZI_DIR, OUTPUT);
  pinMode(ROT_AZI_ENABLED, OUTPUT);
  pinMode(ROT_AZI_LIMIT_SWITCH,INPUT_PULLUP);
  pinMode(ROT_ELE_LIMIT_SWITCH,INPUT_PULLUP);
  
  digitalWrite(ROT_AZI_STEP, HIGH);
  digitalWrite(ROT_AZI_DIR, LOW);
  digitalWrite(ROT_AZI_ENABLED, HIGH);

  Serial.begin(9600);
  Serial.println("OK -999");
  delay(1000);

  commandRotorAziMz("ROT_AZI_MZ");
  delay(1000);
}

void loop()
{
  String result = "ERR UNKNOWN";
  String commandName = "";

  if (Serial.available()) {
    char in = Serial.read();

    if (in == 13 || in == 10) {
      commandName = getCommandValue(cmd, ' ', 0);
      Serial.println("ECHO:" + cmd);

      if (commandName == "HELO") {
        Serial.println("KI5HDH / DK9MBS MagLoop controller firmware V0.1");
        result="OK";
      }

      if (commandName == "ROT_AZI") {
        result=commandRotorAzi(cmd);
      }

      if (commandName == "ROT_AZI_MZ") {
        result=commandRotorAziMz(cmd);
      }

      if (commandName == "MZ") {
        result = commandMZ(cmd);
      }

      if (commandName == "ML") {
        result = commandML(cmd);
      }

      if (commandName == "MR") {
        result = commandMR(cmd);
      }

      if (commandName == "MA") {
        result = commandAbsolute(cmd);
      }

      if (commandName == "SZ") {
        result = commandSZ(cmd);
      }

      if (commandName == "SP") {
        result = commandSP(cmd);
      }

      if (commandName == "CAL") {
        result = commandCal(cmd);
      }
      Serial.println("CAP_POS:"+String(currentPos));
      Serial.println("ROT_AZI_POS:0");
      Serial.println("LAST_CMD:"+cmd);
      Serial.println(result);
      cmd = "";
      return;
    } else {
      cmd += in;
    }
  }
}

String commandRotorAzi(String cmd) {
  String dir=getCommandValue(cmd, ' ', 1);
  int steps=getCommandValue(cmd, ' ', 2).toInt();

  moveRotorStepperAzi(dir, steps,1);

   if(digitalRead(ROT_AZI_LIMIT_SWITCH)) {
      if(dir=="L") {
        moveRotorStepperAzi("R",ROT_STEPS_AFTER_LIMIT,0);
      } else {
        moveRotorStepperAzi("L",ROT_STEPS_AFTER_LIMIT,0);
      }
    }

  return "OK";
}

String commandRotorAziMz(String cmd) {
  //if (digitalRead(ROT_AZI_LIMIT_SWITCH) == HIGH) moveRotorStepperAzi("L", ROT_STEPS_AFTER_LIMIT,0);
  
  while (digitalRead(ROT_AZI_LIMIT_SWITCH) == LOW) {
    moveRotorStepperAzi("L", 1,1);
  }
  moveRotorStepperAzi("R", ROT_STEPS_AFTER_LIMIT,0);

  return "OK";
}

String commandCal(String cmd) {
  moveStepper(calSteps * -1, MOTOR_RPM);
  return "OK";
}

String commandSP(String cmd) {
  return "OK " + String(currentPos);
}

String commandSZ(String cmd) {
  currentPos = 0;
  return "OK";
}

String commandMZ(String cmd) {
  if (currentPos > 0) {
    moveStepper(currentPos * -1, MOTOR_RPM);
  }
  currentPos = 0;
  return "OK "+String(currentPos);
}

String commandAbsolute(String cmd) {
  int pos = getCommandValue(cmd, ' ', 1).toInt();
  int steps = 0;

  steps = pos - currentPos;

  if (steps + currentPos < minPos) steps = 0;
  if (steps + currentPos > maxPos) steps = maxPos;

  currentPos += steps;

  moveStepper(steps, MOTOR_RPM);
  return "OK "+String(currentPos);
}

String commandML(String cmd) {
  int steps = getCommandValue(cmd, ' ', 1).toInt();

  if (currentPos + steps > maxPos) {
    steps = maxPos - currentPos;
  }

  currentPos += steps;
  moveStepper(steps, MOTOR_RPM);
  return "OK "+String(currentPos);
}

String commandMR(String cmd) {
  int steps = getCommandValue(cmd, ' ', 1).toInt();

  if (currentPos - steps < minPos) {
    steps = currentPos;
  }

  currentPos -= steps;
  moveStepper(steps * -1, MOTOR_RPM);
  return "OK "+String(currentPos);
}

/*
 * 
 * Helper functions
 * 
 */
void moveStepper(int steps, int rpm) {
  if (steps < 0) {
    dir = "R";
  } else {
    dir = "L";
  }

  stepper.setSpeed(rpm);
  stepper.step(steps);

  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);

}


void moveRotorStepperAzi(String dir, int steps , int readLimitSwitch) {
  if(dir=="R") {
    digitalWrite(ROT_AZI_DIR, LOW);
  } else {
    digitalWrite(ROT_AZI_DIR, HIGH);
  }
  
  digitalWrite(ROT_AZI_ENABLED, LOW);
  for(int i = 0; i < steps_rotate_360*steps; i++) {
    
    if(digitalRead(ROT_AZI_LIMIT_SWITCH) == HIGH && readLimitSwitch==1){
      delay(10);
      break;
    }
    
    digitalWrite(ROT_AZI_STEP, HIGH);
    delay(1);
    digitalWrite(ROT_AZI_STEP, LOW);
    delay(1);
  }
  digitalWrite(ROT_AZI_ENABLED, HIGH);
}


/*
   getCommandValue
   Return a part of a complete Stepper command.
   Example:
   getCommandValue("MF 100", ' ',2)
   Return 100
*/
String getCommandValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
