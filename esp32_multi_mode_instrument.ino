/*
  ESP32 Multi-Mode Musical Instrument
  With OLED display, volume buttons, instrument-change button, and on/off button

  ---------------------------------------------------------------------------
  WORKING OF THE PROJECT
  ---------------------------------------------------------------------------

  This code makes an ESP32 work as a small digital musical instrument.

  1. Audio generation
     - The ESP32 does not play stored audio files.
     - It creates sound in real time using simple synthesis:
       sine waves, harmonics, noise, envelopes, filters, and soft clipping.
     - The generated audio samples are sent through I2S to a MAX98357A
       amplifier module.
     - The MAX98357A drives the speaker.

  2. Instrument modes
     - There are 5 instrument modes:
       Flute, Piano, Guitar, Bell, and Drums.
     - The instrument-change button cycles through these modes.
     - Every press changes to the next instrument:
       Flute -> Piano -> Guitar -> Bell -> Drums -> Flute.
     - The OLED display shows the currently selected instrument.

  3. Key/pad sensors
     - There are 14 digital sensor inputs.
     - All sensors are active LOW.
     - That means a sensor is considered pressed when its GPIO reads LOW.
     - The code uses INPUT_PULLUP, so each sensor should connect its GPIO
       pin to GND when activated.
     - In Flute, Piano, Guitar, and Bell modes, the 14 sensors play notes.
     - In Drum mode, the same 14 sensors trigger different drum sounds.

  4. Note mapping for melodic instruments
     Sensor  1, GPIO4  -> C4,  261.63 Hz
     Sensor  2, GPIO5  -> D4,  293.66 Hz
     Sensor  3, GPIO6  -> E4,  329.63 Hz
     Sensor  4, GPIO7  -> F4,  349.23 Hz
     Sensor  5, GPIO8  -> G4,  392.00 Hz
     Sensor  6, GPIO9  -> A4,  440.00 Hz
     Sensor  7, GPIO10 -> B4,  493.88 Hz
     Sensor  8, GPIO11 -> C5,  523.25 Hz
     Sensor  9, GPIO12 -> D5,  587.33 Hz
     Sensor 10, GPIO13 -> E5,  659.25 Hz
     Sensor 11, GPIO14 -> F5,  698.46 Hz
     Sensor 12, GPIO15 -> G5,  783.99 Hz
     Sensor 13, GPIO21 -> A5,  880.00 Hz
     Sensor 14, GPIO38 -> C6, 1046.50 Hz

  5. Drum mapping in Drums mode
     Sensor  1, GPIO4  -> Kick
     Sensor  2, GPIO5  -> Snare
     Sensor  3, GPIO6  -> Closed Hat
     Sensor  4, GPIO7  -> Clap
     Sensor  5, GPIO8  -> Mid Tom
     Sensor  6, GPIO9  -> Low Tom
     Sensor  7, GPIO10 -> High Tom
     Sensor  8, GPIO11 -> Rimshot
     Sensor  9, GPIO12 -> Crash
     Sensor 10, GPIO13 -> Ride
     Sensor 11, GPIO14 -> Cowbell
     Sensor 12, GPIO15 -> Open Hat
     Sensor 13, GPIO21 -> Tambourine
     Sensor 14, GPIO38 -> Shaker

  6. Volume control
     - The volume-up and volume-down buttons change volume from 0 to 10.
     - The volume is controlled in software by scaling the generated audio
       samples before they are sent to the amplifier.
     - Volume 0 sends silence.
     - The OLED display shows the current volume level and a volume bar.

  7. On/off control
     - The on/off button toggles the instrument between ON and OFF.
     - When OFF:
       - all active notes and drum sounds are stopped,
       - silence is sent to the I2S amplifier,
       - the OLED display is turned off,
       - sensor, mode, and volume controls are ignored.
     - Pressing the on/off button again turns the OLED back on and the
       instrument is ready to play.

  8. OLED display
     - The OLED shows:
       - project title,
       - selected instrument,
       - volume level,
       - last played key or drum pad,
       - volume bar.

  ---------------------------------------------------------------------------
  REQUIRED CONNECTIONS
  ---------------------------------------------------------------------------

  1. MAX98357A I2S amplifier to ESP32
     MAX98357A DIN      -> ESP32 GPIO16
     MAX98357A BCLK     -> ESP32 GPIO18
     MAX98357A LRC / WS -> ESP32 GPIO17
     MAX98357A VIN      -> 5V or 3.3V
     MAX98357A GND      -> ESP32 GND
     MAX98357A SD       -> 3.3V, or leave according to your module design
     Speaker +          -> Speaker positive terminal
     Speaker -          -> Speaker negative terminal

  2. OLED display, 128x64 SSD1306 I2C
     OLED VCC -> ESP32 3.3V
     OLED GND -> ESP32 GND
     OLED SDA -> ESP32 GPIO25
     OLED SCL -> ESP32 GPIO26
     I2C address used in code: 0x3C

  3. Control buttons, active LOW
     Each button connects between the ESP32 GPIO pin and GND.
     No external pull-up resistor is required because INPUT_PULLUP is used.

     Instrument-change button -> GPIO23 to GND
     On/off button            -> GPIO27 to GND
     Volume-up button         -> GPIO32 to GND
     Volume-down button       -> GPIO33 to GND

  4. Sensor/key inputs, active LOW
     Each sensor should pull its GPIO pin to GND when touched/pressed/active.
     The code enables internal pull-ups using INPUT_PULLUP.

     Sensor  1 -> GPIO4  to GND when active
     Sensor  2 -> GPIO5  to GND when active
     Sensor  3 -> GPIO6  to GND when active
     Sensor  4 -> GPIO7  to GND when active
     Sensor  5 -> GPIO8  to GND when active
     Sensor  6 -> GPIO9  to GND when active
     Sensor  7 -> GPIO10 to GND when active
     Sensor  8 -> GPIO11 to GND when active
     Sensor  9 -> GPIO12 to GND when active
     Sensor 10 -> GPIO13 to GND when active
     Sensor 11 -> GPIO14 to GND when active
     Sensor 12 -> GPIO15 to GND when active
     Sensor 13 -> GPIO21 to GND when active
     Sensor 14 -> GPIO38 to GND when active

  5. Common ground
     All GND pins must be connected together:
     ESP32 GND, MAX98357A GND, OLED GND, sensor GND, and button GND.

  ---------------------------------------------------------------------------
  IMPORTANT NOTES
  ---------------------------------------------------------------------------

  - Install these Arduino libraries:
    Adafruit GFX Library
    Adafruit SSD1306

  - GPIO6 to GPIO11 are connected to flash memory on many ESP32 boards.
    On those boards, using GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, and GPIO11
    as external sensors may cause boot or upload problems. If your board has
    this issue, move those sensors to safer free GPIO pins and update the
    sensorPins array below.

  - GPIO38 is input-only and may not have an internal pull-up on some ESP32
    variants. If the GPIO38 sensor does not work reliably, add an external
    pull-up resistor to 3.3V or move that sensor to another GPIO.
*/

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <string.h>

