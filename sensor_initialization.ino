#include <HX711.h>

// ----------- Sensor Pins -------------
#define FSR_PIN           A0      // Force Sensor
#define LM35_PIN          A1      // LM35 Temperature Sensor
#define MQ2_PIN           A2      // MQ2 Gas Sensor (Analog)
#define FLAME_SENSOR_PIN  2       // Flame Sensor (Digital)
#define TRIG_PIN          9       // Ultrasonic TRIG
#define ECHO_PIN          10      // Ultrasonic ECHO
#define BUZZER_PIN        8       // Buzzer

// ----------- MQ2 Gas Sensor Constants -------------
#define RL_VALUE                  5.0   // kΩ
#define RO_CLEAN_AIR_FACTOR       9.83
#define CALIBRATION_SAMPLE_TIMES  50
#define CALIBRATION_SAMPLE_INTERVAL 500
#define READ_SAMPLE_TIMES         5
#define READ_SAMPLE_INTERVAL      50

#define GAS_LPG   0
#define GAS_CO    1
#define GAS_SMOKE 2

float LPGCurve[3]   = {2.3, 0.21, -0.47};
float COCurve[3]    = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro = 10; // Initial estimation

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);

  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Calibrating MQ2 gas sensor...");
  Ro = MQCalibration(MQ2_PIN);
  Serial.print("Calibration complete. Ro = ");
  Serial.print(Ro);
  Serial.println(" kΩ");
}

// ---------- Main Loop ----------
void loop() {
  readForceSensor();
  readTemperature();
  readFlameSensor();
  readGasSensor();
  float distance = readUltrasonic();

  controlBuzzer(distance);

  delay(1000); // Delay between sensor reads
}

// ========== SENSOR FUNCTIONS ==========

void readForceSensor() {
  int force = analogRead(FSR_PIN);
  Serial.print("FSR = ");
  Serial.print(force);
  Serial.print(" => ");
  if (force > 100) {
    Serial.print("Helmet Worn");
  } else {
    Serial.print("Helmet Not Worn");
  }
  Serial.print(" | ");
}

void readTemperature() {
  int adc = analogRead(LM35_PIN);
  float temperature = (adc * 5.0 * 100.0) / 1023.0;
  Serial.print("Temp = ");
  Serial.print(temperature);
  Serial.print(" °C | ");
}

void readFlameSensor() {
  int status = digitalRead(FLAME_SENSOR_PIN);
  Serial.print("Flame = ");
  Serial.print(status == HIGH ? "DETECTED" : "SAFE");
  Serial.print(" | ");
}

void readGasSensor() {
  float rs = MQRead(MQ2_PIN) / Ro;

  int lpg = MQGetGasPercentage(rs, GAS_LPG);
  int co = MQGetGasPercentage(rs, GAS_CO);
  int smoke = MQGetGasPercentage(rs, GAS_SMOKE);

  Serial.print("LPG = ");
  Serial.print(lpg);
  Serial.print("ppm, CO = ");
  Serial.print(co);
  Serial.print("ppm, Smoke = ");
  Serial.print(smoke);
  Serial.println("ppm");
}

float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.0343) / 2; // cm

  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void controlBuzzer(float distance) {
  if (distance < 70) {
    tone(BUZZER_PIN, 1000); // Alert tone
  } else {
    noTone(BUZZER_PIN);
  }
}

// ========== MQ2 SUPPORT FUNCTIONS ==========

float MQResistanceCalculation(int raw_adc) {
  return (RL_VALUE * (1023.0 - raw_adc)) / raw_adc;
}

float MQCalibration(int mq_pin) {
  float val = 0;
  for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val /= CALIBRATION_SAMPLE_TIMES;
  val /= RO_CLEAN_AIR_FACTOR;
  return val;
}

float MQRead(int mq_pin) {
  float rs = 0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  return rs / READ_SAMPLE_TIMES;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  switch (gas_id) {
    case GAS_LPG: return MQGetPercentage(rs_ro_ratio, LPGCurve);
    case GAS_CO: return MQGetPercentage(rs_ro_ratio, COCurve);
    case GAS_SMOKE: return MQGetPercentage(rs_ro_ratio, SmokeCurve);
    default: return 0;
  }
}

int MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return pow(10, (((log10(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0]));
}
