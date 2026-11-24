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
const int BUZZER_PIN = 8;   // 부저 + 다리
const int BUZZER_PIN2 = 9;  // (최대 소리) 부저 − 다리를 GND 대신 9번에 꽂으면 2배 크게
const int KNOB_PIN   = A0;
const int RGB_R = 10, RGB_G = 11, RGB_B = 12;   // RGB 라이트 R·G·B (GND는 파란 − 줄)
const int MIC_PIN = A1;                          // 소리 센서 AO
const bool MIC_DIRECT = true;   // 소리 센서를 아두이노 A1~A4에 바로 꽂았을 때: A2 = GND, A3 = 5V로 전기를 줘요 (판에 꽂았어도 상관없어요)

// ---------- 설정 (여기만 바꾸면 됨) ----------
const bool USE_KNOB      = true;  // 회전 센서(A0)로 소리 크기 조절. 센서가 없으면 false
const bool USE_RGB       = true;  // RGB 라이트: 글자마다 새 색, 음마다 무지개색, 소리 크기를 빛으로
const bool USE_MIC       = true;  // 소리 센서: 소리를 빛으로 보여주고, mic 입력하면 소리로 모스부호 치기
const bool BREATH_SOS    = true;  // 숨·목소리로 치기: 짧게 = 점 · 길게(0.6초~) = 대시 · 아주 길게(2.5초~) = HERE 자동 송신
const int  MIC_THRESHOLD = 40;    // 소리로 칠 때 '소리'로 보는 기준의 최댓값. 실제 기준은 방 소음에 맞춰 자동으로 더 낮아져요 (목소리·쿵쿵도 잡게)
const bool LOUD_MODE     = true;  // 9번 핀도 같이 써서 최대 소리 (부저 − 다리가 9번에 있을 때). GND에 있으면 상관없어요
const bool BUZZER_ON     = true;  // false면 부저는 조용 (소리는 웹사이트에서만)
const bool BUZZER_ACTIVE = false; // 음 높이가 안 바뀌는 부저(능동 부저)면 true
const bool TWO_BUTTONS   = true;  // 버튼 2개: 2번 = 점, 3번 = 대시(꾹). false면 버튼 1개로 짧게/길게
const int  VOLUME        = 10;     // 소리 크기: 1(아주 작게) ~ 10(가장 크게). 회전 센서를 쓰면 센서가 정해요
const int  BASE_FREQ     = 800;   // 모스부호기 소리 높이 (Hz) — 원래 높이
const bool LETTER_MELODY = true;  // (C) 글자가 확정될 때마다 그 글자의 음을 울려요. C=도 D=레 E=미 F=파 G=솔 A=라 B=시

// (A)(B) 음 세트: [이름, 점 음, 대시 음]. 점은 높게, 대시는 낮게(5도 아래).
// 점 버튼과 대시 버튼을 '동시에' 누르면 다음 세트로 바뀌어요.
const char* const PITCH_NAME[5] = { "원래", "도", "미", "솔", "높은 도" };
const int PITCH_DOT[5]  = { 800, 523, 659, 784, 1047 };
const int PITCH_DASH[5] = { 533, 349, 440, 523, 698 };
int pitchSet = 0;
int dotFreq()  { return PITCH_DOT[pitchSet]; }
int dashFreq() { return PITCH_DASH[pitchSet]; }

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
//  소리: 타이머1이 정확한 박자로 부저를 울려요 (원래 삐 소리)
//  한 번 떨릴 때 켜 두는 시간이 짧을수록 소리가 작아져요.
// =============================================================
volatile uint16_t onTicks = 0, offTicks = 0;
volatile bool phaseOn = false;

int knobVolume = VOLUME;     // 회전 센서가 정한 소리 크기 (0~10)
int volumeNow() { return USE_KNOB ? knobVolume : VOLUME; }

ISR(TIMER1_COMPA_vect) {               // 부저 = 8번 핀 = PB0
  if (phaseOn) { PORTB &= ~_BV(0); if (LOUD_MODE) PORTB |=  _BV(1); phaseOn = false; OCR1A = offTicks; }
  else         { PORTB |=  _BV(0); if (LOUD_MODE) PORTB &= ~_BV(1); phaseOn = true;  OCR1A = onTicks;  }
}

void timerStop() {
  noInterrupts();
  TIMSK1 &= ~_BV(OCIE1A);
  TCCR1B = 0;
  phaseOn = false;
  PORTB &= ~(_BV(0) | _BV(1));
  interrupts();
}

int noteBase = 0;
bool noteActive = false;

void buzzerHW(int f) {
  int vol = volumeNow();
  if (!BUZZER_ON || vol <= 0) { timerStop(); return; }   // 소리 0 = 조용
  unsigned long period = 2000000UL / f;
  // 한 칸에 소리가 약 3dB씩 바뀌게: 10 = 가장 큰 소리(50%), 1 = 아주 작게(약 1.4%)
  float amp = pow(10.0, (constrain(vol, 1, 10) - 10) * 3.0 / 20.0);
  unsigned long on = (unsigned long)(period * asin(amp) / PI);
  if (on < 8) on = 8;
  noInterrupts();
  onTicks = on - 1;
  offTicks = period - on - 1;
  if (!(TIMSK1 & _BV(OCIE1A))) {
    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS11);
    TCNT1 = 0;
    phaseOn = true; PORTB |= _BV(0);
    OCR1A = onTicks;
    TIFR1 = _BV(OCF1A);
    TIMSK1 |= _BV(OCIE1A);
  }
  interrupts();
}