#define I2S_DOUT 16
#define I2S_BCLK 18
#define I2S_LRC 17

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 256
#define KEY_COUNT 14
#define MODE_BUTTON_PIN 23
#define VOLUME_UP_PIN 32
#define VOLUME_DOWN_PIN 33
#define POWER_BUTTON_PIN 27
#define OLED_SDA_PIN 25
#define OLED_SCL_PIN 26
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C
#define MODE_COUNT 5
#define BUTTON_DEBOUNCE_MS 220

const int sensorPins[KEY_COUNT] = {
  4,5,6,7,8,9,10,
  11,12,13,14,15,
  21,38
};

const float notes[KEY_COUNT] = {
  261.63,293.66,329.63,349.23,392.00,440.00,493.88,
  523.25,587.33,659.25,698.46,783.99,880.00,1046.50
};

const char *modeNames[MODE_COUNT] = {
  "Flute",
  "Piano",
  "Guitar",
  "Bell",
  "Drums"
};

const char *drumNames[KEY_COUNT] = {
  "Kick","Snare","Closed Hat","Clap","Mid Tom","Low Tom","High Tom",
  "Rimshot","Crash","Ride","Cowbell","Open Hat","Tambourine","Shaker"
};

const float DRUM_TWO_PI = 6.2831853f;
const float MASTER_GAIN = 24500.0f;
const int MIN_VOLUME = 0;
const int MAX_VOLUME = 10;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int currentMode = 0;
int volumeLevel = 6;
bool instrumentOn = true;
bool modeWasPressed = false;
bool volumeUpWasPressed = false;
bool volumeDownWasPressed = false;
bool powerWasPressed = false;
unsigned long lastModeChangeMs = 0;
unsigned long lastVolumeChangeMs = 0;
unsigned long lastPowerChangeMs = 0;
bool displayAvailable = false;
bool displayDirty = true;
char lastPlayed[24] = "Ready";
bool wasPressed[KEY_COUNT] = {
  false,false,false,false,false,false,false,
  false,false,false,false,false,false,false
};

