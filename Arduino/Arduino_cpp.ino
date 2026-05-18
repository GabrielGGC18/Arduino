#include <Arduino.h>

// ====== Pinos ======
constexpr uint8_t PIN_PIR     = 7;
constexpr uint8_t PIN_BUZZER  = 8;
constexpr uint8_t PIN_LED     = 13;
constexpr uint8_t PIN_BTN     = 2;  
// ====== Parametros ======
constexpr unsigned long WARMUP_MS         = 30000UL;  
constexpr unsigned long DEBOUNCE_MS       = 50;       
constexpr uint8_t       CONFIRM_SAMPLES   = 3;       
constexpr unsigned long HOLD_MS           = 3000UL; 
constexpr unsigned long COOLDOWN_MS       = 2000UL;   
constexpr unsigned long STUCK_THRESHOLD   = 120000UL;
constexpr unsigned long SILENCE_MS        = 30000UL;  
constexpr unsigned long BEEP_PERIOD_MS    = 500;     
constexpr unsigned long HEARTBEAT_MS      = 5000;    


enum class State : uint8_t { WARMUP, IDLE, ALARM, COOLDOWN, FAULT, SILENCED };

struct Context {
  State    state            = State::WARMUP;
  unsigned long stateEnter  = 0;
  unsigned long lastMotion  = 0;
  unsigned long lastDebounce= 0;
  unsigned long lastBeep    = 0;
  unsigned long lastHeartbeat = 0;
  unsigned long highSince   = 0;
  unsigned long silenceUntil= 0;
  uint8_t  rawLast          = LOW;
  uint8_t  rawStable        = LOW;
  uint8_t  confirmCount     = 0;
  uint32_t eventCount       = 0;
  bool     beepPhase        = false;
} ctx;

// ====== Helpers ======
void transition(State next) {
  ctx.state = next;
  ctx.stateEnter = millis();
  ctx.beepPhase = false;
  digitalWrite(PIN_BUZZER, LOW);
  switch (next) {
    case State::WARMUP:   Serial.println(F("[STATE] WARMUP")); digitalWrite(PIN_LED, LOW); break;
    case State::IDLE:     Serial.println(F("[STATE] IDLE"));   digitalWrite(PIN_LED, LOW); break;
    case State::ALARM:    Serial.println(F("[STATE] ALARM"));  digitalWrite(PIN_LED, HIGH); break;
    case State::COOLDOWN: Serial.println(F("[STATE] COOLDOWN")); digitalWrite(PIN_LED, LOW); break;
    case State::FAULT:    Serial.println(F("[STATE] FAULT (sensor preso)")); break;
    case State::SILENCED: Serial.println(F("[STATE] SILENCED")); digitalWrite(PIN_LED, LOW); break;
  }
}

uint8_t readPirDebounced(unsigned long now) {
  uint8_t raw = digitalRead(PIN_PIR);
  if (raw != ctx.rawLast) {
    ctx.lastDebounce = now;
    ctx.rawLast = raw;
  }
  if ((now - ctx.lastDebounce) >= DEBOUNCE_MS && raw != ctx.rawStable) {
    ctx.rawStable = raw;
  }
  return ctx.rawStable;
}

bool confirmMotion(uint8_t stable) {
  if (stable == HIGH) {
    if (ctx.confirmCount < 255) ctx.confirmCount++;
  } else {
    ctx.confirmCount = 0;
  }
  return ctx.confirmCount >= CONFIRM_SAMPLES;
}

void handleBeep(unsigned long now) {
  if ((now - ctx.lastBeep) >= BEEP_PERIOD_MS) {
    ctx.lastBeep = now;
    ctx.beepPhase = !ctx.beepPhase;
    digitalWrite(PIN_BUZZER, ctx.beepPhase ? HIGH : LOW);
    digitalWrite(PIN_LED,    ctx.beepPhase ? HIGH : LOW);
  }
}

void checkStuckSensor(unsigned long now, uint8_t stable) {
  if (stable == HIGH) {
    if (ctx.highSince == 0) ctx.highSince = now;
    if ((now - ctx.highSince) >= STUCK_THRESHOLD && ctx.state != State::FAULT) {
      transition(State::FAULT);
    }
  } else {
    ctx.highSince = 0;
  }
}

bool buttonPressedEdge() {
  static uint8_t last = HIGH;
  static unsigned long lastChange = 0;
  uint8_t cur = digitalRead(PIN_BTN);
  unsigned long now = millis();
  if (cur != last && (now - lastChange) > 30) {
    lastChange = now;
    last = cur;
    if (cur == LOW) return true;
  }
  last = cur;
  return false;
}

void heartbeat(unsigned long now) {
  if ((now - ctx.lastHeartbeat) >= HEARTBEAT_MS) {
    ctx.lastHeartbeat = now;
    Serial.print(F("[HB] state="));
    Serial.print((uint8_t)ctx.state);
    Serial.print(F(" events="));
    Serial.print(ctx.eventCount);
    Serial.print(F(" uptime_s="));
    Serial.println(now / 1000UL);
  }
}

// ====== Setup ======
void setup() {
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(9600);
  while (!Serial && millis() < 2000) {}
  Serial.println(F("=== PIR Alarm v2 ==="));
  transition(State::WARMUP);
}


void loop() {
  unsigned long now = millis();
  uint8_t stable = readPirDebounced(now);
  heartbeat(now);
  checkStuckSensor(now, stable);

  if (buttonPressedEdge()) {
    if (ctx.state == State::FAULT) {
      ctx.highSince = 0;
      transition(State::IDLE);
    } else {
      ctx.silenceUntil = now + SILENCE_MS;
      transition(State::SILENCED);
    }
  }

  switch (ctx.state) {
    case State::WARMUP:
      if ((now - ctx.stateEnter) >= WARMUP_MS) {
        transition(State::IDLE);
      }
      break;

    case State::IDLE:
      if (confirmMotion(stable)) {
        ctx.eventCount++;
        ctx.lastMotion = now;
        Serial.print(F("[EVT] motion #"));
        Serial.println(ctx.eventCount);
        transition(State::ALARM);
      }
      break;

    case State::ALARM:
      if (stable == HIGH) ctx.lastMotion = now;
      handleBeep(now);
      if ((now - ctx.lastMotion) >= HOLD_MS) {
        transition(State::COOLDOWN);
      }
      break;

    case State::COOLDOWN:
      if ((now - ctx.stateEnter) >= COOLDOWN_MS) {
        ctx.confirmCount = 0;
        transition(State::IDLE);
      }
      break;

    case State::SILENCED:
      if (now >= ctx.silenceUntil) {
        transition(State::IDLE);
      }
      break;

    case State::FAULT:
      // pisca lento p/ sinalizar falha
      if ((now - ctx.lastBeep) >= 1000) {
        ctx.lastBeep = now;
        ctx.beepPhase = !ctx.beepPhase;
        digitalWrite(PIN_LED, ctx.beepPhase ? HIGH : LOW);
      }
      break;
  }
}
