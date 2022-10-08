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

uint32 period, duty;

unsigned long brightnessChangeMs = 0;

static int pos;
unsigned long lastTick;

uint32 io_info[PWM_CHANNELS][3] = {
  {PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 , 5}
};

uint32 pwm_duty_init[PWM_CHANNELS] = {0};

RotaryEncoder *encoder = nullptr;

volatile bool btn_pressed = false;
bool is_on = false;

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

  pos = Config::getBrightness();
  encoder->setPosition(pos);

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

    //empiric formula that gives high period values for low duty cycle (in order to achieve low brightess with higher accuracy) 
    //and lower period values for higher duty cycle (to avoid flickering)
    period = MIN_PERIOD + (uint32)roundf((MAX_PERIOD-MIN_PERIOD)*powf(2,-20.0*dc));
    duty = (uint32)roundf(dc*period);

    pwm_set_period(period);
    pwm_set_duty(duty,0);
    pwm_start();

}

void loop()
{
  //encoder->tick();

  int newPos = encoder->getPosition();
  //int newPos = millis()/1000;
  if (newPos > 1000) newPos = 1000;

  if ((brightnessChangeMs != 0) && millis() > brightnessChangeMs + 5000) {
    Config::saveBrightness(pos);
    brightnessChangeMs = 0;
  }

  if (pos != newPos) {

    if (!is_on) {
      pos = 0;
      is_on = true;
    }
    unsigned long tickDiff = millis() - lastTick;
    lastTick = millis();

    //empiric formula that gives the brighthess increase/decrease dependent on knob rotation (faster rotation -> more change)
    int increase =  1 + (int)roundf(99.0*powf(2,-0.03*tickDiff));

    if (newPos > pos) {
      newPos = pos + increase;
    } else {
      newPos = pos - increase;
    }

    if (newPos < 1 ) {
      newPos = 1;
    }
    if (newPos > 1000) {
      newPos = 1000;
    }
    encoder->setPosition(newPos);
    pos = newPos;

    setBrigthess(pos);
    Serial.printf("pos:%d td:%ld", pos, tickDiff);Serial.println();

    brightnessChangeMs = millis();

  } // if encoder was ticked

  if (btn_pressed) {
    Serial.print("btn");
    if (is_on) {
      setBrigthess(0);
      is_on = false;
      Serial.println(":off");
    } else {
      setBrigthess(pos);
      is_on = true;
      Serial.println(":on");
    }
    btn_pressed = false;
  }

  Ntp::loop();

  Web::loop();

} // loop ()