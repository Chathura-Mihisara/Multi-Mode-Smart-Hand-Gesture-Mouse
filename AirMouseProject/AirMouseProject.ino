#include "AirMouseProject.h"


#define mode_Button 15
#define left_Button 5
#define right_Button 4
#define middle_Button 18
#define laser 23
#define bluetoothStatus 2

TaskHandle_t gyroTaskHandle;
TaskHandle_t oledTaskHandle;
AirMouseProject d;
AirMouseProject timer1;
AirMouseProject timer2;
AirMouseProject b1;
AirMouseProject b2;
AirMouseProject leftButton;
bool isBTconnect = true;
bool isMouse = true;




void setup() {
  Serial.begin(115200);
  d.OLEDbegin();
  delay(1000);

  d.BluetoothMouseBegin();
  d.RtcCheck();
  delay(1000);
  d.GyroCheck();
  d.pinModeRegister(mode_Button, INPUT_PULLUP);
  d.pinModeRegister(left_Button, INPUT_PULLUP);
  d.pinModeRegister(right_Button, INPUT_PULLUP);
  d.pinModeRegister(middle_Button, INPUT_PULLUP);
  d.pinModeRegister(laser, OUTPUT);
  d.pinModeRegister(bluetoothStatus, OUTPUT);
  d.angleLastTime = millis();
  xTaskCreatePinnedToCore(gyroTask, "GyroTask", 4096, NULL, 1, &gyroTaskHandle, 0);
  xTaskCreatePinnedToCore(oledTask, "OledTask", 4096, NULL, 1, &oledTaskHandle, 0);
}

void gyroTask(void *parameter) {
  for (;;) {
    if (isMouse == true) {
      d.MouseMove();
      d.digitalWriteRegister(laser, LOW);
    } else {
      d.digitalWriteRegister(laser, HIGH);
    }

    if (d.isConnected) {
      isBTconnect = true;
    } else {
      isBTconnect = false;
    }

    if (b2.PressAction(mode_Button) == true) {
      isMouse = !isMouse;
    }

    d.MouseLeftButton(left_Button);
    d.MouseRightButton(right_Button);
    d.MouseMiddleButton(middle_Button);

    vTaskDelay(1);
  }
}

void oledTask(void *parameter) {
  for (;;) {

    displayPrint();
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void loop() {
}


void displayPrint() {
  d.BatteryPre();

  if (isMouse == true) {
    d.Mode("MOUSE");
  } else {
    d.Mode("LASER");
  }
  
  if (isBTconnect == false) {
    d.BTlogo(1, bluetoothStatus);
  } else {
    d.BTlogo(0, bluetoothStatus);
  }


  d.Time(0);

  d.OledDisplay();
}
