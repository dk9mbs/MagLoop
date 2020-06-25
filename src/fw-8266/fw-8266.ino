
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


#define SETUPPIN D1 //5 
#define CAP_MOTOR_RPM 20
#define ROT_AZI_STEP 2
#define ROT_AZI_DIR 3
#define ROT_AZI_ENABLED D4
#define ROT_AZI_LIMIT_SWITCH D2
#define ROT_ELE_LIMIT_SWITCH D3
#define ROT_STEPS_AFTER_LIMIT 1

#define C_STEPPER_PIN1 D5
#define C_STEPPER_PIN2 D6
#define C_STEPPER_PIN3 D7
#define C_STEPPER_PIN4 D8


WiFiClient espClient;
ESP8266WebServer httpServer(80);
CheapStepper stepper (C_STEPPER_PIN1,C_STEPPER_PIN2,C_STEPPER_PIN3,C_STEPPER_PIN4); 

HTTPClient http;

enum cap_state {START,RUNNING};

boolean runSetup=false;

// AZIMUT ROTOR (horizontal)
const int rot_azi_steps_rotate_360 = 200;
int rot_aziCurrentPos=0;

class StepperRequest {
  public:
    static int _newPos;
    static int getNewPos();
    static void setNewPos(int value);
};

int StepperRequest::_newPos=-1;
int StepperRequest::getNewPos() {
  return StepperRequest::_newPos;
}
void StepperRequest::setNewPos(int value) {
  StepperRequest::_newPos=value;
}

class CapStepper{
  public:
    int currentPos=0;
    int targetPos=0;
    int minPos=0;
    int maxPos=8500;
    int calSteps=100;
    CapStepper();
    void init(CheapStepper& stepper);    
    void move(int steps, int rpm);
    void run();
    void clearPins();
    int getStepsLeft();
  private:
    CheapStepper* _stepper;
};


CapStepper::CapStepper() {

}

int CapStepper::getStepsLeft() {
  return _stepper->getStepsLeft();
}

void CapStepper::init(CheapStepper& stepper) {
  _stepper=&stepper;
}
void CapStepper::run () {
  _stepper->run();
  
}
void CapStepper::move(int steps, int rpm) {
  currentPos += steps;

  _stepper->setRpm(rpm);
  bool moveClockwise;
  if (steps < 0) {
    moveClockwise = false;
    steps=steps*-1;
  } else {
    moveClockwise = true;
  }

  _stepper->newMove(moveClockwise, steps);
}

void CapStepper::clearPins() {
    digitalWrite(C_STEPPER_PIN1, LOW);
    digitalWrite(C_STEPPER_PIN2, LOW);
    digitalWrite(C_STEPPER_PIN3, LOW);
    digitalWrite(C_STEPPER_PIN4, LOW);
}

CapStepper cap;


void setup()
{
  cap.init(stepper);
  
  Serial.begin(115200);
  
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
  
    pinMode(ROT_AZI_STEP, OUTPUT);
    pinMode(ROT_AZI_DIR, OUTPUT);
    pinMode(ROT_AZI_ENABLED, OUTPUT);
    pinMode(ROT_AZI_LIMIT_SWITCH,INPUT_PULLUP);
    pinMode(ROT_ELE_LIMIT_SWITCH,INPUT_PULLUP);
    
    digitalWrite(ROT_AZI_STEP, HIGH);
    digitalWrite(ROT_AZI_DIR, LOW);
    digitalWrite(ROT_AZI_ENABLED, HIGH);
    
    Serial.println("Waiting for commands over http...");
    delay(1000);
    
    // Invert beachten!!!
    //cap.move(cap_maxPos, CAP_MOTOR_RPM);
    //cap.move(cap_maxPos*-1, CAP_MOTOR_RPM);
    
  } 
}

void loop()
{
  httpServer.handleClient(); 
  cap.run();
  //delay(1);
  capStepperStateMaschine(cap);
  
}

void capStepperStateMaschine(const CapStepper& stepper) {
  enum {START, CHANGEREQUEST, MOVING};
  static int state=START;
  
  int newPos=StepperRequest::getNewPos();

  switch (state) {
    case START:
      if(newPos>=0){
          Serial.println("Statemaschine is running...");
          cap.targetPos=newPos;
          StepperRequest::setNewPos(-1);
          state=CHANGEREQUEST;
      }
      break;
    case CHANGEREQUEST:
      Serial.println("Statemaschine in state CHANGEREQUEST");
      if(cap.targetPos == cap.currentPos){
        state=START;
      } else {
        cap.move(cap.targetPos-cap.currentPos,CAP_MOTOR_RPM);
        state=MOVING;
      }
      break;
    case MOVING:
      if(cap.getStepsLeft()==0){
        cap.currentPos=cap.targetPos;
        cap.clearPins();
        state=START;
      }
      break;
  }
  
}

