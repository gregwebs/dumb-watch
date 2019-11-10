#include <TinyScreen.h>
#include <RTCZero.h>
RTCZero RTCZ;

// TinyScreen display = TinyScreen(TinyScreenDefault);
TinyScreen display = TinyScreen(TinyScreenPlus);

unsigned long millisOffsetCount = 0;
uint32_t startTime = 0;
uint32_t sleepTime = 0;

void wakeHandler() {
  SerialMonitorInterface.println("wake up!");
  if (sleepTime) {
    millisOffsetCount += (RTCZ.getEpoch() - sleepTime);
    sleepTime = 0;
  }
}

void RTCwakeHandler() {
  //not used
}

void watchSleep() {
  // if (doVibrate) return;
  sleepTime = RTCZ.getEpoch();
  RTCZ.standbyMode();
}

uint8_t vibratePin = 4;
uint8_t vibratePinActive = HIGH;
uint8_t vibratePinInactive = LOW;

void setup_default_screen(){

}
void setup_display() {
  /* 
  for (int i = 0; i < 20; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  // Original code did not have a gap here
  for (int i = 22; i < 27; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  */
  for (int i = 0; i < 27; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  // Currently terminal won't work if these are pulled
  // pinMode(28, INPUT_PULLUP);
  // pinMode(29, INPUT_PULLUP);
  // These are buttons?
  // pinMode(31, INPUT_PULLUP);
  // pinMode(32, INPUT_PULLUP);
  pinMode(TSP_PIN_BT1, INPUT_PULLUP);
  pinMode(TSP_PIN_BT2, INPUT_PULLUP);
  pinMode(TSP_PIN_BT3, INPUT_PULLUP);
  pinMode(TSP_PIN_BT4, INPUT_PULLUP);

  pinMode(42, INPUT_PULLUP);
  pinMode(44, INPUT_PULLUP);
  pinMode(45, INPUT_PULLUP);
  pinMode(A4, INPUT);
  pinMode(2, INPUT);

  // Originally was FALLING
  attachInterrupt(TSP_PIN_BT1, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT2, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT3, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT4, wakeHandler, FALLING);

  // pinMode(vibratePin, OUTPUT);
  // digitalWrite(vibratePin, vibratePinInactive);

  RTCZ.begin();
  RTCZ.setTime(16, 15, 1);//h,m,s
  //RTCZ.setDate(25, 7, 19);//d,m,y
  display.begin();
  display.setFlip(true);
  // 0-15
  display.setBrightness(10);
  initHomeScreen();

  // https://github.com/arduino/ArduinoCore-samd/issues/142
  // Clock EIC in sleep mode so that we can use pin change interrupts
  // The RTCZero library will setup generic clock 2 to XOSC32K/32
  // and we'll use that for the EIC. Increases power consumption by ~50uA
  GCLK->CLKCTRL.reg = uint16_t(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2 | GCLK_CLKCTRL_ID( GCLK_CLKCTRL_ID_EIC_Val ) );
  while (GCLK->STATUS.bit.SYNCBUSY) {}
}

void fillScreen() {
  // Draw a rectangle the same size as the screen and fill it with a color.
  // drawRect(x, y, w, h, fill (0 or 1), r, g, b)
  display.drawRect(0, 0, display.xMax, display.yMax, 1,random(255),random(255),random(255));
  requestScreenOn();
}

uint8_t rewriteTime = true;
void initHomeScreen() {
  display.clearWindow(0, 12, 96, 64);
  rewriteTime = true;
  updateMainDisplay();
}

unsigned long sleepTimer = 0;
int sleepTimeout = 5;
uint8_t displayOn = 1;
// only read accelResult
void loop_display(struct AccelResult *accelResult){
  checkButtons();

  if (millisOffset() > sleepTimer + ((unsigned long)sleepTimeout*1000ul)) {
    if (displayOn) {
      SerialMonitorInterface.println("display off");
      displayOn = 0;
      display.off();
    }
    // watchSleep();
  }
}

uint8_t buttonReleased = 1;

int requestScreenOn() {
    SerialMonitorInterface.println("request screen on!");
  sleepTimer = millisOffset();
  if (!displayOn) {
    updateMainDisplay();
    displayOn = 1;
    display.on();
    return 1;
  }
  return 0;
}

uint8_t newDisplayState;
const uint8_t displayStateHome = 0x01;
const uint8_t displayStateSetTime = 0x02;
void checkButtons() {
  byte buttons = display.getButtons();
  //SerialMonitorInterface.println(buttons);
  //char data[3];
  //sprintf(data, "%d", result);
  if (display.getButtons(TSButtonUpperLeft) && display.getButtons(TSButtonUpperRight)) {
    SerialMonitorInterface.println("double press!");
    newDisplayState = displayStateSetTime;
    buttonReleased = 1;
    buttonPress(buttons);
    return;
  }
  if (buttonReleased && buttons) {
    SerialMonitorInterface.println("Pressed!");
    if (displayOn) buttonPress(buttons);
    requestScreenOn();
    buttonReleased = 0;
  }
  if (!buttonReleased && !(buttons & 0x0F)) {
    SerialMonitorInterface.println("buttons released");
    buttonReleased = 1;
  }
}

uint8_t timeSettingPosition = 0;
uint8_t blink_colon = 0;
void displaySetTime(uint8_t buttons) {
  int change = 0;
  int currentHour, currentMinute, currentSecond;
  if (buttons & TSButtonUpperLeft) {
    if (buttons & TSButtonUpperRight) {
      timeSettingPosition++;
      timeSettingPosition = timeSettingPosition % 3;
      change = 0;
    } else {
      change = 1;
    }
  } else if (buttons & TSButtonUpperRight) {
    change = -1;
  }
  if (change != 0) {
    currentHour = RTCZ.getHours();
    currentMinute = RTCZ.getMinutes();
    currentSecond = RTCZ.getSeconds();
    if (timeSettingPosition == 0) {
      RTCZ.setTime(currentHour + change, currentMinute, currentSecond);
    }
    if (timeSettingPosition == 1) {
      RTCZ.setTime(currentHour, currentMinute + change, currentSecond);
    }
    if (timeSettingPosition == 2) {
      RTCZ.setTime(currentHour, currentMinute, currentSecond + change);
    }
  }
  rewriteTime = true;
  blink_colon = !blink_colon;
  updateTimeDisplay();
}


void (*menuHandler)(uint8_t) = NULL;
uint8_t currentDisplayState = displayStateHome;
void buttonPress(uint8_t buttons) {
  updateMainDisplay();
  if (currentDisplayState == displayStateHome) {
    if (newDisplayState == displayStateSetTime) {
      SerialMonitorInterface.println("Setting Time!");
  timeSettingPosition++;
  timeSettingPosition = timeSettingPosition % 3;
        displaySetTime(0x0);
  currentDisplayState = displayStateSetTime;
    }
  } else if (currentDisplayState == displayStateSetTime) {
    if (newDisplayState == displayStateHome) {
  currentDisplayState = displayStateHome;
  updateMainDisplay();
    } else {
        displaySetTime(buttons);
    }
  }
}

unsigned long mainDisplayUpdateInterval = 300;
unsigned long lastMainDisplayUpdate = 0;
uint8_t lastSetBrightness = 100;
int brightness = 3;
void updateMainDisplay() {
      SerialMonitorInterface.println("update display");
  updateTimeDisplay();
  displayBattery();
  lastMainDisplayUpdate = millisOffset();
}

uint32_t millisOffset() {
#if defined (ARDUINO_ARCH_AVR)
  return millis();
#elif defined(ARDUINO_ARCH_SAMD)
  return (millisOffsetCount * 1000ul) + millis();
#endif
}

void writeColon() {
  if (!blink_colon) {
      SerialMonitorInterface.println("colon");
    display.write(' ');
  } else {
      SerialMonitorInterface.println("space");
    display.write(':');
  }
}

uint8_t lastAMPMDisplayed = 0;
uint8_t lastHourDisplayed = -1;
uint8_t lastMinuteDisplayed = -1;
uint8_t lastSecondDisplayed = -1;
const FONT_INFO& font10pt = thinPixel7_10ptFontInfo;
const FONT_INFO& font22pt = liberationSansNarrow_22ptFontInfo;
uint8_t defaultFontColor = TS_8b_Red;
uint8_t defaultFontBG = TS_8b_Black;
uint8_t inactiveFontColor = TS_8b_Gray;
uint8_t inactiveFontBG = TS_8b_Black;
uint8_t timeY = 18;
void updateTimeDisplay() {
  // TODO: use getPrintWidth for proper pixel counting
  int currentHour, currentMinute, currentSecond;
  currentHour = RTCZ.getHours();
  currentMinute = RTCZ.getMinutes();
  currentSecond = RTCZ.getSeconds();
  char displayX = 2;
  int hour12 = currentHour;
  int AMPM = 1;
  if (hour12 > 12) {
    AMPM = 2;
    hour12 -= 12;
  }
  if ((AMPM == 2) && (hour12 > 6)) {
    defaultFontColor = TS_8b_Red;
    display.setBrightness(5);
      } else if ((AMPM == 1) && (hour12 < 8)) {
    defaultFontColor = TS_8b_Red;
    display.setBrightness(5);
      } else {
    defaultFontColor = TS_8b_White;
    display.setBrightness(10);
  }
  display.fontColor(defaultFontColor, defaultFontBG);
  if (rewriteTime || lastHourDisplayed != hour12) {
    if (lastAMPMDisplayed != AMPM) {
      if (AMPM == 2) {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      display.setFont(font10pt);
      display.setCursor(displayX + 67, 3);
      display.print(F("AM"));
      if (AMPM == 2) {
        display.fontColor(defaultFontColor, defaultFontBG);
      } else {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      display.setCursor(displayX + 82, 3);
      display.print(F("PM"));
      display.fontColor(defaultFontColor, defaultFontBG);
    }
    display.setFont(font22pt);
    lastHourDisplayed = hour12;
    display.setCursor(displayX, timeY);
    if (lastHourDisplayed < 10)display.print('0');
    display.print(lastHourDisplayed);
    displayX = 14 + 14;
    display.setCursor(displayX, timeY);
    writeColon();
  }

  if (rewriteTime || lastMinuteDisplayed != currentMinute) {
    display.setFont(font22pt);
    lastMinuteDisplayed = currentMinute;
    displayX = 14 + 14 + 6;
    display.setCursor(displayX, timeY);
    if (lastMinuteDisplayed < 10)display.print('0');
    display.print(lastMinuteDisplayed);
    displayX = 14 + 14 + 6 + 14 + 14;
    display.setCursor(displayX, timeY);
    writeColon();
  }

  if (rewriteTime || lastSecondDisplayed != currentSecond) {
    display.setFont(font22pt);
    lastSecondDisplayed = currentSecond;
    displayX = 14 + 14 + 14 + 14 + 6 + 6;
    display.setCursor(displayX, timeY);
    if (lastSecondDisplayed < 10)display.print('0');
    display.print(lastSecondDisplayed);
  }
  rewriteTime = false;
}

void displayBattery() {
  int result = 0;
  //http://atmel.force.com/support/articles/en_US/FAQ/ADC-example
  SYSCTRL->VREF.reg |= SYSCTRL_VREF_BGOUTEN;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SAMPCTRL.bit.SAMPLEN = 0x1;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->INPUTCTRL.bit.MUXPOS = 0x19;         // Internal bandgap input
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC
  // Start conversion
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Clear the Data Ready flag
  ADC->INTFLAG.bit.RESRDY = 1;
  // Start conversion again, since The first conversion after the reference is changed must not be used.
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Store the value
  while ( ADC->INTFLAG.bit.RESRDY == 0 );   // Waiting for conversion to complete
  uint32_t valueRead = ADC->RESULT.reg;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  SYSCTRL->VREF.reg &= ~SYSCTRL_VREF_BGOUTEN;
  result = (((1100L * 1024L) / valueRead) + 5L) / 10L;
  uint8_t x = 7;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t red, green;

  if (result > 325) {
    green = 63;
    red = 0;
  } else if (result > 250) {
    red = 63;
    green = 63;
  } else {
    red = 63;
    green = 0;
  }
  if (result > 325) {
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF); //left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF); //top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1, 0xFF); //bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1, 0xFF); //right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2, 0xFF); //right border
  for (uint8_t i = 0; i < length; i++) {
    display.drawLine(x + i, y, x + i, y + height, red, green, 0);
  }
  }

  // debug print of
  char data[3];
  sprintf(data, "%d", result);
  display.setFont(font10pt);
  display.setCursor(x + length + 1, y);
  display.print(data);
}

/*
void buttonLoop() {
  display.setCursor(0, 0);
  //getButtons() function can be used to test if any button is pressed, or used like:
  //getButtons(TSButtonUpperLeft) to test a particular button, or even like:
  //getButtons(TSButtonUpperLeft|TSButtonUpperRight) to test multiple buttons
  //results are flipped as you would expect when setFlip(true)
  if (display.getButtons(TSButtonUpperLeft)) {
    display.println("Pressed!");
  } else {
    display.println("          ");
  }
  display.setCursor(0, 54);
  if (display.getButtons(TSButtonLowerLeft)) {
    display.println("Pressed!");
  } else {
    display.println("          ");
  }
  display.setCursor(95 - display.getPrintWidth("Pressed!"), 0);
  if (display.getButtons(TSButtonUpperRight)) {
    display.println("Pressed!");
  } else {
    display.println("          ");
  }
  display.setCursor(95 - display.getPrintWidth("Pressed!"), 54);
  if (display.getButtons(TSButtonLowerRight)) {
    display.println("Pressed!");
  } else {
    display.println("          ");
  }
}
*/

void hardwareDrawCommands(){
  //Accelerated drawing commands are executed by the display controller
  //clearScreen();//clears entire display- the same as clearWindow(0,0,96,64)
  display.clearScreen();
  //drawRect(x stary, y start, width, height, fill, 8bitcolor);//sets specified OLED controller memory to an 8 bit color, fill is a boolean- TSRectangleFilled or TSRectangleNoFill
  display.drawRect(10,10,76,44,TSRectangleFilled,TS_8b_Red);
  //drawRect(x stary, y start, width, height, fill, red, green, blue);//like above, but uses 6 bit color values. Red and blue ignore the LSB.
  display.drawRect(15,15,66,34,TSRectangleFilled,20,30,60);
  //clearWindow(x start, y start, width, height);//clears specified OLED controller memory
  display.clearWindow(20,20,56,24);
  //drawLine(x1, y1, x2, y2, 8bitcolor);//draw a line from (x1,y1) to (x2,y2) with an 8 bit color
  display.drawLine(0,0,95,63,TS_8b_Green);
  //drawLine(x1, y1, x2, y2, red, green, blue);//like above, but uses 6 bit color values. Red and blue ignore the LSB.
  display.drawLine(0,63,95,0,0,63,0);
  delay(1000);
  //use 16 bit version of drawLine to fade a rectangle from blue to green:
  for(int i=0;i<64;i++){
    display.drawLine(0,i,95,i,0,i,63-i);
  }
  delay(1000);
}