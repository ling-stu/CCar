// ====== 基本函式庫與專案模組 ======
// 功能：Wi-Fi AP、HTTP 伺服器、WebSocket、網頁內容、全向輪控制、伺服機控制
// 常見問題：編譯失敗 → 確認已安裝 ESP32 與 ESP32Servo 套件
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "web.h"
#include "omni.h"
#include <ESP32Servo.h>

// ====== AP Wi-Fi 設定 ======
// 功能：設定主控板作為 AP 熱點
// 可能問題：手機搜不到 SSID → 檢查是否已上電與 AP 模式是否啟動
const char* WIFI_SSID = "CAR_1";
const char* WIFI_PASS = "12345678";

// ====== 伺服器與通訊 ======
// 功能：HTTP(80) 提供控制頁，WebSocket(81) 接收控制指令
// 可能問題：按鈕無反應 → 確認前端連到 81 埠、無防火牆阻擋
WebServer server(80);
WebSocketsServer ws(81);
IPAddress IP;

// ====== 相機伺服機 ======
// 功能：控制鏡頭左右轉
// 可能問題：伺服不動 → 檢查 5V 供電、GND 共地、PWM 腳位 26 是否正確
Servo camservo;
const int servoPin = 26;   // servo PWM 腳位
unsigned long lastMove = 0; // 上次移動時間
int deg = 90;               // 伺服角度初值

// ====== 馬達與 PWM 腳位 ======
// 功能：三顆全向輪驅動腳位定義
// 可能問題：方向相反 → 調整正負號或交換 IN1/IN2 腳位
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

// ====== 網路參數與全向輪物件 ======
// 功能：AP 固定 IP、攝影機 IP、全向輪參數設定
// 可能問題：看不到影像 → 確認 `camIP` 與攝影機實際 IP 相同
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

// ====== HTTP 首頁 ======
// 功能：回傳控制頁 HTML，並把 CAM IP 塞入頁面
// 可能問題：頁面空白 → 確認 `web.h` 的 PAGE_HTML 是否正確
void handleRoot() {
  String page = PAGE_HTML;
  page.replace("%CAM_IP%", camIP);
  server.send(200, "text/html", page);
}


// ====== WebSocket 事件處理 ======
// 功能：接收前端按鈕指令並控制車體/鏡頭
// 可能問題：控制無反應 → 檢查前端送出的字串是否與此處一致
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
    } else if (msg == "BTNCamR"){
      spin_cam(1);
    } else if (msg == "BTNCamL"){
      spin_cam(-1);
    }
    last_order = millis();
}

// ====== 啟動 AP ======
// 功能：啟動 ESP32 AP 並設定固定 IP
// 可能問題：手機連不上 → 確認密碼至少 8 碼、網路名稱無特殊字元
void connectWiFi() {
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  IP = WiFi.softAPIP();
  Serial.printf("AP activate!  IP = %s\n", IP.toString().c_str());
}

// ====== 伺服鏡頭轉動 ======
// 功能：依方向逐步改變伺服角度
// 可能問題：角度卡死或抖動 → 增加更新間隔或限制角度範圍
void spin_cam(int dir) {
  if (millis() - lastMove > 20) {
    lastMove = millis();
    camservo.write(deg);
    deg+=dir;
    if (deg >= 150) deg = 0;
    if (deg <= 30) deg = 0;
  }
}
// ====== 初始化 ======
// 功能：序列埠、AP、HTTP/WS、伺服機初始化
// 可能問題：伺服無法動作 → 檢查 attach 腳位與脈寬設定
void setup() {
  Serial.begin(115200);
  connectWiFi();
  server.on("/", handleRoot);
  server.begin();
  ws.begin();
  ws.onEvent(wsEvent);
  camservo.setPeriodHertz(50);           // 50Hz
  camservo.attach(servoPin, 500, 2500);  // min/max 脈寬(微秒)，可微調
}

// ====== 主迴圈 ======
// 功能：維持 HTTP 與 WebSocket 服務
// 可能問題：控制延遲 → 減少 loop 內 delay 或降低送包頻率
void loop() {
  server.handleClient();
  ws.loop();
  delay(10);
}
