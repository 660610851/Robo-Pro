#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN    21
#define SCL_PIN    22
#define SERVO_FREQ 50
#define SERVO_NUM  6

#define EN_PIN   25
#define IN1_PIN  26
#define IN2_PIN  27
#define PWM_FREQ 1000
#define PWM_RES  8

#define PULSE_MIN 100
#define PULSE_MAX 600

Adafruit_PWMServoDriver pwm(0x40);

uint16_t homePos[SERVO_NUM] = {
    355, 300, 600, 405, 520, 110
};

uint16_t currentPos[SERVO_NUM];
// ===== ความเร็วแต่ละ joint (steps) =====
// น้อย = เร็ว, มาก = ช้า
int jointSteps[SERVO_NUM] = {
    30,  // Servo 0 : Base
    40,  // Servo 1 : Shoulder
    40,  // Servo 2 : Elbow
    20,  // Servo 3 : Wrist
    20,   // Servo 4 : Gripper (เร็วสุด)
    20   // Servo 5 : Hole (เร็วสุด)
};

// ===== Non-blocking move =====
bool          moving       = false;
int           moveStep     = 0;
int           moveSteps    = 20;
int           moveDelayMs  = 10;
unsigned long lastMoveTime = 0;
uint16_t      moveStart[SERVO_NUM];
uint16_t      moveTarget[SERVO_NUM];
int           moveSingleIdx = -1;  // -1 = all

void startMove(int idx, uint16_t target) {
    moveSingleIdx    = idx;
    moveStart[idx]   = currentPos[idx];
    moveTarget[idx]  = constrain(target, PULSE_MIN, PULSE_MAX);
    moveStep         = 0;
    moveSteps        = jointSteps[idx];;
    moving           = true;
    lastMoveTime     = millis();
}

void startMoveAll(uint16_t target[]) {
    moveSingleIdx = -1;
    for (int i = 0; i < SERVO_NUM; i++) {
        moveStart[i]  = currentPos[i];
        moveTarget[i] = constrain(target[i], PULSE_MIN, PULSE_MAX);
    }
    moveStep     = 0;
    moveSteps    = 30;
    moving       = true;
    lastMoveTime = millis();
}

void updateMove() {
    if (!moving) return;
    if (millis() - lastMoveTime < (unsigned long)moveDelayMs) return;
    lastMoveTime = millis();

    moveStep++;

    if (moveSingleIdx >= 0) {
        int i = moveSingleIdx;
        uint16_t p = moveStart[i] + (int)(moveTarget[i] - moveStart[i]) * moveStep / moveSteps;
        pwm.setPWM(i, 0, constrain(p, PULSE_MIN, PULSE_MAX));
        if (moveStep >= moveSteps) {
            currentPos[i] = moveTarget[i];
            moving = false;
            Serial.printf("OK:SERVO:%d:%d\n", i, currentPos[i]);
        }
    } else {
        for (int i = 0; i < SERVO_NUM; i++) {
            uint16_t p = moveStart[i] + (int)(moveTarget[i] - moveStart[i]) * moveStep / moveSteps;
            pwm.setPWM(i, 0, constrain(p, PULSE_MIN, PULSE_MAX));
        }
        if (moveStep >= moveSteps) {
            for (int i = 0; i < SERVO_NUM; i++)
                currentPos[i] = moveTarget[i];
            moving = false;
            Serial.println("OK:ALL");
        }
    }
}

void sendStatus() {
    Serial.print("POS:");
    for (int i = 0; i < SERVO_NUM; i++) {
        Serial.print(currentPos[i]);
        if (i < SERVO_NUM - 1) Serial.print(",");
    }
    Serial.println();
}

void pumpOn(int spd) {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, constrain(spd, 0, 255));
    Serial.printf("OK:PUMP:ON:%d\n", spd);
}

void pumpOff() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, 0);
    Serial.println("OK:PUMP:OFF");
}

void handleCommand(String cmd) {
    cmd.trim();

    if (cmd == "HOME") {
        startMoveAll(homePos);
        Serial.println("OK:HOME");
    }
    else if (cmd == "STATUS") {
        sendStatus();
    }
    else if (cmd == "GRIP:OPEN") {
        startMove(4, 520);
    }
    else if (cmd == "GRIP:CLOSE") {
        startMove(4, 300);
    }
    else if (cmd == "PUMP:ON") {
        pumpOn(205);
    }
    else if (cmd == "PUMP:OFF") {
        pumpOff();
    }
    else if (cmd.startsWith("PUMP:")) {
        pumpOn(cmd.substring(5).toInt());
    }
    else if (cmd.startsWith("SERVO:")) {
        int f = cmd.indexOf(':');
        int s = cmd.indexOf(':', f + 1);
        int num      = cmd.substring(f + 1, s).toInt();
        uint16_t val = cmd.substring(s + 1).toInt();
        if (num >= 0 && num < SERVO_NUM)
            startMove(num, val);
        else
            Serial.println("ERR:INVALID_SERVO");
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
        if (idx == SERVO_NUM) startMoveAll(target);
        else Serial.println("ERR:INVALID_DATA");
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

    for (int i = 0; i < SERVO_NUM; i++) {
        currentPos[i] = homePos[i];
        pwm.setPWM(i, 0, homePos[i]);
        delay(200);
    }

    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    ledcAttach(EN_PIN, PWM_FREQ, PWM_RES);
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, 0);

    Serial.println("=== ESP32 ARM + PUMP READY ===");
}

void loop() {
    // รับคำสั่งตลอดเวลา ไม่ถูก block โดย delay
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }

    // อัปเดต movement แบบ non-blocking
    updateMove();
}
