
#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

#include <CheapStepper.h>
#include "dk9mbs_tools.h"
#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>


#define SETUPPIN 5 
#define CAP_MOTOR_RPM 20
#define ROT_AZI_STEP 2
#define ROT_AZI_DIR 3
#define ROT_AZI_ENABLED D4
#define ROT_AZI_LIMIT_SWITCH D2
#define ROT_ELE_LIMIT_SWITCH D3
#define ROT_STEPS_AFTER_LIMIT 1

WiFiClient espClient;
ESP8266WebServer httpServer(80);
CheapStepper stepper (D5,D6,D7,D8); 
HTTPClient http;

boolean runSetup=false;
int state=0; // Status from Statemachine
// Cap. stepper definition
int cap_currentPos = 0;
int cap_minPos = 0;
int cap_maxPos = 8500;
int cap_calSteps = 100;

// AZIMUT ROTOR (horizontal)
const int rot_azi_steps_rotate_360 = 200;
int rot_aziCurrentPos=0;

void setup()
{


  Serial.begin(9600);

  setupIo();
  setupFileSystem();

  if(digitalRead(SETUPPIN)==0) runSetup=true;
  
  Serial.print("Setup:");
  Serial.println(digitalRead(SETUPPIN));
  Serial.print("adminpwd: ");
  Serial.println(readConfigValue("adminpwd"));
  
  if(runSetup) {
    setupWifiAP();
    setupHttpAdmin();
  } else {
    setupHttpAdmin();
    setupWifiSTA(readConfigValue("ssid").c_str(), readConfigValue("password").c_str(), readConfigValue("mac").c_str());
  } // setup 


  pinMode(ROT_AZI_STEP, OUTPUT);
  pinMode(ROT_AZI_DIR, OUTPUT);
  pinMode(ROT_AZI_ENABLED, OUTPUT);
  pinMode(ROT_AZI_LIMIT_SWITCH,INPUT_PULLUP);
  pinMode(ROT_ELE_LIMIT_SWITCH,INPUT_PULLUP);
  
  digitalWrite(ROT_AZI_STEP, HIGH);
  digitalWrite(ROT_AZI_DIR, LOW);
  digitalWrite(ROT_AZI_ENABLED, HIGH);

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
  httpServer.handleClient(); 
  
  String result = "ERR UNKNOWN";
  static String cmd;
  
  if (Serial.available()) {
    char in = Serial.read();

    if (in == 13 || in == 10) {
      Serial.println("ECHO:" + cmd);
      result=command(cmd);
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


String command(String& cmd) {
  String result = "ERR UNKNOWN";
  String commandName = "";
  commandName = getCommandValue(cmd, ' ', 0);
  
  if (commandName == "HELO") {
    result="KI5HDH / DK9MBS MagLoop controller firmware V0.1\nOK";
  }
  
  if (commandName == "ROT_AZI") {
    result=commandRotorAzi(cmd);
  }
  
  if (commandName == "ROT_AZI_MZ") {
    result=commandRotorAziMz(cmd);
  }
  
  if (commandName == "ROT_AZI_MAX") {
    result=commandRotorAziMax(cmd);
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
  cmd = "";

  return result;
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

String commandRotorAziMax(String cmd) {
  //if (digitalRead(ROT_AZI_LIMIT_SWITCH) == HIGH) moveRotorStepperAzi("L", ROT_STEPS_AFTER_LIMIT,0);
  int count=0;
  
  while (digitalRead(ROT_AZI_LIMIT_SWITCH) == LOW) {
    moveRotorStepperAzi("R", 1,1);
    count++;
    Serial.println("ROT_AZI_POS:"+count);
  }
  moveRotorStepperAzi("L", ROT_STEPS_AFTER_LIMIT,0);

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














String createIoTDeviceAddress(String postfix) {
  String address=String(readConfigValue("mac")+"."+postfix);
  address.replace("-","");
  return address;  
}

void setupHttpAdmin() {
  httpServer.on("/",handleHttpSetup);
  httpServer.on("/api",handleHttpApi);
  httpServer.onNotFound(handleHttp404);
  httpServer.begin();
}

void handleHttpApi() {
  httpServer.send(200, "text/html", "api"); 
}

void handleHttpSetup() {
    String pwd = readConfigValue("adminpwd");
    if (!httpServer.authenticate("admin", pwd.c_str())) {
      return httpServer.requestAuthentication();
    }
      
    if(httpServer.hasArg("CMD")) {
      Serial.println(httpServer.arg("CMD"));
      handleSubmit();
    }

    if(httpServer.hasArg("FORMATFS")) {
      Serial.println("Format FS");
      handleFormat();
    }
    if(httpServer.hasArg("RESET")) {
      Serial.println("Reset ...");
      handleReset();
    }

    String html =
    "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>DK9MBS/KI5HDH IoT Sensor</title>"
    "<style>"
    "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
    "</style>"
    "</head>"
    "<body>"
    "<h1>Setup shell by dk9mbs</h1>"
    "<FORM action=\"/\" method=\"post\">"
    "<P>Wlan:"
    "<INPUT type=\"hidden\" name=\"CMD\" value=\"SAVE\"><BR>"
    "<div style=\"border-style: solid; border-width:thin; border-color: #000000;padding: 2px;margin: 1px;\"><div>ssid</div><INPUT style=\"width:99%;\" type=\"text\" name=\"SSID\" value=\""+ readConfigValue("ssid") +"\"></div>"
    "<div style=\"border-style: solid; border-width:thin; border-color: #000000;padding: 2px;margin: 1px;\"><div>Password</div><INPUT style=\"width:99%;\" type=\"text\" name=\"PASSWORD\" value=\""+ readConfigValue("password") +"\"></div>"
    "<div style=\"border-style: solid; border-width:thin; border-color: #000000;padding: 2px;margin: 1px;\"><div>MAC (A4-CF-12-DF-69-00)</div><INPUT style=\"width:99%;\" type=\"text\" name=\"MAC\" value=\""+ readConfigValue("mac") +"\"></div>"
    "</P>"
    "<P>Network:"
    "<div style=\"border-style: solid; border-width:thin; border-color: #000000;padding: 2px;margin: 1px;\"><div>Hostname</div><INPUT style=\"width:99%;\" type=\"text\" name=\"HOSTNAME\" value=\""+ readConfigValue("hostname") +"\"></div>"
    "</P>"
    "<P>Admin portal"
    "<div style=\"border-style: solid; border-width:thin; border-color: #000000;padding: 2px;margin: 1px;\"><div>Admin Password</div><INPUT style=\"width:99%;\" type=\"text\" name=\"ADMINPWD\" value=\""+ readConfigValue("adminpwd") +"\"></div>"
    "</P>"
    "</P>"
    "<div>"
    "<INPUT type=\"submit\" value=\"Save\">"
    "<INPUT type=\"submit\" name=\"RESET\" value=\"Save and Reset\">"
    "<INPUT type=\"submit\" name=\"FORMATFS\" value=\"!!! Format fs !!!\">"
    "</div>"
    "</FORM>"
    "</body>"
    "</html>";
    httpServer.send(200, "text/html", html); 
}

void handleSubmit() {
  saveConfigValue("mac", httpServer.arg("MAC"));
  saveConfigValue("ssid", httpServer.arg("SSID"));
  saveConfigValue("password", httpServer.arg("PASSWORD"));
  saveConfigValue("adminpwd", httpServer.arg("ADMINPWD"));
  saveConfigValue("hostname", httpServer.arg("HOSTNAME"));
}

void handleReset() {
  httpServer.send(200, "text/plain", "restart ..."); 
  ESP.restart();
}

void handleFormat() {
  Serial.print("Format fs ... ");
  SPIFFS.format();
  setupFileSystem();
  Serial.println("ready");
}

void handleHttp404() {
  httpServer.send(404, "text/plain", "404: Not found"); 
}

void setupIo() {
  ESP.eraseConfig();
  WiFi.setAutoConnect(false);
  pinMode(SETUPPIN,INPUT);
}

void setupFileSystem() {
  if(!SPIFFS.begin()) {
    SPIFFS.format();
    SPIFFS.begin(); 
  }

  
  if(!SPIFFS.exists(getConfigFilename("mac"))) {
    String mac=WiFi.macAddress();
    mac.replace(":","-");
    saveConfigValue("mac", mac);
  }
  if(!SPIFFS.exists(getConfigFilename("ssid"))) saveConfigValue("ssid", "wlan-ssid");
  if(!SPIFFS.exists(getConfigFilename("password"))) saveConfigValue("password", "wlan-password");
  if(!SPIFFS.exists(getConfigFilename("adminpwd"))) saveConfigValue("adminpwd", "123456789ff");
  if(!SPIFFS.exists(getConfigFilename("hostname"))) saveConfigValue("hostname", "node");
  if(!SPIFFS.exists(getConfigFilename("pubtopic"))) saveConfigValue("pubtopic", "temp/sensor");
}

void setupWifiAP(){
  Serial.println("Setup shell is starting ...");
  String pwd=readConfigValue("adminpwd");

  if(pwd==""){
    Serial.println("Use default password!");
    pwd="0000";
  }
  
  Serial.print("Password for AP:");
  Serial.println(pwd);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("sensor.iot.dk9mbs.de", pwd);

  Serial.println("AP started");

}

void setupWifiSTA(const char* ssid, const char* password, const char* newMacStr) {
  uint8_t mac[6];
  byte newMac[6];
  parseBytes(newMacStr, '-', newMac, 6, 16);

  WiFi.setAutoReconnect(true);

  if(newMacStr != "") {
    wifi_set_macaddr(0, const_cast<uint8*>(newMac));
    Serial.println("mac address is set");
  }
  
  wifi_station_set_hostname(readConfigValue("hostname").c_str());
  Serial.print("Hostname ist set: ");
  Serial.println(readConfigValue("hostname"));
  
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  Serial.print("Password:");
  Serial.println("***********");
  
  WiFi.begin(ssid, password);
  
  Serial.println("after WiFi.begin():");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  WiFi.macAddress(mac);
  
  Serial.println("WiFi connected");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  Serial.printf("Mac address:%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.printf("Mac address:%s\n", WiFi.macAddress().c_str());
  Serial.print("Subnet mask:");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway:");
  Serial.println(WiFi.gatewayIP());
  //WiFi.printDiag(Serial);

}
