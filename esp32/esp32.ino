#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "web.h"
#include "omni.h"

const char* WIFI_SSID = "largan_DIT_1";
const char* WIFI_PASS = "largan_DIT";

WebServer server(80);
WebSocketsServer ws(81);
IPAddress IP;


#define LED_PIN 2
#define PWM1_PIN 14
#define PWM2_PIN 15
#define motor1IN1 18
#define motor1IN2 19
#define motor2IN1 21
#define motor2IN2 22
#define motor3IN1 27
#define motor3IN2 13
#define motor1PWM 25
#define motor2PWM 23
#define motor3PWM 4

static int last_order;
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
String camIP = "192.168.4.20";
Omni3 robot(5.8, 12.49,                       // R, L
            motor2PWM, motor2IN1, motor2IN2,  // pwm1, dir1_1, dir1_2
            motor3PWM, motor3IN1, motor3IN2,  // pwm2, dir2_1, dir2_2
            motor1PWM, motor1IN1, motor1IN2,  // pwm3, dir3_1, dir3_2
            255);                             // pwmMax

void handleRoot() {
  String page = PAGE_HTML;
  page.replace("%CAM_IP%", camIP);
  server.send(200, "text/html", page);
}


void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {

  if (type != WStype_TEXT) return;
  String msg = (char*)payload;
  //if ((last_order - millis()) < 2000) {
  Serial.printf("%s \n",msg);
    if (msg == "BTNF") {
      Serial.printf("F");
      robot.drive(-150, 0, 0);
    } else if (msg == "BTNB") {
      Serial.printf("B");
      robot.drive(150, 0, 0);
    } else if (msg == "BTNR") {
      Serial.printf("R");
      robot.drive(1, -200, 0);
    } else if (msg == "BTNL") {
      Serial.printf("L");
      robot.drive(1, 200, 0);
    } else if (msg == "S") {
      Serial.printf("Stop");
      robot.drive(0, 0, 0);
    } else if (msg == "BTNC") {
      robot.drive(0, 0, 10);
    } else if (msg == "BTNCC") {
      robot.drive(0, 0, -10);
    } else if (msg == "LED_ON"){
      digitalWrite(15,1);
    } else if (msg == "LED_OFF"){
      digitalWrite(15,0);
    }
    last_order = millis();
  // } else {
  //   robot.drive(0, 0, 0);
  // }
}

void connectWiFi() {
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  IP = WiFi.softAPIP();
  Serial.printf("AP activate!  IP = %s\n", IP.toString().c_str());
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  server.on("/", handleRoot);
  server.begin();
  ws.begin();
  ws.onEvent(wsEvent);
  pinMode(15, OUTPUT);
  digitalWrite(15,0);
}

void loop() {
  server.handleClient();
  ws.loop();
  delay(10);
}