/*
 * 
 * Helper functions
 * 
 */

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
  httpServer.on("/control", handleControlMagLoop);
  httpServer.onNotFound(handleHttp404);
  httpServer.begin();
}

void handleHttpApi() {
  String cmd=httpServer.arg("command");
  String steps=httpServer.arg("steps");

  if(cmd=="CAL") {
    cap.move(steps.toInt(),CAP_MOTOR_RPM);
  } else if(cmd='MOVE') {
    StepperRequest::setNewPos(steps.toInt());
  }

  delay(1);
  httpServer.send(200, "text/html", "api"); 
}

void handleControlMagLoop() {
  String html=
      "<!DOCTYPE HTML>"
    "<html>"
    "<head>"
      "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
      "<title>Controlcenter</title>"
      "<style>"
      "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
      "</style>"
      "<script langauage=\"javascript\">"
      "function requestControl(pos){"
      "var request=new XMLHttpRequest();"
      "request.open('GET','/api?command=MOVE&steps='+pos+'');"
      "request.send();"
      "}"
      "</script>"
      "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css\">"
      "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js\"></script>"
      "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js\"></script>"
    "</head>"
    "<body>"
    "<nav class=\"navbar navbar-inverse\">"
    "  <div class=\"container-fluid\">"
    "    <div class=\"navbar-header\">"
    "      <a class=\"navbar-brand\" href=\"#\">MagLoop ControlCenter</a>"
    "    </div>"
    "    <ul class=\"nav navbar-nav\">"
    "      <li class=\"active\"><a href=\"#\">Home</a></li>"
    "      <li class=\"dropdown\">"
    "        <a class=\"dropdown-toggle\" data-toggle=\"dropdown\" href=\"#\">Page 1"
    "        <span class=\"caret\"></span></a>"
    "        <ul class=\"dropdown-menu\">"
    "          <li><a href=\"#\">Page 1-1</a></li>"
    "          <li><a href=\"#\">Page 1-2</a></li>"
    "          <li><a href=\"#\">Page 1-3</a></li>"
    "        </ul>"
    "      </li>"
    "      <li><a href=\"#\">Page 2</a></li>"
    "      <li><a href=\"#\">Page 3</a></li>"
    "    </ul>"
    "  </div>"
    "</nav>"
    "<FORM action=\"/\" method=\"post\">"
    "<P>"
    "<div class=\"row\">"
    "  <div class=\"col-sm-5\"></div>"
    "  <div class=\"col-sm-2\"><button type=\"button\" class=\"btn btn-primary btn-lg btn-block\" onclick=\"requestControl(8500)\">Maximales C</button></div>"
    "  <div class=\"col-sm-5\"></div>"
    "</div>"
    "<div class=\"row\">"
    "  <div class=\"col-sm-5\"></div>"
    "  <div class=\"col-sm-2\"><button type=\"button\" class=\"btn btn-primary btn-lg btn-block\" onclick=\"requestControl(0)\">Minimales</button></div>"
    "  <div class=\"col-sm-5\"></div>"
    "</div>"
    "<div class=\"row\">"
    "  <div class=\"col-sm-5\"></div>"
    "    <div class=\"col-sm-2\">"
    "      <div class=\"dropdown\">"
    "        <button class=\"btn btn-primary dropdown-toggle btn-lg btn-block\" type=\"button\" data-toggle=\"dropdown\">Favoriten"
    "        <span class=\"caret\"></span></button>"
    "        <ul class=\"dropdown-menu\">"
    "          <li><a onclick=\"alert('hallo');\" href=\"#\">14.074 MHz</a></li>"
    "          <li><a href=\"#\">14.078 MHz</a></li>"
    "          <li><a href=\"#\">14.230 MHz</a></li>"
    "        </ul>"
    "      </div>"    
    "    </div>"
    "  <div class=\"col-sm-5\"></div>"
    "</div>"
    "</P>"
    "</FORM>"
    "</body>"
    "</html>";

  httpServer.send(200,"text/html", html);
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
    "<title>DK9MBS/KI5HDH MagLoop Setup</title>"
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

/*
 * Rotor
 * 
 *
 */
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
