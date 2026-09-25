// Chrome Dino @ StickS3 —— 主循环（60fps 目标）
#include <M5Unified.h>
#include "app_states.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  if (M5.Display.width() < M5.Display.height()) {
    M5.Display.setRotation(M5.Display.getRotation() ^ 1);
  }
  app_init();
}

void loop() {
  M5.update();
  app_update();
  // 串口电量查询：发送 'b' 返回 电量%/电压mV/充电状态/电流mA
  if (Serial.available()) {
    int c = Serial.read();
    if (c == 'b') {
      Serial.printf("batt=%d%% voltage=%dmV charging=%d current=%dmA\n",
                    (int)M5.Power.getBatteryLevel(),
                    (int)M5.Power.getBatteryVoltage(),
                    (int)M5.Power.isCharging(),
                    (int)M5.Power.getBatteryCurrent());
    }
  }
  M5.delay(1);
}
