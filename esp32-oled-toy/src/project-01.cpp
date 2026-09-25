#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Eye layout
#define EYE_RADIUS 16
#define LEFT_EYE_X  40
#define RIGHT_EYE_X 88
#define EYE_Y       32

// Button (ESP32 built-in BOOT)
#define BUTTON_PIN 0

// Emotion list
enum Emotion {
  EMO_NORMAL,
  EMO_HAPPY,
  EMO_SAD,
  EMO_ANGRY,
  EMO_SURPRISED,
  EMO_SLEEPY,
  EMO_LOVE,
  EMO_WINK,
  EMO_COUNT
};

Emotion currentEmotion = EMO_NORMAL;
unsigned long lastEmotionChange = 0;
const unsigned long EMOTION_INTERVAL = 5000;  // auto-cycle every 5s

// Button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounce = 0;

// Blink state
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkStartTime = 0;
int blinkPhase = 0;

// Pupil offset for look-around
int pupilX = 0;
int pupilY = 0;
unsigned long lastLookTime = 0;

// Idle animation (pupil sway)
unsigned long lastAnimTime = 0;

// ---------- Helper: draw pupil with offset ----------
void drawPupil(int cx, int cy, int radius, int px, int py) {
  display.fillCircle(cx + px, cy + py, radius, SSD1306_BLACK);
}

// ---------- Emotion: NORMAL ----------
void drawNormal(int eyelidHeight) {
  display.fillCircle(LEFT_EYE_X,  EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);

  if (eyelidHeight < EYE_RADIUS) {
    drawPupil(LEFT_EYE_X,  EYE_Y, EYE_RADIUS / 2, pupilX, pupilY);
    drawPupil(RIGHT_EYE_X, EYE_Y, EYE_RADIUS / 2, pupilX, pupilY);
  }

  if (eyelidHeight > 0) {
    display.fillRect(LEFT_EYE_X  - EYE_RADIUS - 1, EYE_Y - EYE_RADIUS - 1,
                     EYE_RADIUS * 2 + 2, eyelidHeight, SSD1306_BLACK);
    display.fillRect(RIGHT_EYE_X - EYE_RADIUS - 1, EYE_Y - EYE_RADIUS - 1,
                     EYE_RADIUS * 2 + 2, eyelidHeight, SSD1306_BLACK);
  }
}

// ---------- Emotion: HAPPY (^ ^) ----------
void drawHappy() {
  // Left eye arc
  display.fillRect(LEFT_EYE_X - EYE_RADIUS, EYE_Y, EYE_RADIUS * 2, EYE_RADIUS + 2, SSD1306_BLACK);
  display.drawCircle(LEFT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.drawCircle(LEFT_EYE_X, EYE_Y, EYE_RADIUS - 1, SSD1306_WHITE);
  display.fillRect(LEFT_EYE_X - EYE_RADIUS - 1, EYE_Y, EYE_RADIUS * 2 + 2, EYE_RADIUS + 2, SSD1306_BLACK);

  // Right eye arc
  display.fillRect(RIGHT_EYE_X - EYE_RADIUS, EYE_Y, EYE_RADIUS * 2, EYE_RADIUS + 2, SSD1306_BLACK);
  display.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.drawCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS - 1, SSD1306_WHITE);
  display.fillRect(RIGHT_EYE_X - EYE_RADIUS - 1, EYE_Y, EYE_RADIUS * 2 + 2, EYE_RADIUS + 2, SSD1306_BLACK);
}

