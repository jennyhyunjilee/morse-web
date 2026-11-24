// =============================================================
//  모스부호기 + STILL HERE (원할 때만 켜기)
// -------------------------------------------------------------
//  전원을 켜면 [모스부호기]로 시작해요.
//    점 버튼(2번) = 점(.) 노랑 · 대시 버튼(3번) 꾹 = 대시(-) 빨강 · 잠시 쉬면 글자 확정(초록 깜빡)
//    (버튼이 1개면 TWO_BUTTONS = false: 짧게 = 점, 길게 = 대시)
//    Serial Monitor에 영어를 쓰고 Enter → 모스부호로 자동 송신
//
//  [STILL HERE 켜기]
//    점 버튼을 3초 동안 꾹 누르고 있기 (1.5초부터 초록이 깜빡이며 세어 줘요)
//    또는 Serial Monitor에 still 입력
//
//  [STILL HERE 도중]
//    버튼을 잠깐 누르고 있기 = 그 자리에서 멈춤 (여백). 떼면 이어서.
//    버튼을 3초 꾹 = 끄고 모스부호기로 돌아가기
//    또는 Serial Monitor에 stop 입력
//    끝까지 다 나오면 저절로 모스부호기로 돌아가요.
// =============================================================

// ---------- 핀 번호 ----------
const int BUTTON_PIN = 2;   // 점 버튼 (버튼이 1개면 점·대시 둘 다)
const int DASH_BUTTON_PIN = 3;  // 대시 버튼 (두 번째 버튼, 다른 쪽 다리는 GND)
const int DOT_LED    = 5;   // 노랑 = 점 (서서히 밝아지기 가능)
const int DASH_LED   = 6;   // 빨강 = 대시 (서서히 밝아지기 가능)
const int LETTER_LED = 7;   // 초록 = 글자 확정
const int SPEAKER_PIN = 11;  // 부저 + 다리 (예전 8번 → 11번으로 옮겨요)
const int KNOB_PIN   = A0;

// ---------- 설정 (여기만 바꾸면 됨) ----------
const bool USE_KNOB      = true;  // 회전 센서(A0)로 소리 크기 조절. 센서가 없으면 false
const bool BUZZER_ON     = true;  // false면 부저는 조용 (소리는 웹사이트에서만)
const bool BUZZER_ACTIVE = false; // 음 높이가 안 바뀌는 부저(능동 부저)면 true
const bool TWO_BUTTONS   = true;  // 버튼 2개: 2번 = 점, 3번 = 대시(꾹). false면 버튼 1개로 짧게/길게
const int  SOUND_STYLE   = 1;     // 소리: 0 = 삐 · 1 = 하프 · 2 = 피아노 · 3 = 화음(도·미·솔)
const int  VOLUME        = 5;     // 소리 크기: 1(아주 작게) ~ 10(가장 크게). 회전 센서를 쓰면 센서가 정해요
const int  BASE_FREQ     = 660;   // 모스부호기 소리 높이 (Hz)

const bool STILL_1MIN    = true;  // STILL HERE 길이: true = 약 1분, false = 약 84초
const bool STILL_REPEAT  = false; // true면 끌 때까지 STILL HERE를 계속 반복
const unsigned long SWITCH_HOLD = 3000;   // 이만큼(ms) 꾹 누르면 모드 바꾸기

// 모스부호기
const unsigned long DASH_TIME  = 250;   // 이보다 오래 누르면 대시
const unsigned long LETTER_GAP = 800;   // 글자 확정까지 가장 길게 기다리는 시간 (실제로는 내 치는 속도에 맞춰 짧아져요)
const unsigned long WORD_GAP   = 2000;  // 손 떼고 이만큼 지나면 띄어쓰기
const unsigned long NEW_GAP    = 5000;  // 손 떼고 이만큼 지나면 문장 끝 → 새로 시작
const unsigned long UNIT       = 120;   // 자동 송신 점 길이
const unsigned long DEBOUNCE   = 20;

