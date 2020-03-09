#include <CheapStepper.h>

CheapStepper stepper (8,9,10,11); 

#define CAP_MOTOR_RPM 20
#define ROT_AZI_STEP 2
#define ROT_AZI_DIR 3
#define ROT_AZI_ENABLED 4
#define ROT_AZI_LIMIT_SWITCH 5
#define ROT_ELE_LIMIT_SWITCH 6
#define ROT_STEPS_AFTER_LIMIT 1

// Cap. stepper definition
int cap_currentPos = 0;
int cap_minPos = 0;
int cap_maxPos = 8500;
int cap_calSteps = 100;

// AZIMUT ROTOR (horizontal)
int rot_azi_steps_rotate_360 = 200;

//String cmd;

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

  // Invert beachten!!!
  moveCapStepper(cap_maxPos, CAP_MOTOR_RPM);
  Serial.println("CAP_POS:"+String(cap_currentPos));
  moveCapStepper(cap_maxPos*-1, CAP_MOTOR_RPM);
  Serial.println("CAP_POS:"+String(cap_currentPos));
  commandRotorAziMz("");
}

void loop()
{
  String result = "ERR UNKNOWN";
  String commandName = "";
  //String cmd;
  static String cmd;
  
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

      if (commandName=="CAP_MAX") {
        result = commandMoveCapAbsolute(cmd, cap_minPos);
      }
      if (commandName=="CAP_MIN") {
        result = commandMoveCapAbsolute(cmd, cap_maxPos);
      }
      if (commandName == "CAP_ABSOLUTE") {
        int pos = getCommandValue(cmd, ' ', 1).toInt();
        result = commandMoveCapAbsolute(cmd, pos);
      }

      if (commandName == "CAP_DOWN") {
        int steps = getCommandValue(cmd, ' ', 1).toInt();
        result = commandCapMove(cmd,steps, "DOWN");
      }

      if (commandName == "CAP_UP") {
        int steps = getCommandValue(cmd, ' ', 1).toInt();
        result = commandCapMove(cmd,steps, "UP");
      }


      if (commandName == "SZ") {
        result = commandSZ(cmd);
      }

      if (commandName == "SP") {
        result = commandSP(cmd);
      }

      if (commandName == "CAL") {
        result = commandCapCal(cmd);
      }
      Serial.println("CAP_POS:"+String(cap_currentPos));
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

String commandCapCal(String cmd) {
  moveCapStepper(cap_calSteps * -1, CAP_MOTOR_RPM);
  return "OK";
}

String commandSP(String cmd) {
  return "OK " + String(cap_currentPos);
}

String commandSZ(String cmd) {
  cap_currentPos = 0;
  return "OK";
}

String commandMoveCapAbsolute(String cmd, int pos) {
  int steps = 0;

  steps = pos - cap_currentPos;

  if (steps + cap_currentPos < cap_minPos) steps = 0;
  if (steps + cap_currentPos > cap_maxPos) steps = cap_maxPos;

  moveCapStepper(steps, CAP_MOTOR_RPM);
  return "OK "+String(cap_currentPos);
}

String commandCapMove(String cmd, int steps, String dir) {
  if (dir == "DOWN" && cap_currentPos + steps > cap_maxPos) {
    steps = cap_maxPos - cap_currentPos;
  }

  if (dir == "UP" && cap_currentPos - steps < cap_minPos) {
    steps = cap_currentPos;
  }

  if (dir == "UP") {
    steps=steps*-1;
  }

  moveCapStepper(steps, CAP_MOTOR_RPM);
  return "OK "+String(cap_currentPos);
}


/*
 * 
 * Helper functions
 * 
 */
void moveCapStepper(int steps, int rpm) {
  cap_currentPos += steps;

  stepper.setRpm(rpm);
  bool moveClockwise;
  if (steps < 0) {
    moveClockwise = false;
    steps=steps*-1;
  } else {
    moveClockwise = true;
  }

  for (int s=0; s<steps; s++){
    stepper.step(moveClockwise);
  }

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
  for(int i = 0; i < rot_azi_steps_rotate_360*steps; i++) {
    
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
