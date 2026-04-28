#include <Arduino.h>

const int servoPin1 = 22;
const int servoPin2 = 23;

const int ch1 = 0;
const int ch2 = 1;

const int freq = 50;
const int resolution = 16;

const int CH1_START_ANGLE = 90;   // ch1 初始位置
const float DEG_PER_MM = 20.0;     // 1mm 对应多少度，需要你实测调整
const int STEP_COUNT = 6;         // 上升 6 次

int ch1Angle = CH1_START_ANGLE;

void servoWrite(int channel, int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  int us = map(angle, 0, 180, 500, 2400);
  int duty = (int)((us / 20000.0) * 65535);

  ledcWrite(channel, duty);
}

// ch2 戳一下，保持你原来的内容
void punchOnce() {
  servoWrite(ch2, 50);
  delay(800);

  servoWrite(ch2, 10);
  delay(800);
}

// ch1 上升 1mm
void ch1MoveUp1mm() {
  int target = CH1_START_ANGLE + (int)(DEG_PER_MM * (ch1Angle - CH1_START_ANGLE) / DEG_PER_MM + DEG_PER_MM);

  ch1Angle += DEG_PER_MM;

  if (ch1Angle > 180) ch1Angle = 180;

  servoWrite(ch1, ch1Angle);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ledcSetup(ch1, freq, resolution);
  ledcAttachPin(servoPin1, ch1);

  ledcSetup(ch2, freq, resolution);
  ledcAttachPin(servoPin2, ch2);

  servoWrite(ch1, CH1_START_ANGLE);
  delay(1000);

  servoWrite(ch2, 0);
  delay(500);

  for (int i = 0; i < STEP_COUNT; i++) {
    servoWrite(ch2, 50);
    delay(800);

    servoWrite(ch2, 0);
    delay(300);

    servoWrite(ch2, 50);
    delay(800);
    
    ch1Angle = CH1_START_ANGLE + (i + 1) * DEG_PER_MM;
    if (ch1Angle > 180) ch1Angle = 180;

    servoWrite(ch1, ch1Angle);
    delay(500);
  }
}

void loop() {
  // 不循环，避免一直重复打
}