void soundSetup() { pinMode(BUZZER_PIN, OUTPUT); pinMode(BUZZER_PIN2, OUTPUT); digitalWrite(BUZZER_PIN2, LOW); }

void buzzerOn(int f) {                                     // 소리 시작
  if (BUZZER_ACTIVE) { if (BUZZER_ON && volumeNow() > 0) digitalWrite(BUZZER_PIN, HIGH); return; }
  noteBase = f; noteActive = true;
  buzzerHW(f);
}
void buzzerOff() {                                         // 소리 끄기
  noteActive = false;
  if (BUZZER_ACTIVE) { digitalWrite(BUZZER_PIN, LOW); return; }
  timerStop();
}
void noteRelease() { buzzerOff(); }
void updateSound() {}
void soundDelay(unsigned long ms) { delay(ms); }

// =============================================================
//  RGB 라이트: 타이머2가 1초에 8000번 R·G·B를 켰다 껐다 해서 여러 색을 만들어요 (32단계 밝기)
// =============================================================
volatile uint8_t rgbR = 0, rgbG = 0, rgbB = 0, rgbTick = 0;
ISR(TIMER2_COMPA_vect) {                        // R=10(PB2) G=11(PB3) B=12(PB4)
  rgbTick = (rgbTick + 1) & 31;
  uint8_t p = PORTB & ~(_BV(2) | _BV(3) | _BV(4));
  if (rgbTick < rgbR) p |= _BV(2);
  if (rgbTick < rgbG) p |= _BV(3);
  if (rgbTick < rgbB) p |= _BV(4);
  PORTB = p;
}
void rgbSetup() {
  if (!USE_RGB) return;
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); pinMode(RGB_B, OUTPUT);
  TCCR2A = _BV(WGM21); TCCR2B = _BV(CS21); OCR2A = 249;   // 16MHz/8/250 = 8000번
  TIMSK2 |= _BV(OCIE2A);
}
void setRGB(int r, int g, int b) {              // 0~255
  if (!USE_RGB) return;
  rgbR = (r + 4) >> 3; rgbG = (g + 4) >> 3; rgbB = (b + 4) >> 3;
}
// 색상환(0~359도)과 밝기(0~255)로 색 만들기
void setHue(int h, int v) {
  h = ((h % 360) + 360) % 360;
  int x = (long)v * (60 - abs((h % 120) - 60)) / 60;
  if (h < 60) setRGB(v, x, 0); else if (h < 120) setRGB(x, v, 0); else if (h < 180) setRGB(0, v, x);
  else if (h < 240) setRGB(0, x, v); else if (h < 300) setRGB(x, 0, v); else setRGB(v, 0, x);
}
int letterHue(char c) {                         // 글자마다 다른 색 (A~Z가 무지개를 한 바퀴)
  if (c >= 'A' && c <= 'Z') return (c - 'A') * 360 / 26;
  if (c >= '0' && c <= '9') return (c - '0') * 36 + 15;
  return 0;
}
int freqHue(int f) {                            // 음 높이마다 색: 도 빨강 → 레 주황 → 미 노랑 … 시 보라 (옥타브마다 반복)
  float semis = 12.0 * log((float)f / 261.63) / log(2.0);
  int pc = ((int)floor(semis + 0.5) % 12 + 12) % 12;
  return pc * 25;                               // 0(빨강) ~ 275(보라)
}
unsigned long rgbHoldUntil = 0;
void rgbFlash(int hue, int v, unsigned long ms) { setHue(hue, v); rgbHoldUntil = millis() + ms; }

// =============================================================
//  소리 센서: 짧은 시간 동안 가장 큰 값 − 가장 작은 값 = 지금 소리 크기
// =============================================================
int micFloor = 3 * 16, micThOn = 20;            // 바닥 소음(×16)과 지금 기준 (소리 센서)
bool micMode = false;                           // 마지막 입력이 소리였으면 글자·띄어쓰기 시간을 넉넉하게
int meterLv = 0; unsigned long flashUntil = 0;   // 빛 미터 (천천히 식는 소리 크기)
int micLevel = 0;                               // 0~ (조용하면 한 자리, 박수·목소리면 수십~수백)
int micMin = 1023, micMax = 0;
unsigned long micWindow = 0;
void micUpdate() {
  if (!USE_MIC) return;
  int v = analogRead(MIC_PIN);
  if (v < micMin) micMin = v;
  if (v > micMax) micMax = v;
  if (millis() - micWindow >= 25) {
    int prev = micLevel;
    micLevel = micMax - micMin; micMin = 1023; micMax = 0; micWindow = millis();
    meterLv = max(micLevel, meterLv * 4 / 5);                      // 빛은 천천히 식어요 (숨 쉬듯)
    if (micLevel > prev * 3 && micLevel > micThOn * 3) flashUntil = millis() + 70;   // 갑자기 큰 소리 = 하얀 반짝
  }
}

// 소리 크기 → 색: 작을수록 보라·파랑, 커질수록 청록 → 초록 → 노랑 → 주황 → 빨강, 아주 크면 하양
void soundColor(int lv, unsigned long now) {
  int lo = max(8, micThOn * 2 / 3);
  if (now < flashUntil) { setRGB(200, 200, 200); return; }
  if (lv <= lo) { setRGB(0, 0, 0); return; }
  float t = log((float)lv / lo) / log(500.0 / lo);
  if (t > 1) t = 1;
  if (t > 0.92) { int w = (t - 0.92) / 0.08 * 255; setRGB(255, 120 + w / 2, w); return; }   // 아주 큼: 빨강 → 하양
  setHue(270 - (int)(t * 290), 30 + (int)(t * 225));
}