uint32_t rngState = 0x12345678;

struct NoteVoice {
  bool active;
  int mode;
  float freq;
  float phase;
  float amp;
  float age;
};

struct DrumVoice {
  bool active;
  uint32_t pos;
  uint32_t total;
  float phase1;
  float phase2;
  float filter1;
  float filter2;
};

NoteVoice noteVoices[KEY_COUNT];
DrumVoice kick = {false,0,0,0,0,0,0};
DrumVoice snare = {false,0,0,0,0,0,0};
DrumVoice closedHat = {false,0,0,0,0,0,0};
DrumVoice clap = {false,0,0,0,0,0,0};
DrumVoice midTom = {false,0,0,0,0,0,0};
DrumVoice lowTom = {false,0,0,0,0,0,0};
DrumVoice highTom = {false,0,0,0,0,0,0};
DrumVoice rimshot = {false,0,0,0,0,0,0};
DrumVoice crash = {false,0,0,0,0,0,0};
DrumVoice ride = {false,0,0,0,0,0,0};
DrumVoice cowbell = {false,0,0,0,0,0,0};
DrumVoice openHat = {false,0,0,0,0,0,0};
DrumVoice tambourine = {false,0,0,0,0,0,0};
DrumVoice shaker = {false,0,0,0,0,0,0};

float currentVolume()
{
  if(volumeLevel <= 0) return 0.0f;
  return (float)volumeLevel / (float)MAX_VOLUME;
}

void setLastPlayed(const char *text)
{
  strncpy(lastPlayed, text, sizeof(lastPlayed) - 1);
  lastPlayed[sizeof(lastPlayed) - 1] = '\0';
  displayDirty = true;
}

void muteAllSounds()
{
  for(int i = 0; i < KEY_COUNT; i++)
  {
    noteVoices[i].active = false;
  }

  kick.active = false;
  snare.active = false;
  closedHat.active = false;
  clap.active = false;
  midTom.active = false;
  lowTom.active = false;
  highTom.active = false;
  rimshot.active = false;
  crash.active = false;
  ride.active = false;
  cowbell.active = false;
  openHat.active = false;
  tambourine.active = false;
  shaker.active = false;
}

