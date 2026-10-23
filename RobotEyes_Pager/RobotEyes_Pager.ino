/*
 Robot Eyes - LilyGO T-LoRa Pager (SX1262, ESP32-S3)
 Development sketch, NOT hardware-tested.
 Based on LilyGoLib 'instance' and LV_Helper LVGL initialization.
 Keyboard: any key cycles expression
 Rotary: turn to look left/right, click to blink
 Tilt: move pupils while the pager is tilted; needs a working BHI260AP.
*/
#include <Arduino.h>
#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <math.h>

static lv_obj_t *eyes[2], *pupils[2], *bars[2], *marks[2];
static float gazeX = 0, gazeY = 0, targetX = 0, targetY = 0;
static float zeroX = 0, zeroY = 0;
static bool imuReady = false;
static int expression = 0;
static uint32_t lastAction = 0, nextBlink = 0, blinkEnd = 0, nextLook = 0, lastImu = 0;
static constexpr int eyeCenterX[2] = {164, 316};
static constexpr int eyeCenterY = 102;

static float clip(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

static lv_obj_t *shape(lv_obj_t *parent, int x, int y, int w, int h,
                       lv_color_t color, int radius) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_color(o, color, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(o, 0, 0);
  lv_obj_set_style_pad_all(o, 0, 0);
  lv_obj_set_style_shadow_width(o, 0, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}

static void updateFace(uint32_t now) {
  bool sleep = (now - lastAction) > 25000;
  bool blink = (now < blinkEnd) || sleep;
  int w = expression == 2 ? 86 : 96;
  int h = expression == 1 ? 54 : (expression == 2 ? 80 : (expression == 3 ? 30 : 68));
  if (blink) h = 7;
  for (int i = 0; i < 2; ++i) {
    int cx = eyeCenterX[i];
    lv_obj_set_pos(eyes[i], cx-w/2, eyeCenterY-h/2);
    lv_obj_set_size(eyes[i], w, h);
    lv_obj_set_style_radius(eyes[i], h/2, 0);
    if (blink || h < 35) {
      lv_obj_add_flag(pupils[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_remove_flag(pupils[i], LV_OBJ_FLAG_HIDDEN);
      int offsetX = (int)(gazeX * (w/2 - 20));
      int offsetY = (int)(gazeY * (h/2 - 18));
      lv_obj_set_pos(pupils[i], cx + offsetX-14, eyeCenterY + offsetY-14);
    }
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t found = instance.begin();
  beginLvglHelper(instance);
  instance.setBrightness(10);
  lv_obj_t *screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x001524), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 2; i++) {
    int cx = eyeCenterX[i];
    eyes[i] = shape(screen, cx-48, eyeCenterY-34, 96, 68, lv_color_white(), 34);
    pupils[i] = shape(screen, cx-14, eyeCenterY-14, 28, 28, lv_color_hex(0x06172D), 14);
    bars[i] = shape(screen, cx-25, 161, 50, 6, lv_color_hex(0x00A9D8), 3);
  }
  shape(screen, 234, 178, 6, 5, lv_color_hex(0x00A9D8), 3);
  shape(screen, 241, 181, 6, 5, lv_color_hex(0x00A9D8), 3);
  shape(screen, 379, 56, 5, 5, lv_color_hex(0x00A9D8), 3);
  shape(screen, 390, 57, 4, 4, lv_color_hex(0x00A9D8), 3);

#ifdef HW_BHI260AP_ONLINE
  imuReady = (found & HW_BHI260AP_ONLINE) != 0;
#else
  imuReady = true; // Driver initialization happened in instance.begin(); read if available.
#endif
  if (imuReady) {
   // instance.sensor.configure(SENSOR_ID_ACC_PASS, 50);
    delay(160);
    instance.sensor.update();
    float z = 0;
   // instance.sensor.getAccelerometer(zeroX, zeroY, z);
  }
  lastAction = millis();
  nextBlink = lastAction + 2000;
  nextLook = lastAction + 1400;
}

void loop() {
  uint32_t now = millis();
  instance.loop(); // Services keyboard and peripherals.
  char key = 0;
  while (instance.getKeyChar(&key) > 0) {
    expression = (expression + 1) % 4;
    blinkEnd = now + 100;
    lastAction = now;
  }
  RotaryMsg_t rotary = instance.getRotary();
  if (rotary.enc_diff != 0) {
    targetX = clip(targetX + rotary.enc_diff * 0.3f, -1, 1);
    lastAction = now;
  }
  if (rotary.centerBtnClicked) {
    blinkEnd = now + 180;
    lastAction = now;
  }
  if (imuReady && (now - lastImu >= 60)) {
    lastImu = now;
    instance.sensor.update();
    float x = 0, y = 0, z = 0;
    // instance.sensor.getAccelerometer(x, y, z);
    float tx = clip((x - zeroX) * 0.20f, -1, 1);
    float ty = clip((y - zeroY) * 0.20f, -1, 1);
    if (fabsf(tx) + fabsf(ty) > 0.18f) {
      targetX = tx;
      targetY = ty;
      lastAction = now;
    }
  }
  if (now > nextLook && now - lastAction > 1600) {
    targetX = random(-80, 81) / 100.0f;
    targetY = random(-55, 56) / 100.0f;
    nextLook = now + random(1100, 3400);
  }
  if (now > nextBlink) {
    blinkEnd = now + 140;
    nextBlink = now + random(2000, 5200);
  }
  gazeX += (targetX - gazeX) * 0.20f;
  gazeY += (targetY - gazeY) * 0.20f;
  static uint32_t lastFrame = 0;
  if (now - lastFrame >= 40) {
    lastFrame = now;
    updateFace(now);
  }
  lv_timer_handler();
  delay(2);
}
