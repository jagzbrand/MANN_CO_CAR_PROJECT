#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#include "Motors.h"
#include "Sensors.h"
#include "OLED.h"

// ---------------- CONFIG ----------------
const uint8_t SW0_PIN = 21;   // Emergency-stop button

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Servo (ultrasonic pivot)
Servo headServo;
const int SERVO_PIN = 2;

// Bluetooth (SoftwareSerial)
SoftwareSerial BT(9, 10); // RX, TX

// ------------ Line Sensors (2 sensors: left, right) ------------
static const uint8_t kNumColorSensors = 2;
// Order: { LEFT_SENSOR_PIN, RIGHT_SENSOR_PIN }
static const uint8_t kSensorPins[kNumColorSensors] = { A0, A1 };
static const bool ACTIVE_LOW  = true;
static const bool USE_PULLUPS = true;

static void initColorSensorPins() {
  for (uint8_t i = 0; i < kNumColorSensors; i++) {
    pinMode(kSensorPins[i], USE_PULLUPS ? INPUT_PULLUP : INPUT);
  }
}

// bits: bit0 = left sensor, bit1 = right sensor
static uint8_t readColorBits() {
  uint8_t bits = 0;
  for (uint8_t i = 0; i < kNumColorSensors; i++) {
    int v = digitalRead(kSensorPins[i]);
    bool onLine = ACTIVE_LOW ? (v == LOW) : (v == HIGH);
    if (onLine) bits |= (1u << i);
  }
  return bits;
}

// For 2 sensors we return position: left=0.0, right=1.0, both=0.5
static float computeLinePosition(uint8_t bits, float lastKnownPos) {
  if (bits == 0) return lastKnownPos; // keep last known (or treat as "lost")
  if (bits == 1) return 0.0f;   // left
  if (bits == 2) return 1.0f;   // right
  return 0.5f; // both
}

static float gLastLinePos = 0.5f; // start at center

// ------------- Emergency STOP -------------
bool emergencyStop = false;
int lastStableBtn = HIGH;
int lastReadBtn   = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

static void handleEmergencyStopToggle() {
  int reading = digitalRead(SW0_PIN);
  if (reading != lastReadBtn) { lastDebounceTime = millis(); lastReadBtn = reading; }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != lastStableBtn) {
      lastStableBtn = reading;
      if (lastStableBtn == LOW) {
        emergencyStop = !emergencyStop;
        if (emergencyStop) stopMotors();
      }
    }
  }
}

// ------------- Modes -------------
enum Mode : uint8_t { MODE_PROX = 0, MODE_LF = 1, MODE_MANUAL = 2 };
Mode currentMode = MODE_MANUAL; // start idle

// Manual movement flags
bool movingF = false, movingB = false, turningL = false, turningR = false;

// Line-follow control
int   LF_basePWM = 160;
float LF_kP      = 70.0f;
int   LF_minPWM  = 90;
int   LF_maxPWM  = 255;

// Proximity thresholds
const int OBSTACLE_STOP_CM   = 30;
const int PROX_FWD_THRESHOLD = 30;

// ----- Servo Sweep -----
int   scanPosDeg = 90;
int   scanDir    = +1;
const int SCAN_MIN_DEG   = 0;   // adjust physically if servo can't reach full 0
const int SCAN_MAX_DEG   = 180;
const int SCAN_STEP_DEG  = 15;
const unsigned long SERVO_UPDATE_MS = 20;
unsigned long lastServoUpdate = 0;

// ----- Timing -----
unsigned long previousMillis = 0;
const unsigned long interval = 20;

// -------- Bluetooth helpers --------
static void btSendHelp() {
  BT.println(F("[H]elp  [P]rox  [V]Line  [M]Manual  [S]top  [Q]uery"));
  Serial.println(F("[H]elp  [P]rox  [V]Line  [M]Manual  [S]top  [Q]uery"));
}

static void btSendQuery(int dist, float linePos) {
  BT.print(F("MODE=")); BT.print(currentMode == MODE_PROX ? F("PROX") : currentMode==MODE_LF?F("LINE-FOLLOW"):F("MANUAL"));
  BT.print(F("  DIST=")); BT.print(dist);
  BT.print(F("cm  LINEPOS=")); BT.println(linePos, 2);
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(9600);
  BT.begin(9600);
  
  Wire.begin(); // important for OLED

  // Motors
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotors();

  // Ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  // Button
  pinMode(SW0_PIN, INPUT_PULLUP);

  // Servo
  headServo.attach(SERVO_PIN);
  headServo.write(90);

  // Line sensors
  initColorSensorPins();

  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();

  BT.println("Bluetooth ready!");
  Serial.println("Bluetooth ready!");
  btSendHelp();
}