void updateDisplay()
{
  if(!displayAvailable || !displayDirty || !instrumentOn) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MULTI MODE");

  display.setCursor(72, 0);
  display.print("VOL ");
  display.print(volumeLevel);
  display.print("/");
  display.print(MAX_VOLUME);

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(modeNames[currentMode]);

  display.setTextSize(1);
  display.setCursor(0, 43);
  display.print("Last: ");
  display.print(lastPlayed);

  int barWidth = map(volumeLevel, MIN_VOLUME, MAX_VOLUME, 0, 118);
  display.drawRect(0, 55, 120, 8, SSD1306_WHITE);
  if(barWidth > 0)
  {
    display.fillRect(2, 57, barWidth, 4, SSD1306_WHITE);
  }

  display.display();
  displayDirty = false;
}

void turnDisplayOff()
{
  if(!displayAvailable) return;

  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  displayDirty = true;
}

void turnDisplayOn()
{
  if(!displayAvailable) return;

  display.ssd1306_command(SSD1306_DISPLAYON);
  displayDirty = true;
}

void setupDisplay()
{
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  if(display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    displayAvailable = true;
    display.clearDisplay();
    display.display();
    displayDirty = true;
  }
  else
  {
    displayAvailable = false;
    Serial.println("OLED not found");
  }
}

uint32_t msToSamples(uint16_t ms)
{
  return (uint32_t)((SAMPLE_RATE * (uint32_t)ms) / 1000);
}

float randomNoise()
{
  rngState = rngState * 1664525 + 1013904223;
  float value = (float)((rngState >> 9) & 0x7FFFFF) / 4194303.0f;
  return (value * 2.0f) - 1.0f;
}

float lowpass(float input, float &state, float alpha)
{
  state += alpha * (input - state);
  return state;
}

float highpass(float input, float &state, float alpha)
{
  float low = lowpass(input, state, alpha);
  return input - low;
}

float softclip(float x)
{
  if(x > 1.0f) x = 1.0f;
  if(x < -1.0f) x = -1.0f;
  return x - (x * x * x) / 3.0f;
}

float squareFromPhase(float phase)
{
  return sinf(phase) >= 0.0f ? 1.0f : -1.0f;
}

float envPow(float t, int power)
{
  float e = 1.0f - t;
  for(int i = 1; i < power; i++)
  {
    e *= (1.0f - t);
  }
  return e;
}

void triggerVoice(DrumVoice &voice, uint16_t durationMs)
{
  voice.active = true;
  voice.pos = 0;
  voice.total = msToSamples(durationMs);
  voice.phase1 = 0.0f;
  voice.phase2 = 0.0f;
  voice.filter1 = 0.0f;
  voice.filter2 = 0.0f;
}

void triggerDrum(int index)
{
  Serial.print(drumNames[index]);
  Serial.println(" hit");
  setLastPlayed(drumNames[index]);

  switch(index)
  {
    case 0: triggerVoice(kick, 240); break;
    case 1: triggerVoice(snare, 190); break;
    case 2: triggerVoice(closedHat, 95); break;
    case 3: triggerVoice(clap, 170); break;
    case 4: triggerVoice(midTom, 260); break;
    case 5: triggerVoice(lowTom, 320); break;
    case 6: triggerVoice(highTom, 220); break;
    case 7: triggerVoice(rimshot, 80); break;
    case 8: triggerVoice(crash, 900); break;
    case 9: triggerVoice(ride, 520); break;
    case 10: triggerVoice(cowbell, 190); break;
    case 11: triggerVoice(openHat, 340); break;
    case 12: triggerVoice(tambourine, 420); break;
    case 13: triggerVoice(shaker, 230); break;
  }
}

void triggerNote(int index)
{
  noteVoices[index].active = true;
  noteVoices[index].mode = currentMode;
  noteVoices[index].freq = notes[index];
  noteVoices[index].phase = 0.0f;
  noteVoices[index].age = 0.0f;

  if(currentMode == 0) noteVoices[index].amp = 1.25f;
  else if(currentMode == 1) noteVoices[index].amp = 1.45f;
  else if(currentMode == 2) noteVoices[index].amp = 1.35f;
  else noteVoices[index].amp = 1.10f;

  Serial.print(modeNames[currentMode]);
  Serial.print(" note ");
  Serial.println(index + 1);

  snprintf(lastPlayed, sizeof(lastPlayed), "Key %d", index + 1);
  displayDirty = true;
}

