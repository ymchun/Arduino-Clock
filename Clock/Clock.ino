#include <LowPower.h>
#include <SparkFunDS3234RTC.h>
#include <LiquidCrystal_I2C.h>
#include <avr/sleep.h>

// DeadOn RTC Chip-select pin
#define DS13074_CS_PIN 10

// push button pins
#define SETTING_MODE_PIN 3
#define INCREMENT_PIN 4
#define DECREMENT_PIN 5

// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3f, 16, 2);

// define day name
const char *dayOfWeek[7] {
    "SUN", "MON", "TUE", "WED",
    "THU", "FRI", "SAT"
};

// define month name
const char *months[12] {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// define characters
const uint8_t deg_c[8] = {
    B11000,
    B11000,
    B00111,
    B01000,
    B01000,
    B01000,
    B00111
};

// the second recently updated
unsigned char lastSync = 255;

// temperature, multiples of 0.25 deg C
unsigned char temperature = 0;

// time format
bool use12Hours = true;
bool isPM = true;

// counters for time
unsigned char counterSecond = 0;
unsigned char counterMinute = 0;
unsigned char counterHour = 0;

// counters for date
unsigned char counterDay = 0;
unsigned char counterDate = 0;
unsigned char counterMonth = 0;
unsigned int counterYear = 0;

// display for temperature
String displayTemperature;

// display for time
String displayPeriod; // AM or PM
String displayHour;
String displayMinute;
String displaySecond;

// display for date
String displayYear;
String displayMonth;
String displayDate;
String displayDay;

// setting mode
unsigned char settingStage = 0;

// debounce
const unsigned int debounceDelay = 500;
unsigned long lastDebounce = 0;

void switchSetting() {
    if ((millis() - lastDebounce) > debounceDelay) {
        // next setting
        settingStage++;
        // reset debounce time
        lastDebounce = millis();
    }
}

void setDisplay(String &display, unsigned char value) {
    if (value <= 9) {
        display = "0" + String(value);
    } else {
        display = String(value);
    }
}

void setPeriodDisplay(String &display) {
    if (use12Hours) {
        display = isPM ? "PM" : "AM";
    } else {
        display = "--";
    }
}

void setTemperatureDisplay(String &display, unsigned char value) {
    display = String(value);
}

void setYearDisplay(String &display, unsigned int year) {
    if (year > 99) {
        display = "2" + String(year);
    } else if (year > 9) {
        display = "20" + String(year);
    } else {
        display = "200" + String(year);
    }
}

void setup() {
    // disable ADC
    ADCSRA = 0;

    // Disable the analog comparator by setting the ACD bit
    // (bit 7) of the ACSR register to one.
    ACSR = B10000000;

    // Disable digital input buffers on all analog input pins
    // by setting bits 0-5 of the DIDR0 register to one.
    // Of course, only do this if you are not using the analog
    // inputs for your project.
    DIDR0 = DIDR0 | B00111111;

    // set pin mode
    pinMode(SETTING_MODE_PIN, INPUT_PULLUP);
    pinMode(INCREMENT_PIN, INPUT_PULLUP);
    pinMode(DECREMENT_PIN, INPUT_PULLUP);

    // setup interrupt
    attachInterrupt(digitalPinToInterrupt(SETTING_MODE_PIN), switchSetting, FALLING);

    // initialize the LCD
    lcd.begin();
    lcd.createChar(0, deg_c);

    // initialize the clock
    rtc.begin(DS13074_CS_PIN);

    // reset debounce time
    lastDebounce = millis();
}

void loop() {
    if (settingStage == 0) {
        // clock view

        // update clock values
        rtc.update();

        if (rtc.second() != lastSync) {
            use12Hours = rtc.is12Hour();
            isPM = rtc.pm();

            // get time
            counterSecond = rtc.second();
            counterMinute = rtc.minute();
            counterHour = rtc.hour();

            // get date
            counterDay = rtc.day();
            counterDate = rtc.date();
            counterMonth = rtc.month();
            counterYear = rtc.year();

            // get temperature
            temperature = rtc.temperature();

            // get clock string values
            setDisplay(displaySecond, counterSecond);
            setDisplay(displayMinute, counterMinute);
            setDisplay(displayHour, counterHour);
            setPeriodDisplay(displayPeriod);
            setDisplay(displayDate, counterDate);
            setYearDisplay(displayYear, counterYear);
            setTemperatureDisplay(displayTemperature, temperature);
            displayDay = dayOfWeek[counterDay - 1];
            displayMonth = months[counterMonth - 1];

            // clear all content
            lcd.clear();

            // first line
            lcd.setCursor(0, 0);
            // print date & day
            lcd.print(displayDay + " " + displayDate + " " + displayMonth + " " + displayYear);

            // second line
            lcd.setCursor(0, 1);
            // print time
            lcd.print(displayHour + ":" + displayMinute + ":" + displaySecond + " " + displayPeriod + " " + displayTemperature);
            // print temperature symbol
            lcd.print((char) 0);

            // update last second
            lastSync = counterSecond;
        }

        // sleep
        LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);

    } else {
        // setting view

        // clear all content
        lcd.clear();

        // set year
        if (settingStage == 1) {
            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterYear++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterYear--;
            // check value
            if (counterYear > 100) counterYear = 100;
            if (counterYear < 0) counterYear = 0;

            // set year
            setYearDisplay(displayYear, counterYear);

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Year:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayYear);

        } else if(settingStage == 2) {
            // set month

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterMonth++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterMonth--;
            // check value
            if (counterMonth > 12) counterMonth = 12;
            if (counterMonth < 1) counterMonth = 1;

            // set month
            displayMonth = months[counterMonth - 1];

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Month:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayMonth);

        } else if(settingStage == 3) {
            // set date of month

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterDate++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterDate--;
            // check value
            if (counterDate > 31) counterDate = 31;
            if (counterDate < 1) counterDate = 1;

            // set date
            setDisplay(displayDate, counterDate);

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Date:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayDate);

        } else if(settingStage == 4) {
            // set day of week

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterDay++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterDay--;
            // check value
            if (counterDay > 7) counterDay = 7;
            if (counterDay < 1) counterDay = 1;

            // set day
            displayDay = dayOfWeek[counterDay - 1];

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Day:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayDay);

        } else if(settingStage == 5) {
            // set hour

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterHour++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterHour--;
            // check value
            if (counterHour > 23) counterHour = 23;
            if (counterHour < 0) counterHour = 0;

            // set hour
            setDisplay(displayHour, counterHour);

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Hour:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayHour);

        } else if(settingStage == 6) {
            // set minute

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterMinute++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterMinute--;
            // check value
            if (counterMinute > 59) counterMinute = 59;
            if (counterMinute < 0) counterMinute = 0;

            // set minute
            setDisplay(displayMinute, counterMinute);

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Minute:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displayMinute);

        } else if(settingStage == 7) {
            // set second

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) counterSecond++;
            if (digitalRead(DECREMENT_PIN) == LOW) counterSecond--;
            // check value
            if (counterSecond > 59) counterSecond = 59;
            if (counterSecond < 0) counterSecond = 0;

            // set second
            setDisplay(displaySecond, counterSecond);

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Set Second:");

            // second line
            lcd.setCursor(0, 1);
            lcd.print(">: " + displaySecond);

        } else if(settingStage == 8) {
            // set time format

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW || digitalRead(DECREMENT_PIN) == LOW) {
                use12Hours = !use12Hours;
            }

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Use 12 hour:");

            // second line
            lcd.setCursor(0, 1);

            if (use12Hours) {
                lcd.print(">: Yes");
            } else {
                lcd.print(">: No");
            }

        } else if(settingStage == 9) {
            // set period

            // skip this page when use 24 hours format
            if (use12Hours) {
                // check button click
                if (digitalRead(INCREMENT_PIN) == LOW || digitalRead(DECREMENT_PIN) == LOW) {
                    isPM = !isPM;
                }

                // first line
                lcd.setCursor(0, 0);
                lcd.print("Period:");

                // second line
                lcd.setCursor(0, 1);

                if (isPM) {
                    lcd.print(">: PM");
                } else {
                    lcd.print(">: AM");
                }
            } else {
                settingStage++;
            }

        } else if(settingStage == 10) {
            // confirm

            // check button click
            if (digitalRead(INCREMENT_PIN) == LOW) {
                // go back
                settingStage = 0;

                if (use12Hours) {
                    // enable 12 hours format
                    rtc.set12Hour();
                    // adjust hour to 12 hours format
                    if (counterHour - 12 > 0) {
                        counterHour = counterHour - 12;
                    }
                    // set clock
                    rtc.setTime(counterSecond, counterMinute, counterHour, isPM, counterDay, counterDate, counterMonth, counterYear);
                } else {
                    // enable 24 hours format
                    rtc.set24Hour();
                    // adjust hour to 24 hours format
                    if (counterHour + 12 < 24) {
                        counterHour = counterHour + 12;
                    }
                    // set clock
                    rtc.setTime(counterSecond, counterMinute, counterHour, counterDay, counterDate, counterMonth, counterYear);
                }
            }

            if (digitalRead(DECREMENT_PIN) == LOW) {
                // go back
                settingStage = 0;
            }

            // first line
            lcd.setCursor(0, 0);
            lcd.print("Confirm?");

            // second line
            lcd.setCursor(0, 1);
            lcd.print("Y              N");

        } else {
            settingStage = 0;
        }
        // sleep
        delay(125);
    }
}
