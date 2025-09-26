#include <FastLED.h>
#include <avr/pgmspace.h>
#include <math.h>

#define LED_PIN     6
#define ROWS        10
#define COLS        22
#define NUM_LEDS    (ROWS*COLS)
#define BRIGHTNESS  32
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// ---------------- Fireworks Particles ----------------
struct Rocket {
  int16_t x, y;
  int8_t vx, vy;
  uint8_t hue;
  bool active;
};

struct Spark {
  int16_t x, y;
  int8_t vx, vy;
  uint8_t hue;
  uint8_t life;
  bool active;
};

#define MAX_ROCKETS 5
#define MAX_SPARKS 40
Rocket rockets[MAX_ROCKETS];
Spark sparks[MAX_SPARKS];

// ---------------- Precomputed Lookup Tables ----------------
#define NUM_ANGLES 32
const int8_t PROGMEM cosTable[NUM_ANGLES] = {
   10,  9,  9,  8,  7,  5,  3,  0,
   -3, -5, -7, -8, -9, -9, -10, -10,
   -10, -9, -9, -8, -7, -5, -3,  0,
    3,  5,  7,  8,  9,  9,  10,  10
};
const int8_t PROGMEM sinTable[NUM_ANGLES] = {
    0,  3,  5,  7,  8,  9,  9, 10,
   10,  9,  9,  8,  7,  5,  3,  0,
    0, -3, -5, -7, -8, -9, -9,-10,
  -10, -9, -9, -8, -7, -5, -3,  0
};

// ---------------- XY Mapping ----------------
static uint16_t XY(uint8_t x, uint8_t y) {
  return (y * COLS) + ((y & 1) ? (COLS - 1 - x) : x);
}

// ---------------- Corner Rainbow State ----------------
uint8_t currentCorner = 0;
uint8_t fadeLevel = 255;
bool fadingOut = false;
uint32_t lastSwitch = 0;

// ---------------- Effect Management ----------------
typedef bool (*EffectFunc)();
bool rainbowCycle();
bool colorChase();
bool cometEffect();
bool glitterEffect();
bool juggleEffect();
bool fireEffect();
bool fireworksEffect();
bool diagonalRainbowEffect();

EffectFunc effects[] = {
  rainbowCycle,
  colorChase,
  cometEffect,
  glitterEffect,
  juggleEffect,
  fireEffect,
  fireworksEffect,
  diagonalRainbowEffect   // added diagonal/corner rainbow
};
const uint8_t numEffects = sizeof(effects)/sizeof(effects[0]);
uint8_t currentEffect = 0;
uint8_t effectRound = 0;
uint16_t tickCounter = 0;

// ---------------- Setup ----------------
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  randomSeed(analogRead(0));
}

void loop() {
  if (effects[currentEffect]()) nextEffect();
}

// ---------------- Effect Controller ----------------
void nextEffect() {
  currentEffect = (currentEffect + 1) % numEffects;
  effectRound = 0;
  tickCounter = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// ---------------- Effects ----------------
bool rainbowCycle() {
  EVERY_N_MILLISECONDS(20) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV((i*256/NUM_LEDS + tickCounter) & 255,255,255);
    }
    FastLED.show();
    tickCounter++;
    if (tickCounter>=256){tickCounter=0; effectRound++;}
  }
  return (effectRound>=1);
}

bool colorChase() {
  EVERY_N_MILLISECONDS(50) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    leds[tickCounter%NUM_LEDS] = CHSV((tickCounter*5)&255,255,255);
    FastLED.show();
    tickCounter++;
    if (tickCounter%NUM_LEDS==0) effectRound++;
  }
  return (effectRound>=1);
}

bool cometEffect() {
  EVERY_N_MILLISECONDS(30) {
    fadeToBlackBy(leds, NUM_LEDS, 40);
    leds[tickCounter%NUM_LEDS] = CHSV((millis()/10)&255,255,255);
    FastLED.show();
    tickCounter++;
    if (tickCounter%(NUM_LEDS*2)==0) effectRound++;
  }
  return (effectRound>=1);
}

bool glitterEffect() {
  EVERY_N_MILLISECONDS(40) {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    if (random8()<80) leds[random16(NUM_LEDS)]+=CRGB::White;
    FastLED.show();
    tickCounter++;
    if (tickCounter>200){tickCounter=0; effectRound++;}
  }
  return (effectRound>=1);
}

bool juggleEffect() {
  EVERY_N_MILLISECONDS(15) {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue=0;
    for (int i=0;i<8;i++){
      leds[beatsin16(i+7,0,NUM_LEDS-1)]|=CHSV(dothue,200,255);
      dothue+=32;
    }
    FastLED.show();
    tickCounter++;
    if(tickCounter>500){tickCounter=0; effectRound++;}
  }
  return (effectRound>=1);
}

