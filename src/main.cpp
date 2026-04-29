#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "111";
const char* password = "00000000";

WebServer server(80);

const int servoPin1 = 22;
const int servoPin2 = 23;

const int ch1 = 0;
const int ch2 = 1;

const int freq = 50;
const int resolution = 16;

const int CH1_START_ANGLE = 90;
const float DEG_PER_MM = 20.0;
const int STEP_COUNT = 6;

int dotAngle[6];

void servoWrite(int channel, int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  int us = map(angle, 0, 180, 500, 2400);
  int duty = (int)((us / 20000.0) * 65535);

  ledcWrite(channel, duty);
}

// 你的动作方式：ch2 初始化在50，每次打孔 50 -> 0 -> 50
void punchOnce() {
  servoWrite(ch2, 50);
  delay(800);

  servoWrite(ch2, 0);
  delay(300);

  servoWrite(ch2, 50);
  delay(800);
}

void punchDot(int dotNumber) {
  if (dotNumber < 1 || dotNumber > 6) return;

  int index = dotNumber - 1;

  servoWrite(ch1, dotAngle[index]);
  delay(500);

  punchOnce();
}

void printOneCell(String cell) {
  cell.trim();

  for (int i = 0; i < cell.length(); i++) {
    char c = cell.charAt(i);
    if (c >= '1' && c <= '6') {
      punchDot(c - '0');
    }
  }
}

void printBrailleCode(String code) {
  code.trim();

  int start = 0;

  while (start < code.length()) {
    int spaceIndex = code.indexOf(' ', start);
    String cell;

    if (spaceIndex == -1) {
      cell = code.substring(start);
      start = code.length();
    } else {
      cell = code.substring(start, spaceIndex);
      start = spaceIndex + 1;
    }

    if (cell.length() > 0) {
      printOneCell(cell);
      delay(1200);
    }
  }
}

void handlePrint() {
  if (!server.hasArg("code")) {
    server.send(400, "text/plain; charset=utf-8", "缺少 code 参数");
    return;
  }

  String code = server.arg("code");

  server.send(200, "text/plain; charset=utf-8", "ESP32收到点位：" + code);

  printBrailleCode(code);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ledcSetup(ch1, freq, resolution);
  ledcAttachPin(servoPin1, ch1);

  ledcSetup(ch2, freq, resolution);
  ledcAttachPin(servoPin2, ch2);

  for (int i = 0; i < STEP_COUNT; i++) {
    dotAngle[i] = CH1_START_ANGLE + i * DEG_PER_MM;
    if (dotAngle[i] > 180) dotAngle[i] = 180;

    Serial.print("Dot ");
    Serial.print(i + 1);
    Serial.print(" angle = ");
    Serial.println(dotAngle[i]);
  }

  servoWrite(ch1, CH1_START_ANGLE);
  delay(1000);

  // 你要求初始化必须是50
  servoWrite(ch2, 50);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.println("正在连接WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("ESP32 IP地址：http://");
  Serial.println(WiFi.localIP());

  server.on("/print", handlePrint);
  server.begin();

  Serial.println("打印服务器已启动");
}

void loop() {
  server.handleClient();
}