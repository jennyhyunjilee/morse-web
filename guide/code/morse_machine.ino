// =============================================================
//  아두이노 모스부호기 (Arduino Morse Code Machine)
// -------------------------------------------------------------
//  [받기 = 손으로 치기]
//    점 버튼(2번)을 누르면 점(.)        → 노란 LED
//    대시 버튼(3번)을 꾹 누르면 대시(-) → 빨간 LED
//    (버튼이 1개면 TWO_BUTTONS = false: 짧게 = 점, 길게 = 대시)
//    잠시 손을 떼고 있으면 글자로 바꿔서 시리얼 모니터에 출력
//    → 초록 LED가 깜빡이면 "글자 확정"
//
//  [보내기 = 자동 송신]
//    시리얼 모니터에 SOS, HELLO 같은 영어/숫자를 입력하고 Enter
//    → LED + 부저로 모스부호를 자동 재생
//
//  [회전 센서(선택)]
//    손잡이를 돌리면 삐- 소리의 높이(주파수)가 바뀜
// =============================================================

// ---------- 핀 번호 ----------
const int BUTTON_PIN = 2;   // 점 버튼 (다른 쪽 다리는 GND). 버튼이 1개면 점·대시 둘 다
const int DASH_BUTTON_PIN = 3;  // 대시 버튼 (두 번째 버튼, 다른 쪽 다리는 GND)
const int DOT_LED    = 5;   // 노란 LED = 점
const int DASH_LED   = 6;   // 빨간 LED = 대시
const int LETTER_LED = 7;   // 초록 LED = 글자 확정 (선택)
const int BUZZER_PIN = 8;   // 부저 +
const int KNOB_PIN   = A0;  // 회전 센서 OUT (선택)

// ---------- 설정 (여기만 바꾸면 됨) ----------
const bool TWO_BUTTONS   = true;  // 버튼 2개: 2번 = 점, 3번 = 대시(꾹). false면 버튼 1개로 짧게/길게
const bool USE_KNOB      = false; // 회전 센서를 연결했으면 true 로
const bool BUZZER_ACTIVE = false; // 시작음이 전부 같은 높이면 true 로 (능동 부저)
const int  BASE_FREQ     = 800;   // 기본 소리 높이 (Hz). 숫자가 클수록 높은 소리

const unsigned long DASH_TIME  = 250;  // 이 시간(ms)보다 오래 누르면 대시
const unsigned long LETTER_GAP = 800;  // 글자 확정까지 가장 길게 기다리는 시간 (실제로는 내 치는 속도에 맞춰 짧아져요)
const unsigned long WORD_GAP   = 2000; // 손 떼고 이만큼 지나면 띄어쓰기
const unsigned long NEW_GAP    = 5000; // 손 떼고 이만큼 지나면 문장 끝 → 새로 시작
const unsigned long UNIT       = 120;  // 자동 송신 속도: 점 1개 길이(ms)
const unsigned long DEBOUNCE   = 20;   // 버튼 떨림 무시 시간

// ---------- 모스부호 표 (A~Z, 0~9) ----------
const char* MORSE[36] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",   // A-J
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",     // K-T
  "..-", "...-", ".--", "-..-", "-.--", "--..",                            // U-Z
  "-----", ".----", "..---", "...--", "....-",                             // 0-4
  ".....", "-....", "--...", "---..", "----."                              // 5-9
};

// ---------- 상태 변수 ----------
String symbols = "";        // 지금 치고 있는 글자의 점/대시
String message = "";        // 지금까지 해독한 문장
bool pressed = false;
bool wordSpaceDone = true;
unsigned long pressStart = 0;
unsigned long releaseTime = 0;
int lastFreq = 0;
unsigned long avgGap = 180;     // 점·대시 사이에 평소 쉬는 시간 (치면서 저절로 맞춰져요)

// 글자 확정 시간 = 평소 쉬는 시간의 3배 (0.35초 ~ LETTER_GAP)
unsigned long letterGapNow() { return constrain(avgGap * 3, 350UL, LETTER_GAP); }

