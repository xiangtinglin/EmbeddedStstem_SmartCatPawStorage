
#include <HX711.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <math.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// HX711 Pins
HX711 scale;
#define DT 3
#define SCK 2

// Servo Motor 1
Servo paw;
#define SERVO_PIN 9

// Servo Motor 2
Servo paw2;
#define SERVO2_PIN 8

// Buzzer（無源）
#define BUZZER_PIN 10

// LEDs
#define LED1 11
#define LED2 12
#define LED3 13

// Baseline
long BASE_RAW = 0;

// EMA
float ema_raw = 0;
const float EMA_ALPHA = 0.65;

// Kalman
float kalman_weight = 0;
float P = 1, Q = 0.35, R = 1.0;

// ======================================================
// Kalman
// ======================================================
float kalmanFilter(float m) {
  P = P + Q;
  float K = P / (P + R);
  kalman_weight = kalman_weight + K * (m - kalman_weight);
  P = (1 - K) * P;
  return kalman_weight;
}

// ======================================================
// 喵叫
// ======================================================
void Meow_Advanced() {

  for (int f = 1800; f >= 900; f -= 12) {
    tone(BUZZER_PIN, f);
    delay(4);
  }
  delay(30);

  for (int i = 0; i < 15; i++) {
    tone(BUZZER_PIN, 500 + random(0, 30));
    delay(20);
  }

  for (int f = 1200; f <= 1600; f += 25) {
    tone(BUZZER_PIN, f);
    delay(5);
  }
  delay(20);

  for (int i = 0; i < 25; i++) {
    int f = 1400 + (sin(i * 0.4) * 120);
    tone(BUZZER_PIN, f);
    delay(15);
  }

  noTone(BUZZER_PIN);
}

// ======================================================
// 校正提示音
// ======================================================
void CalibrationBeep() {
  for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, 2500);
    delay(100);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

// ======================================================
// 穩定狀態檢查
// ======================================================
int stableState = -1;
int currentState = -1;
unsigned long stateStartTime;
unsigned long STABLE_TIME = 2000;

// ======================================================
// Sleep Mode
// ======================================================
bool isSleeping = false;
unsigned long idleStartTime = 0;
unsigned long sleepStartTime = 0;   // ⭐ 新增：Sleep 開始時間
const unsigned long SLEEP_WAIT = 15000; // 15 秒 idle → sleep
const unsigned long SLEEP_SCREEN_OFF = 5000; // ⭐ Sleep 5 秒後關背光

void enterSleepMode() {

  isSleeping = true;

  // detach servo
  if (paw.attached()) paw.detach();
  if (paw2.attached()) paw2.detach();

  // LED off
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  noTone(BUZZER_PIN);

  lcd.setCursor(0, 1);
  lcd.print("SLEEP MODE       ");

  sleepStartTime = millis();   // ⭐ 記錄 Sleep 開始時間
}

void wakeUp() {

  isSleeping = false;

  lcd.backlight();   // ⭐ 喚醒時亮螢幕

  paw.attach(SERVO_PIN);
  paw2.attach(SERVO2_PIN);
  paw.write(90);
  paw2.write(90);

  lcd.setCursor(0, 1);
  lcd.print("WAKING...        ");

  delay(300);
}

// ======================================================
// Setup
// ======================================================
void setup() {

  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  paw.attach(SERVO_PIN);
  paw2.attach(SERVO2_PIN);

  paw.write(90);
  paw2.write(90);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  scale.begin(DT, SCK);

  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  delay(250);

  BASE_RAW = scale.read();
  ema_raw = BASE_RAW;
  kalman_weight = 0;

  lcd.clear();
  lcd.print("Ready Base:");
  lcd.print(BASE_RAW);

  CalibrationBeep();
  delay(250);

  idleStartTime = millis();
}

// ======================================================
// Loop
// ======================================================
void loop() {

  // -------------------- 重量濾波 --------------------
  long raw = scale.read();
  long diff = raw - ema_raw;

  const int BIG_CHANGE = 400;

  if (diff < -BIG_CHANGE) {
    ema_raw = raw;
    kalman_weight = 0;
  }
  else if (diff > BIG_CHANGE) {
    ema_raw = raw;
  }
  else {
    ema_raw = ema_raw * (1 - EMA_ALPHA) + raw * EMA_ALPHA;
  }

  float delta = ema_raw - BASE_RAW;
  float weight_g = delta / 180.0;

  if (abs(weight_g) < 0.2) weight_g = 0;
  weight_g = kalmanFilter(weight_g);
  if (abs(weight_g) < 0.15) weight_g = 0;

  lcd.setCursor(0, 0);
  lcd.print("Weight:");
  lcd.print(weight_g, 1);
  lcd.print(" g   ");


  // ======================================================
  // Sleep Mode（Idle < 15g）
  // ======================================================
  if (weight_g < 15) {

    static bool wasIdle = false;

    if (!wasIdle) {
      idleStartTime = millis();
      wasIdle = true;
    }

    // Idle 達 15 秒 → Sleep
    if (!isSleeping && millis() - idleStartTime > SLEEP_WAIT) {
      enterSleepMode();
      return;
    }

    // ⭐ Sleep 中 → 5 秒後關閉 LCD 背光
    if (isSleeping && millis() - sleepStartTime > SLEEP_SCREEN_OFF) {
      lcd.noBacklight();
    }

    if (isSleeping) return;

  } else {

    // 若 weight >= 15 → 喚醒
    static bool wasIdle = false;
    wasIdle = false;

    if (isSleeping) {
      wakeUp();
    }

    idleStartTime = millis();
  }


  // ======================================================
  // 以下完全保持原始動作邏輯不變
  // ======================================================

  if (weight_g < 15) currentState = 0;
  else if (weight_g > 15 && weight_g <= 100) currentState = 1;
  else if (weight_g > 100 && weight_g <= 1300) currentState = 2;
  else if (weight_g > 1300) currentState = 3;
  else currentState = -1;

  static int previousState = -1;

  if (currentState != previousState) {
    stateStartTime = millis();
    previousState = currentState;
  }

  if (millis() - stateStartTime < STABLE_TIME) return;

  stableState = currentState;

  // -------------------- 超重 --------------------
  if (stableState == 3) {

    lcd.backlight();  // 確保警告時亮起螢幕

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);

    Meow_Advanced();

    lcd.setCursor(0, 1);
    lcd.print("ALERT: >1.3kg!");

    paw.write(90);
    paw2.write(90);
    return;
  }

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  // -------------------- Idle --------------------
  if (stableState == 0) {

    if (paw.attached()) paw.detach();
    if (paw2.attached()) paw2.detach();

    lcd.setCursor(0, 1);
    lcd.print("Status: IDLE      ");

    return;
  }

  // -------------------- ACTION --------------------
  if (stableState == 1 || stableState == 2) {

    lcd.backlight();

    lcd.setCursor(0, 1);
    lcd.print("Status: ACTION    ");

    if (!paw.attached()) paw.attach(SERVO_PIN);
    if (!paw2.attached()) paw2.attach(SERVO2_PIN);

    paw.write(90);
    paw2.write(90);
    delay(200);

    if (stableState == 1) {
      paw.write(30);
      paw2.write(30);
      delay(350);
    }

    if (stableState == 2) {
      paw.write(150);
      paw2.write(150);
      delay(350);
    }

    paw.write(90);
    paw2.write(90);
    delay(200);
  }

  delay(10);
}
