/*  Omni3.h  ------------------------------------------------------------ */
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
  void drive(float Vx, float Vy, float W);   // m/s, m/s, rad/s
  void forward(float spd)  { drive( spd, 0, 0); }
  void backward(float spd) { drive(-spd, 0, 0); }
  void right(float spd)    { drive( 0, -spd, 0); }
  void left(float spd)     { drive( 0,  spd, 0); }
  void rotate(float w)     { drive( 0, 0, w);    }
  void stop()              { setWheelPWM(0,0,0); }

private:
  /* ----- 內部 ----- */
  void inverseKinematics(float Vx, float Vy, float W, float w[3]);
  void setWheelPWM(float w1, float w2, float w3);

  float _R, __L;
  uint8_t _pwm[3];
  uint8_t _dir[3][2];
  uint8_t _pwmMax;
};