void handleModeButton()
{
  bool pressed = (digitalRead(MODE_BUTTON_PIN) == LOW);
  unsigned long now = millis();

  if(pressed && !modeWasPressed && now - lastModeChangeMs > BUTTON_DEBOUNCE_MS)
  {
    currentMode++;
    if(currentMode >= MODE_COUNT) currentMode = 0;

    muteAllSounds();
    setLastPlayed("Ready");
    lastModeChangeMs = now;
    Serial.print("MODE: ");
    Serial.println(modeNames[currentMode]);
  }

  modeWasPressed = pressed;
}

void handleVolumeButtons()
{
  bool upPressed = (digitalRead(VOLUME_UP_PIN) == LOW);
  bool downPressed = (digitalRead(VOLUME_DOWN_PIN) == LOW);
  unsigned long now = millis();

  if(now - lastVolumeChangeMs > BUTTON_DEBOUNCE_MS)
  {
    if(upPressed && !volumeUpWasPressed && volumeLevel < MAX_VOLUME)
    {
      volumeLevel++;
      lastVolumeChangeMs = now;
      displayDirty = true;
      Serial.print("VOLUME: ");
      Serial.println(volumeLevel);
    }
    else if(downPressed && !volumeDownWasPressed && volumeLevel > MIN_VOLUME)
    {
      volumeLevel--;
      lastVolumeChangeMs = now;
      displayDirty = true;
      Serial.print("VOLUME: ");
      Serial.println(volumeLevel);
    }
  }

  volumeUpWasPressed = upPressed;
  volumeDownWasPressed = downPressed;
}

void handlePowerButton()
{
  bool pressed = (digitalRead(POWER_BUTTON_PIN) == LOW);
  unsigned long now = millis();

  if(pressed && !powerWasPressed && now - lastPowerChangeMs > BUTTON_DEBOUNCE_MS)
  {
    instrumentOn = !instrumentOn;
    lastPowerChangeMs = now;

    if(instrumentOn)
    {
      setLastPlayed("Ready");
      turnDisplayOn();
      Serial.println("POWER: ON");
    }
    else
    {
      muteAllSounds();
      i2s_zero_dma_buffer(I2S_NUM_0);
      turnDisplayOff();
      Serial.println("POWER: OFF");
    }
  }

  powerWasPressed = pressed;
}

void updateSensors()
{
  if(!instrumentOn) return;

  for(int i = 0; i < KEY_COUNT; i++)
  {
    bool pressed = (digitalRead(sensorPins[i]) == LOW);

    if(pressed && !wasPressed[i])
    {
      if(currentMode == 4)
      {
        triggerDrum(i);
      }
      else
      {
        triggerNote(i);
      }
    }

    wasPressed[i] = pressed;
  }
}

float renderNoteVoice(NoteVoice &voice)
{
  if(!voice.active) return 0.0f;

  float sample = 0.0f;

  if(voice.mode == 0)
  {
    sample = sinf(voice.phase) + 0.12f * sinf(2.0f * voice.phase);
    voice.amp *= 0.999965f;
  }
  else if(voice.mode == 1)
  {
    sample = sinf(voice.phase)
           + 0.36f * sinf(2.0f * voice.phase)
           + 0.18f * sinf(3.0f * voice.phase);
    voice.amp *= 0.99945f;
  }
  else if(voice.mode == 2)
  {
    sample = sinf(voice.phase)
           + 0.30f * sinf(2.0f * voice.phase)
           + 0.10f * sinf(4.0f * voice.phase);
    voice.amp *= 0.99935f;
  }
  else
  {
    sample = sinf(voice.phase)
           + 0.55f * sinf(2.42f * voice.phase)
           + 0.25f * sinf(3.83f * voice.phase);
    voice.amp *= 0.99972f;
  }

  if(voice.mode == 1 || voice.mode == 2 || voice.mode == 3)
  {
    float attackSamples = 180.0f;
    if(voice.age < attackSamples)
    {
      sample *= voice.age / attackSamples;
    }
  }

  voice.phase += DRUM_TWO_PI * voice.freq / SAMPLE_RATE;
  if(voice.phase > DRUM_TWO_PI) voice.phase -= DRUM_TWO_PI;
  voice.age += 1.0f;

  if(voice.amp < 0.001f)
  {
    voice.active = false;
    return 0.0f;
  }

  return sample * voice.amp * 0.20f;
}

