#include <Arduino.h>

const int servoPin1 = 22;
const int servoPin2 = 23;

const int ch1 = 0;
const int ch2 = 1;

const int freq = 50;
const int resolution = 16;

const int CH1_START_ANGLE = 90;
const float DEG_PER_MM = 20.0;
const int STEP_COUNT = 6;

int ch1Angle = CH1_START_ANGLE;

// 6个触点对应的ch1角度
int dotAngle[6];

void servoWrite(int channel, int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  int us = map(angle, 0, 180, 500, 2400);
  int duty = (int)((us / 20000.0) * 65535);

  ledcWrite(channel, duty);
}

// 保持你原来的打孔动作
void punchOnce() {
  servoWrite(ch2, 50);
  delay(800);

  servoWrite(ch2, 0);
  delay(300);

  servoWrite(ch2, 50);
  delay(800);
}

// 打某一个点，dotIndex = 0~5，对应盲文点1~点6
void punchDot(int dotIndex) {
  ch1Angle = dotAngle[dotIndex];

  servoWrite(ch1, ch1Angle);
  delay(500);

  punchOnce();
}

// bit0~bit5 对应 点1~点6
void printBrailleCell(byte pattern) {
  for (int i = 0; i < 6; i++) {
    if (pattern & (1 << i)) {
      punchDot(i);
    }
  }
}

// 示例中文盲文表：先用于测试
// 后面可以继续往这里加汉字
byte getChineseBraille(String word) {
  if (word == "你") return 0b000101; // 点1 点3
  if (word == "好") return 0b011001; // 点1 点4 点5
  if (word == "中") return 0b001101; // 点1 点3 点4
  if (word == "我") return 0b010111; // 点1 点2 点3 点5
  if (word == "是") return 0b001110; // 点2 点3 点4

  return 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ledcSetup(ch1, freq, resolution);
  ledcAttachPin(servoPin1, ch1);

  ledcSetup(ch2, freq, resolution);
  ledcAttachPin(servoPin2, ch2);

  // 记录6个触点位置
  for (int i = 0; i < STEP_COUNT; i++) {
    dotAngle[i] = CH1_START_ANGLE + i * DEG_PER_MM;
    if (dotAngle[i] > 180) dotAngle[i] = 180;
  }

  servoWrite(ch1, CH1_START_ANGLE);
  delay(1000);

  servoWrite(ch2, 50);
  delay(500);

  Serial.println("请输入一个汉字：你 / 好 / 中 / 我 / 是");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    byte pattern = getChineseBraille(input);

    if (pattern == 0) {
      Serial.println("这个汉字还没有加入表");
      return;
    }

    Serial.print("开始打印：");
    Serial.println(input);

    printBrailleCell(pattern);

    Serial.println("打印完成");
  }
}