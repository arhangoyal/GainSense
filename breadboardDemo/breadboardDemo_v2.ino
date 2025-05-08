#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LEDBackpack.h>

// Vibration Motor Settings
#define MOTOR_PIN 9         // Motor connected to Arduino Pin D9
#define BUZZ_THRESHOLD 5000 // Buzz once when TUT reaches threshold

// Initialize devices
Adafruit_MPU6050 mpu;  // IMU: MPU6050
Adafruit_7segment display = Adafruit_7segment();  // 7 segment display

// Rep Tracking Variables
int repCount = 0;
bool lifting = false;
bool lowering = false;
bool buzzedThisRep = false;  // Prevents multiple buzzes per rep
unsigned long repStartTime = 0;
unsigned long repDuration = 0;

// Tunable Thresholds
float LIFT_THRESHOLD = 9.5;   // Detect lifting (Z-axis acceleration drops)
float LOWER_THRESHOLD = 11.5; // Detect lowering (Z-axis acceleration rises)
float MIN_REP_TIME = 3000;    // Min rep time required (seconds * 1000) to avoid false reps
float MIN_MOVEMENT = 3;       // Ignore small movements (fine adjustments)


void setup() {
    Serial.begin(115200);
    Wire.begin();  // Initialize I2C bus

    // Initialize MPU6050 and set parameters
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) { delay(10); }
    }
    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);  // Configure on-chip Digital Low Pass Filter to 21 Hz to smoothen sensor data by reducing high-frequency noise

    // Initialize 7-Segment Display
    display.begin(0x70);  // Default I2C address for Adafruit display
    display.clear();
    display.writeDisplay();

    // Setup Vibration Motor
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);  // Ensure motor is off at start

    Serial.println("I2C Devices Ready!");
    delay(100);
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float accelZ = a.acceleration.z;  // Get Z-acceleration
    unsigned long currentTime = millis();

    Serial.print("Z Acceleration: ");
    Serial.println(accelZ);

    // Track Time Under Tension (TUT) Live
    unsigned long liveTUT = (lifting) ? (currentTime - repStartTime) / 1000 : 0; // Live TUT in seconds

    // Update 7-Segment Display (Live TUT on Left 2 Digits, Rep Count on Right 2 Digits)
    display.clear();
    display.writeDigitNum(0, (liveTUT / 10) % 10);  // Left-most digit (TUT tens)
    display.writeDigitNum(1, liveTUT % 10);         // Second-left digit (TUT ones)
    display.drawColon(true);                        // Enable colon (visual separator)
    display.writeDigitNum(3, (repCount / 10) % 10); // Third digit (Rep tens)
    display.writeDigitNum(4, repCount % 10);        // Right-most digit (Rep ones)
    display.writeDisplay();

    // Ignore small movements (example: bar adjustments)
    if (abs(accelZ - 11.5) < MIN_MOVEMENT) {
        return;
    }

    // Detect upward motion (lifting starts)
    if (accelZ < LIFT_THRESHOLD && !lifting) {
        repStartTime = currentTime;  // Start counting time
        lifting = true;
        lowering = false;
        buzzedThisRep = false;  // Reset buzz state for new rep
    }

    // Buzz when TUT reaches threshold DURING the rep
    if (lifting && !buzzedThisRep && (currentTime - repStartTime) >= BUZZ_THRESHOLD) {
        Serial.println("🔔 Buzzing! Time Under Tension reached!");
        analogWrite(MOTOR_PIN, 150);  // Vibrate at medium intensity (PWM 0-255)
        delay(500);  // Vibrate for 0.5s
        analogWrite(MOTOR_PIN, 0);  // Turn off vibration
        buzzedThisRep = true;  // Prevent multiple buzzes per rep
    }

    // Detect downward motion (lowering starts)
    if (lifting && accelZ > LOWER_THRESHOLD) {
        lowering = true;
    }

    // Count rep when full cycle (up + down) is completed
    if (lifting && lowering) {
        repDuration = millis() - repStartTime;  // Calculate total TUT

        // Ensure rep is valid (rep time must be >= MIN_REP_TIME)
        if (repDuration >= MIN_REP_TIME) {
            repCount++;
            Serial.print("✅ Rep Count: ");
            Serial.println(repCount);
            Serial.print("⏱ Rep Duration: ");
            Serial.print(repDuration / 1000.0);
            Serial.println(" sec");
        }

        // Reset for next rep
        lifting = false;
        lowering = false;
    }

    delay(100);
}