// ------------------- LOOP -------------------
void loop() {
  unsigned long now = millis();
  if (now - previousMillis < interval) return;
  previousMillis = now;

  handleEmergencyStopToggle();

  // Servo sweep (PROX mode)
  if (!emergencyStop && currentMode == MODE_PROX) {
    if (now - lastServoUpdate >= SERVO_UPDATE_MS) {
      lastServoUpdate = now;
      scanPosDeg += scanDir * SCAN_STEP_DEG;
      if (scanPosDeg <= SCAN_MIN_DEG || scanPosDeg >= SCAN_MAX_DEG) {
        scanDir = -scanDir;
        scanPosDeg = constrain(scanPosDeg, SCAN_MIN_DEG, SCAN_MAX_DEG);
      }
      headServo.write(scanPosDeg);
    }
  }

  // Sensors
  int distance_cm = readDistanceCM();
  uint8_t lineBits = readColorBits();
  float linePos = computeLinePosition(lineBits, gLastLinePos);
  gLastLinePos = linePos;

  // Bluetooth input and command handling
  while (BT.available()) {
    char c = BT.read();
    switch(c) {
      case 'h': case 'H': btSendHelp(); break;
      case 'p': case 'P': currentMode = MODE_PROX; emergencyStop = false; stopMotors(); BT.println(F("Mode -> PROXIMITY")); break;
      case 'v': case 'V': currentMode = MODE_LF;  emergencyStop = false; stopMotors(); BT.println(F("Mode -> LINE-FOLLOW")); break;
      case 'm': case 'M': currentMode = MODE_MANUAL; movingF = movingB = turningL = turningR = false; stopMotors(); BT.println(F("Mode -> MANUAL (idle)")); break;

      // Manual movement
      case 'f': case 'F': if(currentMode==MODE_MANUAL){ movingF=!movingF; movingB=turningL=turningR=false; movingF?moveForward():stopMotors(); } break;
      case 'b': case 'B': if(currentMode==MODE_MANUAL){ movingB=!movingB; movingF=turningL=turningR=false; movingB?moveBackward():stopMotors(); } break;
      case 'l': case 'L':
        if(currentMode==MODE_MANUAL){
          if(movingF){ turningL = !turningL; turningL?turnLeftForward():moveForward(); }
          else if(movingB){ turningL = !turningL; turningL?turnLeftBackward():moveBackward(); }
          else { turningL = !turningL; turningL?turnLeft():stopMotors(); }
        } break;
      case 'r': case 'R':
        if(currentMode==MODE_MANUAL){
          if(movingF){ turningR = !turningR; turningR?turnRightForward():moveForward(); }
          else if(movingB){ turningR = !turningR; turningR?turnRightBackward():moveBackward(); }
          else { turningR = !turningR; turningR?turnRight():stopMotors(); }
        } break;

      case 's': case 'S': emergencyStop=true; stopMotors(); BT.println(F("EMERGENCY STOP")); break;
      case 'q': case 'Q': btSendQuery(distance_cm,linePos); break;
    }
  }

  // Behavior for PROX and LINE modes
  if (emergencyStop) {
    stopMotors();
  } else if (currentMode==MODE_PROX) {
    if(distance_cm==0) stopMotors();
    else if(distance_cm>=PROX_FWD_THRESHOLD) moveForward();
    else turnLeft();
  } else if (currentMode==MODE_LF) {
    if(distance_cm>0 && distance_cm<OBSTACLE_STOP_CM) stopMotors();
    else {
      float center = 0.5f;
      if(lineBits==0){ // lost line
        stopMotors();
        static unsigned long lostSince=0;
        if(lostSince==0) lostSince=now;
        if(now-lostSince>200){
          if(gLastLinePos<center) turnLeft();
          else turnRight();
        }
        if(now-lostSince>600) lostSince=0;
      } else {
        float error = center - linePos;
        int turn = (int)(LF_kP*error);
        int leftPWM = constrain(LF_basePWM-turn,LF_minPWM,LF_maxPWM);
        int rightPWM= constrain(LF_basePWM+turn,LF_minPWM,LF_maxPWM);
        moveForward(leftPWM,rightPWM);
      }
    }
  }

  // OLED display
  display.clearDisplay();
  printCentered("MAARTEEN",0,1);
  printCentered(currentMode==MODE_PROX?"MODE: PROXIMITY":currentMode==MODE_LF?"MODE: LINE-FOLLOW":"MODE: MANUAL",10,1);
  if(emergencyStop) printCentered("EMERGENCY STOP",32,1);
  else printCentered(String(distance_cm)+"cm",48,1);
  display.display();
}
