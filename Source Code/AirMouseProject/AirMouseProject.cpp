#include <Arduino.h>
#include "AirMouseProject.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BleMouse.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_MPU6050.h>
#include "RTClib.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SPEED 10
float angleZ = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
BleMouse bleMouse("CM_MOUSE");
BLEServer* pServer;
RTC_DS1307 rtc;

AirMouseProject::AirMouseProject() {

  batteryVoltage = 4.2;  //4.2
  x = 0;
  y = 1;
  w = 20;  // battery width
  h = 10;  // battery height
  capWidth = 2;

  startTime1 = 0;
  endTime1 = 0;
  currentTime1 = 0;
  laststate1 = HIGH;
  isPress1 = false;
  isPressPrint1 = false;
  isConnected = false;

  Tstarttime = 0;
  Tcurrenttime = 0;

  sleepMPU = true;

  wasconnected = false;
  middleButtonState = false;
}


// ===== Custom pinMode using registers =====
void AirMouseProject ::pinModeRegister(uint8_t pin, uint8_t mode) {
  // Disable output driver first (default = input)
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1 << pin));
  gpio_iomux_out(pin, PIN_FUNC_GPIO, false);

  switch (mode) {
    case OUTPUT:
      REG_WRITE(GPIO_ENABLE_W1TS_REG, (1 << pin));
      break;

    case INPUT:
      gpio_pullup_dis((gpio_num_t)pin);
      gpio_pulldown_dis((gpio_num_t)pin);
      break;

    case INPUT_PULLUP:
      gpio_pullup_en((gpio_num_t)pin);
      gpio_pulldown_dis((gpio_num_t)pin);
      break;

    case INPUT_PULLDOWN:
      gpio_pulldown_en((gpio_num_t)pin);
      gpio_pullup_dis((gpio_num_t)pin);
      break;
  }
}

// ===== Custom digitalWrite using registers =====
void AirMouseProject::digitalWriteRegister(uint8_t pin, uint8_t value) {
  if (value) {
    REG_WRITE(GPIO_OUT_W1TS_REG, (1 << pin));  // Set HIGH
  } else {
    REG_WRITE(GPIO_OUT_W1TC_REG, (1 << pin));  // Set LOW
  }
}

// ===== Custom digitalRead using registers =====
int AirMouseProject ::digitalReadRegister(uint8_t pin) {
  // Read the input register and extract the bit for this pin
  return (REG_READ(GPIO_IN_REG) & (1 << pin)) ? HIGH : LOW;
}

void AirMouseProject ::OLEDbegin() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
}


void AirMouseProject ::OledDisplay() {
  display.display();
}


void AirMouseProject ::OledClear() {
  display.clearDisplay();
}

void AirMouseProject ::RtcCheck() {

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

float maxVoltage = 4.2;
float minVoltage = 3.3;
void AirMouseProject ::BatteryPre() {

  int raw = analogRead(32);
  float voltage = (raw / 4095.0) * 3.3;
  voltage = voltage * 2;
  int percent = map(voltage * 100, minVoltage * 100, maxVoltage * 100, 0, 100);
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  int batteryLevel = percent;
  batteryLevel = 52;

  display.clearDisplay();


  display.drawRect(x, y, w, h, WHITE);
  display.fillRect(x + w, y + (h / 3), capWidth, h / 3, WHITE);

  int fillWidth = map(batteryLevel, 0, 100, 0, w - 2);
  display.fillRect(x + 1, y + 1, fillWidth, h - 2, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x + w + 5, y + 1);
  display.print(batteryLevel);
  display.print("%");
}




void AirMouseProject ::Mode(String mode) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x + w + 35, y + 1);
  display.print(mode);
}

