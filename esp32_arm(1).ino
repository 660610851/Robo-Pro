#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define DEBUG 1
#if DEBUG
#define DBG(x) Serial.println(x)
#else
#define DBG(x)
#endif

#define SDA_PIN    21
#define SCL_PIN    22
#define SERVO_FREQ 50
#define SERVO_NUM  6

#define EN_PIN   25
#define IN1_PIN  26
#define IN2_PIN  27

#define PWM_FREQ 1000
#define PWM_RES  8
#define PWM_CHANNEL 0

#define PULSE_MIN 100
#define PULSE_MAX 600

Adafruit_PWMServoDriver pwm(0x40);

// ===== POSITIONS =====
uint16_t homePos[SERVO_NUM]  = {355,200,600,405,520,110};
uint16_t readyPos[SERVO_NUM] = {355,260,530,405,520,110};
uint16_t currentPos[SERVO_NUM];

// ===== Motion Control =====
bool moving      = false;
int  moveStep    = 0;
int  moveSteps   = 30;   // ← ปรับได้ผ่าน SPEED:N  (น้อย=เร็ว)
int  moveDelayMs = 10;
unsigned long lastMoveTime = 0;
uint16_t moveStart[SERVO_NUM];
uint16_t moveTarget[SERVO_NUM];
int moveSingleIdx = -1;

// ===== START MOVE SINGLE =====
void startMove(int idx, uint16_t target){
    moveSingleIdx    = idx;
    moveStart[idx]   = currentPos[idx];
    moveTarget[idx]  = constrain(target, PULSE_MIN, PULSE_MAX);
    moveStep         = 0;
    // moveSteps ใช้ค่าปัจจุบัน (ที่ SPEED: เซ็ตไว้)
    moving           = true;
    lastMoveTime     = millis();
}

// ===== START MOVE ALL =====
void startMoveAll(uint16_t target[]){
    moveSingleIdx = -1;
    for(int i = 0; i < SERVO_NUM; i++){
        moveStart[i]  = currentPos[i];
        moveTarget[i] = constrain(target[i], PULSE_MIN, PULSE_MAX);
    }
    moveStep     = 0;
    // moveSteps ใช้ค่าปัจจุบัน (ที่ SPEED: เซ็ตไว้)
    moving       = true;
    lastMoveTime = millis();
}

// ===== UPDATE MOVE =====
void updateMove(){
    if(!moving) return;
    if(millis() - lastMoveTime < moveDelayMs) return;
    lastMoveTime = millis();
    moveStep++;

    if(moveSingleIdx >= 0){
        int i = moveSingleIdx;
        uint16_t p = moveStart[i] +
            (moveTarget[i] - moveStart[i]) * moveStep / moveSteps;
        pwm.setPWM(i, 0, p);
        if(moveStep >= moveSteps){
            currentPos[i] = moveTarget[i];
            moving = false;
        }
    } else {
        for(int i = 0; i < SERVO_NUM; i++){
            uint16_t p = moveStart[i] +
                (moveTarget[i] - moveStart[i]) * moveStep / moveSteps;
            pwm.setPWM(i, 0, p);
        }
        if(moveStep >= moveSteps){
            for(int i = 0; i < SERVO_NUM; i++)
                currentPos[i] = moveTarget[i];
            moving = false;
        }
    }
}

// ===== PUMP =====
void pumpOn(int spd){
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(PWM_CHANNEL, constrain(spd, 0, 255));
}
void pumpOff(){
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(PWM_CHANNEL, 0);
}

// ===== COMMAND HANDLER =====
void handleCommand(String cmd){
    cmd.trim();
    Serial.println(cmd);

    if(cmd == "HOME"){
        startMoveAll(homePos);
    }
    else if(cmd == "READY"){
        startMoveAll(readyPos);
    }
    else if(cmd == "GRIP:OPEN"){
        startMove(4, 520);
    }
    else if(cmd == "GRIP:CLOSE"){
        startMove(4, 300);
    }
    else if(cmd == "PUMP:ON"){
        pumpOn(200);
    }
    else if(cmd == "PUMP:OFF"){
        pumpOff();
    }
    // ===== NEW: SPEED:N =====
    else if(cmd.startsWith("SPEED:")){
        int val = cmd.substring(6).toInt();
        moveSteps = constrain(val, 3, 150);
        Serial.print("SPEED SET: ");
        Serial.println(moveSteps);
    }
    else if(cmd.startsWith("SERVO:")){
        int f = cmd.indexOf(':');
        int s = cmd.indexOf(':', f+1);
        int num = cmd.substring(f+1, s).toInt();
        int val = cmd.substring(s+1).toInt();
        startMove(num, val);
    }
    else if(cmd.startsWith("ALL:")){
        uint16_t target[SERVO_NUM];
        String data = cmd.substring(4);
        int idx = 0;
        while(data.length() > 0 && idx < SERVO_NUM){
            int comma = data.indexOf(',');
            if(comma == -1){
                target[idx++] = data.toInt();
                break;
            }
            target[idx++] = data.substring(0, comma).toInt();
            data = data.substring(comma+1);
        }
        if(idx == SERVO_NUM)
            startMoveAll(target);
    }
}

// ===== SETUP =====
void setup(){
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    pwm.begin();
    pwm.setPWMFreq(SERVO_FREQ);
    delay(100);
    for(int i = 0; i < SERVO_NUM; i++){
        currentPos[i] = homePos[i];
        pwm.setPWM(i, 0, homePos[i]);
    }
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(EN_PIN, PWM_CHANNEL);
    Serial.println("ESP32 ARM READY");
}

// ===== LOOP =====
void loop(){
    if(Serial.available()){
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }
    updateMove();
}