// ---------- Emotion: SAD ----------
void drawSad() {
  // Droopy eyes — pupils at bottom
  display.fillCircle(LEFT_EYE_X,  EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.fillCircle(LEFT_EYE_X,  EYE_Y + 5, EYE_RADIUS / 2, SSD1306_BLACK);
  display.fillCircle(RIGHT_EYE_X, EYE_Y + 5, EYE_RADIUS / 2, SSD1306_BLACK);

  // Sad eyebrows (outer down, inner up — like \  /)
  display.drawLine(LEFT_EYE_X - EYE_RADIUS, EYE_Y - EYE_RADIUS - 4,
                   LEFT_EYE_X + EYE_RADIUS, EYE_Y - EYE_RADIUS - 10, SSD1306_WHITE);
  display.drawLine(RIGHT_EYE_X - EYE_RADIUS, EYE_Y - EYE_RADIUS - 10,
                   RIGHT_EYE_X + EYE_RADIUS, EYE_Y - EYE_RADIUS - 4, SSD1306_WHITE);

  // Tear drop
  display.fillCircle(LEFT_EYE_X + 8, EYE_Y + EYE_RADIUS + 3, 2, SSD1306_WHITE);
}

// ---------- Emotion: ANGRY ----------
void drawAngry() {
  display.fillCircle(LEFT_EYE_X,  EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  drawPupil(LEFT_EYE_X,  EYE_Y, EYE_RADIUS / 2, 0, 0);
  drawPupil(RIGHT_EYE_X, EYE_Y, EYE_RADIUS / 2, 0, 0);

  // Angry eyebrows (inner down — like \  /)
  display.fillRect(LEFT_EYE_X - EYE_RADIUS, EYE_Y - EYE_RADIUS - 5,
                   EYE_RADIUS, 4, SSD1306_WHITE);
  display.fillRect(RIGHT_EYE_X, EYE_Y - EYE_RADIUS - 5,
                   EYE_RADIUS, 4, SSD1306_WHITE);

  // Slanted eyebrows
  display.drawLine(LEFT_EYE_X - EYE_RADIUS, EYE_Y - EYE_RADIUS - 2,
                   LEFT_EYE_X + EYE_RADIUS, EYE_Y - EYE_RADIUS - 12, SSD1306_WHITE);
  display.drawLine(RIGHT_EYE_X - EYE_RADIUS, EYE_Y - EYE_RADIUS - 12,
                   RIGHT_EYE_X + EYE_RADIUS, EYE_Y - EYE_RADIUS - 2, SSD1306_WHITE);
}

// ---------- Emotion: SURPRISED ----------
void drawSurprised() {
  display.fillCircle(LEFT_EYE_X,  EYE_Y, EYE_RADIUS + 2, SSD1306_WHITE);
  display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS + 2, SSD1306_WHITE);
  // Small pupils
  drawPupil(LEFT_EYE_X,  EYE_Y, 3, 0, 0);
  drawPupil(RIGHT_EYE_X, EYE_Y, 3, 0, 0);
  // Open mouth (O)
  display.drawCircle(SCREEN_WIDTH / 2, EYE_Y + EYE_RADIUS + 10, 5, SSD1306_WHITE);
}

// ---------- Emotion: SLEEPY ----------
void drawSleepy() {
  // Half closed eyes — draw bottom half circles
  display.fillCircle(LEFT_EYE_X,  EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  display.fillCircle(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  // Cover top half
  display.fillRect(LEFT_EYE_X  - EYE_RADIUS - 1, EYE_Y - EYE_RADIUS - 1,
                   EYE_RADIUS * 2 + 2, EYE_RADIUS, SSD1306_BLACK);
  display.fillRect(RIGHT_EYE_X - EYE_RADIUS - 1, EYE_Y - EYE_RADIUS - 1,
                   EYE_RADIUS * 2 + 2, EYE_RADIUS, SSD1306_BLACK);
  // Pupils at bottom
  drawPupil(LEFT_EYE_X,  EYE_Y + 3, EYE_RADIUS / 3, 0, 0);
  drawPupil(RIGHT_EYE_X, EYE_Y + 3, EYE_RADIUS / 3, 0, 0);

  // Zzz text
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(105, 5);
  display.print("Z");
  display.setCursor(112, 10);
  display.print("z");
  display.setCursor(118, 15);
  display.print("z");
}

// ---------- Emotion: LOVE ----------
void drawHeart(int cx, int cy, int size, uint16_t color) {
  // Heart using two circles + triangle
  display.fillCircle(cx - size / 2, cy - size / 4, size / 2, color);
  display.fillCircle(cx + size / 2, cy - size / 4, size / 2, color);
  display.fillTriangle(cx - size, cy,
                       cx + size, cy,
                       cx, cy + size, color);
}

void drawLove() {
  drawHeart(LEFT_EYE_X,  EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  drawHeart(RIGHT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
}

// ---------- Emotion: WINK ----------
void drawWink() {
  // Left eye normal
  display.fillCircle(LEFT_EYE_X, EYE_Y, EYE_RADIUS, SSD1306_WHITE);
  drawPupil(LEFT_EYE_X, EYE_Y, EYE_RADIUS / 2, pupilX, pupilY);

  // Right eye closed — flat line
  display.fillRect(RIGHT_EYE_X - EYE_RADIUS, EYE_Y - 2,
                   EYE_RADIUS * 2, 4, SSD1306_WHITE);
}

// ---------- Master draw ----------
void renderFace(int eyelidHeight) {
  display.clearDisplay();

  switch (currentEmotion) {
    case EMO_NORMAL:    drawNormal(eyelidHeight); break;
    case EMO_HAPPY:     drawHappy();              break;
    case EMO_SAD:       drawSad();                break;
    case EMO_ANGRY:     drawAngry();              break;
    case EMO_SURPRISED: drawSurprised();          break;
    case EMO_SLEEPY:    drawSleepy();             break;
    case EMO_LOVE:      drawLove();               break;
    case EMO_WINK:      drawWink();               break;
  }

  display.display();
}

// ---------- Blink handler (only in NORMAL) ----------
void handleBlink() {
  unsigned long now = millis();
  unsigned long dur = 120;
  unsigned long since = now - blinkStartTime;

  if (blinkPhase == 1) {
    int h = map(since, 0, dur / 2, 0, EYE_RADIUS + 2);
    h = constrain(h, 0, EYE_RADIUS + 2);
    renderFace(h);
    if (since >= dur / 2) blinkPhase = 2;
  } else if (blinkPhase == 2) {
    int h = map(since - dur / 2, 0, dur / 2, EYE_RADIUS + 2, 0);
    h = constrain(h, 0, EYE_RADIUS + 2);
    renderFace(h);
    if (since >= dur) {
      blinkPhase = 0;
      isBlinking = false;
      lastBlinkTime = now;
    }
  }
}

// ---------- Next emotion ----------
void nextEmotion() {
  currentEmotion = (Emotion)((currentEmotion + 1) % EMO_COUNT);
  lastEmotionChange = millis();
  Serial.print("Emotion changed to: ");
  Serial.println((int)currentEmotion);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  randomSeed(analogRead(0));
  Serial.println("Emotion eyes ready!");

  lastBlinkTime = millis();
  lastLookTime = millis();
  lastEmotionChange = millis();
}

void loop() {
  unsigned long now = millis();

  // Button — press for next emotion
  bool reading = digitalRead(BUTTON_PIN);
  if (reading == LOW && lastButtonState == HIGH && (now - lastDebounce) > 250) {
    lastDebounce = now;
    nextEmotion();
  }
  lastButtonState = reading;

  // Auto-cycle emotions
  if (now - lastEmotionChange > EMOTION_INTERVAL) {
    nextEmotion();
  }

  // Look around (only NORMAL)
  if (currentEmotion == EMO_NORMAL && !isBlinking &&
      (now - lastLookTime > random(1500, 3000))) {
    pupilX = random(-6, 7);
    pupilY = random(-4, 5);
    lastLookTime = now;
  }

  // Blink (only NORMAL)
  if (currentEmotion == EMO_NORMAL && !isBlinking &&
      (now - lastBlinkTime > random(2000, 5000))) {
    isBlinking = true;
    blinkPhase = 1;
    blinkStartTime = now;
  }

  // Render
  if (isBlinking && currentEmotion == EMO_NORMAL) {
    handleBlink();
  } else {
    renderFace(0);
  }

  delay(30);
}