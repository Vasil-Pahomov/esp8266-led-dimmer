#include <Arduino.h>
#include <RotaryEncoder.h>
#include <math.h>
#include <FS.h>

#include "web.h"
#include "ntp.h"
#include "config.h"

extern "C" {
  #include <pwm.h>
}

#define PIN_IN1 14
#define PIN_IN2 12
#define PIN_PWM D1
#define PIN_BTN 0

#define PWM_CHANNELS 1

//PWM unit is 200 ns
#define MIN_PERIOD 500 // minimum PWM period, units. 500 units correspond to 10KHz
#define MAX_PERIOD 50000 // maximum PWM period, units. 50k units correspond to 200Hz

unsigned long lastManualBrightnessChangeMs = 0; // last manual brightness change time (used to track timeout to save brighthess value)

int curBrightness; // current brightness value

long autoBrightnessChangeSpeed; // speed of automatic brightness change, in units per millisecond
unsigned long autoBrightnessChangeStartMs; // time of automatic brigthness change start 
unsigned long autoBrightnessChangeEndMs; // end time of automatic brightness change
int autoBrightnessChangeStartValue; // starting value of brigthness for automatic change

int prevPos; // previous encoder position

uint32 io_info[PWM_CHANNELS][3] = {
  {PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 , 5}
};

uint32 pwm_duty_init[PWM_CHANNELS] = {0};

RotaryEncoder *encoder = nullptr;

volatile bool btn_pressed = false;

IRAM_ATTR void checkPosition()
{
  encoder->tick(); // just call tick() to check the state.
}

IRAM_ATTR void pressButton()
{
  btn_pressed = true;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);

  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN), pressButton, FALLING);

  pinMode(PIN_PWM, OUTPUT);
  digitalWrite(PIN_PWM,LOW);
  delay(1000);

  pwm_init(200, pwm_duty_init, PWM_CHANNELS, io_info);
  pwm_start();

  Web::setup(!digitalRead(PIN_BTN));
  Ntp::setup();
  Config::setup();

  curBrightness = 0;
  encoder->setPosition(curBrightness);
  prevPos = curBrightness;

} // setup()

//value is from 9 (fully off) to 1000 (100% duty)
void setBrigthess(unsigned int value) {

    //idea taken here: https://electronics.stackexchange.com/a/550434
    float dc = (powf(100.0, value / 1000.0) - 1.0) / 99.0;
/*
    //https://www.johndcook.com/blog/2010/10/20/best-rational-approximation/

    uint32 a = 0, b = 1, c = 1, d = 1;

    while (b <= MAX_PERIOD && d <= MAX_PERIOD)
    {
        float mediant = (float)(a+c)/(b+d);
        //Serial.printf(" med:%f, a=%d b=%d c=%d d=%d\r\n",mediant, a,b,c,d);

        if ( abs(dc - mediant) < 1E-6 ) {
          if (b + d <= MAX_PERIOD) {
            duty = a + c;
            period = b + d;
          } else if (d > b) {
            duty = c;
            period = d;
          } else {
            duty = a;
            period = b; 
          }
          break;
        }
        else if (dc > mediant) {
            a = a + c;
            b = b + d;
        } else {
          c = a + c;
          d = b + d;
        }
    }

    if (b > MAX_PERIOD) {
      duty = c;
      period = d;
    } else if (d > MAX_PERIOD) {
      duty = a;
      period = b;
    }
*/

/*
    if (dc > DUTY_CYCLE_THRESHOLD) {
      duty = (uint32)roundf(dc*MIN_PERIOD);
      period = MIN_PERIOD;
    } else {
      period = (uint32)roundf(1.0/dc);
      duty = 1;
      if (period > MAX_PERIOD) { 
        period = MAX_PERIOD;
        duty = 0;
      }
    }
*/

    uint32 period, duty;

    //empiric formula that gives high period values for low duty cycle (in order to achieve low brightess with higher accuracy) 
    //and lower period values for higher duty cycle (to avoid flickering)
    period = MIN_PERIOD + (uint32)roundf((MAX_PERIOD-MIN_PERIOD)*powf(2,-20.0*dc));
    duty = (uint32)roundf(dc*period);

    pwm_set_period(period);
    pwm_set_duty(duty,0);
    pwm_start();

}


void startBrightAutoChange(int target, int durationMsec) {
  autoBrightnessChangeStartValue = curBrightness;
  autoBrightnessChangeSpeed = 1000000L * (target - autoBrightnessChangeStartValue) / durationMsec;
  autoBrightnessChangeStartMs = millis();
  autoBrightnessChangeEndMs = autoBrightnessChangeStartMs + durationMsec;

  Serial.print(curBrightness);Serial.print('>');Serial.print(target);Serial.print(' ');
  Serial.print(autoBrightnessChangeStartMs);Serial.print('>');Serial.print(autoBrightnessChangeEndMs);
  Serial.print('@');Serial.print(autoBrightnessChangeSpeed);
  Serial.println();

}

void loop()
{
  if (autoBrightnessChangeSpeed != 0) {
    curBrightness = autoBrightnessChangeStartValue + autoBrightnessChangeSpeed * (long) (millis()-autoBrightnessChangeStartMs) / 1000000L;
    if (curBrightness < 0) curBrightness = 0;
    if (curBrightness > 10000) curBrightness = 1000;
    //Serial.print(millis()-autoBrightnessChangeStartMs);Serial.print('-');Serial.print(curBrightness);Serial.println();
    setBrigthess(curBrightness);

     if (millis() > autoBrightnessChangeEndMs)
     {
      autoBrightnessChangeSpeed = 0;
     }
  }

  int newPos = encoder->getPosition();
  if (newPos > 1000) newPos = 1000;

  if (prevPos!= newPos) {

    unsigned long tickDiff = millis() - lastManualBrightnessChangeMs;

    //empiric formula that gives the brighthess increase/decrease dependent on knob rotation (faster rotation -> more change)
    int increase =  1 + (int)roundf(99.0*powf(2,-0.03*tickDiff));

    if (newPos > curBrightness) {
      newPos = curBrightness + increase;
    } else {
      newPos = curBrightness - increase;
    }

    if (newPos < 0 ) {
      newPos = 0;
    }
    if (newPos > 1000) {
      newPos = 1000;
    }
    encoder->setPosition(newPos);
    curBrightness = newPos;

    prevPos = newPos;

    setBrigthess(curBrightness);
    Serial.printf("pos:%d td:%ld", curBrightness, tickDiff);Serial.println();

    lastManualBrightnessChangeMs = millis();

    autoBrightnessChangeSpeed = 0; //stop automatic brigthness change on manual knob rotation

  } // if encoder was ticked

  if ((lastManualBrightnessChangeMs != 0) && millis() > lastManualBrightnessChangeMs + 1000) {
    Config::saveBrightness(curBrightness);
    lastManualBrightnessChangeMs = 0;
  }

  if (btn_pressed) {
    Serial.print("btn");
    if (curBrightness != 0) {
      Serial.println(":off");
      startBrightAutoChange(0, Config::getOffDurationMs());
    } else {
      Serial.println(":on");
      unsigned int bri = Config::getBrightness();
      if (bri == 0) bri = Config::getDefaultBrightness();
      startBrightAutoChange(bri, Config::getOnDurationMs());
    }
    btn_pressed = false;
  }

  Ntp::loop();

  Web::loop();

} // loop ()