// self watering pot
// nano every + dfrobot SEN0193 moisture sensor + adafruit 3v pump (4547)
// pump goes thru a DFR0457 mosfet module on D3
//
// the hollow ring only holds ~30mL and the pump fills it way faster than
// the 2.5mm holes drain, so if you just run the pump till the soil reads wet
// it overflows and floods one side. so instead it does pulses:
//   fill ring -> wait for it to drain -> repeat -> wait for water to soak
//   down to the sensor -> check again
//
// serial cmds (9600 baud):
//   r = read sensor   t = time how long the ring takes to fill
//   p = one pulse     w = water now
//   c = clear error   s = status

#define PUMP_PIN 3
#define SENSOR_PIN A0
#define SENSOR_PWR 7    // sensor only gets power when reading

// calibration, use 'r' w/ sensor in air (dry) and in a cup of water (wet)
int dryRaw = 520;
int wetRaw = 260;

int dryPct = 35;   // start watering under this
int wetPct = 55;   // stop at this. gap so it doesnt flip flop

unsigned long checkEvery = 30UL * 60 * 1000;  // 30 min
unsigned long fillTime = 1500;   // ms to fill the ring once, get this from 't'
unsigned long drainTime = 30000; // let ring empty before next pulse
unsigned long soakTime = 10UL * 60 * 1000;   // 10 min for water to reach sensor
int pumpPower = 150;  // pump is 3V, we run on 5V. 150/255*5 = ~3V
int pulsesPerRound = 2;

int maxPulsesDay = 12;   // ~360mL a day max, in case the sensor dies
int maxNoRise = 3;       // rounds w/ no change before i assume cup is empty
int minRise = 3;         // % rise that counts as "it worked"

// states
#define IDLE 0
#define PUMPING 1
#define DRAINING 2
#define SOAKING 3
#define ERR 4
int state = IDLE;

unsigned long stateStart = 0;
unsigned long lastCheck = 0;
unsigned long dayStart = 0;
bool firstCheck = true;
int pulsesLeft = 0;
int pulsesToday = 0;
int noRiseCount = 0;
int startPct = 0;  // moisture when the round started

void pumpOn() { analogWrite(PUMP_PIN, pumpPower); }
void pumpOff() { analogWrite(PUMP_PIN, 0); }

int readRaw() {
  digitalWrite(SENSOR_PWR, HIGH);
  delay(200); // give it a sec to settle
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(SENSOR_PIN);
    delay(10);
  }
  digitalWrite(SENSOR_PWR, LOW);
  return total / 10;
}

int readPct() {
  int raw = readRaw();
  // capacitive sensors read HIGHER when dry so its flipped
  int pct = constrain(map(raw, dryRaw, wetRaw, 0, 100), 0, 100);
  Serial.print("Moisture: ");
  Serial.print(pct);
  Serial.print("%  (raw ");
  Serial.print(raw);
  Serial.println(")");
  return pct;
}

void setState(int s) {
  state = s;
  stateStart = millis();
}

void startPulse() {
  if (pulsesToday >= maxPulsesDay) {
    Serial.println("Daily limit reached, skipping till tomorrow");
    lastCheck = millis();
    setState(IDLE);
    return;
  }
  pulsesToday++;
  Serial.print("Pulse ");
  Serial.println(pulsesToday);
  pumpOn();
  setState(PUMPING);
}

void startRound(int pct) {
  startPct = pct;
  pulsesLeft = pulsesPerRound;
  Serial.println("Watering...");
  startPulse();
}

// t command. pump runs until you hit enter, then prints how long it took
void timeFill() {
  delay(100); // the newline from typing 't' shows up a bit later, wait for it
  while (Serial.available()) Serial.read();
  Serial.println("Pump on, hit enter when the ring is full");
  unsigned long t0 = millis();
  pumpOn();
  while (!Serial.available()) {
    if (millis() - t0 > 20000) break; // dont run forever if i walk away
  }
  pumpOff();
  unsigned long took = millis() - t0;
  while (Serial.available()) Serial.read();
  Serial.print("Fill time: ");
  Serial.print(took);
  Serial.println(" ms, put this in fillTime");
}

void checkSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();

  if (c == 'r') readPct();
  if (c == 't' && (state == IDLE || state == ERR)) timeFill();
  if (c == 'p' && state == IDLE) {
    Serial.println("one pulse");
    pumpOn();
    delay(fillTime);
    pumpOff();
  }
  if (c == 'w' && state == IDLE) startRound(readPct());
  if (c == 'c' && state == ERR) {
    noRiseCount = 0;
    pulsesToday = 0;  // cup was just refilled, the dry pulses shouldnt count
    lastCheck = millis();
    Serial.println("Error cleared");
    setState(IDLE);
  }
  if (c == 's') {
    Serial.print("State: ");
    Serial.print(state);
    Serial.print("  pulses today: ");
    Serial.println(pulsesToday);
    readPct();
  }
}

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(SENSOR_PWR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pumpOff();
  Serial.begin(9600);
  Serial.println("pot ready. cmds: r t p w c s");
  dayStart = millis();
}

void loop() {
  unsigned long now = millis();
  checkSerial();

  // new day, reset the counter
  if (now - dayStart >= 24UL * 60 * 60 * 1000) {
    dayStart = now;
    pulsesToday = 0;
  }

  if (state == IDLE) {
    if (firstCheck || now - lastCheck >= checkEvery) {
      firstCheck = false;
      lastCheck = now;
      int pct = readPct();
      if (pct < dryPct) startRound(pct);
    }
  }
  else if (state == PUMPING) {
    if (now - stateStart >= fillTime) {
      pumpOff();
      setState(DRAINING);
    }
  }
  else if (state == DRAINING) {
    if (now - stateStart >= drainTime) {
      pulsesLeft--;
      if (pulsesLeft > 0) startPulse();
      else setState(SOAKING);
    }
  }
  else if (state == SOAKING) {
    if (now - stateStart >= soakTime) {
      int pct = readPct();
      if (pct >= wetPct) {
        Serial.println("Soil is wet enough, done");
        noRiseCount = 0;
        lastCheck = millis();
        setState(IDLE);
      } else {
        if (pct - startPct < minRise) noRiseCount++;
        else noRiseCount = 0;

        if (noRiseCount >= maxNoRise) {
          // 3 rounds and nothing changed, probably out of water
          pumpOff();
          Serial.println("ERROR: moisture isnt going up, cup empty or tube came out?");
          Serial.println("refill and send c");
          setState(ERR);
        } else {
          startRound(pct);  // still dry, go again
        }
      }
    }
  }
  else if (state == ERR) {
    digitalWrite(LED_BUILTIN, (now / 250) % 2); // blink fast
    return;
  }

  digitalWrite(LED_BUILTIN, state == PUMPING); // led on while pumping
  // Serial.println(state);
}