// STILL HERE
const char STILL_MESSAGE[] = "STILL HERE";
const int  STILL_FREQ      = 660;                                    // 신호 = 미
const int  EXCESS_FREQ[5]  = { 784, 880, 1047, 1175, 1319 };          // 솔 라 도 레 미
const int  SCALE[10]       = { 220, 262, 294, 330, 392, 440, 523, 587, 659, 784 };

// ---------- 모스부호 표 ----------
const char* MORSE[36] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

const char* morseFor(char c) {
  if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
  if (c >= 'A' && c <= 'Z') return MORSE[c - 'A'];
  if (c >= '0' && c <= '9') return MORSE[26 + (c - '0')];
  return 0;
}
char decode(const String& code) {
  for (int i = 0; i < 36; i++) if (code == MORSE[i]) return i < 26 ? 'A' + i : '0' + (i - 26);
  return '?';
}

// =============================================================
//  소리: 아두이노가 1초에 16000번 계산해서 "줄을 튕기는 소리"를 만들어요
//  (Karplus-Strong 방식). 부저 + 다리는 11번 핀에 꽂아요.
// =============================================================
#define SR      16000      // 1초에 계산하는 횟수
#define VOICES  3          // 동시에 울릴 수 있는 줄 수 (화음용)
#define MAXN    150        // 가장 낮은 음 ≈ 16000 / 150 = 107Hz

volatile int8_t  vbuf[VOICES][MAXN];
volatile uint8_t vlen[VOICES], vidx[VOICES], vDamp[VOICES];
volatile bool    vOn[VOICES];
volatile uint8_t outVol = 5;
volatile bool    beepOn = false;
volatile uint16_t beepInc = 0, beepPhase = 0;
uint8_t  nextVoice = 0;
uint16_t rnd = 0xACE1;

int knobVolume = VOLUME;     // 회전 센서가 정한 소리 크기 (0~10)
int volumeNow() { return USE_KNOB ? knobVolume : VOLUME; }

int8_t noise() { rnd ^= rnd << 7; rnd ^= rnd >> 9; rnd ^= rnd << 8; return (int8_t)(rnd & 0xFF); }

ISR(TIMER1_COMPA_vect) {
  int16_t s = 0;
  for (uint8_t v = 0; v < VOICES; v++) {
    if (!vOn[v]) continue;
    uint8_t i = vidx[v], j = i + 1;
    if (j >= vlen[v]) j = 0;
    int16_t a = vbuf[v][i], b = vbuf[v][j];
    s += a;
    vbuf[v][i] = (int8_t)((((a + b) >> 1) * (int16_t)vDamp[v]) >> 8);   // 줄이 조금씩 잦아들어요
    vidx[v] = j;
  }
  if (beepOn) { beepPhase += beepInc; s += (beepPhase & 0x8000) ? 45 : -45; }
  s = (s * (int16_t)outVol) >> 3;
  if (s > 127) s = 127;
  if (s < -127) s = -127;
  OCR2A = (uint8_t)(s + 128);
}

void soundSetup() {
  pinMode(SPEAKER_PIN, OUTPUT);
  if (BUZZER_ACTIVE) return;
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);   // 11번 핀: 아주 빠른 PWM (62.5kHz, 안 들려요)
  TCCR2B = _BV(CS20);
  OCR2A = 128;
  TCCR1A = 0;                                         // 타이머1: 1초에 16000번 계산
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 16000000UL / SR - 1;
  TIMSK1 |= _BV(OCIE1A);
  outVol = BUZZER_ON ? volumeNow() : 0;
}