bool pressedAny() { return digitalRead(BUTTON_PIN) == LOW || (TWO_BUTTONS && digitalRead(DASH_BUTTON_PIN) == LOW); }

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
unsigned long knobShowUntil = 0, knobBeepUntil = 0;
bool knobFeedback = true;      // 모스부호기에서만: 돌리면 노란 LED 밝기 + 삑 소리로 알려줘요

void readKnob() {
  static unsigned long last = 0;
  // 알려주기 끝내기
  if (knobBeepUntil && millis() > knobBeepUntil) { knobBeepUntil = 0; if (!pressedAny()) buzzerOff(); }
  if (knobShowUntil && millis() > knobShowUntil) { knobShowUntil = 0; if (!pressedAny()) analogWrite(DOT_LED, 0); }
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
  if (noteActive) buzzerHW(noteBase);               // 울리는 중이면 바로 반영
  if (knobFeedback && !pressedAny()) {               // 보이게·들리게: 노란 LED 밝기 = 크기, 그 크기로 짧게 삑
    analogWrite(DOT_LED, knobVolume * 25);
    setHue(map(knobVolume, 0, 10, 240, 0), knobVolume ? 60 + knobVolume * 19 : 0);   // RGB: 작으면 파랑, 크면 빨강
    rgbHoldUntil = millis() + 700;
    knobShowUntil = millis() + 700;
    buzzerOn(BASE_FREQ);
    knobBeepUntil = millis() + 90;
  }
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
  int hue = freqHue(f);
  for (unsigned long t = 0; t < up && !stopNow; t += 10)   { int lv = curve(t * 1000 / up);          setLevel(pin, lv); if (!stopNow) setHue(hue, lv); wait(10); }
  for (unsigned long t = 0; t < down && !stopNow; t += 10) { int lv = curve(1000 - t * 1000 / down); setLevel(pin, lv); if (!stopNow) setHue(hue, lv); wait(10); }
  setLevel(pin, 0); setRGB(0, 0, 0); quiet();
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
    for (int i = 0; i <= 40 && !stopNow; i++) { setRGB(80 + i * 4, 80 + i * 4, 80 + i * 4); sound(1319 + (2400 - 1319) * (long)i / 40); wait(30); }   // 빛이 하얗게 차올라요
    setLevel(DOT_LED, 0); setLevel(DASH_LED, 0); setLetter(false); buzzerOff(); setRGB(0, 0, 0);   // 한순간에 끊김
  }
  if (!stopNow) {                                  // 정적: 어둠 속에 푸른 잔상이 잠깐 떠올랐다 사라져요
    Serial.println(F("[3] 정적"));
    unsigned long sil = STILL_1MIN ? 5000 : 8000;
    wait(800);
    for (int k = 0; k < 40 && !stopNow; k++) { int v = k < 10 ? k * 6 : (40 - k) * 2; setRGB(0, v / 2, v); wait((sil - 800) / 40); }
    setRGB(0, 0, 0);
  }
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
  knobFeedback = false;
  stopNow = false;
  cue();
  Serial.println(F("\n=== STILL HERE 시작 (버튼 3초 꾹 또는 stop = 끄기) ==="));
  do { runStillHere(); } while (STILL_REPEAT && !stopNow);
  stopNow = false;
  dotLevel = dashLevel = soundFreq = 0; letterOn = false;
  allOff();
  analogWrite(DOT_LED, 0); analogWrite(DASH_LED, 0);
  Serial.println(F("\n=== 모스부호기로 돌아왔어요 ==="));
  knobFeedback = true;
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

int currentFreq() { return dotFreq(); }
void soundOn(int f) { buzzerOn(f); lastFreq = f; }
void soundOff()     { noteRelease(); }

int sendHue = -1;   // 0 이상이면 송신하는 동안 RGB를 그 색 하나로 (SOS = 빨강)
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
      digitalWrite(led, HIGH); soundOn(isDash ? dashFreq() : dotFreq());
      if (sendHue >= 0) setHue(sendHue, 255); else setHue(isDash ? 320 : 50, 220);   // 점 노랑 · 대시 자홍
      soundDelay(isDash ? UNIT * 3 : UNIT);
      digitalWrite(led, LOW); soundOff(); setRGB(0, 0, 0);
      soundDelay(UNIT);
    }
    if (sendHue < 0) { setHue(letterHue(c), 160); soundDelay(UNIT); setRGB(0, 0, 0); soundDelay(UNIT); }   // 글자 끝 = 그 글자 색
    else soundDelay(UNIT * 2);
  }
  Serial.println(F(">> 송신 끝"));
}