// ---------- 소리 ----------
int currentFreq() {
  if (USE_KNOB) return map(analogRead(KNOB_PIN), 0, 1023, 200, 2000);
  return BASE_FREQ;
}

void soundOn(int freq) {
  if (BUZZER_ACTIVE) digitalWrite(BUZZER_PIN, HIGH);
  else tone(BUZZER_PIN, freq);
  lastFreq = freq;
}

void soundOff() {
  if (BUZZER_ACTIVE) digitalWrite(BUZZER_PIN, LOW);
  else noTone(BUZZER_PIN);
}

// ---------- 해독: ".-" → 'A' ----------
char decode(const String& code) {
  for (int i = 0; i < 36; i++) {
    if (code == MORSE[i]) {
      if (i < 26) return 'A' + i;
      return '0' + (i - 26);
    }
  }
  return '?';
}

// ---------- 송신: "SOS" → ... --- ... ----------
void sendText(String text) {
  text.toUpperCase();
  Serial.print(F(">> 송신: "));
  Serial.println(text);

  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];

    if (c == ' ') {               // 단어 사이 = 점 7개 길이 (앞에서 3개 이미 쉼)
      delay(UNIT * 4);
      continue;
    }

    int idx = -1;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= '0' && c <= '9') idx = 26 + (c - '0');
    if (idx < 0) continue;        // 표에 없는 글자는 건너뜀

    const char* code = MORSE[idx];
    Serial.print("   ");
    Serial.print(c);
    Serial.print("  ");
    Serial.println(code);

    for (int j = 0; code[j] != '\0'; j++) {
      bool isDash = (code[j] == '-');
      int led = isDash ? DASH_LED : DOT_LED;
      digitalWrite(led, HIGH);
      soundOn(currentFreq());
      delay(isDash ? UNIT * 3 : UNIT);   // 대시 = 점 3개 길이
      digitalWrite(led, LOW);
      soundOff();
      delay(UNIT);                       // 점/대시 사이 = 점 1개 길이
    }
    delay(UNIT * 2);                     // 글자 사이 = 점 3개 길이
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

void blinkLetterLed() {
  digitalWrite(LETTER_LED, HIGH);
  delay(150);
  digitalWrite(LETTER_LED, LOW);
}

// =============================================================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // 내부 저항 사용 → 버튼에 저항 필요 없음
  pinMode(DASH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOT_LED, OUTPUT);
  pinMode(DASH_LED, OUTPUT);
  pinMode(LETTER_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);

  // --- 자가 점검 1: LED 노랑 → 빨강 → 초록 차례로 켜기 ---
  int leds[3] = {DOT_LED, DASH_LED, LETTER_LED};
  for (int i = 0; i < 3; i++) {
    digitalWrite(leds[i], HIGH);
    delay(300);
    digitalWrite(leds[i], LOW);
  }

  // --- 자가 점검 2: 낮은 음 → 중간 음 → 높은 음 ---
  //  음 높이가 달라지면 수동 부저 (주파수 바꾸기 가능)
  //  셋 다 같은 "삐"면 능동 부저 → 위에서 BUZZER_ACTIVE = true
  int testFreq[3] = {400, 800, 1600};
  for (int i = 0; i < 3; i++) {
    soundOn(testFreq[i]);
    delay(250);
    soundOff();
    delay(100);
  }

  Serial.println(F("=== 모스부호기 준비 완료 ==="));
  if (TWO_BUTTONS) Serial.println(F("점 버튼(2번) = 점(.)   대시 버튼(3번) 꾹 = 대시(-)"));
  else Serial.println(F("버튼: 짧게 = 점(.)  길게 = 대시(-)"));
  Serial.println(F("영어/숫자를 입력하고 Enter → 자동 송신"));
  Serial.println(F("새 문장: 5초 쉬기 (또는 new 입력)"));
}

// =============================================================
// ---------- 대시 버튼 (3번, 두 번째 버튼) ----------
bool dashPressed = false;
unsigned long dashStart = 0;

