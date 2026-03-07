#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN 22
#define SCL_PIN 23
#define SERVO_FREQ 50
#define SERVO_NUM  6

Adafruit_PWMServoDriver pwm(0x40);

uint16_t homePos[SERVO_NUM] = {
  360, // Servo 0 : Basec
  255, // Servo 1 : Shoulder
  510, // Servo 2 : Elbow
  400, // Servo 3 : Wrist
  450, // Servo 4 : Gripper
  405  // Servo 5 : Hole
};

uint16_t currentPos[SERVO_NUM];

// ===== Smooth Movement =====
void moveSmooth(uint16_t target[], int steps, int delayMs) {
  for (int s = 1; s <= steps; s++) {
    for (int i = 0; i < SERVO_NUM; i++) {
      uint16_t p = currentPos[i] +
                   (int)(target[i] - currentPos[i]) * s / steps;
      p = constrain(p, 150, 600);
      pwm.setPWM(i, 0, p);
    }
    delay(delayMs);
  }
  for (int i = 0; i < SERVO_NUM; i++) {
    currentPos[i] = constrain(target[i], 150, 600);
  }
  Serial.println("OK:ALL");
}

// ===== ฟังก์ชัน =====
void moveToHome() {
  Serial.println("Moving to HOME...");
  moveSmooth(homePos, 30, 20);
  Serial.println("OK:HOME");
}

void moveServo(int num, uint16_t pulse) {
  if (num < 0 || num >= SERVO_NUM) {
    Serial.println("ERR:INVALID_SERVO");
    return;
  }
  pulse = constrain(pulse, 150, 600);

  // smooth สำหรับ servo ตัวเดียว
  int start = currentPos[num];
  int steps = 20;
  for (int s = 1; s <= steps; s++) {
    uint16_t p = start + (int)(pulse - start) * s / steps;
    pwm.setPWM(num, 0, p);
    delay(10);
  }
  currentPos[num] = pulse;
  Serial.printf("OK:SERVO:%d:%d\n", num, pulse);
}

void sendStatus() {
  Serial.print("POS:");
  for (int i = 0; i < SERVO_NUM; i++) {
    Serial.print(currentPos[i]);
    if (i < SERVO_NUM - 1) Serial.print(",");
  }
  Serial.println();
}

void handleCommand(String cmd) {
  cmd.trim();

  if (cmd == "HOME") {
    moveToHome();
  }
  else if (cmd == "STATUS") {
    sendStatus();
  }
  else if (cmd == "GRIP:OPEN") {
    moveServo(4, 520);
  }
  else if (cmd == "GRIP:CLOSE") {
    moveServo(4, 300);
  }
  else if (cmd.startsWith("SERVO:")) {
    int f = cmd.indexOf(':');
    int s = cmd.indexOf(':', f + 1);
    int num      = cmd.substring(f + 1, s).toInt();
    uint16_t val = cmd.substring(s + 1).toInt();
    moveServo(num, val);
  }
  else if (cmd.startsWith("ALL:")) {
    uint16_t target[SERVO_NUM];
    String data = cmd.substring(4);
    int idx = 0;

    while (data.length() > 0 && idx < SERVO_NUM) {
      int comma = data.indexOf(',');
      if (comma == -1) {
        target[idx++] = data.toInt();
        break;
      }
      target[idx++] = data.substring(0, comma).toInt();
      data = data.substring(comma + 1);
    }

    if (idx == SERVO_NUM) {
      moveSmooth(target, 50, 20);
      // ปรับความเร็วตรงนี้
      // moveSmooth(target, 30, 20);  // ช้าลง
      // moveSmooth(target, 10, 10);  // เร็วขึ้น
    } else {
      Serial.println("ERR:INVALID_DATA");
    }
  }
  else {
    Serial.println("ERR:UNKNOWN_CMD");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(100);

  // เซ็ตตำแหน่งเริ่มต้น
  for (int i = 0; i < SERVO_NUM; i++) {
    currentPos[i] = homePos[i];
    pwm.setPWM(i, 0, homePos[i]);
    delay(200);
  }

  Serial.println("=== ESP32 ARM READY ===");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    handleCommand(cmd);
  }
}