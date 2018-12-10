#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MotorShield.h>
#include "index.h"

Adafruit_INA219 ina219;

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *myMotor = AFMS.getMotor(4);

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
//at 255 maximum speed
// 1 rev = 12.4 seconds = 12400 ms
// 1 rev slow = 36 seconds=36000 ms
// from all the way out to all the way in = 610327 ms = 49 revolutions

int stallCurrent = 45;
int slow = 100;
int fast = 255;
int CLOCKWISE = 0;
int COUNTERCLOCKWISE = 1;
int MotorSpeed = slow;
int MotorDirection = CLOCKWISE;
long location = 0;
int stop = 1;
long focuserPosition = 0;
int initFocus = 0;

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;

long lastCheck = millis();

void handleRoot();              // function prototypes for HTTP handlers
void handleLogin();
void handleNotFound();
void serveDocument();

void setup(void) {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);

  AFMS.begin();  // create with the default frequency 1.6KHz

  // Set the speed to start, from 0 (off) to 255 (max speed)
  myMotor->setSpeed(fast);
  myMotor->run(FORWARD);
  // turn on motor
  myMotor->run(RELEASE);

  uint32_t currentFrequency;

  Serial.println("Hello!");

  ina219.begin();
  ina219.setCalibration_16V_400mA();

  WiFi.softAP("focuser", "");   // add Wi-Fi networks you want to connect to

  if (MDNS.begin("focuser")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/getFocuserPosition", getFocuserPosition);
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                            // Actually start the server
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();                     // Listen for HTTP requests from clients
  if ((millis() - lastCheck) > 1000) {
    if (stop) {
      Serial.println("Stopped");
    }
    lastCheck = millis();

    checkStall();

    if (!stop) {
      if (MotorDirection == CLOCKWISE) {
        if (MotorSpeed == slow) {
          focuserPosition = focuserPosition + (1000 * 0.34);
        } else {
          focuserPosition = focuserPosition + 1000;
        }
        myMotor->setSpeed(MotorSpeed);
        myMotor->run(BACKWARD);  //clockwise
      } else {
        if (MotorSpeed == slow) {
          focuserPosition = focuserPosition - (1000 * 0.34);
        } else {
          focuserPosition = focuserPosition - 1000;
        }
        myMotor->setSpeed(MotorSpeed);
        myMotor->run(FORWARD);  //clockwise
      }

      if (focuserPosition < 0) {
        Serial.println("Focuser at 0, stopping");
        focuserPosition = 0;
        stop = 1;
        myMotor->run(RELEASE);
      }

      //just get it close
      if ((location < focuserPosition + 2000 && location > focuserPosition - 2000 ) && (location != 0)) {
        location = 0;
        stop = 1;
      }

    } else {
      myMotor->run(RELEASE);
    }
    serveDocument();
  }
  delay(1);

}

void checkStall() {
  Serial.print("Current:");
  current_mA = ina219.getCurrent_mA();
  Serial.println(current_mA);

  //shuntvoltage = ina219.getShuntVoltage_mV();
  //busvoltage = ina219.getBusVoltage_V();
  //power_mW = ina219.getPower_mW();
  //loadvoltage = busvoltage + (shuntvoltage / 1000);
  //Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  //Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  //Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  //Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  //Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  if (current_mA > stallCurrent) {
    Serial.println("Stall");
    //back it off a bit
    if (MotorDirection == CLOCKWISE) {
      myMotor->setSpeed(slow);
      myMotor->run(FORWARD);  //clockwise
    } else {
      myMotor->setSpeed(slow);
      myMotor->run(BACKWARD);
    }
    delay(2000);
    myMotor->run(RELEASE);
    stop = 1;
    if (initFocus) {
      focuserPosition = 0;
      initFocus = 0;
    }
  }
}

void handleRoot() {                          // When URI / is requested, send a web page with a button to toggle the LED
  serveDocument();
}

void handleLogin() {
  if (server.arg("action") == "Init") {
    Serial.println("Init");
    stop = 0;
    MotorSpeed = fast;
    MotorDirection = COUNTERCLOCKWISE;
    focuserPosition = 1000000;
    initFocus = 1;
  }

  if (server.arg("action") == "start") {
    Serial.println("Start");
    stop = 0;

    if (server.arg("speed") == "slow") {
      Serial.println("Slow");
      MotorSpeed = slow;
    }

    if (server.arg("speed") == "fast") {
      Serial.println("Fast");
      MotorSpeed = fast;
    }
    if (server.arg("direction") == "CW") {
      MotorDirection = CLOCKWISE;
    }

    if (server.arg("direction") == "CCW") {
      MotorDirection = COUNTERCLOCKWISE;
    }
  }

  if (server.arg("action") == "stop") {
    Serial.println("Stop");
    stop = 1;
    initFocus = 0;
  }

  if (server.arg("action") == "goto") {
    Serial.println("Goto");
    location = server.arg("location").toInt();
    Serial.println(location);
    if (location > focuserPosition) {
      MotorDirection = CLOCKWISE;
      MotorSpeed = fast;
      stop = 0;
    } else {
      MotorDirection = COUNTERCLOCKWISE;
      MotorSpeed = fast;
      stop = 0;
    }
  }
  serveDocument();
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void getFocuserPosition() {
  String pos = String(focuserPosition);
  server.send(200, "text/plain", pos); //Send ADC value only to client ajax request
}

void serveDocument() {
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}