float renderKick()
{
  if(!kick.active) return 0.0f;
  if(kick.pos >= kick.total) { kick.active = false; return 0.0f; }
  float t = (float)kick.pos / (float)kick.total;
  float freq = 48.0f + 105.0f * (1.0f - t) * (1.0f - t);
  kick.phase1 += DRUM_TWO_PI * freq / SAMPLE_RATE;
  if(kick.phase1 > DRUM_TWO_PI) kick.phase1 -= DRUM_TWO_PI;
  float clickEnv = kick.pos < msToSamples(7) ? 1.0f - ((float)kick.pos / (float)msToSamples(7)) : 0.0f;
  float click = highpass(randomNoise(), kick.filter1, 0.55f) * clickEnv * 0.16f;
  float body = sinf(kick.phase1) * envPow(t, 4) * 1.10f;
  kick.pos++;
  return body + click;
}

float renderSnare()
{
  if(!snare.active) return 0.0f;
  if(snare.pos >= snare.total) { snare.active = false; return 0.0f; }
  float t = (float)snare.pos / (float)snare.total;
  snare.phase1 += DRUM_TWO_PI * 185.0f / SAMPLE_RATE;
  if(snare.phase1 > DRUM_TWO_PI) snare.phase1 -= DRUM_TWO_PI;
  float body = sinf(snare.phase1) * envPow(t, 3) * 0.32f;
  float wire = highpass(randomNoise(), snare.filter1, 0.38f) * envPow(t, 2) * 0.85f;
  float crack = snare.pos < msToSamples(9) ? highpass(randomNoise(), snare.filter2, 0.62f) * 0.22f : 0.0f;
  snare.pos++;
  return body + wire + crack;
}

float renderTom(DrumVoice &tom, float startFreq, float endFreq, float level)
{
  if(!tom.active) return 0.0f;
  if(tom.pos >= tom.total) { tom.active = false; return 0.0f; }
  float t = (float)tom.pos / (float)tom.total;
  float sweep = (1.0f - t) * (1.0f - t);
  float freq = endFreq + (startFreq - endFreq) * sweep;
  tom.phase1 += DRUM_TWO_PI * freq / SAMPLE_RATE;
  if(tom.phase1 > DRUM_TWO_PI) tom.phase1 -= DRUM_TWO_PI;
  float tone = sinf(tom.phase1) * envPow(t, 3) * level;
  float attack = tom.pos < msToSamples(5) ? highpass(randomNoise(), tom.filter1, 0.42f) * 0.08f : 0.0f;
  tom.pos++;
  return tone + attack;
}

float renderHat(DrumVoice &hat, float durationLevel, int decayPower)
{
  if(!hat.active) return 0.0f;
  if(hat.pos >= hat.total) { hat.active = false; return 0.0f; }
  float t = (float)hat.pos / (float)hat.total;
  float n = highpass(randomNoise(), hat.filter1, 0.72f);
  hat.phase1 += DRUM_TWO_PI * 6200.0f / SAMPLE_RATE;
  hat.phase2 += DRUM_TWO_PI * 8100.0f / SAMPLE_RATE;
  if(hat.phase1 > DRUM_TWO_PI) hat.phase1 -= DRUM_TWO_PI;
  if(hat.phase2 > DRUM_TWO_PI) hat.phase2 -= DRUM_TWO_PI;
  float metal = (squareFromPhase(hat.phase1) + squareFromPhase(hat.phase2)) * 0.12f;
  hat.pos++;
  return (n * 0.75f + metal) * envPow(t, decayPower) * durationLevel;
}