// 줄 하나 튕기기
void pluck(int f, uint8_t amp, bool bright) {
  uint16_t n = SR / f;
  if (n > MAXN) n = MAXN;
  if (n < 4) n = 4;
  int8_t tmp[MAXN];
  int16_t prev = 0;
  for (uint16_t i = 0; i < n; i++) {
    int16_t x = (int16_t)noise() * amp / 128;
    if (!bright) { x = (x + prev) / 2; prev = x; }      // 하프: 부드럽게
    tmp[i] = (int8_t)x;
  }
  uint8_t v = nextVoice; nextVoice = (nextVoice + 1) % VOICES;
  noInterrupts();
  vOn[v] = false;
  for (uint16_t i = 0; i < n; i++) vbuf[v][i] = tmp[i];
  vlen[v] = n; vidx[v] = 0;
  vDamp[v] = bright ? 255 : 254;                          // 피아노: 더 오래 · 하프: 조금 빨리
  vOn[v] = true;
  interrupts();
}

bool noteActive = false;
void buzzerOn(int f) {                                     // 소리 시작
  if (BUZZER_ACTIVE) { if (BUZZER_ON && volumeNow() > 0) digitalWrite(SPEAKER_PIN, HIGH); return; }
  noteActive = true;
  if (SOUND_STYLE == 0) { noInterrupts(); beepInc = (uint32_t)f * 65536UL / SR; beepOn = true; interrupts(); }
  else if (SOUND_STYLE == 1) pluck(f, 110, false);                      // 하프
  else if (SOUND_STYLE == 2) pluck(f, 120, true);                       // 피아노
  else { pluck(f, 80, false); pluck(f * 5 / 4, 70, false); pluck(f * 3 / 2, 70, false); }   // 화음 (도·미·솔)
}
void noteRelease() {                                       // 손을 뗐을 때: 삐는 멈추고, 줄 소리는 자연스럽게 잦아들어요
  noteActive = false;
  if (BUZZER_ACTIVE) { digitalWrite(SPEAKER_PIN, LOW); return; }
  beepOn = false;
}
void buzzerOff() {                                         // 즉시 모두 끄기 (임계점 끊김, 여백)
  noteActive = false;
  if (BUZZER_ACTIVE) { digitalWrite(SPEAKER_PIN, LOW); return; }
  noInterrupts();
  beepOn = false;
  for (uint8_t v = 0; v < VOICES; v++) vOn[v] = false;
  interrupts();
}
void updateSound() {}                                      // (예전 코드와 맞추려고 남겨 둠)
void soundDelay(unsigned long ms) { delay(ms); }

// 회전 센서: 돌리면 소리 크기가 바뀌고, Serial Monitor에 숫자와 막대로 보여줘요
int lastKnobRaw = -100;
void printKnob(int raw) {
  Serial.print(F("\n[회전 센서] 소리 크기 "));
  Serial.print(knobVolume);
  Serial.print(F("/10  "));
  for (int i = 1; i <= 10; i++) Serial.print(i <= knobVolume ? F("■") : F("□"));
  Serial.print(F("  (센서 값 "));
  Serial.print(raw);
  Serial.println(F(" / 1023)"));
}
void readKnob() {
  static unsigned long last = 0;
  if (!USE_KNOB || millis() - last < 60) return;
  last = millis();
  int raw = analogRead(KNOB_PIN);
  if (abs(raw - lastKnobRaw) < 15) return;          // 조금 흔들리는 건 무시
  lastKnobRaw = raw;
  int v = (raw + 51) / 102;                           // 0~1023 → 0~10
  if (v > 10) v = 10;
  if (v == knobVolume) return;
  knobVolume = v;
  printKnob(raw);
  outVol = BUZZER_ON ? knobVolume : 0;               // 바로 반영
}

void allOff() {
  buzzerOff();
  digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW); digitalWrite(LETTER_LED, LOW);
}
// 모드가 바뀔 때 초록 3번 깜빡
void cue() {
  allOff();
  for (int i = 0; i < 3; i++) { digitalWrite(LETTER_LED, HIGH); delay(120); digitalWrite(LETTER_LED, LOW); delay(120); }
}

