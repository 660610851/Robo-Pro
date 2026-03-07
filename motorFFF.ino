#include <Arduino.h>

// ===== Motor Left =====
#define L_M1A 32
#define L_M1B 33
#define L_M2A 25
#define L_M2B 26

// ===== Motor Right =====
#define R_M1A 15
#define R_M1B 19
#define R_M2A 21
#define R_M2B 4

// ===== Encoder =====
#define ENC_LB_A 35
#define ENC_LB_B 13
#define ENC_LF_A 14
#define ENC_LF_B 27
#define ENC_RB_A 16
#define ENC_RB_B 17
#define ENC_RF_A 5
#define ENC_RF_B 18

#define PWM_FREQ 1000
#define PWM_RES  8

volatile long enc_LB = 0, enc_LF = 0, enc_RB = 0, enc_RF = 0;

void IRAM_ATTR isr_LB() { enc_LB += (digitalRead(ENC_LB_A) == digitalRead(ENC_LB_B)) ? -1 : 1; }
void IRAM_ATTR isr_LF() { enc_LF += (digitalRead(ENC_LF_A) == digitalRead(ENC_LF_B)) ? -1 : 1; }
void IRAM_ATTR isr_RB() { enc_RB += (digitalRead(ENC_RB_A) == digitalRead(ENC_RB_B)) ? -1 : 1; }
void IRAM_ATTR isr_RF() { enc_RF += (digitalRead(ENC_RF_A) == digitalRead(ENC_RF_B)) ? -1 : 1; }

int currentSpeed = 205;

void setMotor(int pinA, int pinB, int speed) {
    if (speed > 0) {
        ledcWrite(pinA, speed);
        ledcWrite(pinB, 0);
    } else if (speed < 0) {
        ledcWrite(pinA, 0);
        ledcWrite(pinB, -speed);
    } else {
        ledcWrite(pinA, 0);
        ledcWrite(pinB, 0);
    }
}

void driveAll(int leftSpeed, int rightSpeed) {
    setMotor(L_M1A, L_M1B, leftSpeed);
    setMotor(L_M2A, L_M2B, leftSpeed);
    setMotor(R_M1A, R_M1B, rightSpeed);
    setMotor(R_M2A, R_M2B, rightSpeed);
}

void sendEncoder() {
    Serial.printf("ENC:%ld,%ld,%ld,%ld\n", enc_LB, enc_LF, enc_RB, enc_RF);
}

void handleCommand(String cmd) {
    cmd.trim();
    if      (cmd == "FORWARD")   { driveAll( currentSpeed,  currentSpeed); Serial.println("OK:FORWARD");  }
    else if (cmd == "BACKWARD")  { driveAll(-currentSpeed, -currentSpeed); Serial.println("OK:BACKWARD"); }
    else if (cmd == "LEFT")      { driveAll(-currentSpeed,  currentSpeed); Serial.println("OK:LEFT");     }
    else if (cmd == "RIGHT")     { driveAll( currentSpeed, -currentSpeed); Serial.println("OK:RIGHT");    }
    else if (cmd == "STOP")      { driveAll(0, 0);                         Serial.println("OK:STOP");     }
    else if (cmd == "ENC")       { sendEncoder(); }
    else if (cmd == "RESET_ENC") {
        enc_LB = enc_LF = enc_RB = enc_RF = 0;
        Serial.println("OK:RESET_ENC");
    }
    else if (cmd.startsWith("SPEED:")) {
        currentSpeed = constrain(cmd.substring(6).toInt(), 0, 255);
        Serial.printf("OK:SPEED:%d\n", currentSpeed);
    }
    else { Serial.println("ERR:UNKNOWN_CMD"); }
}

void setup() {
    Serial.begin(115200);

    // PWM setup (Core 3.x)
    ledcAttach(L_M1A, PWM_FREQ, PWM_RES);
    ledcAttach(L_M1B, PWM_FREQ, PWM_RES);
    ledcAttach(L_M2A, PWM_FREQ, PWM_RES);
    ledcAttach(L_M2B, PWM_FREQ, PWM_RES);
    ledcAttach(R_M1A, PWM_FREQ, PWM_RES);
    ledcAttach(R_M1B, PWM_FREQ, PWM_RES);
    ledcAttach(R_M2A, PWM_FREQ, PWM_RES);
    ledcAttach(R_M2B, PWM_FREQ, PWM_RES);

    // Encoder
    pinMode(ENC_LB_A, INPUT_PULLUP); pinMode(ENC_LB_B, INPUT_PULLUP);
    pinMode(ENC_LF_A, INPUT_PULLUP); pinMode(ENC_LF_B, INPUT_PULLUP);
    pinMode(ENC_RB_A, INPUT_PULLUP); pinMode(ENC_RB_B, INPUT_PULLUP);
    pinMode(ENC_RF_A, INPUT_PULLUP); pinMode(ENC_RF_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENC_LB_A), isr_LB, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_LF_A), isr_LF, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_RB_A), isr_RB, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_RF_A), isr_RF, CHANGE);

    driveAll(0, 0);
    Serial.println("=== ESP32 DRIVE READY ===");
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }

    static unsigned long lastEnc = 0;
    if (millis() - lastEnc >= 200) {
        lastEnc = millis();
        sendEncoder();
    }
}