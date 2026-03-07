#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVO_FREQ 50
#define SERVO_NUM 6

Adafruit_PWMServoDriver pwm(0x40);

// ===== HOME POSITION (pulse 150–600) =====
// ปรับตามโครงสร้างแขนกลของคุณ
uint16_t homePos[SERVO_NUM] = {
  355, // Servo 0 : Base mid
  200, // Servo 1 : Shoulder mid
  600, // Servo 2 : Elbow
  405, // Servo 3 : Wrist
  520, // Servo 4 : gripper
  110// Servo 5 : hole
  
};

void moveToHome() {
  for (int i = 0; i < SERVO_NUM; i++) {
    pwm.setPWM(i, 0, homePos[i]);
    delay(300); // ลดแรงกระชาก
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(20);

  Serial.println("Move to HOME position");
  moveToHome();
  Serial.println("HOME done");
}

void loop() {
  // ว่างไว้
}