// Serial Monitor에서 한 줄 받기 (안 끝났으면 false)
String serialLine = "";
bool readLine(String& out) {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLine.length() == 0) continue;
      out = serialLine; out.trim(); serialLine = "";
      return true;
    }
    if (serialLine.length() < 60) serialLine += c;
  }
  return false;
}

// =============================================================
//  STILL HERE
// =============================================================
bool stopNow = false;
int  dotLevel = 0, dashLevel = 0, soundFreq = 0;
bool letterOn = false;

void setLevel(int pin, int v) {
  if (stopNow) return;
  if (pin == DOT_LED) dotLevel = v; else dashLevel = v;
  analogWrite(pin, v);
}
void setLetter(bool on) { if (stopNow) return; letterOn = on; digitalWrite(LETTER_LED, on ? HIGH : LOW); }
void sound(int f) { if (stopNow) return; soundFreq = f; buzzerOn(f); }
void quiet()      { soundFreq = 0; noteRelease(); }

// 버튼: 잠깐 누르고 있으면 멈춤(여백), 3초 넘게 누르면 끄기. 멈춘 시간(ms)을 돌려줘요.
bool anyDown() { return digitalRead(BUTTON_PIN) == LOW || (TWO_BUTTONS && digitalRead(DASH_BUTTON_PIN) == LOW); }

unsigned long holdIfPressed() {
  if (!anyDown()) return 0;
  unsigned long t0 = millis();
  analogWrite(DOT_LED, 0); analogWrite(DASH_LED, 0); digitalWrite(LETTER_LED, LOW); buzzerOff();
  Serial.print(F(" ( 여백 ) "));
  delay(30);
  while (anyDown()) {
    if (millis() - t0 >= SWITCH_HOLD && !stopNow) { stopNow = true; cue(); }
  }
  delay(30);
  if (stopNow) return millis() - t0;
  analogWrite(DOT_LED, dotLevel); analogWrite(DASH_LED, dashLevel);
  digitalWrite(LETTER_LED, letterOn ? HIGH : LOW);
  if (soundFreq > 0) buzzerOn(soundFreq);
  return millis() - t0;
}

void wait(unsigned long ms) {
  unsigned long start = millis();
  while (!stopNow && millis() - start < ms) {
    updateSound();
    readKnob();
    start += holdIfPressed();
    String line;
    if (readLine(line)) {
      line.toLowerCase();
      if (line == "stop") { stopNow = true; cue(); }
    }
  }
}

void flash(int pin, unsigned long ms, int f) {
  setLevel(pin, 255); sound(f); wait(ms); setLevel(pin, 0); quiet();
}
int curve(unsigned long p) { return (int)(p * p / 3922UL); }
void breathe(int pin, unsigned long ms, int f) {
  unsigned long up = ms * 4 / 10, down = ms - up;
  sound(f);
  for (unsigned long t = 0; t < up && !stopNow; t += 10)   { setLevel(pin, curve(t * 1000 / up));          wait(10); }
  for (unsigned long t = 0; t < down && !stopNow; t += 10) { setLevel(pin, curve(1000 - t * 1000 / down)); wait(10); }
  setLevel(pin, 0); quiet();
}
void letterGap(unsigned long gap, bool blink) {
  unsigned long b = min(60UL, gap / 2);
  if (blink && b >= 15) { setLetter(true); wait(b); setLetter(false); wait(gap - b); }
  else wait(gap);
}

