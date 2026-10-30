/*
  Self-Watering Pot
  -----------------
  Board:   Arduino Nano (any ATmega328P board works: Uno, Pro Mini 5V, etc.)
  Sensor:  Capacitive soil moisture sensor (v1.2 / v2.0)
  Pump:    3-5 V mini DC water pump, switched by a logic-level N-MOSFET

  How it waters:
  The hollow channel around the rim holds about 30 mL, and the pump fills it
  much faster than the drip holes drain it. Running the pump until the soil
  reads wet would overflow the channel and flood one side of the pot, so the
  code waters in pulses:

    1. Run the pump just long enough to fill the channel once  (FILL_MS)
    2. Stop and let the channel drain through the drip holes    (DRAIN_MS)
    3. Repeat a few times                                        (PULSES_PER_ROUND)
    4. Wait for the water to soak down to the sensor             (SOAK_MS)
    5. Check the soil again. If it's still dry, do another round.

  Safety:
  - A daily pulse cap stops a bad sensor from emptying the cup onto your table.
  - If several rounds in a row don't raise the moisture reading, it assumes
    the cup is empty or the tube fell out, stops, and blinks the LED.

  Serial commands (Serial Monitor, 9600 baud, "Newline" line ending):
    r  read the sensor (raw value and percent), used for calibration
    t  timing mode: the pump runs until you send any key, then prints the time.
       Watch the channel, press Enter the moment it's full, and put the
       number in FILL_MS.
    p  run one pulse (FILL_MS)
    w  start a watering round now
    c  clear the error state
    s  print status
*/

// ---------------- Pins ----------------
const uint8_t PUMP_PIN         = 3;   // PWM pin -> MOSFET gate
const uint8_t SENSOR_PIN       = A0;  // sensor AOUT
const uint8_t SENSOR_POWER_PIN = 7;   // sensor VCC (only powered while reading)
const uint8_t LED_PIN          = LED_BUILTIN;

// ---------------- Sensor calibration ----------------
// DRY_RAW: send 'r' with the sensor in dry air
// WET_RAW: send 'r' with the sensor standing in a glass of water up to its line
int DRY_RAW = 520;
int WET_RAW = 260;

// ---------------- Watering behaviour ----------------
const uint8_t DRY_PERCENT = 35;   // start watering below this
const uint8_t WET_PERCENT = 55;   // stop once it reaches this (the gap keeps it from flip-flopping)

const unsigned long CHECK_INTERVAL_MS = 30UL * 60UL * 1000UL; // check soil every 30 min
const unsigned long FILL_MS           = 1500;                  // pump time to fill channel once (calibrate with 't')
const uint8_t       PUMP_POWER        = 255;                   // 0-255, lower = slower fill (some pumps stall below ~150)
const unsigned long DRAIN_MS          = 30UL * 1000UL;         // let the channel empty between pulses
const uint8_t       PULSES_PER_ROUND  = 2;                     // ~60 mL per round
const unsigned long SOAK_MS           = 10UL * 60UL * 1000UL;  // wait for water to reach the sensor

const uint8_t MAX_PULSES_PER_DAY   = 12;  // ~360 mL/day hard limit
const uint8_t MAX_ROUNDS_NO_CHANGE = 3;   // rounds with no moisture rise before "cup empty" error
const uint8_t MIN_RISE_PERCENT     = 3;   // what counts as "the soil got wetter"

// ---------------- State ----------------
enum State { IDLE, PUMPING, DRAINING, SOAKING, ERROR_STATE };
State state = IDLE;

unsigned long stateStart    = 0;
unsigned long lastCheck     = 0;
unsigned long dayStart      = 0;
bool          firstCheck    = true;
uint8_t       pulsesLeft    = 0;
uint8_t       pulsesToday   = 0;
uint8_t       roundsNoRise  = 0;
int           roundBaseline = 0;

// ---------------- Pump ----------------
void pumpOn()  { analogWrite(PUMP_PIN, PUMP_POWER); }
void pumpOff() { analogWrite(PUMP_PIN, 0); }

// ---------------- Sensor ----------------
int readRaw() {
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(200);                       // let the sensor settle
  long total = 0;
  for (uint8_t i = 0; i < 10; i++) {
    total += analogRead(SENSOR_PIN);
    delay(10);
  }
  digitalWrite(SENSOR_POWER_PIN, LOW);
  return total / 10;
}

int rawToPercent(int raw) {
  // Capacitive sensors read higher when dry, so the range is inverted
  return constrain(map(raw, DRY_RAW, WET_RAW, 0, 100), 0, 100);
}

int readPercent() {
  int raw = readRaw();
  int pct = rawToPercent(raw);
  Serial.print(F("Moisture: "));
  Serial.print(pct);
  Serial.print(F("%  (raw "));
  Serial.print(raw);
  Serial.println(F(")"));
  return pct;
}