float renderClap()
{
  if(!clap.active) return 0.0f;
  if(clap.pos >= clap.total) { clap.active = false; return 0.0f; }
  uint32_t o1 = 0, o2 = msToSamples(13), o3 = msToSamples(27), decay = msToSamples(38);
  float env = 0.0f;
  if(clap.pos >= o1 && clap.pos < o1 + decay) env += 1.0f - (float)(clap.pos - o1) / (float)decay;
  if(clap.pos >= o2 && clap.pos < o2 + decay) env += 0.75f - (float)(clap.pos - o2) / (float)decay * 0.75f;
  if(clap.pos >= o3 && clap.pos < o3 + decay) env += 0.55f - (float)(clap.pos - o3) / (float)decay * 0.55f;
  float noise = highpass(randomNoise(), clap.filter1, 0.45f);
  clap.pos++;
  return noise * env * 0.62f;
}

float renderRimshot()
{
  if(!rimshot.active) return 0.0f;
  if(rimshot.pos >= rimshot.total) { rimshot.active = false; return 0.0f; }
  float t = (float)rimshot.pos / (float)rimshot.total;
  rimshot.phase1 += DRUM_TWO_PI * 920.0f / SAMPLE_RATE;
  if(rimshot.phase1 > DRUM_TWO_PI) rimshot.phase1 -= DRUM_TWO_PI;
  float stick = squareFromPhase(rimshot.phase1) * 0.52f;
  float snap = highpass(randomNoise(), rimshot.filter1, 0.58f) * 0.34f;
  rimshot.pos++;
  return (stick + snap) * envPow(t, 5);
}

float renderCymbal(DrumVoice &cymbal, float level, int decayPower, bool rideMode)
{
  if(!cymbal.active) return 0.0f;
  if(cymbal.pos >= cymbal.total) { cymbal.active = false; return 0.0f; }
  float t = (float)cymbal.pos / (float)cymbal.total;
  float noise = highpass(randomNoise(), cymbal.filter1, 0.67f);
  float f1 = rideMode ? 2600.0f : 3400.0f;
  float f2 = rideMode ? 4100.0f : 5300.0f;
  cymbal.phase1 += DRUM_TWO_PI * f1 / SAMPLE_RATE;
  cymbal.phase2 += DRUM_TWO_PI * f2 / SAMPLE_RATE;
  if(cymbal.phase1 > DRUM_TWO_PI) cymbal.phase1 -= DRUM_TWO_PI;
  if(cymbal.phase2 > DRUM_TWO_PI) cymbal.phase2 -= DRUM_TWO_PI;
  float metal = (squareFromPhase(cymbal.phase1) * 0.20f) + (squareFromPhase(cymbal.phase2) * 0.16f);
  cymbal.pos++;
  return (noise * 0.72f + metal) * envPow(t, decayPower) * level;
}

float renderCowbell()
{
  if(!cowbell.active) return 0.0f;
  if(cowbell.pos >= cowbell.total) { cowbell.active = false; return 0.0f; }
  float t = (float)cowbell.pos / (float)cowbell.total;
  cowbell.phase1 += DRUM_TWO_PI * 540.0f / SAMPLE_RATE;
  cowbell.phase2 += DRUM_TWO_PI * 800.0f / SAMPLE_RATE;
  if(cowbell.phase1 > DRUM_TWO_PI) cowbell.phase1 -= DRUM_TWO_PI;
  if(cowbell.phase2 > DRUM_TWO_PI) cowbell.phase2 -= DRUM_TWO_PI;
  float tone = (squareFromPhase(cowbell.phase1) * 0.46f) + (squareFromPhase(cowbell.phase2) * 0.38f);
  float attack = cowbell.pos < msToSamples(5) ? highpass(randomNoise(), cowbell.filter1, 0.52f) * 0.12f : 0.0f;
  cowbell.pos++;
  return (tone + attack) * envPow(t, 3) * 0.62f;
}