// 지금 문장을 마무리하고 새로 시작 (초록 2번 깜빡)
bool sentenceMusic(String m);   // 아래에 있어요
void newSentence() {
  message.trim();
  if (message.length() > 0) {
    Serial.print(F("=== 문장 끝: "));
    Serial.print(message);
    Serial.println(F("  → 새로 치세요 ==="));
  }
  String played = message;
  message = "";
  symbols = "";
  wordSpaceDone = true;
  for (int i = 0; i < 2; i++) { digitalWrite(LETTER_LED, HIGH); delay(100); digitalWrite(LETTER_LED, LOW); delay(100); }
  sentenceMusic(played);   // 친 문장이 악보면 연주
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
    soundOn(dashFreq());
    delay(DEBOUNCE);
    return true;
  }
  if (dDown && dashPressed) {
    if (!dashCounted && now - dashStart >= DASH_TIME) {               // 0.25초 넘으면 그 순간 대시
      dashCounted = true;
      digitalWrite(DASH_LED, HIGH); setHue(320, 220);   // 대시 = 자홍빛
      symbols += '-'; micMode = false;
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

// =============================================================
//  음계 (도레미파솔라시) — 글자 음과 멜로디 연주에 함께 써요
// =============================================================
const int NOTE_HZ[7] = { 262, 294, 330, 349, 392, 440, 494 };   // 도 레 미 파 솔 라 시 (4옥타브)
// 음이름 글자: C=도 D=레 E=미 F=파 G=솔 A=라 B=시
int noteIndex(char c) {
  if (c >= 'a' && c <= 'g') c = c - 'a' + 'A';
  switch (c) { case 'C': return 0; case 'D': return 1; case 'E': return 2; case 'F': return 3;
               case 'G': return 4; case 'A': return 5; case 'B': return 6; }
  return -1;
}

// (C) 글자마다 음
//  · A~G는 진짜 음이름 그대로 (C=도, D=레, E=미 …) → 모스부호로 C C G G A A G 를 치면 '작은 별'이 돼요
//  · 숫자 1~8 = 도레미파솔라시도 (숫자 악보처럼)
//  · 나머지 글자(H~Z)는 한 옥타브 위 도레미… 를 차례로
int letterNote(char c) {
  int k = noteIndex(c);
  if (k >= 0) return NOTE_HZ[k] * 2;                          // 5옥타브: 부저에서 잘 들려요
  if (c >= '1' && c <= '7') return NOTE_HZ[c - '1'] * 2;
  if (c == '8') return NOTE_HZ[0] * 4;
  if (c >= 'H' && c <= 'Z') return NOTE_HZ[(c - 'H') % 7] * 4;
  if (c == '0') return NOTE_HZ[0];
  return 196;                                                   // ? = 낮은 소리
}

// 멜로디 연주
//  대문자 A~G = 도레미… (5옥타브) · 소문자 a~g = 한 옥타브 위 · 숫자 3~6 바로 앞 음의 옥타브 바꾸기는 안 써요
//  '-' = 앞 음을 한 박 더 길게 · '_' = 한 박 쉼 · 그 밖의 글자(띄어쓰기, |)는 무시
bool anyButtonDown();
void playMelody(const char* m, unsigned int beat) {
  Serial.print(F(">> 연주: ")); Serial.println(m);
  bool byButton = false;
  for (int i = 0; m[i]; i++) {
    if (anyButtonDown()) { byButton = true; Serial.println(F(">> 멈춤 (버튼)")); break; }   // 버튼 누르면 멈춤
    String l;
    if (readLine(l)) { l.toLowerCase(); if (l == "stop") { Serial.println(F(">> 멈춤 (stop)")); break; } }   // stop 입력해도 멈춤
    char c = m[i];
    if (c == '_') { delay(beat); continue; }
    int k = noteIndex(c);
    if (k < 0) continue;
    int f = NOTE_HZ[k] * ((c >= 'a' && c <= 'g') ? 4 : 2);
    int len = 1;
    while (m[i + 1] == '-') { len++; i++; }
    unsigned int d = beat * len;
    digitalWrite(DOT_LED, HIGH); setHue(freqHue(f), 255);   // 음마다 무지개색
    buzzerOn(f);
    delay(d * 85 / 100);
    buzzerOff();
    digitalWrite(DOT_LED, LOW); setRGB(0, 0, 0);
    delay(d * 15 / 100);
  }
  buzzerOff(); digitalWrite(DOT_LED, LOW);
  if (byButton) { while (anyButtonDown()) { } delay(40); }        // 멈춘 버튼은 점·대시로 치지 않아요
  digitalWrite(LETTER_LED, HIGH); delay(120); digitalWrite(LETTER_LED, LOW);
  Serial.println(F(">> 연주 끝"));
}
bool anyButtonDown() { return digitalRead(BUTTON_PIN) == LOW || (TWO_BUTTONS && digitalRead(DASH_BUTTON_PIN) == LOW); }

// 들어 있는 곡
const char SONG_STAR[]  = "CCGGAAG- FFEEDDC- GGFFEED- GGFFEED- CCGGAAG- FFEEDDC-";        // 반짝반짝 작은 별
const char SONG_BELL[]  = "GGAAGGE- GGEED--_ GGAAGGE- GEDEC--";                         // 학교종이 땡땡땡
const char SONG_SCALE[] = "CDEFGABc- cBAGFEDC-";                                           // 도레미파솔라시도
const char SONG_PLANE[] = "EDCDEEE- DDD- EGG- EDCDEEE- DDEDC--";                            // 떴다 떴다 비행기
const char SONG_JINGLE[] = "EEE- EEE- EGCDE--- FFFFF EEEE EDDED- G-- EEE- EEE- EGCDE--- FFFFF EEEE GGFDC---";   // 징글벨 (크리스마스)
const char SONG_SILENT[] = "G-AGE-- G-AGE-- d-dB-- c-cG-- A-AcBA G-AGE-- A-AcBA G-AGE-- d-dfdB c--e-- cGEGFD C---";   // 고요한 밤 (크리스마스)

// 모스부호로 친 문장을 노래로 (문장 끝 = 5초 쉬었을 때)
//  · STAR / BELL / PLANE / SCALE / JINGLE / SILENT 를 치면 그 곡
//  · 그 밖에는 A~G(도레미…)와 숫자 1~8을 이어서 연주. T(—) = 앞 음 길게, 띄어쓰기 = 쉼
bool sentenceMusic(String m) {
  m.trim();
  String name = m; name.replace(" ", "");
  if (name == "STAR")  { playMelody(SONG_STAR, 330);  return true; }
  if (name == "BELL")  { playMelody(SONG_BELL, 300);  return true; }
  if (name == "PLANE") { playMelody(SONG_PLANE, 300); return true; }
  if (name == "SCALE") { playMelody(SONG_SCALE, 260); return true; }
  if (name == "JINGLE") { playMelody(SONG_JINGLE, 220); return true; }
  if (name == "SILENT") { playMelody(SONG_SILENT, 380); return true; }
  char buf[70]; int n = 0, notes = 0;
  for (unsigned int i = 0; i < m.length() && n < 68; i++) {
    char c = m[i];
    if (c >= 'A' && c <= 'G')      { buf[n++] = c; notes++; }
    else if (c >= '1' && c <= '8') { buf[n++] = "CDEFGABc"[c - '1']; notes++; }
    else if (c == 'T' && n > 0)    { buf[n++] = '-'; }
    else if (c == ' ' && n > 0)    { buf[n++] = '_'; }
  }
  buf[n] = 0;
  if (notes < 2) return false;                                 // 음이 2개 안 되면 연주 안 해요 (예: SOS)
  Serial.println(F(">> 친 문장을 악보로 연주해요"));
  playMelody(buf, 330);
  return true;
}

// ---------- 소리로 모스부호 치기 (mic 입력으로 켜고 끔) ----------
bool micKey = false, micDown = false, micCounted = false;
unsigned long micStart = 0, micQuietAt = 0, quietSince = 0;
bool handleMicKey(unsigned long now) {
  if (!USE_MIC || !micKey) return false;
  if (noteActive) { quietSince = now; return false; }            // 부저가 울리는 동안은 듣지 않아요 (제 소리에 반응 안 하게)
  if (now - quietSince < 150) return false;
  bool loud = micLevel > MIC_THRESHOLD;
  if (loud && !micDown) {
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;
    micDown = true; micCounted = false; micStart = now; micQuietAt = 0;
    digitalWrite(DOT_LED, HIGH); setHue(50, 200);
    return true;
  }
  if (micDown) {
    if (loud) micQuietAt = 0; else if (!micQuietAt) micQuietAt = now;
    if (!micCounted && now - micStart >= DASH_TIME && loud) {         // 0.25초 넘게 계속 소리 = 대시
      micCounted = true; digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, HIGH); setHue(320, 220);
    }
    if (micQuietAt && now - micQuietAt > 90) {                        // 조용해지면 끝
      char sym = micCounted ? '-' : '.';
      symbols += sym; Serial.print(sym); wordSpaceDone = false;
      micDown = false; releaseTime = now;
      digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW); setRGB(0, 0, 0);
    }
    return true;
  }
  return false;
}

