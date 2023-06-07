#include <Arduino.h>
#include <FastLED.h>

#define THUB 8             // DATA PIN FOR THUB
#define CABINET 7          // DATA PIN FOR CABINET
#define LED_COUNT 100      // LED COUNT ON THE STRIP/3
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
unsigned long animation_last_time = 0;
unsigned long animation_step_delay = 500;
int animation_direction = -1;
unsigned long alive_timer = 0;

#pragma region basic
void setup()
{
  Serial.begin(9600);
  // LED SETUP
  FastLED.addLeds<CHIPSET, THUB, COLOR_SEQUENCE>(LEDs, LED_COUNT);
  FastLED.addLeds<CHIPSET, CABINET, COLOR_SEQUENCE>(LEDs, LED_COUNT);
  randomSeed(analogRead(0));
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  // Process incoming serial color_data if available
  reader();
  // Do selected animation with selected color
  do_animation();
  // Send alive signal
  tick_alive_timer();
}
#pragma endregion

#pragma region steps
void do_animation()
{
  if (run_animation)
  {
    switch (animation_mode)
    {
    case 0:
      ANIMATION_0(); // Homogen
      break;
    case 1:
      ANIMATION_1(); // FlashWithInverted
      break;
    case 2:
      ANIMATION_2(); // Breathing
      break;
    }
  }
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
      RESET_STATES();
      run_animation = true;
      animation_mode = tmp[0];
      break;
    case 3:
      RESET_STATES();
      SET_ALL_TO(0, 0, 0);
      FastLED.show();
      break;
    case 4:
      FastLED.show();
      break;
    }
    tmp[0] = 0;
    tmp[1] = 0;
    tmp[2] = 0;
  }
}

void tick_buit_in_led()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
}

void tick_alive_timer()
{
  if (millis() - alive_timer >= 500)
  {
    Serial.println("alive");
    alive_timer = millis();
  }
}
#pragma endregion

void RESET_STATES()
{
  run_animation = false;
  animation_last_time = 0;
  animation_step = 0;
  current_color[0] = 0;
  current_color[1] = 0;
  current_color[2] = 0;
  SET_ALL_TO(color_data[0], color_data[1], color_data[2]);
  FastLED.setBrightness(BRIGHTNESS);
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
  case 'C': // Color
    type = 0;
    break;
  case 'B': // Brightness
    type = 1;
    break;
  case 'A': // Animation
    type = 2;
    break;
  case 'R': // Reset
    type = 3;
    break;
  case 'S': // Show
    type = 4;
    break;
  default:
    return -1;
    break;
  }
  Serial.readBytes(buffer, 1);
  for (int i = 5; i > type; i--)
  {
    tick_buit_in_led();
  }
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
  // C;##;##;##;
  char Log[12];
  Log[0] = 'C';
  Log[1] = ';';
  for (int i = 0; i < color_data_size; i++)
  {
    char tmp[2] = {0, 0};
    INT_TO_HEX(tmp, color_data[i]);
    Log[-1 + 3 * (i + 1)] = tmp[0];
    Log[0 + 3 * (i + 1)] = tmp[1];
    Log[1 + 3 * (i + 1)] = ';';
  }
  Log[11] = 0;
  Serial.println(Log);
  alive_timer = millis();
}

void SET_ALL_TO(char R, char G, char B)
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    LEDs[i] = CRGB(R, G, B);
  }
}

#pragma region animations
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
  if (millis() - animation_last_time >= animation_step_delay)
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
    animation_last_time = millis();
  }
}

void ANIMATION_2()
{
  int current_brightness = FastLED.getBrightness();

  if (current_brightness > 80)
  {
    FastLED.setBrightness(80);
    current_brightness = 80;
  }

  if (millis() - animation_last_time >= animation_step_delay / 5)
  {
    if (current_brightness + animation_direction <= 0 || current_brightness + animation_direction > 80)
    {
      animation_direction *= -1;
    }

    FastLED.setBrightness(current_brightness + animation_direction);
    FastLED.show();

    animation_last_time = millis();
  }
}

#pragma endregion
