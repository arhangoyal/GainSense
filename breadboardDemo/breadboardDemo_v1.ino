#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// Rep Tracking Variables
int repCount = 0;
bool lifting = false;
bool lowering = false;
unsigned long repStartTime = 0;
unsigned long repEndTime = 0;

// **Thresholds (Tuned Based on Your Data)**
float LIFT_THRESHOLD = 9.5;   // Detect lifting (Z-axis acceleration drops)
float LOWER_THRESHOLD = 11.5; // Detect lowering (Z-axis acceleration rises)
float MIN_REP_TIME = 700;     // Minimum time for a full rep (milliseconds)
float MIN_MOVEMENT = 3;       // Min movement time for 1/2 rep. Ignore small movements (fine adjustments)

// **Rep Form Variables**
float maxLift = 12.0; // Highest acceleration during rep
float minLift = 9.0;  // Lowest acceleration during rep

// **Vibration Motor Settings**
#define MOTOR_PIN 9  // Motor connected directly to Arduino Pin 9
#define BUZZ_THRESHOLD 5000  // 5 seconds in milliseconds (adjustable)

void setup() {
    Serial.begin(115200);

    // Initialize MPU6050
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

    // Set sensor parameters
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);  // Configure on-chip Digital Low Pass Filter to 21 Hz, which helps in smoothing sensor data by reducing high-frequency noise

    // **Setup Vibration Motor**
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);  // Ensure motor is off at start

    delay(100);
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float accelZ = a.acceleration.z; // Get Z-axis acceleration
    unsigned long currentTime = millis();

    Serial.print("Z Acceleration: ");
    Serial.println(accelZ);

    // Ignore small movements (fine bar adjustments)
    if (abs(accelZ - 11.5) < MIN_MOVEMENT) {
        return;
    }

    // Detect upward motion (lifting)
    if (accelZ < LIFT_THRESHOLD) {
        if (!lifting) {
            repStartTime = currentTime; // Start rep time
            maxLift = accelZ;  // Track highest lift acceleration
        }
        lifting = true;
        lowering = false;  // Reset lowering state
    }

    // Detect downward motion (lowering)
    if (lifting && accelZ > LOWER_THRESHOLD) {
        lowering = true;
        minLift = accelZ; // Track lowest lift acceleration
    }

    // Count rep when full cycle is complete (up + down)
    if (lifting && lowering) {
        repEndTime = currentTime; // End rep time
        unsigned long repDuration = repEndTime - repStartTime;

        // Check if movement time is valid (avoid detecting jerks)
        if (repDuration >= MIN_REP_TIME) {
            repCount++;
            Serial.print("✅ Rep Count: ");
            Serial.println(repCount);
            Serial.print("⏱ Rep Speed: ");
            Serial.print(repDuration / 1000.0);
            Serial.println(" sec");

            // **Detect Poor Form (Partial Rep)**
            if (maxLift > 10.0 || minLift < 9.0) {
                Serial.println("⚠️ Warning: Partial Rep Detected!");
            }

            // **Trigger Vibration if Time Under Tension (Rep Time) Exceeds Threshold**
            if (repDuration >= BUZZ_THRESHOLD) {
                Serial.println("🔔 Buzzing! Time Under Tension achieved!");
                analogWrite(MOTOR_PIN, 150);  // Vibrate at medium intensity (0-255 range)
                delay(500);  // Vibrate for 0.5s
                analogWrite(MOTOR_PIN, 0);  // Turn off vibration
            }

            // Reset for next rep
            lifting = false;
            lowering = false;
        }
    }

    delay(100);
}
