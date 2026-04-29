#include <Arduino.h>

/* ========= TB6612 X轴 ========= */
#define X_AIN1 25
#define X_AIN2 26
#define X_PWM  27

/* ========= 霍尔编码器 ========= */
#define X_HALL_A 34
#define X_HALL_B 35

/* ========= PWM ========= */
#define PWM_CH_X 0
#define PWM_FREQ 20000
#define PWM_RES  8

/* ========= 机械标定 ========= */
// 重点：这个值需要你实测修改
// 例如：实际走10mm，编码器变化3000，则 COUNT_PER_MM = 300.0
#define COUNT_PER_MM 375.0

/* ========= 控制参数 ========= */
#define PWM_MIN 70
#define PWM_MAX 180

#define STOP_ERR_MM 0.2
#define SLOW_ERR_MM 3.0

volatile long xCount = 0;

long xTargetCount = 0;
bool xMoving = false;

/* ========= 霍尔中断 ========= */
void IRAM_ATTR xHallISR() {
  int b = digitalRead(X_HALL_B);

  if (b == HIGH) {
    xCount--;
  } else {
    xCount++;
  }
}

/* ========= 单位转换 ========= */
long mmToCount(float mm) {
  return (long)(mm * COUNT_PER_MM);
}

float countToMM(long count) {
  return count / COUNT_PER_MM;
}

/* ========= 电机控制 ========= */
void xMotorStop() {
  ledcWrite(PWM_CH_X, 0);
  digitalWrite(X_AIN1, LOW);
  digitalWrite(X_AIN2, LOW);
}

void xMotorRun(int pwm) {
  if (pwm > 0) {
    digitalWrite(X_AIN1, HIGH);
    digitalWrite(X_AIN2, LOW);
    ledcWrite(PWM_CH_X, pwm);
  } else if (pwm < 0) {
    digitalWrite(X_AIN1, LOW);
    digitalWrite(X_AIN2, HIGH);
    ledcWrite(PWM_CH_X, -pwm);
  } else {
    xMotorStop();
  }
}

void moveXToMM(float targetMM) {
  xTargetCount = mmToCount(targetMM);
  xMoving = true;

  Serial.print("目标 X = ");
  Serial.print(targetMM);
  Serial.print(" mm, 目标脉冲 = ");
  Serial.println(xTargetCount);
}

void updateXMotor() {
  if (!xMoving) return;

  long nowCount = xCount;
  long errCount = xTargetCount - nowCount;

  float errMM = errCount / COUNT_PER_MM;
  float absErrMM = fabs(errMM);

  if (absErrMM <= STOP_ERR_MM) {
    xMotorStop();
    xMoving = false;

    Serial.print("X 到位，当前位置 = ");
    Serial.print(countToMM(xCount));
    Serial.print(" mm, count = ");
    Serial.println(xCount);
    return;
  }

  int pwm;

  if (absErrMM > SLOW_ERR_MM) {
    pwm = PWM_MAX;
  } else {
    pwm = map((long)(absErrMM * 100), (long)(STOP_ERR_MM * 100), (long)(SLOW_ERR_MM * 100), PWM_MIN, PWM_MAX);
  }

  if (pwm < PWM_MIN) pwm = PWM_MIN;
  if (pwm > PWM_MAX) pwm = PWM_MAX;

  if (errCount > 0) {
    xMotorRun(pwm);
  } else {
    xMotorRun(-pwm);
  }
}

/* ========= 串口命令 ========= */
void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("mm")) {
    float targetMM = cmd.substring(2).toFloat();
    moveXToMM(targetMM);
  }
  else if (cmd == "zero") {
    noInterrupts();
    xCount = 0;
    interrupts();

    xTargetCount = 0;
    xMoving = false;
    xMotorStop();

    Serial.println("当前位置已设为 X=0mm");
  }
  else if (cmd == "stop") {
    xMotorStop();
    xMoving = false;

    Serial.println("X 已停止");
  }
  else if (cmd == "p") {
    Serial.print("当前位置 X = ");
    Serial.print(countToMM(xCount));
    Serial.print(" mm, count = ");
    Serial.println(xCount);
  }
  else {
    Serial.println("未知命令");
    Serial.println("可用命令：mm10 / mm0 / mm-5 / zero / p / stop");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(X_AIN1, OUTPUT);
  pinMode(X_AIN2, OUTPUT);

  pinMode(X_HALL_A, INPUT);
  pinMode(X_HALL_B, INPUT);

  ledcSetup(PWM_CH_X, PWM_FREQ, PWM_RES);
  ledcAttachPin(X_PWM, PWM_CH_X);

  attachInterrupt(digitalPinToInterrupt(X_HALL_A), xHallISR, RISING);

  xMotorStop();

  Serial.println("X轴毫米闭环控制启动");
  Serial.println("命令：");
  Serial.println("mm10  -> 移动到 X=10mm");
  Serial.println("mm0   -> 回到 X=0mm");
  Serial.println("mm-5  -> 移动到 X=-5mm");
  Serial.println("zero  -> 当前设为0点");
  Serial.println("p     -> 打印当前位置");
  Serial.println("stop  -> 停止");
}

void loop() {
  handleSerial();
  updateXMotor();

  delay(5);
}