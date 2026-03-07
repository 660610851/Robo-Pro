#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ===== PCA9685 =====
#define SDA_PIN    12
#define SCL_PIN    14
#define SERVO_FREQ 50
#define SERVO_NUM  6

// ===== L298N =====
#define EN_PIN  25
#define IN1_PIN 26
#define IN2_PIN 27
#define PWM_FREQ 1000
#define PWM_RES  8

Adafruit_PWMServoDriver pwm(0x40);

uint16_t homePos[SERVO_NUM] = {
  520, // Servo 0 : Base mid
  120, // Servo 1 : Shoulder mid
  600, // Servo 2 : Elbow
  405, // Servo 3 : Wrist
  520, // Servo 4 : gripper
  120// Servo 5 : hole
};

uint16_t currentPos[SERVO_NUM];

// ===== Servo Functions =====
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

// ===== L298N Functions =====
void pumpOn(int spd) {
    spd = constrain(spd, 0, 255);
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, spd);
    Serial.printf("OK:PUMP:ON:%d\n", spd);
}

void pumpOff() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, 0);
    Serial.println("OK:PUMP:OFF");
}

// ===== Command Handler =====
void handleCommand(String cmd) {
    cmd.trim();

    // ===== ARM =====
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
            if (comma == -1) { target[idx++] = data.toInt(); break; }
            target[idx++] = data.substring(0, comma).toInt();
            data = data.substring(comma + 1);
        }
        if (idx == SERVO_NUM) moveSmooth(target, 50, 20);
        else Serial.println("ERR:INVALID_DATA");
    }

    // ===== PUMP =====
    else if (cmd == "PUMP:ON") {
        pumpOn(255);
    }
    else if (cmd == "PUMP:OFF") {
        pumpOff();
    }
    else if (cmd.startsWith("PUMP:")) {
        int spd = cmd.substring(5).toInt();
        pumpOn(spd);
    }

    else {
        Serial.println("ERR:UNKNOWN_CMD");
    }
}

void setup() {
    Serial.begin(115200);

    // ===== Init PCA9685 =====
    Wire.begin(SDA_PIN, SCL_PIN);
    pwm.begin();
    pwm.setPWMFreq(SERVO_FREQ);
    delay(100);

    for (int i = 0; i < SERVO_NUM; i++) {
        currentPos[i] = homePos[i];
        pwm.setPWM(i, 0, homePos[i]);
        delay(200);
    }

    // ===== Init L298N =====
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    ledcAttach(EN_PIN, PWM_FREQ, PWM_RES);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, 0);

    Serial.println("=== ESP32 ARM + PUMP READY ===");
    Serial.println("ARM  : HOME | STATUS | SERVO:n:pulse | ALL:p0,p1,p2,p3,p4,p5");
    Serial.println("GRIP : GRIP:OPEN | GRIP:CLOSE");
    Serial.println("PUMP : PUMP:ON | PUMP:OFF | PUMP:150");
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }
}