// 아무 일도 없을 때 RGB: 소리 센서 크기를 빛으로 (조용하면 꺼짐, 클수록 초록 → 노랑 → 빨강)
void rgbIdle(unsigned long now) {
  if (!USE_RGB || now < rgbHoldUntil || pressed || dashPressed || micDown) return;
  if (USE_MIC) soundColor(meterLv, now);         // 방 소리를 무지개빛으로
  else setRGB(0, 0, 0);
}

void micLevelPrint() {                          // miclevel: 4초 동안 소리 크기를 막대로
  Serial.println(F("[소리 센서] 4초 동안 소리 크기 (박수·목소리를 내 보세요)"));
  for (int k = 0; k < 20; k++) {
    unsigned long t = millis(); int pk = 0;
    while (millis() - t < 200) { micUpdate(); if (micLevel > pk) pk = micLevel; }
    Serial.print(F("  ")); Serial.print(pk); Serial.print(F("\t"));
    for (int i = 0; i < min(40, pk / 8); i++) Serial.print('|');
    if (pk > micThOn) Serial.print(F("  ← 소리로 인식"));
    Serial.println();
  }
  Serial.print(F("조용할 때 소음 ≈ ")); Serial.print(micFloor / 16); Serial.print(F(" · 지금 기준 = ")); Serial.print(micThOn); Serial.println(F(" (방 소음에 맞춰 자동)"));
}

// ---------- 숨(후~) 감지 → 자동 송신 ----------
//  마이크에 바람을 불면 소리 센서 값이 '길게' 커져요. 박수처럼 짧은 소리(0.4초 미만)는 무시해요.
//  부는 동안: RGB가 숨의 세기만큼 푸르게 밝아져요 (숨 = 빛)
//  다 불고 나면: 0.4~1.5초 = SOS (빨강) · 1.5초 넘게 = HERE "여기 있어요" (청록)
const unsigned long BREATH_MIN = 400, BREATH_LONG = 1500;
bool breathOn = false;
unsigned long breathStart = 0, breathQuiet = 0;
bool breathEnabled = BREATH_SOS;
void breathSend(const char* text, int hue) {
  sendHue = hue; sendText(text); sendHue = -1;
  setRGB(0, 0, 0);
  quietSince = millis();                                         // 제 소리 뒤에 바로 다시 듣지 않게
}
bool handleBreath(unsigned long now) {
  if (!USE_MIC || !breathEnabled || micKey) return false;
  if (noteActive) { quietSince = now; breathOn = false; return false; }
  if (now - quietSince < 300) return false;
  bool loud = micLevel > MIC_THRESHOLD;
  if (!breathOn) {
    if (loud && !pressedAny()) { breathOn = true; breathStart = now; breathQuiet = 0; }
    return false;
  }
  if (loud) breathQuiet = 0; else if (!breathQuiet) breathQuiet = now;
  unsigned long dur = now - breathStart;
  if (dur >= BREATH_MIN && loud) {                               // 숨이 빛이 돼요
    int v = constrain(map(micLevel, MIC_THRESHOLD, 300, 50, 255), 50, 255);
    setRGB(v / 4, v * 3 / 5, v); rgbHoldUntil = now + 150;
  }
  if (breathQuiet && now - breathQuiet > 150) {                  // 숨이 끝났어요
    breathOn = false;
    unsigned long len = breathQuiet - breathStart;
    if (len < BREATH_MIN) return false;                          // 짧은 소리(박수 등)는 그냥 넘어가요
    if (len >= BREATH_LONG) { Serial.print(F("\n[숨] 길게 ")); Serial.print(len); Serial.println(F("ms → HERE (여기 있어요)")); breathSend("HERE", 190); }
    else                    { Serial.print(F("\n[숨] 후~ "));  Serial.print(len); Serial.println(F("ms → SOS")); breathSend("SOS", 0); }
    return true;
  }
  return dur >= BREATH_MIN;
}

