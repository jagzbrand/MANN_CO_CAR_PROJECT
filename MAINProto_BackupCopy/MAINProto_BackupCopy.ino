#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#include "Motors.h"
#include "Sensors.h"
#include "OLED.h"
#include "LineTracker.h"

// ---------------- CONFIG ----------------
const uint8_t SW0_PIN = 21;   // Emergency-stop button

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Servo (ultrasonic pivot)
Servo headServo;
const int SERVO_PIN = 2;

// Bluetooth (SoftwareSerial)
SoftwareSerial BT(9, 10); // RX, TX

// ------------ Line Tracker (3 infrared sensors) ------------
// Pins: LEFT = A0, CENTER = A1, RIGHT = A2
static const uint8_t kSensorPins[LineTracker::NUM_SENSORS] = { A0, A1, A2 };
// enable analog mode to better detect line when sensors are higher; tweak threshold if needed
const bool LINE_USE_ANALOG = true;
const int  LINE_ANALOG_THRESHOLD = 500; // lower -> more sensitive to dark line; adjust for your sensors (was 700, lowered for 5cm height)
LineTracker lineTracker(kSensorPins,
                        /*activeLow=*/true,
                        /*usePullups=*/true,
                        LINE_USE_ANALOG,
                        LINE_ANALOG_THRESHOLD);

// Normalized last-known line position in [0.0, 1.0]
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

// ----- Servo Sweep (integrated with "Servo-Proper.ino") -----
int   scanPosDeg = 90;
int   scanDir    = +1;
// Safer mechanical range and smoother motion
const int SCAN_MIN_DEG   = 45;   // avoid hard stops
const int SCAN_MAX_DEG   = 135;
const int SCAN_STEP_DEG  = 10;   // Increased from 5 to 10 for faster sweep
const unsigned long SERVO_UPDATE_MS = 50; // Reduced from 150ms to 50ms for faster movement
unsigned long lastServoUpdate = 0;

// ----- Timing -----
unsigned long previousMillis = 0;
const unsigned long interval = 20;

// ----- Serial Debug Timing -----
unsigned long lastDebugPrint = 0;
const unsigned long DEBUG_INTERVAL = 100; // Print debug info every 100ms

// ----- OLED Update Timing -----
unsigned long lastOLEDUpdate = 0;
const unsigned long OLED_UPDATE_INTERVAL = 200; // Update OLED every 200ms (don't spam I2C)

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