int T_Stat = 0;
void AirMouseProject ::Time(int T_isBlink) {
  DateTime now = rtc.now();
  display.setTextSize(3);
  display.setCursor(18, y + 20);
  if (T_Stat == 0 && T_isBlink == 1) {
    display.setTextColor(BLACK);
    if (now.hour() < 10) { display.print("0"); }
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10) { display.print("0"); }
    display.print(now.minute());
    T_Stat = 1;
  } else {
    display.setTextColor(WHITE);
    if (now.hour() < 10) { display.print("0"); }
    display.print(now.hour());
    display.print(":");
    if (now.minute() < 10) { display.print("0"); }
    display.print(now.minute());
    T_Stat = 0;
  }
  display.setTextSize(1.5);
  display.setCursor(34, y + 48);
  display.print(now.year());
  display.print("-");
  if (now.month() < 10) { display.print("0"); }
  display.print(now.month());
  display.print("-");
  if (now.day() < 10) { display.print("0"); }
  display.print(now.day());
}
int BT_Stat = 0;
const unsigned char btlogo[] PROGMEM = { 0x20, 0xb0, 0x68, 0x30, 0x30, 0x68, 0xb0, 0x20 };
const unsigned char clearBTlogo[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void AirMouseProject ::BTlogo(int BT_isBlink, int LEDpin) {
  if (BT_isBlink == 1 && BT_Stat == 0) {
    display.drawBitmap(120, 1, clearBTlogo, 5, 8, WHITE);
    digitalWriteRegister(LEDpin, LOW);
    BT_Stat = 1;
  } else {
    display.drawBitmap(120, 1, btlogo, 5, 8, WHITE);
    digitalWriteRegister(LEDpin, HIGH);
    BT_Stat = 0;
  }
}


bool laststate2 = HIGH;
bool AirMouseProject ::PressAction(int pin) {
  bool buttonstate2 = digitalReadRegister(pin);
  if (buttonstate2 == LOW && laststate2 == HIGH) {
    laststate2 = buttonstate2;
    return true;
  }
  laststate2 = buttonstate2;
  return false;
}



int AirMouseProject ::flipTimer(int time) {
  Tcurrenttime = millis();
  int du = Tcurrenttime - Tstarttime;

  if (du < time) {
    return 2;
  }
  if (du < 2 * time) {
    return 1;
  }
  if (du >= 2 * time) {
    Tstarttime = Tcurrenttime;
    return 1;
  }
}



void AirMouseProject ::BluetoothMouseBegin() {
  BLEDevice::init("CM_MOUSE");
  pServer = BLEDevice::createServer();
  bleMouse.begin();
  //lastTime = micros();
  pServer->startAdvertising();
  Serial.println("Started BLE Mouse");
}

void AirMouseProject ::GyroCheck() {
  if (!mpu.begin(0x69)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.enableSleep(sleepMPU);
}



void AirMouseProject ::MouseMove() {
  if (bleMouse.isConnected()) {
    isConnected = true;
    wasconnected = true;
    if (sleepMPU) {
      delay(3000);
      Serial.println("MPU6050 awakened!");
      sleepMPU = false;
      mpu.enableSleep(sleepMPU);
      delay(500);
    }

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float y = g.gyro.z * 100;

    float xVal = 0;
    if (!middleButtonState) {
      if (y > -20 && y < 20) {
        bleMouse.move(g.gyro.x * SPEED, 0 * SPEED);
      } else {
        bleMouse.move(g.gyro.x * SPEED, g.gyro.z * -SPEED);
      }

    } else {
      MouseangleZ();
      if (angleZ > 5 && angleZ < 25) {
        delay(100);
        bleMouse.move(0, 0, 1);
      }
      if (angleZ > 25) {
        delay(30);
        bleMouse.move(0, 0, 1);
      }

      if (angleZ < -5 && angleZ > -25) {
        delay(100);
        bleMouse.move(0, 0, -1);
      }

      if (angleZ < -25) {
        delay(30);
        bleMouse.move(0, 0, -1);
      }
    }
  }

  if (!bleMouse.isConnected() && wasconnected) {
    Serial.println("auto DISCONNECT");
    pServer->disconnect(pServer->getConnId());

    delay(500);
    pServer->startAdvertising();
    Serial.println("Advertising auto");


    delay(20);
    wasconnected = false;
    isConnected = false;
  }
}

bool laststate3 = HIGH;
void AirMouseProject ::MouseLeftButton(int pin) {
  bool buttonstate1 = digitalReadRegister(pin);
  if (buttonstate1 == LOW && laststate3 == HIGH) {
    Serial.println("left press");
    bleMouse.press();
  }


  if (buttonstate1 == HIGH && laststate3 == LOW) {
    bleMouse.release();
    Serial.println("left relase");
  }

  laststate3 = buttonstate1;
}


bool laststate4 = HIGH;
void AirMouseProject ::MouseRightButton(int pin) {
  bool buttonstate1 = digitalReadRegister(pin);
  if (buttonstate1 == LOW && laststate4 == HIGH) {
    Serial.println("left press");
    bleMouse.press(MOUSE_RIGHT);
  }


  if (buttonstate1 == HIGH && laststate4 == LOW) {
    bleMouse.release(MOUSE_RIGHT);
    Serial.println("left relase");
  }

  laststate4 = buttonstate1;
}




bool laststate5 = HIGH;
void AirMouseProject ::MouseMiddleButton(int pin) {
  bool buttonstate2 = digitalReadRegister(pin);
  if (buttonstate2 == LOW && laststate5 == HIGH) {
    middleButtonState = true;
    Serial.println("middle press");
    angleZ = 0;
  }


  if (buttonstate2 == HIGH && laststate5 == LOW) {
    middleButtonState = false;
    Serial.println("middeled relase");
  }

  laststate5 = buttonstate2;
}




void AirMouseProject ::MouseangleZ() {

  unsigned long now = millis();

  float dt = (now - angleLastTime) / 1000.0;

  angleLastTime = now;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  // float gz = g.gyro.z ;
  float gz = g.gyro.z;

  float gzx = gz * 100;

  if (gzx > -10 && gzx < 10) {
    angleZ += 0 * 57.2958 * dt;
  } else {
    angleZ += gz * 57.2958 * dt;
  }
  Serial.print(" | Z: ");
  Serial.println(angleZ);
}
