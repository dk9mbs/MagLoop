#include <Stepper.h>

// number of steps in one revolution of your motor
#define STEPS 2038

// set the speed in rpm
#define MOTOR_RPM 6
#define DBG_LED 2

Stepper stepper(STEPS, 8, 10, 9, 11);

String cmd;
String dir = "R";
int currentPos = 0;
int minPos = 0;
int maxPos = 4200;
int calSteps = 100;
int changeInDirectionOffset = 200; // Schlupf der Welle und ungenaunigkeiten der Mechanik

void setup()
{
  pinMode(DBG_LED, OUTPUT);
  digitalWrite(DBG_LED, HIGH);

  Serial.begin(9600);
  Serial.println("*** Welcome to the DK9MBS / KI5HDH MagLoop Stepper Controller V1.0 ***");
  Serial.println("---");
}

void loop()
{
  String result = "ERR UNKNOWN";
  String commandName = "";

  if (Serial.available()) {
    char in = Serial.read();

    if (in == 13 || in == 10) {
      commandName = getCommandValue(cmd, ' ', 0);
      Serial.println("echo:" + cmd);

      if (commandName == "HELO") {
        digitalWrite(DBG_LED,LOW);
        result = "OK";
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

      Serial.println(result);
      cmd = "";
      return;
    } else {
      cmd += in;
    }
  }
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
  return "OK";
}

String commandAbsolute(String cmd) {
  int pos = getCommandValue(cmd, ' ', 1).toInt();
  int steps = 0;

  steps = pos - currentPos;

  if (steps + currentPos < minPos) steps = 0;
  if (steps + currentPos > maxPos) steps = maxPos;

  currentPos += steps;

  moveStepper(steps, MOTOR_RPM);
  return "OK";
}

String commandML(String cmd) {
  int steps = getCommandValue(cmd, ' ', 1).toInt();

  if (currentPos + steps > maxPos) {
    steps = maxPos - currentPos;
  }

  currentPos += steps;
  moveStepper(steps, MOTOR_RPM);
  return "OK";
}

String commandMR(String cmd) {
  int steps = getCommandValue(cmd, ' ', 1).toInt();

  if (currentPos - steps < minPos) {
    steps = currentPos;
  }

  currentPos -= steps;
  moveStepper(steps * -1, MOTOR_RPM);
  return "OK";
}

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
