#include <Arduino.h>
#include "FastLED.h"
#include "MIDI.h"

#include <array>
#include <algorithm>

String buf = "";
bool buf_ready = false;

const String chromatic = "chr";
const String major = "maj";
const String minor = "min";

enum class Scale
{
  Chromatic,
  Major,
  Minor
} current_scale;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, m);

using ColorData = std::array<CRGB, 16>;

ColorData note_state;

const byte NOTE_B2 = 59;
const byte NOTE_D4 = 74;

ColorData leds;
ColorData rainbow = {
  0xff00ff,0xff0099,0xff0033,0xff3300,
  0xff9900,0xffff00,0x99ff00,0x33ff00,
  0xff33,0xff99,0xffff,0x99ff,
  0x33ff,0x3300ff,0x9900ff,0x9900ff
};

ColorData chromatic_scale = {
  0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
  0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
  0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
  0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF
};

ColorData major_scale = {
  0xFFFFFF,0xFFFFFF,0,0xFFFFFF,
  0,0xFFFFFF,0xFFFFFF,0,
  0xFFFFFF,0,0xFFFFFF,0,
  0xFFFFFF,0xFFFFFF,0,0xFFFFFF
};

ColorData minor_scale = {
  0,0xFFFFFF,0,0xFFFFFF,
  0xFFFFFF,0,0xFFFFFF,0,
  0xFFFFFF,0xFFFFFF,0,0xFFFFFF,
  0,0xFFFFFF,0,0xFFFFFF
};

const uint8_t clock_pin = 14;
const uint8_t data_pin = 15;

void setup() {
  Serial.begin(115200);
  buf.reserve(50);

  current_scale = Scale::Chromatic;
  note_state.fill(0xFFFFFF);

  std::copy(rainbow.begin(), rainbow.end(), leds.begin());
  FastLED.addLeds<APA102, data_pin, clock_pin, EOrder::BGR>(
    leds.data(), leds.size());
  FastLED.setBrightness(50);

  m.begin(1);
}

void loop() {

  FastLED.show();

  if (m.read())
  {
    int note = 0;
    switch (m.getType())
    {
    case midi::NoteOn:
      note = m.getData1();
      if (note >= NOTE_B2 || note <= NOTE_D4)
      {
        Serial.println(note);
        note_state[note - 59] = 0;
      }
      break;
    case midi::NoteOff:
      note = m.getData1();
      if (note >= NOTE_B2 || note <= NOTE_D4)
      {
        Serial.println(note);
        note_state[note - 59] = 0xFFFFFF;
      }
      break;
    default:
      break;
    }
  }

  if (buf_ready)
  {
    buf_ready = false;
    if (buf.substring(0, 3) == major)
    {
      current_scale = Scale::Major;
    }
    else if (buf.substring(0, 3) == minor)
    {
      current_scale = Scale::Minor;
    }
    else if (buf.substring(0, 3) == chromatic)
    {
      current_scale = Scale::Chromatic;
    }
    buf = "";
  }

  static uint32_t shift_counter = millis();
  if (millis() - shift_counter > 100)
  {
    std::rotate(
      rainbow.begin(), 
      rainbow.begin()+1, 
      rainbow.end());
    shift_counter = millis();
  }

  const CRGB* scaleptr = nullptr;
  switch (current_scale)
  {
  case Scale::Chromatic:
    scaleptr = chromatic_scale.data();
    break;
  case Scale::Major:
    scaleptr = major_scale.data();
    break;
  case Scale::Minor:
    scaleptr = minor_scale.data();
    break;
  default:
    break;
  }

  for (size_t i = 0; i < leds.size(); i++)
  {
    leds[i] = rainbow[i] & scaleptr[i] & note_state[i];
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char) Serial.read();

    buf += inChar;

    if (inChar == '\n')
    {
      buf_ready = true;
    }
  }
}