// ---------- 톡톡 치기 (손톱으로 마이크 톡, 책상 쿵, 박수 짝) ----------
//  짧은 소리를 '톡'으로 세고, 0.4초 동안 다음 톡이 없으면 묶음이 끝나요.
//  톡 1번 = 점 · 톡톡 2번 = 대시 · 5번 넘게 빠르게 = 긴급 SOS (3번, 최대 소리)
const unsigned long TAP_MAX = 180, TAP_GROUP_GAP = 400;
int tapCount = 0;
unsigned long tapLast = 0;
void emergencySOS() {
  Serial.println(F("\n[긴급] 톡톡톡톡톡 → SOS 3번 (최대 소리)"));
  int saved = knobVolume; knobVolume = 10;               // 회전 센서와 상관없이 가장 크게
  symbols = "";
  for (int i = 0; i < 3; i++) {
    sendHue = 0; sendText("SOS"); sendHue = -1;
    for (int k = 0; k < 3; k++) { setRGB(255, 0, 0); delay(120); setRGB(0, 0, 0); delay(120); }
  }
  knobVolume = saved;
  quietSince = millis();
}
void resolveTaps(unsigned long now) {
  int n = tapCount; tapCount = 0;
  if (n >= 5) { emergencySOS(); releaseTime = millis(); return; }
  if (n >= 3) { Serial.print(F("\n[톡] ")); Serial.print(n); Serial.println(F("번 → (1번 = 점 · 2번 = 대시 · 5번 이상 = 긴급 SOS)")); return; }
  char sym = (n == 1) ? '.' : '-';
  Serial.print(F("\n[톡] ")); Serial.print(n); Serial.print(F("번 → ")); Serial.println(sym == '.' ? F("점") : F("대시"));
  if (symbols.length() == 0) Serial.print(F("  "));
  symbols += sym; Serial.print(sym);
  wordSpaceDone = false; releaseTime = now; micMode = true;
  digitalWrite(sym == '.' ? DOT_LED : DASH_LED, HIGH); setHue(sym == '.' ? 50 : 320, 230);
  delay(120);
  digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW); setRGB(0, 0, 0);
}

// ---------- 숨 · 목소리 · 박수로 모스부호 치기 ----------
//  소리가 '이어지는 동안'을 한 번으로 봐요. 숨은 세기가 오르내려서 사이사이 잠깐 작아져도 0.25초까지는 이어진 걸로 쳐요.
//  짧게(0.6초 미만) = 점 · 길게(0.6초~) = 대시 · 아주 길게(2.5초~) = HERE 자동 송신
//  치는 동안 RGB가 소리 세기만큼 푸르게 · 부저는 조용(제 소리를 다시 듣지 않게)
bool soundKey = BREATH_SOS;
const unsigned long S_DOT_MAX = 600, S_HERE = 2500, S_HANG = 250;
const unsigned long S_LETTER_GAP = 1500, S_WORD_GAP = 3500, S_NEW_GAP = 7000;
int sEnv = 0;
unsigned long sEnvT = 0, sStart = 0, sBelow = 0;
bool sOn = false;
bool handleSoundKey(unsigned long now) {
  if (!USE_MIC || !soundKey) return false;
  if (noteActive) { quietSince = now; sOn = false; sEnv = 0; return false; }
  if (now - quietSince < 400) return false;
  if (now - sEnvT >= 25) { sEnvT = now; sEnv = max(micLevel, sEnv / 2); }   // 툭 같은 짧은 소리도 빨리 끝나게
  if (tapCount && !sOn && now - tapLast > TAP_GROUP_GAP) resolveTaps(now);  // 톡톡 묶음이 끝났으면 뜻 정하기
  // 방이 조용할 때의 소리(바닥 소음)를 천천히 배워서, 그보다 확실히 크면 '소리'로 봐요
  if (!sOn && micLevel < micThOn) micFloor = (micFloor * 31 + micLevel * 16 + 16) / 32;   // 16배로 저장 (소수점 대신)
  micThOn = constrain(micFloor / 16 * 2 + 12, 14, MIC_THRESHOLD);
  const int thOn = micThOn, thOff = max(8, micThOn * 2 / 3);
  if (!sOn) {
    if (sEnv > thOn && !pressedAny()) { sOn = true; sStart = now; sBelow = 0; digitalWrite(DOT_LED, HIGH); }
    return false;
  }
  unsigned long dur = now - sStart;
  if (dur >= S_HERE) setHue(190, 255); else soundColor(sEnv, now);   // 소리 세기 = 무지개빛 (HERE 길이가 되면 청록)
  rgbHoldUntil = now + 150;
  if (dur >= S_DOT_MAX) { digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, HIGH); }   // 대시 길이가 되면 빨강 LED
  if (sEnv < thOff) { if (!sBelow) sBelow = now; } else sBelow = 0;
  unsigned long hang = (sBelow && sBelow - sStart < TAP_MAX) ? 60 : S_HANG;   // 톡: 금방 끝 · 숨·목소리: 0.25초 기다림
  if (sBelow && now - sBelow > hang) {          // 조용해지면 끝
    sOn = false; sEnv = 0;
    unsigned long len = sBelow - sStart;
    digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW); setRGB(0, 0, 0);
    if (len < 15) return true;                    // 아주 짧은 잡음만 무시
    if (len < TAP_MAX) {                          // 톡·쿵·짝 = '톡' 한 번 (묶어서 셉니다)
      tapCount++; tapLast = now;
      setHue(50, 180); rgbHoldUntil = now + 80;
      return true;
    }
    Serial.print(F("\n[소리] ")); Serial.print(len / 1000.0, 2); Serial.print(F("초 → "));
    if (len >= S_HERE) {
      Serial.println(F("아주 길게 = HERE"));
      symbols = ""; breathSend("HERE", 190);
      releaseTime = millis(); micMode = true;
      return true;
    }
    char sym = len < S_DOT_MAX ? '.' : '-';
    Serial.println(sym == '.' ? F("점") : F("대시"));
    if (symbols.length() == 0) Serial.print(F("  "));
    symbols += sym; Serial.print(sym);
    wordSpaceDone = false; releaseTime = now; micMode = true;
    return true;
  }
  return true;
}

