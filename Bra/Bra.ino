/*
Беспроводное бра с возможностью зарядки. Основа - печатная плата из 16 светодиодов WS2812B.
Система работает от аккумулятора 18650. Для заряда используется TP4056. 
*/

#include <FastLED.h>
#include "Button.h"

//#define _DEBUG_
#define LAMP_BR   255

unsigned long Tick = 0;

/*СОСТОЯНИЕ АКБ*/
#define PIN_OK    5
#define PIN_FAIL  6
#define VOLT_PIN  A1

/*Данные диодов*/
#define LED_PIN 3
#define LED_NUM 20
CRGB leds[LED_NUM];

/*Режимы работы*/
#define MODE_DEFAULT    0
#define MODE_SW         1
#define MODE_1H         2
#define MODE_OFF        3
byte  mode  = 0;
unsigned long whenChanged = 0;

#define FAIL_COUNT_CONST  1000
int failCount = 0;
bool chargeFlag = false;

#define HOUR_MS 3600000

#define PIN_MODE  4
Button bMode(PIN_MODE);

void setup() {
  #ifdef _DEBUG_
  Serial.begin(9600);
  #endif

  /*анализ АКБ*/
  pinMode(PIN_FAIL, OUTPUT);
  pinMode(PIN_OK, OUTPUT);

  pinMode(VOLT_PIN, INPUT);
  
  for (int i = 0; i < LED_NUM; i++)
  {
    leds[i].r = 0;
    leds[i].b = 0;
    leds[i].g = 0;
  }

  

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM).setCorrection( TypicalLEDStrip );
}

//цвета для смены режима
CRGB  getFunCol(byte idx){
  CRGB result;
  switch (idx){
    case 0:
      result.r = 255;
      result.g = 0;
      result.b = 128;
      break;
    case 1:
      result.r = 0;
      result.g = 0;
      result.b = 0;
      break;
    case 2:
      result.r = 128;
      result.g = 255;
      result.b = 0;
      break;
    case 3:
      result.r = 0;
      result.g = 0;
      result.b = 0;
      break;
    default:
      result.r = 0;
      result.g = 128;
      result.b = 255;
  }
  return result;
}

void loop() {
  Tick = millis();
  bMode.update(Tick);

  /*обработка кнопки*/
  if (bMode.isReleased() && mode != MODE_SW && !chargeFlag){
    mode++;
    if (mode > 3) mode = 0;
    whenChanged = Tick;
  }
  
  /*анализ АКБ*/
  int r_v = analogRead(VOLT_PIN);

  byte PWM_BLINK = ((millis() / 250)%2 == 0)?10:0;
  /*3.3V - полный швах. Остальное - норм*/
  if (r_v <= 675) {
    if (failCount < FAIL_COUNT_CONST){failCount++;}
    analogWrite(PIN_FAIL, chargeFlag?PWM_BLINK:10);
    digitalWrite(PIN_OK, 0);
  } else {
    failCount = 0;
    digitalWrite(PIN_FAIL, 0);
    analogWrite(PIN_OK, chargeFlag?PWM_BLINK:10);
  }

  if (failCount >= FAIL_COUNT_CONST){
    mode = MODE_OFF;   
    chargeFlag = true; 
  }

  /*обработка режимов*/
  switch (mode) {
    case MODE_OFF: //выключаем все диоды
      for (int i = 0; i < LED_NUM; i++){
        leds[i].r = 0;
        leds[i].g = 0;
        leds[i].b = 0;
      }
    break;    
    case MODE_DEFAULT://просто светим всеми диодами
      for (int i = 0; i < LED_NUM; i++){
        leds[i].r = LAMP_BR;
        leds[i].g = LAMP_BR;
        leds[i].b = LAMP_BR;
      }
    break;
    case MODE_SW:
      if (Tick - whenChanged >= 1000) {
        mode = MODE_1H;
        whenChanged = Tick;
      } else {
        unsigned long delta = Tick - whenChanged;
        byte shift = 4;
        if (delta < 800) shift = 3;
        if (delta < 600) shift = 2;
        if (delta < 400) shift = 1;
        if (delta < 200) shift = 0;
        for (int i = 0; i < LED_NUM; i++){
          leds[i] = getFunCol((shift)%5);
        }
      }
    break;
    case MODE_1H:
      /*1H = 60M = 3600S = 3600000ms*/
      unsigned long D = Tick - whenChanged;
      // D = 0        => br = 255
      // D = HOUR_MS  => br = 0
      byte br = map(D, 0, HOUR_MS, 255, 0);
      for (int i = 0; i < LED_NUM; i++){
        leds[i].r = br;
        leds[i].g = br;
        leds[i].b = br;
      }
      if (D >= HOUR_MS) mode = MODE_OFF;
    break;
  }
  FastLED.show();
  
  #ifdef _DEBUG_
  Serial.print(mode);
  Serial.print(" ");
  Serial.print((float)r_v*5.0f/1023.0f);
  Serial.print(" ");
  Serial.print(r_v);
  Serial.print(" ");  
  Serial.print(chargeFlag);
  Serial.print(" ");  
  Serial.println(failCount);  
  #endif
}