// ----- Serial Debug Helper -----
static void printLineFollowDebug(uint8_t lineBits, float linePos, float error, int leftPWM, int rightPWM) {
  // Read individual sensor states (use analog if analog mode is enabled)
  int leftRaw, centerRaw, rightRaw;
  if (LINE_USE_ANALOG) {
    leftRaw = analogRead(kSensorPins[0]);
    centerRaw = analogRead(kSensorPins[1]);
    rightRaw = analogRead(kSensorPins[2]);
  } else {
    leftRaw = digitalRead(kSensorPins[0]);
    centerRaw = digitalRead(kSensorPins[1]);
    rightRaw = digitalRead(kSensorPins[2]);
  }
  
  bool leftOnLine = (lineBits & 0x01) != 0;
  bool centerOnLine = (lineBits & 0x02) != 0;
  bool rightOnLine = (lineBits & 0x04) != 0;
  
  Serial.print("=== LINE FOLLOW DEBUG ===");
  Serial.print(" | Mode: LINE-FOLLOW");
  Serial.print(" | Emergency: "); Serial.print(emergencyStop ? "YES" : "NO");
  Serial.print(" | Analog: "); Serial.print(LINE_USE_ANALOG ? "YES" : "NO");
  Serial.print(" | Threshold: "); Serial.print(LINE_ANALOG_THRESHOLD);
  Serial.println();
  
  Serial.print("SENSORS: ");
  Serial.print("L="); Serial.print(leftRaw); Serial.print(leftOnLine ? "(LINE)" : "(OFF)");
  Serial.print(" | C="); Serial.print(centerRaw); Serial.print(centerOnLine ? "(LINE)" : "(OFF)");
  Serial.print(" | R="); Serial.print(rightRaw); Serial.print(rightOnLine ? "(LINE)" : "(OFF)");
  Serial.print(" | Bits=0b");
  Serial.print(lineBits, BIN);
  Serial.println();
  
  Serial.print("POSITION: ");
  Serial.print("Raw="); Serial.print(linePos * float(LineTracker::NUM_SENSORS - 1), 2);
  Serial.print(" | Normalized="); Serial.print(linePos, 3);
  Serial.print(" | Error="); Serial.print(error, 3);
  Serial.print(" | Center=0.5");
  Serial.println();
  
  Serial.print("MOTORS: ");
  Serial.print("LeftPWM="); Serial.print(leftPWM);
  Serial.print(" | RightPWM="); Serial.print(rightPWM);
  Serial.print(" | BasePWM="); Serial.print(LF_basePWM);
  Serial.print(" | kP="); Serial.print(LF_kP);
  Serial.println();
  
  Serial.println("------------------------");
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(9600);
  // Don't wait for Serial on ATmega328P - it blocks forever
  delay(500); // Brief delay for Serial to initialize
  
  BT.begin(9600);
  
  Serial.println("Starting setup...");
  Serial.flush();
  
  Wire.begin(); // important for OLED
  delay(100); // Give I2C time to initialize

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
  lineTracker.begin();

  // OLED
  initOLED();

  BT.println("Bluetooth ready!");
  Serial.println("Bluetooth ready!");
  Serial.print("OLED ready? ");
  Serial.println(isOLEDReady() ? "YES" : "NO");
  Serial.println("=== LINE FOLLOWING DEBUG ENABLED ===");
  Serial.print("Line sensors: Analog mode=");
  Serial.print(LINE_USE_ANALOG ? "YES" : "NO");
  Serial.print(", Threshold=");
  Serial.println(LINE_ANALOG_THRESHOLD);
  Serial.println("Tip: If sensors always show OFF, lower threshold. If always ON, raise threshold.");
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

  // Line tracking using LineTracker library
  uint8_t lineBits = lineTracker.readBits();
  // Raw position in [0 .. NUM_SENSORS-1], average of active sensors
  float rawPos = lineTracker.computePosition();
  // Normalize to [0.0 .. 1.0] so center is 0.5
  float linePos = rawPos / float(LineTracker::NUM_SENSORS - 1);
  gLastLinePos = linePos;
  
  // Serial debug for line following (print every DEBUG_INTERVAL ms)
  if (currentMode == MODE_LF && (now - lastDebugPrint >= DEBUG_INTERVAL)) {
    lastDebugPrint = now;
    float center = 0.5f;
    float error = center - linePos;
    // Calculate what PWMs would be used (or are being used)
    int turn = (int)(LF_kP * error);
    int leftPWM = constrain(LF_basePWM - turn, LF_minPWM, LF_maxPWM);
    int rightPWM = constrain(LF_basePWM + turn, LF_minPWM, LF_maxPWM);
    printLineFollowDebug(lineBits, linePos, error, leftPWM, rightPWM);
  }

  // Bluetooth input and command handling
  while (BT.available()) {
    char c = BT.read();
    // Debug: show received Bluetooth character
    Serial.print(F("[BT] Received: '"));
    Serial.print(c);
    Serial.println("'");
    switch(c) {
      case 'h': case 'H': btSendHelp(); break;
      case 'p': case 'P': currentMode = MODE_PROX; emergencyStop = false; stopMotors(); BT.println(F("Mode -> PROXIMITY")); break;
      case 'v': case 'V': currentMode = MODE_LF;  emergencyStop = false; stopMotors(); BT.println(F("Mode -> LINE-FOLLOW")); break;
      case 'm': case 'M':
        // Clear emergency stop when explicitly switching to MANUAL
        emergencyStop = false;
        currentMode = MODE_MANUAL;
        movingF = movingB = turningL = turningR = false;
        stopMotors();
        BT.println(F("Mode -> MANUAL (idle)"));
        break;

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

  // OLED display using updated library (throttled to avoid blocking)
  if (now - lastOLEDUpdate >= OLED_UPDATE_INTERVAL) {
    lastOLEDUpdate = now;
    const char* stateText = emergencyStop ? "STOP" : 
                           (currentMode == MODE_LF) ? "FOLLOWING" :
                           (currentMode == MODE_PROX) ? "SCANNING" : "IDLE";
    // Map mode enum to OLED expected values: 0=MANUAL, 1=LINE, 2=PROX
    uint8_t oledMode = (currentMode == MODE_MANUAL) ? 0 : 
                       (currentMode == MODE_LF) ? 1 : 2;
    updateOLED(oledMode, stateText, distance_cm);
  }
}