// ---------------- Watering steps ----------------
void enter(State s) {
  state = s;
  stateStart = millis();
}

void startPulse() {
  if (pulsesToday >= MAX_PULSES_PER_DAY) {
    Serial.println(F("Daily limit reached, skipping until tomorrow."));
    lastCheck = millis();
    enter(IDLE);
    return;
  }
  pulsesToday++;
  Serial.print(F("Pulse "));
  Serial.print(pulsesToday);
  Serial.println(F(" today"));
  pumpOn();
  enter(PUMPING);
}

void startRound(int baseline) {
  roundBaseline = baseline;
  pulsesLeft = PULSES_PER_ROUND;
  Serial.println(F("Watering round started."));
  startPulse();
}

void finishWatering() {
  roundsNoRise = 0;
  lastCheck = millis();
  Serial.println(F("Soil is wet enough. Done."));
  enter(IDLE);
}

void enterError(const __FlashStringHelper* why) {
  pumpOff();
  Serial.print(F("ERROR: "));
  Serial.println(why);
  Serial.println(F("Refill the cup / check the tube, then send 'c'."));
  enter(ERROR_STATE);
}

// ---------------- Serial commands ----------------
void timingMode() {
  while (Serial.available()) Serial.read();   // clear buffer
  Serial.println(F("Pump ON. Press Enter the moment the channel is full..."));
  unsigned long t0 = millis();
  pumpOn();
  while (!Serial.available()) {
    if (millis() - t0 > 20000UL) break;       // safety stop after 20 s
  }
  pumpOff();
  unsigned long elapsed = millis() - t0;
  while (Serial.available()) Serial.read();
  Serial.print(F("Fill time: "));
  Serial.print(elapsed);
  Serial.println(F(" ms  -> set FILL_MS to this"));
}

void printStatus() {
  const char* names[] = {"IDLE", "PUMPING", "DRAINING", "SOAKING", "ERROR"};
  Serial.print(F("State: "));
  Serial.println(names[state]);
  Serial.print(F("Pulses today: "));
  Serial.print(pulsesToday);
  Serial.print(F(" / "));
  Serial.println(MAX_PULSES_PER_DAY);
  readPercent();
}

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'r': readPercent(); break;
    case 't': if (state == IDLE || state == ERROR_STATE) timingMode(); break;
    case 'p':
      if (state == IDLE) {
        Serial.println(F("Single pulse."));
        pumpOn(); delay(FILL_MS); pumpOff();
      }
      break;
    case 'w': if (state == IDLE) startRound(readPercent()); break;
    case 'c':
      if (state == ERROR_STATE) {
        roundsNoRise = 0;
        Serial.println(F("Error cleared."));
        lastCheck = millis();
        enter(IDLE);
      }
      break;
    case 's': printStatus(); break;
  }
}

// ---------------- Setup / loop ----------------
void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pumpOff();
  Serial.begin(9600);
  Serial.println(F("Self-watering pot ready. Commands: r t p w c s"));
  dayStart = millis();
}

void loop() {
  unsigned long now = millis();
  handleSerial();

  // Reset the daily pulse counter every 24 h
  if (now - dayStart >= 24UL * 60UL * 60UL * 1000UL) {
    dayStart = now;
    pulsesToday = 0;
  }

  switch (state) {
    case IDLE:
      if (firstCheck || now - lastCheck >= CHECK_INTERVAL_MS) {
        firstCheck = false;
        lastCheck = now;
        int pct = readPercent();
        if (pct < DRY_PERCENT) startRound(pct);
      }
      break;

    case PUMPING:
      if (now - stateStart >= FILL_MS) {
        pumpOff();
        enter(DRAINING);
      }
      break;

    case DRAINING:
      if (now - stateStart >= DRAIN_MS) {
        pulsesLeft--;
        if (pulsesLeft > 0) startPulse();
        else if (state == DRAINING) enter(SOAKING);  // startPulse may have hit the daily cap
      }
      break;

    case SOAKING:
      if (now - stateStart >= SOAK_MS) {
        int pct = readPercent();
        if (pct >= WET_PERCENT) {
          finishWatering();
        } else {
          if (pct - roundBaseline < MIN_RISE_PERCENT) roundsNoRise++;
          else roundsNoRise = 0;

          if (roundsNoRise >= MAX_ROUNDS_NO_CHANGE) {
            enterError(F("moisture isn't rising - cup empty or tube loose?"));
          } else {
            startRound(pct);
          }
        }
      }
      break;

    case ERROR_STATE:
      digitalWrite(LED_PIN, (now / 250) % 2);   // fast blink
      return;
  }
  digitalWrite(LED_PIN, state == PUMPING);       // LED on while pumping
}