// (B) 점 버튼과 대시 버튼을 동시에 누르면 음 세트가 바뀌어요
bool checkPitchChange(unsigned long now) {
  if (!TWO_BUTTONS) return false;
  if (!(digitalRead(BUTTON_PIN) == LOW && digitalRead(DASH_BUTTON_PIN) == LOW)) return false;
  // 먼저 눌린 버튼이 친 기호는 지워요
  if (pressed && TWO_BUTTONS && symbols.length() > 0 && symbols[symbols.length() - 1] == '.') symbols.remove(symbols.length() - 1);
  if (dashPressed && dashCounted && symbols.length() > 0 && symbols[symbols.length() - 1] == '-') symbols.remove(symbols.length() - 1);
  pitchSet = (pitchSet + 1) % 5;
  buzzerOff();
  digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, LOW);
  Serial.print(F("\n[음 바꾸기] "));
  Serial.print(PITCH_NAME[pitchSet]);
  Serial.print(F("  (점 "));  Serial.print(dotFreq());
  Serial.print(F("Hz · 대시 ")); Serial.print(dashFreq());
  Serial.println(F("Hz)"));
  setHue(freqHue(dotFreq()), 255); rgbHoldUntil = millis() + 900;   // RGB = 새 음의 색
  for (int i = 0; i <= pitchSet; i++) { digitalWrite(LETTER_LED, HIGH); delay(90); digitalWrite(LETTER_LED, LOW); delay(90); }   // 초록 = 몇 번째 세트
  buzzerOn(dotFreq());  delay(160); buzzerOff(); delay(40);       // 미리 듣기: 점 음 → 대시 음
  buzzerOn(dashFreq()); delay(260); buzzerOff();
  while (digitalRead(BUTTON_PIN) == LOW || digitalRead(DASH_BUTTON_PIN) == LOW) { }   // 두 손 다 뗄 때까지
  delay(DEBOUNCE);
  pressed = false; dashPressed = false; dashCounted = false; switchArmed = false;
  releaseTime = millis();
  return true;
}

