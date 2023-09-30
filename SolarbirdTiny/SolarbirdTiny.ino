// nick@bitfasching.de, Kazoosh, 2021
// For ATtiny85 @ 8 MHz

//Do you need serial debug messages?
#define DEBUGMODE

// I/O pins //

#define PIN_PIEZO_A 3
#define PIN_PIEZO_B 4
#define PIN_LED_1_ANODE 0
#define PIN_LED_2_ANODE 1
#define PIN_LED_CATHODES 2


// High-level access to I/O and system features //

#include "system/leds.h"
#include "system/piezo.h"
#include "system/lowpower.h"
#include "serialout.h"


// Bird voice //

#include "voices/blackbird.h"
Blackbird Bird;


// Behaviour settings //

const bool runStartupTest = false;
const bool powerDownAfterStartIfDark = false;
const unsigned long powerDownAfterStartIfDarkTimeoutMillis = 5 * 1000;
const bool lightLEDsWhenPlaying = true;
const int activityMaximum = 8; // should be divisible by the difference of activeSoundInterval*
const int activeSoundIntervalMinimum = 1;
const int activeSoundIntervalMaximum = 5; // should be a power of 2 greater than the minimum
const int idleSoundIntervalMinimum = 60 * 10; // all intervals must be smaller than 32768
const int idleSoundIntervalMaximum = 60 * 60;
const int nightModeSleepSeconds = 2;
const bool silentAtNight = false;


// Main logic //

void setup()
{
  // wait to make sure programmer is high-Z
  // and be gentle with the power source
  delay( 100 );
#ifdef DEBUGMODE
  SerialOut::printString("\nH E L L O\n");
#endif

  if ( runStartupTest )
  {
    // startup test: flash LEDs
    LEDs::on();
    delay( 50 );
    LEDs::off();
    delay( 500 );

    // startup test: play all melodies once with fixed delay in between
    for ( int i = 0; i < Bird.numMelodies; i++ ) {
      Bird.playSingleMelody( i, lightLEDsWhenPlaying );
      delay( 1000 );
    }

    delay( 1000 );
  }

  if ( powerDownAfterStartIfDark )
  {
    // check if environment is not dark
    bool dark = true;
    unsigned long startTime = millis();
    while ( millis() - startTime <= powerDownAfterStartIfDarkTimeoutMillis ) {
      if ( LEDs::senseRawBrightness( false ) > 0 ) {
        dark = false;
        break;
      }
    }
    if ( dark ) {
      // no brightness sensed within defined time window, power down and don't wake up until external reset
      Piezo::on();
      Piezo::tone( 1E6 / 220, 1E6 );
      Piezo::off();
      LowPower::powerDown();
    }
  }

  // initialize random number generator with external source of randomness
  pinMode( PIN_LED_1_ANODE, INPUT ); pinMode( PIN_LED_2_ANODE, INPUT );
  randomSeed( analogRead( PIN_LED_1_ANODE ) ^ analogRead( PIN_LED_2_ANODE ) );
}

void loop()
{
  static int lastBrightness = 0;
  static int activity = 0;
  static bool active = false;
  static int cyclesBeforeNextSound = 0;

  // measure current brightness (logarithmic value, proportional to order of magnitude)
  int brightness = LEDs::senseBrightness();
#ifdef DEBUGMODE
  SerialOut::printString("\nbrightness = ");
  SerialOut::printHex(brightness);
#endif

  // determine if in darkness (i.e., night mode)
  bool dark = lastBrightness == 0 && brightness == 0;

  // compare with previous brightness (don't care for sign) and save for next cycle
  int brightnessActivity = abs( lastBrightness - brightness );
  lastBrightness = brightness;

  // accumulate activity within limit
  activity = activity + brightnessActivity;
  activity = min( activityMaximum, activity );
#ifdef DEBUGMODE
  SerialOut::printString("\nactivity = ");
  SerialOut::printHex(activity);
#endif

  // true if just became active in current cycle
  bool justBecameActive = false;

  // sensed activity?
  if ( activity > 0 )
  {
    // flash LEDs, the more activity the longer
    LEDs::on();
    delay( activity * 2 );
    LEDs::off();

    // just became active?
    if ( !active )
    {
      // set flag and remember active state
      justBecameActive = true;
      active = true;
    }
  }
  else
  {
    // clear active state
    active = false;
  }

  // time to make noise?
  // either after scheduled interval or on transition from idle to active
  if ( cyclesBeforeNextSound == 0 || justBecameActive )
  {
    // play sound
    Bird.play( lightLEDsWhenPlaying );

    // determine waiting time before next sound (both for active and idle case)
    if ( activity > 0 )
    {
      // active case = mapping of max activity to min interval plus one cycle of randomness
      cyclesBeforeNextSound = (activityMaximum - activity) * (activeSoundIntervalMaximum - activeSoundIntervalMinimum) / activityMaximum + activeSoundIntervalMinimum + random( 0, 1 + 1 );
    }
    else
    {
      cyclesBeforeNextSound = random( idleSoundIntervalMinimum, idleSoundIntervalMaximum + 1 );
    }
  }
  else
  {
    // stop counter when in night mode, i.e., remain silent at night
    if ( !dark || !silentAtNight ) {
      cyclesBeforeNextSound--;
    }
  }

  // decrease activity level
  if ( activity > 0 ) {
    activity--;
  }

  // wait 1 second by day (2 seconds by night) until next cycle
  LowPower::sleepSeconds( dark && !active ? nightModeSleepSeconds : 1 );
}
