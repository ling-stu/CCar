// ====== 相機與 Wi-Fi 函式庫 ======
// 功能：啟用 ESP32-CAM 與 Wi-Fi 連線
// 可能問題：編譯失敗 → 確認已選對 ESP32-CAM 開發板與安裝相機庫
#include "esp_camera.h"
#include <WiFi.h>


// ====== Wi-Fi 連線設定 ======
// 功能：連上主控板 AP
// 可能問題：連線逾時 → 確認 SSID/密碼與 AP 是否開啟
const char* WIFI_SSID = "CAR_1";
const char* WIFI_PASS = "12345678";

// ====== 固定 IP ======
// 功能：固定相機 IP 方便主控頁面串流
// 可能問題：主控頁面無法開啟影像 → 確認此 IP 與主控頁面一致
//static ip
IPAddress local_IP(192, 168, 4, 20);
IPAddress gateway (192, 168, 4,  1);
IPAddress subnet  (255, 255, 255, 0);

// ====== 相機腳位 (AI-Thinker ESP32-CAM) ======
// 功能：定義相機模組硬體腳位
// 可能問題：畫面黑/初始化失敗 → 確認模組型號與腳位對應
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ====== HTTP 伺服器 ======
// 功能：提供 /stream 串流
WiFiServer server(80);

// ====== 初始化相機 ======
// 功能：設定解析度、畫質、緩衝區並初始化感測器
// 可能問題：啟動失敗 → 檢查 PSRAM 是否正常、解析度是否過高
bool initCamera()
{
  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0  = Y2_GPIO_NUM;
  cfg.pin_d1  = Y3_GPIO_NUM;
  cfg.pin_d2  = Y4_GPIO_NUM;
  cfg.pin_d3  = Y5_GPIO_NUM;
  cfg.pin_d4  = Y6_GPIO_NUM;
  cfg.pin_d5  = Y7_GPIO_NUM;
  cfg.pin_d6  = Y8_GPIO_NUM;
  cfg.pin_d7  = Y9_GPIO_NUM;
  cfg.pin_xclk= XCLK_GPIO_NUM;
  cfg.pin_pclk= PCLK_GPIO_NUM;
  cfg.pin_vsync=VSYNC_GPIO_NUM;
  cfg.pin_href= HREF_GPIO_NUM;
  cfg.pin_sscb_sda = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn     = PWDN_GPIO_NUM;
  cfg.pin_reset    = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size   = FRAMESIZE_VGA;   // QVGA|VGA|SVGA|XGA|SXGA|UXGA
  cfg.jpeg_quality = 30;              // 0~63, 越小畫質越好
  cfg.fb_count     = 3;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed (0x%X)\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  //s->set_vflip(s, 1);
  s->set_special_effect(s, 0); // 0=No Effect
  s->set_whitebal(s, 1);       // 開白平衡
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);        // 0=Auto
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_sharpness(s, 0);
  s->set_exposure_ctrl(s, 1);  // AE on
  s->set_gain_ctrl(s, 1);      // AG on
  s->set_aec2(s, 1);           // AEC2（很多模組開了更穩）
  s->set_ae_level(s, 0);       // -2..2，0較中性（想更亮可改 1，但可能更慢）
  s->set_agc_gain(s, 0);       // 0=auto（部分版本這行是 set_agc_gain / set_gainceiling 才有效)
  // 限制增益上限：避免暗處把增益拉爆導致噪點與壓縮負擔（也間接減少卡頓）
  s->set_gainceiling(s, (gainceiling_t)4); 
  // 常見枚舉約略：0=2x,1=4x,2=8x,3=16x,4=32x,5=64x,6=128x
  // 這裡設 32x（偏穩），想更亮可改 5(64x)，但更噪、更吃壓縮
  return true;
}

// ====== 連線至主控 AP ======
// 功能：設定固定 IP 並加入 AP
// 可能問題：IP 衝突 → 改成不同的固定 IP
void connectWiFi()
{
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Wi-Fi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
  }
  Serial.printf("\n\nIP = %s\n", WiFi.localIP().toString().c_str());
}


// ====== HTTP 回應標頭 ======
// 功能：允許跨網域並禁用快取
const char HDR_OK[] PROGMEM = 
  "HTTP/1.1 200 OK\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "Cache-Control: no-cache\r\n";


// ====== MJPEG 串流處理 ======
// 功能：持續送出 JPEG frame 給瀏覽器
// 可能問題：卡頓/掉幀 → 降低解析度或提高 `jpeg_quality`
void handleStream(WiFiClient& client)
{
  client.print(FPSTR(HDR_OK));
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n");

  while (client.connected())
  {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    if (!client.connected()) break;
    delay(1);
  }
}

// ====== 錯誤指示燈 ======
// 功能：相機初始化失敗時閃燈提示
void sparkle(){
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  delay(100);
}


// ====== 初始化 ======
// 功能：序列埠、相機、Wi-Fi、伺服器
// 可能問題：不出畫面 → 先看序列埠是否顯示 IP
void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  pinMode(4, OUTPUT);
  if (!initCamera())
  {
    sparkle();
    Serial.println("Camera failure, halting.");
    while (true) delay(1000);
  }
  connectWiFi();
  server.begin();
}

// ====== 主迴圈 ======
// 功能：監聽 HTTP 連線並提供串流
// 可能問題：連線後立刻中斷 → 檢查供電是否足夠
void loop()
{
  if (WiFi.status() != WL_CONNECTED) { // wifi斷線重連
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectWiFi();  
    return;   
  }

  WiFiClient client = server.available(); 
  if (!client) {
    delay(1);
    return;
  }

  String req = client.readStringUntil('\r');
  client.readStringUntil('\n');
  Serial.printf("Req: %s\n", req.c_str());

  if (req.startsWith("GET /stream"))
    handleStream(client);
  else
    client.println("HTTP/1.1 404 Not Found\r\n\r\n");

  client.stop();
}
