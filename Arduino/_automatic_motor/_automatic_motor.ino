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
const int STOP_THROTTLE = 1000;   // 1000 us: Motor stop / ESC low endpoint
// 1310 us: Low-RPM ground idle (just above your 1300 us threshold, zero lift)
const int SAFE_PROP_IDLE = 1301;  

void setAll(int us) {
  esc1.writeMicroseconds(us);
  esc2.writeMicroseconds(us);
  esc3.writeMicroseconds(us);
  esc4.writeMicroseconds(us);
}

void setup() {
  Serial.begin(115200);

  // Standard 50 Hz PWM servo pulse configuration
  esc1.setPeriodHertz(50);
  esc2.setPeriodHertz(50);
  esc3.setPeriodHertz(50);
  esc4.setPeriodHertz(50);

  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  esc3.attach(ESC3_PIN, 1000, 2000);
  esc4.attach(ESC4_PIN, 1000, 2000);

  // Initialize all ESCs at stop throttle
  setAll(STOP_THROTTLE);
  
  Serial.println("\n===========================================");
  Serial.println("  SAFE PROP-ON GROUND IDLE VERIFICATION    ");
  Serial.println("===========================================");
  Serial.println("Starting 6-second safety countdown...");
  Serial.println("Ensure everyone is clear of propeller arcs.");
  
  // 6-second delay to connect battery and step back safely
  delay(6000);
}

// Helper function: Pulse a single motor at safe idle
void pulsePropMotor(Servo &esc, const char *motor_name, int duration_ms) {
  Serial.print(">> Spooling up ");
  Serial.print(motor_name);
  Serial.print(" at safe idle (");
  Serial.print(SAFE_PROP_IDLE);
  Serial.println(" us)...");

  esc.writeMicroseconds(SAFE_PROP_IDLE);
  delay(duration_ms);
  esc.writeMicroseconds(STOP_THROTTLE);
  
  Serial.print(">> ");
  Serial.print(motor_name);
  Serial.println(" stopped.");
  delay(2000); // 2-second pause to inspect
}

void loop() {
  Serial.println("\n--- Starting Sequential Propeller Sweep ---");

  // 1. Motor 1 (2 seconds)
  pulsePropMotor(esc1, "Motor 1 (Front-Left)", 2000);

  // 2. Motor 2 (2 seconds)
  pulsePropMotor(esc2, "Motor 2 (Front-Right)", 2000);

  // 3. Motor 3 (2 seconds)
  pulsePropMotor(esc3, "Motor 3 (Rear-Right)", 2000);

  // 4. Motor 4 (2 seconds)
  pulsePropMotor(esc4, "Motor 4 (Rear-Left)", 2000);

  // Synchronized all-motor test
  Serial.println("\n>> Spooling ALL 4 PROPS simultaneously (Zero-Lift Idle: 2 sec)...");
  setAll(SAFE_PROP_IDLE);
  delay(2000);
  setAll(STOP_THROTTLE);
  Serial.println(">> ALL MOTORS CUT OFF.");

  // 8-second rest before repeating
  Serial.println("\nCycle finished. Pausing 8 seconds...");
  delay(8000);
}