void morseLoop() {
  updateSound();
  readKnob();
  micUpdate();
  bool down = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long now = millis();
  rgbIdle(now);
  if (checkPitchChange(now)) return;            // (B) 두 버튼 동시 = 음 바꾸기
  if (handleDashButton(now)) return;
  if (handleSoundKey(now)) return;              // 숨·목소리·박수로 치기

  if (down && !pressed) {
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;   // 글자 안에서 쉰 시간 기억
    pressed = true; pressStart = now; switchArmed = false;
    soundOn(currentFreq());
    digitalWrite(DOT_LED, HIGH); setHue(50, 200);   // 점 = 노란빛
    if (TWO_BUTTONS) { symbols += '.'; Serial.print('.'); wordSpaceDone = false; micMode = false; }   // 점 버튼: 누르는 순간 점
    delay(DEBOUNCE);
  }
  else if (down && pressed) {
    unsigned long held = now - pressStart;
    if (!TWO_BUTTONS && held >= DASH_TIME && held < SWITCH_HOLD / 2) {
      digitalWrite(DOT_LED, LOW); digitalWrite(DASH_LED, HIGH);
      if (noteBase != dashFreq()) soundOn(dashFreq());   // 길어지면 대시 음으로
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
    if (symbols.length() > 0 && now - releaseTime > (micMode ? S_LETTER_GAP : letterGapNow())) {
      char letter = decode(symbols);
      message += letter;
      if (message.length() > 40) message = message.substring(message.length() - 40);
      Serial.print(F("   ->  ")); Serial.print(letter);
      Serial.print(F("      [ ")); Serial.print(message); Serial.print(F(" ]"));
      if (letter == '?') Serial.print(F("   (글자 사이를 조금 더 쉬어 보세요)"));
      Serial.println();
      symbols = "";
      digitalWrite(LETTER_LED, HIGH);
      if (letter == '?') setRGB(255, 0, 0); else setHue(letterHue(letter), 255);   // 글자마다 새 색
      rgbHoldUntil = millis() + 900;
      if (LETTER_MELODY) { buzzerOn(letterNote(letter)); delay(letter == '?' ? 120 : 220); buzzerOff(); }   // (C) 글자 음
      else delay(150);
      digitalWrite(LETTER_LED, LOW);
    }
    if (!wordSpaceDone && symbols.length() == 0 && now - releaseTime > (micMode ? S_WORD_GAP : WORD_GAP)) {
      message += ' ';
      Serial.println(F("   (띄어쓰기)"));
      wordSpaceDone = true;
    }
    // 오래 쉬면 문장 끝 → 다음부터 새 문장
    if (message.length() > 0 && symbols.length() == 0 && now - releaseTime > (micMode ? S_NEW_GAP : NEW_GAP)) {
      newSentence();
    }
  }

  String line;
  if (readLine(line)) {
    String low = line; low.toLowerCase();
    if (low == "still") stillMode();
    else if (low == "new" || low == "clear") newSentence();
    else if (low == "star")  playMelody(SONG_STAR, 330);           // 반짝반짝 작은 별
    else if (low == "bell")  playMelody(SONG_BELL, 300);           // 학교종이 땡땡땡
    else if (low == "scale") playMelody(SONG_SCALE, 260);          // 도레미파솔라시도
    else if (low == "plane") playMelody(SONG_PLANE, 300);          // 떴다 떴다 비행기
    else if (low == "jingle") playMelody(SONG_JINGLE, 220);        // 징글벨
    else if (low == "silent") playMelody(SONG_SILENT, 380);        // 고요한 밤
    else if (low == "mic" || low == "breath") { soundKey = !soundKey; Serial.println(soundKey ? F("[소리] 숨·목소리로 치기 켜짐: 짧게 = 점 · 길게 = 대시 · 아주 길게 = HERE") : F("[소리] 숨·목소리로 치기 꺼짐")); }
    else if (low == "miclevel") micLevelPrint();
    else if (low == "rgb") { for (int h = 0; h < 360; h += 6) { setHue(h, 255); delay(25); } setRGB(0, 0, 0); }   // RGB 점검: 무지개
    else if (low.startsWith("play ")) playMelody(line.c_str() + 5, 320);   // 내가 쓴 악보: play CCGGAAG-
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
  rgbSetup();
  if (USE_MIC && MIC_DIRECT) {                 // 소리 센서 바로 꽂기: AO=A1 · G=A2 · +=A3 · DO=A4
    pinMode(A2, OUTPUT); digitalWrite(A2, LOW);
    pinMode(A3, OUTPUT); digitalWrite(A3, HIGH);
    pinMode(A4, INPUT);
  }
  if (USE_RGB) { setRGB(255, 0, 0); delay(250); setRGB(0, 255, 0); delay(250); setRGB(0, 0, 255); delay(250); setRGB(0, 0, 0); }   // 점검: 빨 → 초 → 파
  Serial.begin(9600);

  int leds[3] = {DOT_LED, DASH_LED, LETTER_LED};       // 점검: 노랑 → 빨강 → 초록
  for (int i = 0; i < 3; i++) { digitalWrite(leds[i], HIGH); delay(300); digitalWrite(leds[i], LOW); }
  int testFreq[3] = {400, 800, 1600};                    // 점검: 낮은 음 → 높은 음
  for (int i = 0; i < 3; i++) { buzzerOn(testFreq[i]); delay(200); buzzerOff(); delay(80); }

  Serial.println(F("=== 모스부호기 준비 완료 ==="));
  if (TWO_BUTTONS) Serial.println(F("점 버튼(2번) = 점(.)   대시 버튼(3번) 꾹 = 대시(-)"));
  else Serial.println(F("버튼: 짧게 = 점(.)  길게 = 대시(-)"));
  Serial.println(F("영어/숫자를 입력하고 Enter → 자동 송신"));
  Serial.println(F("STILL HERE: 버튼 3초 꾹 (또는 still 입력)"));
  if (USE_KNOB) { lastKnobRaw = analogRead(KNOB_PIN); knobVolume = min(10, (lastKnobRaw + 51) / 102); printKnob(lastKnobRaw); }
  Serial.println(F("새 문장: 5초 쉬기 (또는 new 입력)"));
  if (TWO_BUTTONS) Serial.println(F("음 바꾸기: 점 버튼 + 대시 버튼 동시에 (원래 → 도 → 미 → 솔 → 높은 도)"));
  Serial.println(F("멜로디: star · bell · plane · scale · jingle · silent 입력 (stop 또는 버튼 = 멈춤), 또는 play CCGGAAG- (C=도 D=레 E=미 F=파 G=솔 A=라 B=시)"));
  if (USE_MIC) Serial.println(F("소리 센서: miclevel = 소리 크기 보기"));
  if (USE_MIC && BREATH_SOS) Serial.println(F("숨·목소리·박수로 치기: 짧게 = 점 · 0.6초 넘게 = 대시 · 2.5초 넘게 = HERE (mic = 켜기/끄기)"));
  if (USE_MIC && BREATH_SOS) Serial.println(F("톡톡 치기: 톡 1번 = 점 · 톡톡 2번 = 대시 · 5번 넘게 빠르게 = 긴급 SOS"));
  if (USE_RGB) Serial.println(F("RGB 라이트: 글자마다 새 색 · 음마다 무지개색 · rgb = 무지개 점검"));
}

void loop() {
  morseLoop();
}