void stillSend(const char* text, unsigned long unit, float g, int freq, bool breath, unsigned long breathGap, unsigned long tail) {
  for (int i = 0; text[i] && !stopNow; i++) {
    char c = text[i];
    const char* code = morseFor(c);
    if (!code) continue;
    int f = breath ? SCALE[(c - 'A') % 10] : freq;
    for (int k = 0; code[k] && !stopNow; k++) {
      unsigned long len = (code[k] == '.') ? unit : unit * 3;
      int pin = (code[k] == '.') ? DOT_LED : DASH_LED;
      if (breath) breathe(pin, len, f); else flash(pin, len, f);
      if (code[k + 1]) wait((unsigned long)(g * unit));
    }
    if (stopNow) return;
    Serial.print(c);
    bool last = (text[i + 1] == 0), wordEnd = (text[i + 1] == ' ');
    unsigned long gap;
    if (last)        gap = tail;
    else if (breath) gap = wordEnd ? breathGap * 5 / 2 : breathGap;
    else             gap = (unsigned long)(g * unit * (wordEnd ? 7 : 3));
    if (wordEnd) Serial.print(' ');
    letterGap(gap, !breath);
  }
}

void runStillHere() {
  const unsigned long sUnit = STILL_1MIN ? 120 : 150;
  const unsigned long bUnit = STILL_1MIN ? 220 : 300;
  unsigned long bGap        = STILL_1MIN ? 900 : 1600;

  Serial.println(F("\n[1] 신호"));
  wait(STILL_1MIN ? 1000 : 2000);
  Serial.print(F("  "));
  stillSend(STILL_MESSAGE, sUnit, 1.0, STILL_FREQ, false, 0, STILL_1MIN ? 1200 : 2000);

  if (!stopNow) Serial.println(F("\n[2] 과잉"));
  for (int r = 1; r <= 5 && !stopNow; r++) {
    float g = 1.0 - r / 5.0;
    unsigned long unit = (unsigned long)(sUnit * pow(0.8, r));
    Serial.print(F("  "));
    stillSend(STILL_MESSAGE, unit, g, EXCESS_FREQ[r - 1], false, 0, (unsigned long)(7 * g * unit));
    Serial.println();
  }
  if (!stopNow) {
    Serial.println(F("  [임계점]"));
    setLevel(DOT_LED, 255); setLevel(DASH_LED, 255); setLetter(true);
    for (int i = 0; i <= 40 && !stopNow; i++) { sound(1319 + (2400 - 1319) * (long)i / 40); wait(30); }
    setLevel(DOT_LED, 0); setLevel(DASH_LED, 0); setLetter(false); buzzerOff();   // 한순간에 끊김
  }
  if (!stopNow) { Serial.println(F("[3] 정적")); wait(STILL_1MIN ? 5000 : 8000); }
  if (!stopNow) {
    Serial.println(F("[4] 숨"));
    Serial.print(F("  "));
    stillSend(STILL_MESSAGE, bUnit, 1.0, 0, true, bGap, bGap);
    Serial.println();
    setLetter(true);  wait(STILL_1MIN ? 2000 : 3000);
    setLetter(false); wait(STILL_1MIN ? 1500 : 4000);
  }
}

// STILL HERE 시작 → (반복) → 끝나면 모스부호기로
void stillMode() {
  stopNow = false;
  cue();
  Serial.println(F("\n=== STILL HERE 시작 (버튼 3초 꾹 또는 stop = 끄기) ==="));
  do { runStillHere(); } while (STILL_REPEAT && !stopNow);
  stopNow = false;
  dotLevel = dashLevel = soundFreq = 0; letterOn = false;
  allOff();
  analogWrite(DOT_LED, 0); analogWrite(DASH_LED, 0);
  Serial.println(F("\n=== 모스부호기로 돌아왔어요 ==="));
}

// =============================================================
//  모스부호기
// =============================================================
String symbols = "", message = "";
bool pressed = false, wordSpaceDone = true, switchArmed = false;
unsigned long pressStart = 0, releaseTime = 0;
int lastFreq = 0;
unsigned long avgGap = 180;   // 점·대시 사이에 평소 쉬는 시간 (치면서 저절로 맞춰져요)

// 글자 확정 시간 = 평소 쉬는 시간의 3배 (0.35초 ~ LETTER_GAP)
unsigned long letterGapNow() { return constrain(avgGap * 3, 350UL, LETTER_GAP); }

