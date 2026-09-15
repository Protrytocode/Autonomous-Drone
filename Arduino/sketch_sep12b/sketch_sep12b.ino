#include <ESP32Servo.h>

Servo esc1;
Servo esc2;
Servo esc3;
Servo esc4;

// Motor Pin Assignments
const int ESC1_PIN = 25;
const int ESC2_PIN = 26;
const int ESC3_PIN = 27;
const int ESC4_PIN = 14;

// Throttle Calibration Constants
const int STOP_THROTTLE = 1000;   // 1000 us: Stop / ESC low endpoint
const int SPIN_THROTTLE = 1350;   // 1350 us: Reliable spin threshold

void setAll(int us) {
  esc1.writeMicroseconds(us);
  esc2.writeMicroseconds(us);
  esc3.writeMicroseconds(us);
  esc4.writeMicroseconds(us);
}

void setup() {
  // Allow the ESCs to receive standard 50 Hz PWM servo pulses
  esc1.setPeriodHertz(50);
  esc2.setPeriodHertz(50);
  esc3.setPeriodHertz(50);
  esc4.setPeriodHertz(50);

  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  esc3.attach(ESC3_PIN, 1000, 2000);
  esc4.attach(ESC4_PIN, 1000, 2000);

  // Arming sequence: force low idle endpoint
  setAll(STOP_THROTTLE);
  
  // 5-second initial safety pause after boot/battery connection
  delay(5000);
}

// Helper function: Run a single target motor for exactly duration_ms
void pulseMotor(Servo &esc, int pulse_us, int duration_ms) {
  esc.writeMicroseconds(pulse_us);
  delay(duration_ms);
  esc.writeMicroseconds(STOP_THROTTLE);
  delay(1000); // 1-second resting pause between steps
}

void loop() {
  // --- Round-Robin Pattern: 1 Motor at a time (3 seconds each) ---
  
  // 1. Motor 1
  pulseMotor(esc1, SPIN_THROTTLE, 3000);

  // 2. Motor 2
  pulseMotor(esc2, SPIN_THROTTLE, 3000);

  // 3. Motor 3
  pulseMotor(esc3, SPIN_THROTTLE, 3000);

  // 4. Motor 4
  pulseMotor(esc4, SPIN_THROTTLE, 3000);

  // --- Finale: All 4 motors gentle 1.5-second spin ---
  setAll(SPIN_THROTTLE);
  delay(1500);
  setAll(STOP_THROTTLE);

  // 4-second cool-down pause before restarting sequence
  delay(4000);
}
