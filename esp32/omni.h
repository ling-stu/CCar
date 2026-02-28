/*  Omni3.h  ------------------------------------------------------------ */
// ====== 全向輪控制類別宣告 ======
// 功能：提供三輪全向底盤的運動學介面
// 可能問題：方向/速度不符合 → 調整輪子編號對應或檢查半徑/幾何參數
#pragma once
#include <Arduino.h>
class Omni3 {
public:
  Omni3(float R, float L,
            uint8_t pwm1, uint8_t dir1_1, uint8_t dir1_2,
            uint8_t pwm2, uint8_t dir2_1, uint8_t dir2_2,
            uint8_t pwm3, uint8_t dir3_1, uint8_t dir3_2,
            uint8_t pwmMax);

  /* ----- 高階控制 ----- */
  // 功能：以底盤座標速度控制
  // 可能問題：速度太慢/太快 → 調整 pwmMax 或輸入速度縮放
  void drive(float Vx, float Vy, float W);   // m/s, m/s, rad/s
  void forward(float spd)  { drive( spd, 0, 0); }
  void backward(float spd) { drive(-spd, 0, 0); }
  void right(float spd)    { drive( 0, -spd, 0); }
  void left(float spd)     { drive( 0,  spd, 0); }
  void rotate(float w)     { drive( 0, 0, w);    }
  void stop()              { setWheelPWM(0,0,0); }

private:
  /* ----- 內部 ----- */
  // 功能：運動學計算與 PWM 輸出
  // 可能問題：馬達抖動 → 檢查 PWM 頻率與馬達供電穩定度
  void inverseKinematics(float Vx, float Vy, float W, float w[3]);
  void setWheelPWM(float w1, float w2, float w3);

  float _R, __L;
  uint8_t _pwm[3];
  uint8_t _dir[3][2];
  uint8_t _pwmMax;
};
