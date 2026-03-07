// ========================================
// MDD3A Motor Driver + Encoder (ระบุตำแหน่งเป็น cm)
// ล้อเส้นผ่านศูนย์กลาง 12 cm
// Gear ratio 1:260 | Encoder 11 pulses/rev
// Pulse ต่อรอบล้อ = 11 x 260 x 2 (A+B) = 5720
// ========================================

// --- Motor Pin ---
#define M1A 13
#define M1B 12

// --- Encoder Pin ---
#define ENC_A 26  // Yellow: Signal A
#define ENC_B 25  // White:  Signal B

// --- Encoder & Position ---
volatile long encoderCount = 0;
volatile unsigned long lastInterruptTime = 0;

const float WHEEL_DIAMETER_CM   = 12.0;
const float WHEEL_CIRCUMFERENCE = WHEEL_DIAMETER_CM * PI;  // ~37.699 cm
const int   ENCODER_PPR         = 11;     // pulses ต่อรอบของ encoder
const int   GEAR_RATIO          = 260;    // gear ratio 1:260
// ใช้ทั้ง Signal A และ B (x2) จึงได้ 11 x 260 x 2 = 5720 pulses/รอบล้อ
const int   PULSES_PER_REV      = ENCODER_PPR * GEAR_RATIO * 2;  // 5720
const float CM_PER_PULSE        = WHEEL_CIRCUMFERENCE / PULSES_PER_REV;

// กรอง noise: interrupt ต้องห่างกัน >= 1ms
const unsigned long DEBOUNCE_US = 1000;

// --- ISR Signal A ---
void IRAM_ATTR encoderISR_A() {
  unsigned long now = micros();
  if (now - lastInterruptTime < DEBOUNCE_US) return;
  lastInterruptTime = now;

  int stateA = digitalRead(ENC_A);
  int stateB = digitalRead(ENC_B);
  if (stateA != stateB) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

// --- ISR Signal B ---
void IRAM_ATTR encoderISR_B() {
  unsigned long now = micros();
  if (now - lastInterruptTime < DEBOUNCE_US) return;
  lastInterruptTime = now;

  int stateA = digitalRead(ENC_A);
  int stateB = digitalRead(ENC_B);
  if (stateA == stateB) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

// --- แปลง pulse → cm ---
float getPositionCm() {
  return encoderCount * CM_PER_PULSE;
}

void setup() {
  Serial.begin(115200);

  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  stopMotor();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR_A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR_B, CHANGE);

  Serial.println("==== MDD3A Motor + Encoder (cm) ====");
  Serial.print("Gear Ratio       : 1:");
  Serial.println(GEAR_RATIO);
  Serial.print("Pulse ต่อรอบล้อ : ");
  Serial.println(PULSES_PER_REV);
  Serial.print("เส้นรอบวงล้อ   : ");
  Serial.print(WHEEL_CIRCUMFERENCE, 2);
  Serial.println(" cm");
  Serial.print("cm ต่อ 1 pulse  : ");
  Serial.print(CM_PER_PULSE, 6);
  Serial.println(" cm");
  Serial.println("=====================================");
  Serial.println("'w' = เดินหน้า | 's' = ถอยหลัง");
  Serial.println("'x' = หยุด    | 'r' = รีเซ็ตตำแหน่ง");
  Serial.println("'g' = ไปตำแหน่งที่กำหนด (cm)");
  Serial.println("=====================================");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'w': case 'W':
        forward();
        Serial.println(">> เดินหน้า");
        break;

      case 's': case 'S':
        backward();
        Serial.println(">> ถอยหลัง");
        break;

      case 'x': case 'X':
        stopMotor();
        Serial.print(">> หยุด | ตำแหน่ง: ");
        Serial.print(getPositionCm(), 2);
        Serial.println(" cm");
        break;

      case 'r': case 'R':
        encoderCount = 0;
        Serial.println(">> รีเซ็ตตำแหน่ง = 0 cm");
        break;

      case 'g': case 'G':
        Serial.println(">> ใส่ระยะทาง (cm) แล้วกด Enter:");
        while (Serial.available() == 0);
        {
          float targetCm = Serial.parseFloat();
          goToCm(targetCm);
        }
        break;
    }
  }

  // แสดงตำแหน่งทุก 500ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.print("Pulse: ");
    Serial.print(encoderCount);
    Serial.print("  |  ตำแหน่ง: ");
    Serial.print(getPositionCm(), 2);
    Serial.println(" cm");
  }
}

// --- ฟังก์ชันเคลื่อนที่ไปตำแหน่งที่กำหนด (cm) ---
void goToCm(float targetCm) {
  long targetPulse = (long)(targetCm / CM_PER_PULSE);

  Serial.print(">> มุ่งหน้าไป: ");
  Serial.print(targetCm, 2);
  Serial.print(" cm (");
  Serial.print(targetPulse);
  Serial.println(" pulses)");

  if (encoderCount < targetPulse) {
    forward();
    while (encoderCount < targetPulse);
  } else {
    backward();
    while (encoderCount > targetPulse);
  }

  stopMotor();
  Serial.print(">> ถึงแล้ว! ตำแหน่ง: ");
  Serial.print(getPositionCm(), 2);
  Serial.println(" cm");
}

// --- Motor Functions ---
void forward() {
  digitalWrite(M1A, HIGH);
  digitalWrite(M1B, LOW);
}

void backward() {
  digitalWrite(M1A, LOW);
  digitalWrite(M1B, HIGH);
}

void stopMotor() {
  digitalWrite(M1A, LOW);
  digitalWrite(M1B, LOW);
}