int currentFreq() { return BASE_FREQ; }
void soundOn(int f) { buzzerOn(f); lastFreq = f; }
void soundOff()     { noteRelease(); }

void sendText(String text) {
  text.toUpperCase();
  Serial.print(F(">> 송신: ")); Serial.println(text);
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == ' ') { soundDelay(UNIT * 4); continue; }
    const char* code = morseFor(c);
    if (!code) continue;
    Serial.print(F("   ")); Serial.print(c); Serial.print(F("  ")); Serial.println(code);
    for (int j = 0; code[j]; j++) {
      bool isDash = (code[j] == '-');
      int led = isDash ? DASH_LED : DOT_LED;
      digitalWrite(led, HIGH); soundOn(currentFreq());
      soundDelay(isDash ? UNIT * 3 : UNIT);
      digitalWrite(led, LOW); soundOff();
      soundDelay(UNIT);
    }
    soundDelay(UNIT * 2);
  }
  Serial.println(F(">> 송신 끝"));
}

// 지금 문장을 마무리하고 새로 시작 (초록 2번 깜빡)
void newSentence() {
  message.trim();
  if (message.length() > 0) {
    Serial.print(F("=== 문장 끝: "));
    Serial.print(message);
    Serial.println(F("  → 새로 치세요 ==="));
  }
  message = "";
  symbols = "";
  wordSpaceDone = true;
  for (int i = 0; i < 2; i++) { digitalWrite(LETTER_LED, HIGH); delay(100); digitalWrite(LETTER_LED, LOW); delay(100); }
}

// ---------- 대시 버튼 (3번, 두 번째 버튼) ----------
bool dashPressed = false, dashCounted = false;
unsigned long dashStart = 0;

// 대시 버튼을 처리해요. 대시 버튼을 쓰는 중이면 true (그동안 다른 일은 쉬어요)
bool handleDashButton(unsigned long now) {
  if (!TWO_BUTTONS) return false;
  bool dDown = (digitalRead(DASH_BUTTON_PIN) == LOW);
  if (dDown && !dashPressed) {
    if (pressed) return false;                       // 점 버튼을 누르는 중이면 무시
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;
    dashPressed = true; dashCounted = false;
    dashStart = now;
    soundOn(currentFreq());
    delay(DEBOUNCE);
    return true;
  }
  if (dDown && dashPressed) {
    if (!dashCounted && now - dashStart >= DASH_TIME) {               // 0.25초 넘으면 그 순간 대시
      dashCounted = true;
      digitalWrite(DASH_LED, HIGH);
      symbols += '-';
      Serial.print('-');
      wordSpaceDone = false;
    }
    return true;
  }
  if (!dDown && dashPressed) {
    dashPressed = false;
    soundOff();
    digitalWrite(DASH_LED, LOW);
    delay(DEBOUNCE);
    if (dashCounted) releaseTime = now;
    else Serial.println(F(" (대시는 꾹)"));         // 너무 짧으면 안 쳐져요
    return true;
  }
  return false;
}

