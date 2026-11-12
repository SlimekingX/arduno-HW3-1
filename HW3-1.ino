// ==================== 腳位設定 ====================
const int BTN_PIN = 2;          // 按鈕腳位（使用內部上拉電阻）
const int RGB_R_PIN = 9;        // RGB LED 紅色 (PWM 腳位)
const int RGB_G_PIN = 10;       // RGB LED 綠色 (PWM 腳位)
const int RGB_B_PIN = 11;       // RGB LED 藍色 (PWM 腳位)

// 單色 LED 腳位陣列 (請依實際連接腳位修改)
const int MONO_LEDS[] = {3, 5, 6}; 
const int MONO_COUNT = sizeof(MONO_LEDS) / sizeof(MONO_LEDS[0]);

// 若 RGB LED 為共陽極(Common Anode)設為 true；共陰極設為 false
const bool IS_COMMON_ANODE = true; 

// ==================== 顏色資料庫 ====================
struct Color { int r; int g; int b; };
const Color COLOR_LIST[] = {
  {255, 0, 0},     // 0: 紅
  {0, 255, 0},     // 1: 綠
  {0, 0, 255},     // 2: 藍
  {255, 255, 0},   // 3: 黃
  {0, 255, 255},   // 4: 青
  {255, 0, 255},   // 5: 紫
  {255, 255, 255}  // 6: 白
};
const int COLOR_COUNT = sizeof(COLOR_LIST) / sizeof(COLOR_LIST[0]);

// ==================== 狀態與計時變數 ====================
int currentMode = 0;       // 0:常亮, 1:閃爍, 2:呼吸燈, 3:加分彩虹心跳
int currentColorIdx = 0;   // 當前色彩索引

// 按鈕長按 / 短按判斷變數
int lastBtnState = HIGH;
unsigned long btnPressTime = 0;
bool isPressing = false;
const unsigned long LONG_PRESS_TIME = 800; // 長按判斷門檻 (800ms)

// 動態燈效計時器
unsigned long lastUpdate = 0;
int breathBrightness = 0;
int breathStep = 5;
int rainbowHue = 0;

// ==================== 寫入 RGB 顏色 ====================
void setRGB(int r, int g, int b) {
  if (IS_COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
}

// 控制所有單色 LED (帶 PWM 亮度控制)
void setMonoLEDs(int brightness) {
  for (int i = 0; i < MONO_COUNT; i++) {
    analogWrite(MONO_LEDS[i], brightness);
  }
}

// ==================== Setup 初始化 ====================
void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);

  for (int i = 0; i < MONO_COUNT; i++) {
    pinMode(MONO_LEDS[i], OUTPUT);
  }
}

// ==================== Main Loop ====================
void loop() {
  handleButton();          // 偵測按鈕（長按換 Mode，短按換 Color）
  updateLightingEffect(); // 依 Mode 繪製燈效
}

// ==================== 按鈕判斷邏輯 ====================
void handleButton() {
  int reading = digitalRead(BTN_PIN);
  unsigned long now = millis();

  // 按下按鈕瞬間 (HIGH -> LOW)
  if (reading == LOW && lastBtnState == HIGH) {
    btnPressTime = now;
    isPressing = true;
  }
  
  // 放開按鈕瞬間 (LOW -> HIGH)
  if (reading == HIGH && lastBtnState == LOW && isPressing) {
    unsigned long pressDuration = now - btnPressTime;
    
    if (pressDuration >= LONG_PRESS_TIME) {
      // 【長按】：切換模式 (Mode 0 -> 1 -> 2 -> 3 -> 0)
      currentMode = (currentMode + 1) % 4;
    } else if (pressDuration > 50) {
      // 【短按】：切換顏色 (僅在 Mode 0, 1, 2 下有效)
      currentColorIdx = (currentColorIdx + 1) % COLOR_COUNT;
    }
    isPressing = false;
  }

  lastBtnState = reading;
}

// ==================== 燈效輸出控制 ====================
void updateLightingEffect() {
  unsigned long now = millis();
  Color activeColor = COLOR_LIST[currentColorIdx];

  switch (currentMode) {
    case 0: // 【Mode 0: Constant 常亮模式】
      setRGB(activeColor.r, activeColor.g, activeColor.b);
      setMonoLEDs(255);
      break;

    case 1: // 【Mode 1: Blink 閃爍模式】
      if (now - lastUpdate > 250) {
        lastUpdate = now;
        static bool toggle = false;
        toggle = !toggle;
        if (toggle) {
          setRGB(activeColor.r, activeColor.g, activeColor.b);
          setMonoLEDs(255);
        } else {
          setRGB(0, 0, 0);
          setMonoLEDs(0);
        }
      }
      break;

    case 2: // 【Mode 2: Breathing 呼吸燈模式】
      if (now - lastUpdate > 20) {
        lastUpdate = now;
        breathBrightness += breathStep;
        if (breathBrightness <= 0 || breathBrightness >= 255) {
          breathStep = -breathStep;
        }
        
        // 依比率計算呼吸亮度
        int r = (activeColor.r * breathBrightness) / 255;
        int g = (activeColor.g * breathBrightness) / 255;
        int b = (activeColor.b * breathBrightness) / 255;
        setRGB(r, g, b);
        setMonoLEDs(breathBrightness);
      }
      break;

    case 3: // 【Mode 3: Bonus 加分彩虹心跳模式】
      if (now - lastUpdate > 15) {
        lastUpdate = now;

        // 1. RGB 全彩流轉
        rainbowHue = (rainbowHue + 2) % 768;
        int r, g, b;
        if (rainbowHue < 256) {
          r = 255 - rainbowHue; g = rainbowHue; b = 0;
        } else if (rainbowHue < 512) {
          r = 0; g = 255 - (rainbowHue - 256); b = rainbowHue - 256;
        } else {
          r = rainbowHue - 512; g = 0; b = 255 - (rainbowHue - 512);
        }
        setRGB(r, g, b);

        // 2. 單色 LED 脈衝心跳 (咚-咚---)
        int beatCycle = now % 1200;
        if ((beatCycle > 0 && beatCycle < 120) || (beatCycle > 250 && beatCycle < 370)) {
          setMonoLEDs(255);
        } else {
          setMonoLEDs(0);
        }
      }
      break;
  }
}