bool fireEffect() {
  static byte heat[NUM_LEDS];
  EVERY_N_MILLISECONDS(30) {
    for (int i=0;i<NUM_LEDS;i++) heat[i]=qsub8(heat[i],random8(0,((55*10)/NUM_LEDS)+2));
    for (int k=NUM_LEDS-1;k>=2;k--) heat[k]=(heat[k-1]+heat[k-2]+heat[k-2])/3;
    if (random8()<120) heat[random8(7)]=qadd8(heat[random8(7)],random8(160,255));
    for (int j=0;j<NUM_LEDS;j++) leds[j]=HeatColor(heat[j]);
    FastLED.show();
    tickCounter++;
    if(tickCounter>200){tickCounter=0; effectRound++;}
  }
  return (effectRound>=1);
}

bool fireworksEffect() {
  EVERY_N_MILLISECONDS(30) {
    fadeToBlackBy(leds, NUM_LEDS, 40);
    // Launch rockets
    if (random8()<20) {
      for (int i=0;i<MAX_ROCKETS;i++){
        if (!rockets[i].active){
          rockets[i].active=true;
          rockets[i].x=random(COLS);
          rockets[i].y=0;
          rockets[i].vx=(random8()<128)?3:-3;
          rockets[i].vy=random(20,40)/10;
          rockets[i].hue=random8();
          break;
        }
      }
    }
    // Update rockets
    for (int i=0;i<MAX_ROCKETS;i++){
      if (rockets[i].active){
        rockets[i].x+=rockets[i].vx;
        rockets[i].y+=rockets[i].vy;
        rockets[i].vy-=1;
        if (rockets[i].y>=0 && rockets[i].y<ROWS && rockets[i].x>=0 && rockets[i].x<COLS)
          leds[XY((int)rockets[i].x,(int)rockets[i].y)]=CHSV(rockets[i].hue,255,255);
        if (rockets[i].vy<=0){
          rockets[i].active=false;
          for (int s=0;s<MAX_SPARKS;s++){
            if (!sparks[s].active){
              sparks[s].active=true;
              sparks[s].x=rockets[i].x;
              sparks[s].y=rockets[i].y;
              uint8_t angleIndex=random(NUM_ANGLES);
              int8_t cx=pgm_read_byte(&cosTable[angleIndex]);
              int8_t sy=pgm_read_byte(&sinTable[angleIndex]);
              uint8_t speed=random(5,15);
              sparks[s].vx=(cx*speed)/10;
              sparks[s].vy=(sy*speed)/10;
              sparks[s].hue=rockets[i].hue+random8(40);
              sparks[s].life=200;
            }
          }
        }
      }
    }
    // Update sparks
    for (int s=0;s<MAX_SPARKS;s++){
      if (sparks[s].active){
        sparks[s].x+=sparks[s].vx;
        sparks[s].y+=sparks[s].vy;
        sparks[s].vy-=0.3;
        sparks[s].life=qsub8(sparks[s].life,5);
        if(sparks[s].life>0 && sparks[s].x>=0 && sparks[s].x<COLS && sparks[s].y>=0 && sparks[s].y<ROWS)
          leds[XY((int)sparks[s].x,(int)sparks[s].y)]+=CHSV(sparks[s].hue,200,sparks[s].life);
        else sparks[s].active=false;
      }
    }
    FastLED.show();
  }
  return false;
}

// ---------------- Diagonal / Corner Rainbow ----------------
bool diagonalRainbowEffect() {
  EVERY_N_MILLISECONDS(30) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    uint32_t t=millis()/10;
    uint8_t pulse=beatsin8(6,180,255);
    for(uint8_t y=0;y<ROWS;y++){
      for(uint8_t x=0;x<COLS;x++){
        int16_t diag=x+y;
        uint8_t hue=(diag*12+t)&0xFF;
        uint8_t bright=scale8(pulse,fadeLevel);
        leds[XY(x,y)]=ColorFromPalette(RainbowColors_p,hue,bright,LINEARBLEND);
      }
    }
    if(!fadingOut && millis()-lastSwitch>random(3000,5000)) fadingOut=true;
    if(fadingOut){
      if(fadeLevel>10) fadeLevel-=10;
      else{fadeLevel=0; currentCorner=random(0,4); fadingOut=false;}
    } else {
      if(fadeLevel<250) fadeLevel+=10;
      else{fadeLevel=255; lastSwitch=millis();}
    }
    FastLED.show();
  }
  return false;
}
