#include <TinyScreen.h>
#include <RTCZero.h>
RTCZero RTCZ;

TinyScreen display = TinyScreen(TinyScreenDefault);

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
void setup_display() {
  pinMode(42, INPUT_PULLUP);
  pinMode(44, INPUT_PULLUP);
  pinMode(45, INPUT_PULLUP);
  pinMode(A4, INPUT);
  pinMode(2, INPUT);
  RTCZ.begin();
  RTCZ.setTime(16, 15, 1);//h,m,s
  //RTCZ.setDate(25, 7, 16);//d,m,y
  attachInterrupt(TSP_PIN_BT1, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT2, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT3, wakeHandler, FALLING);
  attachInterrupt(TSP_PIN_BT4, wakeHandler, FALLING);

  // pinMode(vibratePin, OUTPUT);
  // digitalWrite(vibratePin, vibratePinInactive);

  display.begin();
  display.setFlip(true);
  initHomeScreen();
  // 0-15
  display.setBrightness(10);

  // https://github.com/arduino/ArduinoCore-samd/issues/142
  // Clock EIC in sleep mode so that we can use pin change interrupts
  // The RTCZero library will setup generic clock 2 to XOSC32K/32
  // and we'll use that for the EIC. Increases power consumption by ~50uA
  GCLK->CLKCTRL.reg = uint16_t(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2 | GCLK_CLKCTRL_ID( GCLK_CLKCTRL_ID_EIC_Val ) );
  while (GCLK->STATUS.bit.SYNCBUSY) {}
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
  if (display.getButtons(TSButtonUpperLeft) && display.getButtons(TSButtonUpperRight)) {
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
  if (lastSetBrightness != brightness) {
    display.setBrightness(brightness);
    lastSetBrightness = brightness;
  }
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
uint8_t defaultFontColor = TS_8b_White;
uint8_t defaultFontBG = TS_8b_Black;
uint8_t inactiveFontColor = TS_8b_Gray;
uint8_t inactiveFontBG = TS_8b_Black;
uint8_t timeY = 14;
void updateTimeDisplay() {
  int currentHour, currentMinute, currentSecond;
#if defined (ARDUINO_ARCH_AVR)
  currentHour = hour();
  currentMinute = minute();
  currentSecond = second();
#elif defined(ARDUINO_ARCH_SAMD)
  currentHour = RTCZ.getHours();
  currentMinute = RTCZ.getMinutes();
  currentSecond = RTCZ.getSeconds();
#endif
  char displayX;
  int hour12 = currentHour;
  int AMPM = 1;
  if (hour12 > 12) {
    AMPM = 2;
    hour12 -= 12;
  }
  display.fontColor(defaultFontColor, defaultFontBG);
  if (rewriteTime || lastHourDisplayed != hour12) {
    display.setFont(font22pt);
    lastHourDisplayed = hour12;
    displayX = 0;
    display.setCursor(displayX, timeY);
    if (lastHourDisplayed < 10)display.print('0');
    display.print(lastHourDisplayed);
    writeColon();
    if (lastAMPMDisplayed != AMPM) {
      if (AMPM == 2)
        display.fontColor(inactiveFontColor, inactiveFontBG);
      display.setFont(font10pt);
      display.setCursor(displayX + 80, timeY - 0);
      display.print(F("AM"));
      if (AMPM == 2) {
        display.fontColor(defaultFontColor, defaultFontBG);
      } else {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      display.setCursor(displayX + 80, timeY + 11);
      display.print(F("PM"));
      display.fontColor(defaultFontColor, defaultFontBG);
    }
  }

  if (rewriteTime || lastMinuteDisplayed != currentMinute) {
    display.setFont(font22pt);
    lastMinuteDisplayed = currentMinute;
    displayX = 14 + 14 - 1;
    display.setCursor(displayX, timeY);
    if (lastMinuteDisplayed < 10)display.print('0');
    display.print(lastMinuteDisplayed);
    writeColon();
  }

  if (rewriteTime || lastSecondDisplayed != currentSecond) {
    display.setFont(font22pt);
    lastSecondDisplayed = currentSecond;
    displayX = 14 + 14 + 14 + 14 - 2;
    display.setCursor(displayX, timeY);
    if (lastSecondDisplayed < 10)display.print('0');
    display.print(lastSecondDisplayed);
  }
  rewriteTime = false;
}

void displayBattery() {
  int result = 0;
#if defined (ARDUINO_ARCH_AVR)
  //http://forum.arduino.cc/index.php?topic=133907.0
  const long InternalReferenceVoltage = 1100L;
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);
  delay(10);
  ADCSRA |= _BV( ADSC );
  while ( ( (ADCSRA & (1 << ADSC)) != 0 ) );
  result = (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L;
  //SerialMonitorInterface.println(result);
  //if(result>440){//probably charging
  uint8_t charging = false;
  if (result > 450) {
    charging = true;
  }
  result = constrain(result - 300, 0, 120);
  uint8_t x = 70;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t amtActive = (result * length) / 120;
  uint8_t red, green, blue;
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF); //left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF); //top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1, 0xFF); //bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1, 0xFF); //right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2, 0xFF); //right border
  for (uint8_t i = 0; i < length; i++) {
    if (i < amtActive) {
      red = 63 - ((63 / length) * i);
      green = ((63 / length) * i);
      blue = 0;
    } else {
      red = 32;
      green = 32;
      blue = 32;
    }
    display.drawLine(x + i, y, x + i, y + height, red, green, blue);
  }
#elif defined(ARDUINO_ARCH_SAMD)
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
  uint8_t x = 70;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t red, green;
  if (result > 325) {
    red = 0;
    green = 63;
  } else {
    red = 63;
    green = 0;
  }
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF); //left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF); //top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1, 0xFF); //bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1, 0xFF); //right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2, 0xFF); //right border
  for (uint8_t i = 0; i < length; i++) {
    display.drawLine(x + i, y, x + i, y + height, red, green, 0);
  }
#endif
}