float renderTambourine()
{
  if(!tambourine.active) return 0.0f;
  if(tambourine.pos >= tambourine.total) { tambourine.active = false; return 0.0f; }
  float t = (float)tambourine.pos / (float)tambourine.total;
  float n = highpass(randomNoise(), tambourine.filter1, 0.74f);
  float pulse = 0.45f + 0.55f * fabsf(sinf((float)tambourine.pos * 0.021f));
  tambourine.pos++;
  return n * envPow(t, 2) * pulse * 0.46f;
}

float renderShaker()
{
  if(!shaker.active) return 0.0f;
  if(shaker.pos >= shaker.total) { shaker.active = false; return 0.0f; }
  float t = (float)shaker.pos / (float)shaker.total;
  float n = highpass(randomNoise(), shaker.filter1, 0.78f);
  shaker.pos++;
  return n * envPow(t, 3) * 0.34f;
}

float renderDrums()
{
  float mix = 0.0f;
  mix += renderKick();
  mix += renderSnare();
  mix += renderHat(closedHat, 0.46f, 5);
  mix += renderClap();
  mix += renderTom(midTom, 230.0f, 92.0f, 0.68f);
  mix += renderTom(lowTom, 170.0f, 62.0f, 0.76f);
  mix += renderTom(highTom, 330.0f, 135.0f, 0.62f);
  mix += renderRimshot();
  mix += renderCymbal(crash, 0.52f, 2, false);
  mix += renderCymbal(ride, 0.34f, 2, true);
  mix += renderCowbell();
  mix += renderHat(openHat, 0.42f, 2);
  mix += renderTambourine();
  mix += renderShaker();
  return mix * 0.82f;
}

void setupI2S()
{
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void generateAudio()
{
  int16_t buffer[BUFFER_SIZE];

  if(!instrumentOn || volumeLevel == MIN_VOLUME)
  {
    memset(buffer, 0, sizeof(buffer));

    size_t bytesWritten;
    i2s_write(I2S_NUM_0,
              buffer,
              sizeof(buffer),
              &bytesWritten,
              portMAX_DELAY);
    return;
  }

  float volume = currentVolume();

  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    float mix = 0.0f;

    for(int v = 0; v < KEY_COUNT; v++)
    {
      mix += renderNoteVoice(noteVoices[v]);
    }

    mix += renderDrums();
    mix = softclip(mix);
    buffer[i] = (int16_t)(mix * MASTER_GAIN * volume);
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0,
            buffer,
            sizeof(buffer),
            &bytesWritten,
            portMAX_DELAY);
}

void setup()
{
  Serial.begin(115200);

  for(int i = 0; i < KEY_COUNT; i++)
  {
    pinMode(sensorPins[i], INPUT_PULLUP);
    noteVoices[i].active = false;
  }

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(VOLUME_UP_PIN, INPUT_PULLUP);
  pinMode(VOLUME_DOWN_PIN, INPUT_PULLUP);
  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  setupI2S();
  setupDisplay();
  updateDisplay();

  Serial.println("MULTI-MODE INSTRUMENT READY");
  Serial.println("Mode button: GPIO23 to GND");
  Serial.println("Power button: GPIO27 to GND");
  Serial.println("Volume up/down: GPIO32/GPIO33 to GND");
  Serial.println("Modes: Flute, Piano, Guitar, Bell, Drums");
  Serial.print("MODE: ");
  Serial.println(modeNames[currentMode]);
}

void loop()
{
  handlePowerButton();

  if(instrumentOn)
  {
    handleModeButton();
    handleVolumeButtons();
    updateSensors();
    updateDisplay();
  }

  generateAudio();
}