void morseLoop() {
  updateSound();
  readKnob();
  bool down = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long now = millis();
  if (handleDashButton(now)) return;

  if (down && !pressed) {
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;   // 글자 안에서 쉰 시간 기억
    pressed = true; pressStart = now; switchArmed = false;
    soundOn(currentFreq());
    digitalWrite(DOT_LED, HIGH);
    if (TWO_BUTTONS) { symbols += '.'; Serial.print('.'); wordSpaceDone = false; }   // 점 버튼: 누르는 순간 점
    delay(DEBOUNCE);
  }
  else if (down && pressed) {
    unsigned long held = now - pressStart;
    if (!TWO_BUTTONS && held >= DASH_TIME && held < SWITCH_HOLD / 2) {
      digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, HIGH);
    }
    if (held >= SWITCH_HOLD / 2) {              // 1.5초부터: 소리 끄고 초록이 깜빡이며 세기
      soundOff(); digitalWrite(DASH_LED, LOW);
      digitalWrite(LETTER_LED, (held / 150) % 2 ? HIGH : LOW);
    }
    if (held >= SWITCH_HOLD && !switchArmed) {  // 3초: 손을 떼면 STILL HERE 시작
      switchArmed = true;
      digitalWrite(LETTER_LED, HIGH);
    }
  }
  else if (!down && pressed) {
    pressed = false; releaseTime = now;
    soundOff();
    digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW); digitalWrite(LETTER_LED, LOW);
    delay(DEBOUNCE);
    if (switchArmed) {                          // 모드 바꾸기 (치던 글자는 버림)
      switchArmed = false; symbols = ""; wordSpaceDone = true;
      stillMode();
      return;
    }
    if (!TWO_BUTTONS) {                        // 버튼 1개: 뗄 때 길이로 점·대시
      char s = (now - pressStart < DASH_TIME) ? '.' : '-';
      symbols += s;
      Serial.print(s);
      wordSpaceDone = false;
    }
  }
  else {
    if (symbols.length() > 0 && now - releaseTime > letterGapNow()) {
      char letter = decode(symbols);
      message += letter;
      if (message.length() > 40) message = message.substring(message.length() - 40);
      Serial.print(F("   ->  ")); Serial.print(letter);
      Serial.print(F("      [ ")); Serial.print(message); Serial.print(F(" ]"));
      if (letter == '?') Serial.print(F("   (글자 사이를 조금 더 쉬어 보세요)"));
      Serial.println();
      symbols = "";
      digitalWrite(LETTER_LED, HIGH); delay(150); digitalWrite(LETTER_LED, LOW);
    }
    if (!wordSpaceDone && symbols.length() == 0 && now - releaseTime > WORD_GAP) {
      message += ' ';
      Serial.println(F("   (띄어쓰기)"));
      wordSpaceDone = true;
    }
    // 오래 쉬면 문장 끝 → 다음부터 새 문장
    if (message.length() > 0 && symbols.length() == 0 && now - releaseTime > NEW_GAP) {
      newSentence();
    }
  }

  String line;
  if (readLine(line)) {
    String low = line; low.toLowerCase();
    if (low == "still") stillMode();
    else if (low == "new" || low == "clear") newSentence();
    else if (low != "stop") sendText(line);
  }
}

// =============================================================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DASH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOT_LED, OUTPUT);
  pinMode(DASH_LED, OUTPUT);
  pinMode(LETTER_LED, OUTPUT);
  soundSetup();
  Serial.begin(9600);

  int leds[3] = {DOT_LED, DASH_LED, LETTER_LED};       // 점검: 노랑 → 빨강 → 초록
  for (int i = 0; i < 3; i++) { digitalWrite(leds[i], HIGH); delay(300); digitalWrite(leds[i], LOW); }
  int testFreq[3] = {440, 660, 880};                    // 점검: 낮은 음 → 높은 음
  for (int i = 0; i < 3; i++) { buzzerOn(testFreq[i]); delay(320); noteRelease(); }
  delay(500);

  Serial.println(F("=== 모스부호기 준비 완료 ==="));
  if (TWO_BUTTONS) Serial.println(F("점 버튼(2번) = 점(.)   대시 버튼(3번) 꾹 = 대시(-)"));
  else Serial.println(F("버튼: 짧게 = 점(.)  길게 = 대시(-)"));
  Serial.println(F("영어/숫자를 입력하고 Enter → 자동 송신"));
  Serial.println(F("STILL HERE: 버튼 3초 꾹 (또는 still 입력)"));
  if (USE_KNOB) { lastKnobRaw = analogRead(KNOB_PIN); knobVolume = min(10, (lastKnobRaw + 51) / 102); outVol = BUZZER_ON ? knobVolume : 0; printKnob(lastKnobRaw); }
  Serial.println(F("새 문장: 5초 쉬기 (또는 new 입력)"));
}

void loop() {
  morseLoop();
}
