#include <Arduino.h>
#include <FastLED.h>

#define DATA 6             // DATA PIN
#define LED_COUNT 100      // LED COUNT ON THE STRIP [0..9]
#define CHIPSET WS2811     // LED DRIVER CHIPSET
#define COLOR_SEQUENCE BRG // SEQUENCE OF COLORS IN DATA STREAM
#define FPS 100            // UPDATES PER SECOND

// DATA DEFINITION
int BRIGHTNESS = 255; // BRIGHTNESS RANGE [0..255]
CRGB LEDs[LED_COUNT];
const int color_data_size = 3;
int color_data[color_data_size];
int tmp[color_data_size + 1];
int current_color[color_data_size];
int animation_mode = 0;
bool run_animation = false;
int animation_step = 0;
unsigned long animation_1_last_time = 0;
unsigned long alive_timer = 0;

void setup()
{
  Serial.begin(9600);
  // LED SETUP
  FastLED.addLeds<CHIPSET, DATA, COLOR_SEQUENCE>(LEDs, LED_COUNT);
  randomSeed(analogRead(0));
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
}

void loop()
{
  // Send alive signal
  tick_alive_timer();
  // Process incoming serial color_data if available
  reader();
  // Do selected animation with selected color
  do_animation();
}

void do_animation()
{
  if (run_animation)
  {
    switch (animation_mode)
    {
    case 0:
      ANIMATION_0();
      break;
    case 1:
      ANIMATION_1();
      break;
    }
  }
}

void reset_states()
{
  run_animation = false;
  animation_1_last_time = 0;
  animation_step = 0;
  current_color[0] = 0;
  current_color[1] = 0;
  current_color[2] = 0;
}

void reader()
{
  if (Serial.available() != 0)
  {
    switch (READ_DATA())
    {
    case 0:
      for (int i = 0; i < color_data_size; i++)
      {
        color_data[i] = tmp[i];
      }
      LOG_DATA();
      run_animation = true;
      break;
    case 1:
      BRIGHTNESS = tmp[0];
      FastLED.setBrightness(BRIGHTNESS);
      FastLED.show();
      run_animation = true;
      break;
    case 2:
      reset_states();
      run_animation = true;
      animation_mode = tmp[0];
      break;
    case 3:
      reset_states();
      SET_ALL_TO(0, 0, 0);
      FastLED.show();
      break;
    }
    tmp[0] = 0;
    tmp[1] = 0;
    tmp[2] = 0;
  }
}

void tick_alive_timer()
{
  if (millis() - alive_timer >= 500)
  {
    Serial.println("alive");
    alive_timer = millis();
  }
}

int READ_DATA()
{
  int multiplyer = 100;
  int index = 0;
  int type = 0;
  char buffer[1] = {0};
  Serial.readBytes(buffer, 1);
  switch (buffer[0])
  {
  case 'C':
    type = 0;
    break;
  case 'B':
    type = 1;
    break;
  case 'A':
    type = 2;
    break;
  case 'R':
    type = 3;
    break;
  default:
    return -1;
    break;
  }
  Serial.readBytes(buffer, 1);
  while (Serial.available() != 0)
  {
    Serial.readBytes(buffer, 1);
    switch (type)
    {
    case 0:
      if (buffer[0] == ';')
      {
        multiplyer = 100;
        index++;
      }
      else if (buffer[0] >= '0' && buffer[0] <= '9')
      {
        tmp[index] += multiplyer * (buffer[0] - '0');
        multiplyer /= 10;
      }
      break;
    case 1:
      if (buffer[0] >= '0' && buffer[0] <= '9')
      {
        tmp[0] += multiplyer * (buffer[0] - '0');
        multiplyer /= 10;
      }
      break;
    case 2:
      if (buffer[0] >= '0' && buffer[0] <= '9')
      {
        tmp[0] = (buffer[0] - '0');
      }
      break;
    }
  }
  return type;
}

void SET_ALL_TO(char R, char G, char B)
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    LEDs[i] = CRGB(R, G, B);
  }
}

void ANIMATION_0()
{
  bool different = false;
  for (int i = 0; i < color_data_size; i++)
  {
    if (color_data[i] != current_color[i])
    {
      different = true;
      current_color[i] = color_data[i];
    }
  }
  if (different)
  {
    SET_ALL_TO(color_data[0], color_data[1], color_data[2]);
    FastLED.show();
  }
}

void ANIMATION_1()
{
  if (millis() - animation_1_last_time >= 500)
  {
    char R = color_data[0];
    char G = color_data[1];
    char B = color_data[2];
    for (int i = 0; i < LED_COUNT; i++)
    {
      if (i % 3 == (animation_step % 2))
      {
        LEDs[i] = CRGB(R, G, B);
      }
      else
      {
        LEDs[i] = CRGB(255 - R, 255 - G, 255 - B);
      }
    }
    FastLED.show();
    if (animation_step > 100)
    {
      animation_step = 0;
    }
    else
    {
      animation_step++;
    }
    animation_1_last_time = millis();
  }
}

void INT_TO_HEX(char *tmp, int incoming)
{
  tmp[0] = incoming / 16 + '0';
  tmp[1] = incoming % 16 + '0';
  for (int i = 0; i < 2; i++)
  {
    if (tmp[i] > '9')
    {
      tmp[i] += 39;
    }
  }
}

void LOG_DATA()
{
  // R;XX;G;XX;B;XX;
  char Log[16];
  char chars[4] = {'R', 'G', 'B'};
  for (int i = 0; i < color_data_size; i++)
  {
    char tmp[2] = {0, 0};
    INT_TO_HEX(tmp, color_data[i]);
    Log[0 + 5 * i] = chars[i];
    Log[1 + 5 * i] = ';';
    Log[2 + 5 * i] = tmp[0];
    Log[3 + 5 * i] = tmp[1];
    Log[4 + 5 * i] = ';';
  }
  Log[15] = 0;
  Serial.println(Log);
}
