/* ------------------------------------------------------------
 * Universal-orientation Rep-Counter  (v1.4 – fixed Fwd/Back logic)
 * Hardware  : MPU6050 + Adafruit 7-segment + Vibration motor
 * Target MCU: ATmega328P (Arduino-as-ISP, no bootloader)
 * -----------------------------------------------------------*/

 #include <avr/io.h>
 #include <util/delay.h>
 #include <Wire.h>
 #include <Adafruit_MPU6050.h>
 #include <Adafruit_Sensor.h>
 #include <Adafruit_LEDBackpack.h>
 #include <math.h>
 
 /* ---------- Pins ---------- */
 #define MOTOR_PIN 3                   // PD3  (PWM)
 #define RST_PIN   1                   // PD1  (active‑low reset)
 #define MODE_PIN  2                   // PD2  (toggle Up/Down ↔ Fwd/Back)
 const uint8_t POT_PIN = A1;           // Time‑Under‑Tension dial
 
 /* ---------- Timing / display ---------- */
 const uint16_t LOOP_DELAY_MS = 100;   // main‑loop cadence
 
 /* ---------- Calibration ---------- */
 const uint16_t CAL_SAMPLES  = 200;
 const uint16_t SAMPLE_DELAY = 5;      // ms between samples
 
 /* ---------- Rep detection ---------- */
 const float THRESH_G      = 0.6f;     // threshold in ±g for both modes
 const float DEAD_ZONE_G   = 0.30f;    // ignore wiggles ±0.3 g
 const float SMOOTH_ALPHA  = 0.25f;    // 0=no smoothing, 1=raw
 
 /* ---------- Vibration motor ---------- */
 const uint8_t MOTOR_DUTY = 150;       // PWM 0‑255
 
 /* ---------- Globals ---------- */
 Adafruit_MPU6050  mpu;
 Adafruit_7segment display = Adafruit_7segment();
 
 sensors_vec_t upVec;   float g0 = 1.0;   // unit “up” & baseline |g|
 float baseAccY = 0.0f;                   // rest‑state forward axis (Y)
 
 // Up/Down mode variables (original)
 uint16_t repCount = 0;
 bool lifting  = false, lowering = false, buzzed = false;
 unsigned long repStart = 0, repDuration = 0;
 
 // Fwd/Back mode variables (re‑worked)
 uint16_t repCountFB = 0;
 bool outward  = false, returnM = false, buzzedFB = false; // renamed for clarity
 unsigned long repStartFB = 0, repDurationFB = 0;
 
 // Mode & UI helpers
 bool modeFB   = false;                 // false = Up/Down | true = Forward/Back
 bool showDone = false, showSec = false, showMode = false;
 unsigned long doneStart = 0, secStart = 0, modeStart = 0;
 
 unsigned long BUZZ_MS = 5000;              // TUT from potentiometer
 unsigned long MIN_REP_MS = 5000;
 
 bool lastReset = HIGH;
 bool lastModeBtn = HIGH;
 
 /* ---------- Helpers ---------- */
 inline void motorOn()  { analogWrite(MOTOR_PIN, MOTOR_DUTY); }
 inline void motorOff() { analogWrite(MOTOR_PIN, 0);          }
 void buzz(unsigned long d) { motorOn(); delay(d); motorOff(); }
 
 float project(const sensors_event_t& a) {
   return a.acceleration.x * upVec.x +
          a.acceleration.y * upVec.y +
          a.acceleration.z * upVec.z;
 }
 
 void clearRepState() {
   // Up/Down
   repCount = 0;
   lifting = lowering = buzzed = false;
   // Fwd/Back
   repCountFB = 0;
   outward = returnM = buzzedFB = false;
   // UI
   showDone = showSec = false;
 }
 
 /* ---------- Gravity & baseline calibration ---------- */
 void calibrateGravity(bool splash) {
   float ax=0, ay=0, az=0;
   sensors_event_t a,g,t;
   for (uint16_t i=0;i<CAL_SAMPLES;++i) {
     mpu.getEvent(&a,&g,&t);
     ax+=a.acceleration.x; ay+=a.acceleration.y; az+=a.acceleration.z;
     delay(SAMPLE_DELAY);
   }
   ax/=CAL_SAMPLES; ay/=CAL_SAMPLES; az/=CAL_SAMPLES;
 
   g0 = sqrt(ax*ax+ay*ay+az*az);
   upVec.x = ax/g0; upVec.y = ay/g0; upVec.z = az/g0;
 
   baseAccY = ay;                       // store rest‑state Y for Fwd/Back mode
 
   if (splash) {
     display.writeDigitAscii(0,'r'); display.writeDigitAscii(1,'E');
     display.writeDigitAscii(3,'d'); display.writeDigitAscii(4,'y');
     display.writeDisplay();
     buzz(800);
   }
 }
 
 /* ---------- Setup ---------- */
 void setup() {
   Serial.begin(115200); Wire.begin();
   if (!mpu.begin()) while (1);
   mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
   mpu.setGyroRange       (MPU6050_RANGE_500_DEG);
   mpu.setFilterBandwidth (MPU6050_BAND_21_HZ);
 
   display.begin(0x70); display.clear(); display.writeDisplay();
   pinMode(MOTOR_PIN,OUTPUT); motorOff();
   pinMode(POT_PIN, INPUT);
   pinMode(RST_PIN, INPUT_PULLUP);
   pinMode(MODE_PIN, INPUT_PULLUP);
 
   calibrateGravity(true);                     // first power‑on
 }
 
 /* ---------- Loop ---------- */
 void loop() {
   unsigned long now = millis();
 
   /* -------- Reset switch -------- */
   bool sw = digitalRead(RST_PIN);
   if (sw==LOW && lastReset==HIGH) {
     clearRepState();
     calibrateGravity(true);                   // re‑orient on press
   }
   lastReset = sw;
 
   /* -------- Mode‑toggle button -------- */
   bool mb = digitalRead(MODE_PIN);
   if (mb==LOW && lastModeBtn==HIGH) {
     modeFB = !modeFB;                         // flip mode
     clearRepState();                          // fresh counters each mode
     showMode = true;  modeStart = now;
   }
   lastModeBtn = mb;
 
   /* -------- Potentiometer (1–10 s) -------- */
   uint8_t secSet = map(analogRead(POT_PIN),0,1023,1,10);
   unsigned long newMs = secSet*1000UL;
   if (newMs != BUZZ_MS) {
     BUZZ_MS = MIN_REP_MS = newMs;
     showSec = true; secStart = now;
   }
 
   /* -------- Get & filter acceleration -------- */
   sensors_event_t a,g,t; mpu.getEvent(&a,&g,&t);
 
   /* Common: projection on gravity (Up/Down) */
   static float filtUD = 0;                      // first loop: 0
   filtUD = SMOOTH_ALPHA * (project(a) - g0) + (1-SMOOTH_ALPHA) * filtUD;
   float deltaG_UD = (fabsf(filtUD) < DEAD_ZONE_G) ? 0 : filtUD;
 
   /* Fwd/Back delta (sensor Y axis) */
   static float filtFB = 0;
   filtFB = SMOOTH_ALPHA * ((a.acceleration.y - baseAccY)/g0) + (1-SMOOTH_ALPHA) * filtFB;
   float deltaG_FB = (fabsf(filtFB) < DEAD_ZONE_G) ? 0 : filtFB;
 
   /* ============================================================= */
   /* -------- Rep state machines  (choose by mode) --------------- */
   /* ============================================================= */
   if (!modeFB) {
     /* ===== Up/Down detection  ===== */
 
     if (!lifting && deltaG_UD < -THRESH_G) {
       lifting = true; lowering = buzzed = false; repStart = now;
     }
     if (lifting && !buzzed && (now-repStart)>=BUZZ_MS) {
       buzz(400); showDone=true; doneStart=now; buzzed=true;
     }
     if (lifting && deltaG_UD >  THRESH_G) lowering = true;
     if (lifting && lowering) {
       repDuration = now - repStart;
       if (repDuration >= MIN_REP_MS) ++repCount;
       lifting = lowering = false;
     }
   }
   else {
     /* ===== Forward/Backwards detection (fixed sign logic) =====
        Movement profile expected by user:
          • Start near chest → ACCELERATE FORWARD (outward) → deltaG_FB > +THRESH_G
          • Hold/slow → reverse back toward chest → deltaG_FB < –THRESH_G
     */
 
     if (!outward && deltaG_FB > THRESH_G) {
       outward = true; returnM = buzzedFB = false; repStartFB = now;
     }
     if (outward && !buzzedFB && (now-repStartFB)>=BUZZ_MS) {
       buzz(400); showDone=true; doneStart=now; buzzedFB=true;
     }
     if (outward && deltaG_FB < -THRESH_G) returnM = true;
     if (outward && returnM) {
       repDurationFB = now - repStartFB;
       if (repDurationFB >= MIN_REP_MS) ++repCountFB;
       outward = returnM = false;
     }
   }
 
   /* -------- Display -------- */
   display.clear();
   if (showMode && (now-modeStart<1000)) {
     display.writeDigitAscii(0, modeFB ? 'F' : 'U');
     display.writeDigitAscii(1, modeFB ? 'B' : 'P');
     display.writeDigitAscii(3, 'M');  display.writeDigitAscii(4, 'd');
   }
   else if (showDone && (now-doneStart<1000)) {
     display.writeDigitAscii(0,'d'); display.writeDigitAscii(1,'O');
     display.writeDigitAscii(3,'n'); display.writeDigitAscii(4,'E');
   }
   else if (showSec && (now-secStart<1000)) {
     display.print(secSet,DEC);
   }
   else {
     showDone = showSec = showMode = false;
     uint8_t tut = 0;
     uint16_t repsShown = 0;
     if (!modeFB) {
       tut = lifting ? (now-repStart)/1000 : 0;
       repsShown = repCount;
     } else {
       tut = outward ? (now-repStartFB)/1000 : 0;
       repsShown = repCountFB;
     }
     display.writeDigitNum(0,(tut/10)%10);
     display.writeDigitNum(1, tut%10);
     display.drawColon(true);
     display.writeDigitNum(3,(repsShown/10)%10);
     display.writeDigitNum(4, repsShown%10);
   }
   display.writeDisplay();
 
   delay(LOOP_DELAY_MS);
 }
 