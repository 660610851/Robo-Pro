#include <Arduino.h>

#define EN_PIN  25
#define IN1_PIN 26
#define IN2_PIN 27

#define PWM_FREQ 1000
#define PWM_RES  8

void setup() {
    Serial.begin(115200);

    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    ledcAttach(EN_PIN, PWM_FREQ, PWM_RES);

    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(EN_PIN, 0);

    Serial.println("=== L298N CONTROL ===");
    Serial.println("ON      = เปิด speed 255");
    Serial.println("OFF     = ปิด");
    Serial.println("S:150   = ตั้ง speed 0-255");
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "ON") {
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            ledcWrite(EN_PIN, 255);
            Serial.println("OK:ON speed:255");
        }
        else if (cmd == "OFF") {
            digitalWrite(IN1_PIN, LOW);
            digitalWrite(IN2_PIN, LOW);
            ledcWrite(EN_PIN, 0);
            Serial.println("OK:OFF");
        }
        else if (cmd.startsWith("S:")) {
            int spd = constrain(cmd.substring(2).toInt(), 0, 255);
            digitalWrite(IN1_PIN, HIGH);
            digitalWrite(IN2_PIN, LOW);
            ledcWrite(EN_PIN, spd);
            Serial.printf("OK:SPEED:%d\n", spd);
        }
        else {
            Serial.println("ERR:UNKNOWN");
        }
    }
}
