#ifndef AirMouseProject_h
#define AirMouseProject_h

class AirMouseProject {
public:
  bool sleepMPU;

  AirMouseProject();

  void pinModeRegister(uint8_t pin, uint8_t mode);
  void digitalWriteRegister(uint8_t pin, uint8_t value);
  int digitalReadRegister(uint8_t pin);
  void OLEDbegin();
  void OledDisplay();
  void OledClear();
  void RtcCheck();
  void BTlogo(int BT_isBlink, int LEDpin);
  void BatteryPre();
  void Mode(String mode);
  void Time(int T_isBlink);
  bool holdPressTimeAction(int time, int pin);
  bool PressAction(int pin);
  int flipTimer(int time);

  void BluetoothMouseBegin();
  void GyroCheck();
  void MouseMove();
  //void BluetoothReset();
  void MouseLeftButton(int pin);
  void MouseRightButton(int pin);
  void MouseMiddleButton(int pin);
  void MouseangleZ();
  bool isConnected;
  unsigned long angleLastTime;


private:

  float batteryVoltage;
  int x;
  int y;
  int w;  // battery width
  int h;  // battery height
  int capWidth;

  long startTime1;
  long endTime1;
  long currentTime1;
  bool laststate1;
  bool isPress1;
  bool isPressPrint1;

  long Tstarttime;
  long Tcurrenttime;

  bool wasconnected;
  bool middleButtonState;
};

#endif
