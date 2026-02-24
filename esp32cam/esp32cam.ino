#include "esp_camera.h"
#include <WiFi.h>


const char* WIFI_SSID = "largan_DIT_7";
const char* WIFI_PASS = "largan_DIT";

//static ip
IPAddress local_IP(192, 168, 4, 20);
IPAddress gateway (192, 168, 4,  1);
IPAddress subnet  (255, 255, 255, 0);

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

WiFiServer server(80);

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
  cfg.frame_size   = FRAMESIZE_QVGA;   // QVGA|VGA|SVGA|XGA|SXGA|UXGA
  cfg.jpeg_quality = 10;              // 0~63, 越小畫質越好
  cfg.fb_count     = 2;

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed (0x%X)\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  //s->set_vflip(s, 1);
  s->set_awb_gain(s,1);
  s->set_whitebal(s,1);
  s->set_exposure_ctrl(s,1);
  s->set_gain_ctrl(s,1);
  s->set_bpc(s,1);
  return true;
}

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


const char HDR_OK[] PROGMEM = 
  "HTTP/1.1 200 OK\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "Cache-Control: no-cache\r\n";


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

void sparkle(){
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  delay(100);
}


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
