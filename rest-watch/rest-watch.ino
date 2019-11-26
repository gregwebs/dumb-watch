#include <TinyScreen.h>
#include <RTCZero.h>

#define DEBUG 1

void onButtonPress(TinyScreen display, RTCZero rtc) {
    byte buttons = display.getButtons();
    displaySetTime(buttons, rtc);
    updateMainDisplay(display, rtc);
    display.on();
}

void setup() {
    Screen_setup(5, &onButtonPress);
}

void loop() {
    Screen_loop();
}

int brightness = 3;
void updateMainDisplay(TinyScreen display, RTCZero rtc) {
    msg("update display");
    updateTimeDisplay(display, rtc);
    displayBattery(display);
}

uint8_t timeSettingPosition = 0;
bool displaySetTime(uint8_t buttons, RTCZero rtc) {
    int change = 0;
    int currentHour, currentMinute, currentSecond;
    if (buttons == TSButtonUpperRight) {
        change = 1;
    } else if (buttons == TSButtonLowerRight) {
        change = -1;
    } else if (buttons == TSButtonUpperLeft) {
        timeSettingPosition++;
        timeSettingPosition = timeSettingPosition % 3;
    }

    if (change != 0) {
        currentHour = rtc.getHours();
        currentMinute = rtc.getMinutes();
        currentSecond = rtc.getSeconds();
        if (timeSettingPosition == 0) {
            rtc.setTime(currentHour + change, currentMinute, currentSecond);
        }
        if (timeSettingPosition == 1) {
            rtc.setTime(currentHour, currentMinute + change, currentSecond);
        }
        if (timeSettingPosition == 2) {
            rtc.setTime(currentHour, currentMinute, currentSecond + change);
        }
        return true;
    }

    return false;
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
void updateTimeDisplay(TinyScreen display, RTCZero rtc) {
  // TODO: use getPrintWidth for proper pixel counting
  int currentHour, currentMinute, currentSecond;
  currentHour = rtc.getHours();
  currentMinute = rtc.getMinutes();
  currentSecond = rtc.getSeconds();
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
    writeColon(display);

    display.setFont(font22pt);
    lastMinuteDisplayed = currentMinute;
    displayX = 14 + 14 + 6;
    display.setCursor(displayX, timeY);
    if (lastMinuteDisplayed < 10)display.print('0');
    display.print(lastMinuteDisplayed);
    displayX = 14 + 14 + 6 + 14 + 14;
    display.setCursor(displayX, timeY);
    writeColon(display);

    display.setFont(font22pt);
    lastSecondDisplayed = currentSecond;
    displayX = 14 + 14 + 14 + 14 + 6 + 6;
    display.setCursor(displayX, timeY);
    if (lastSecondDisplayed < 10)display.print('0');
    display.print(lastSecondDisplayed);
}

uint8_t blink_colon = 0;
void writeColon(TinyScreen display) {
  if (!blink_colon) {
      msg("colon");
    display.write(' ');
  } else {
      msg("space");
    display.write(':');
  }
}

void displayBattery(TinyScreen display) {
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