// 대시 버튼을 처리해요. 대시 버튼을 쓰는 중이면 true (그동안 다른 일은 쉬어요)
bool handleDashButton(unsigned long now) {
  if (!TWO_BUTTONS) return false;
  bool dDown = (digitalRead(DASH_BUTTON_PIN) == LOW);
  if (dDown && !dashPressed) {
    if (pressed) return false;                       // 점 버튼을 누르는 중이면 무시
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;
    dashPressed = true;
    dashStart = now;
    soundOn(currentFreq());
    delay(DEBOUNCE);
    return true;
  }
  if (dDown && dashPressed) {
    if (now - dashStart >= DASH_TIME) digitalWrite(DASH_LED, HIGH);   // 꾹 누르면 빨강
    return true;
  }
  if (!dDown && dashPressed) {
    dashPressed = false;
    soundOff();
    digitalWrite(DASH_LED, LOW);
    delay(DEBOUNCE);
    if (now - dashStart >= DASH_TIME) {              // 0.25초 넘게 눌렀으면 대시
      releaseTime = now;
      symbols += '-';
      Serial.print('-');
      wordSpaceDone = false;
    } else {
      Serial.print(F("(대시는 꾹) "));               // 너무 짧으면 안 쳐져요
    }
    return true;
  }
  return false;
}

// =============================================================
void loop() {
  bool down = (digitalRead(BUTTON_PIN) == LOW);  // 누르면 LOW
  unsigned long now = millis();
  if (handleDashButton(now)) return;             // 대시 버튼을 쓰는 중

  if (down && !pressed) {
    // ---- 막 눌렀을 때: 소리 켜고 일단 노란 불(점) ----
    if (symbols.length() > 0) avgGap = (avgGap * 3 + (now - releaseTime)) / 4;   // 글자 안에서 쉰 시간 기억
    pressed = true;
    pressStart = now;
    soundOn(currentFreq());
    digitalWrite(DOT_LED, HIGH);
    delay(DEBOUNCE);
  }
  else if (down && pressed) {
    // ---- 누르고 있는 중: 길어지면 빨간 불(대시)로 바뀜 ----
    if (!TWO_BUTTONS && now - pressStart >= DASH_TIME) {
      digitalWrite(DOT_LED, LOW);
      digitalWrite(DASH_LED, HIGH);
    }
    // 회전 센서를 돌리면 누르는 동안에도 음 높이가 바뀜
    if (USE_KNOB && !BUZZER_ACTIVE) {
      int f = currentFreq();
      if (abs(f - lastFreq) > 20) soundOn(f);
    }
  }
  else if (!down && pressed) {
    // ---- 막 뗐을 때: 점인지 대시인지 기록 ----
    pressed = false;
    releaseTime = now;
    soundOff();
    digitalWrite(DOT_LED, LOW);
    digitalWrite(DASH_LED, LOW);

    char s = (TWO_BUTTONS || now - pressStart < DASH_TIME) ? '.' : '-';   // 버튼 2개면 점 버튼은 늘 점
    symbols += s;
    Serial.print(s);
    wordSpaceDone = false;
    delay(DEBOUNCE);
  }
  else {
    // ---- 쉬는 중: 오래 쉬면 글자 확정 / 더 오래 쉬면 띄어쓰기 ----
    if (symbols.length() > 0 && now - releaseTime > letterGapNow()) {
      char letter = decode(symbols);
      message += letter;
      Serial.print(F("   ->  "));
      Serial.print(letter);
      Serial.print(F("      [ "));
      Serial.print(message);
      Serial.print(F(" ]"));
      if (letter == '?') Serial.print(F("   (글자 사이를 조금 더 쉬어 보세요)"));
      Serial.println();
      symbols = "";
      blinkLetterLed();
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

  // ---- 시리얼 모니터에서 글자를 받으면 자동 송신 ----
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    String low = line; low.toLowerCase();
    if (low == "new" || low == "clear") newSentence();
    else if (line.length() > 0) sendText(line);
  }
}
