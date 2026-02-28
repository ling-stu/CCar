#include "omni.h"

// ====== 建構式 ======
// 功能：設定輪子參數、腳位與 PWM 上限
// 可能問題：馬達不轉 → 確認 PWM/方向腳位接線與 pinMode

Omni3::Omni3(float R, float L,
             uint8_t pwm1, uint8_t dir1_1, uint8_t dir1_2,
             uint8_t pwm2, uint8_t dir2_1, uint8_t dir2_2,
             uint8_t pwm3, uint8_t dir3_1, uint8_t dir3_2,
             uint8_t pwmMax) {
  _R = R;
  __L = L;
  _pwm[0] = pwm1;
  _pwm[1] = pwm2;
  _pwm[2] = pwm3;
  _dir[0][0] = dir1_1;
  _dir[0][1] = dir1_2;
  _dir[1][0] = dir2_1;
  _dir[1][1] = dir2_2;
  _dir[2][0] = dir3_1;
  _dir[2][1] = dir3_2;
  _pwmMax = pwmMax;

  for (int i = 0; i < 3; i++) {
    pinMode(_pwm[i], OUTPUT);
    for (int k = 0; k < 2; k++) {
      pinMode(_dir[i][k], OUTPUT);
    }
  }
}

// ====== 高階速度控制 ======
// 功能：把 Vx/Vy/W 轉成各輪速度並輸出
// 可能問題：移動方向怪異 → 檢查輪子編號與安裝方向
void Omni3::drive(float Vx, float Vy, float W) {
  float w[3];
  inverseKinematics(Vx, Vy, W, w);
  setWheelPWM(w[0], w[1], w[2]);
}

// ====== 逆運動學 ======
// 功能：底盤速度 → 三輪角速度
// 可能問題：速度比例不對 → 檢查 R(輪半徑)、L(幾何參數)
void Omni3::inverseKinematics(float Vx, float Vy, float W, float w[3]) {
  const float L = __L;
  const float R = _R;

  w[0] = (Vy + L * W) / R; 
  w[1] = (-0.866f * Vx - 0.5f * Vy + L * W) / R;
  w[2] = (0.866f * Vx - 0.5f * Vy + L * W) / R;
}

// ====== 輪速 PWM 輸出 ======
// 功能：正負號決定方向，絕對值決定 PWM
// 可能問題：震動或忽快忽慢 → 檢查供電與 PWM 上限
void Omni3::setWheelPWM(float w1, float w2, float w3) {
  float w[3] = {w1,w2,w3};
  float peak = max(max(abs(w1),abs(w2)),abs(w3));
  if(peak > 1.0f){
    w[0] /= peak; w[1] /= peak; w[2] /= peak;
  }

  /* 2) 輸出 PWM */
  for(int i=0;i<3;i++){
    bool dir = (w[i] >= 0);
    analogWrite(_pwm[i], uint8_t(abs(w[i])*_pwmMax));
    digitalWrite(_dir[i][0], dir);
    digitalWrite(_dir[i][1], !dir);
  }
  Serial.printf("%.3f %.3f %.3f\n", w[0], w[1], w[2]);
}
