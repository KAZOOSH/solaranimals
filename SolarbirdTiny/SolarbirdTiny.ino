// nick@bitfasching.de, Kazoosh, 2021
// For ATtiny85 @ 8 MHz

#define PIN_PIEZO_A 3
#define PIN_PIEZO_B 4
#define PIN_LED_1_ANODE 0
#define PIN_LED_2_ANODE 1
#define PIN_LED_CATHODES 2

const bool runStartupTest = false;
const bool lightLEDsWhenPlaying = true;
const int activityMaximum = 8; // should be divisible by the difference of activeSoundInterval*
const int activeSoundIntervalMinimum = 1;
const int activeSoundIntervalMaximum = 5; // should be a power of 2 greater than the minimum
const int idleDaySoundIntervalMinimum = 60*20;
const int idleDaySoundIntervalMaximum = 60*40;
const int idleNightSoundIntervalMinimum = 60*60*7; // all intervals must be smaller than 32768
const int idleNightSoundIntervalMaximum = 60*60*9;


// High-level access to I/O and system features //

#include "system/leds.h"
#include "system/piezo.h"
#include "system/lowpower.h"


// Bird voices //

#include "voices/blackbird.h"
Blackbird Bird;


// Main logic //

void setup()
{
	// wait to make sure programmer is high-Z
	// and be gentle with the power source
	delay( 100 );

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

	// determine if in darkness (i.e., night mode)
	bool dark = lastBrightness == 0 && brightness == 0;

	// compare with previous brightness (don't care for sign) and save for next cycle
	int brightnessActivity = abs( lastBrightness - brightness );
	lastBrightness = brightness;

	// accumulate activity within limit
	activity = activity + brightnessActivity;
	activity = min( activityMaximum, activity );

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
			cyclesBeforeNextSound = (activityMaximum-activity) * (activeSoundIntervalMaximum-activeSoundIntervalMinimum) / activityMaximum + activeSoundIntervalMinimum + random( 0, 1+1 );
		}
		else
		{
			if ( dark )
			{
				// night mode
				cyclesBeforeNextSound = random( idleNightSoundIntervalMinimum, idleNightSoundIntervalMaximum+1 );
			}
			else
			{
				// day mode
				cyclesBeforeNextSound = random( idleDaySoundIntervalMinimum, idleDaySoundIntervalMaximum+1 );
			}
		}
	}
	else
	{
		// decrease counter
		cyclesBeforeNextSound--;
	}

	// decrease activity level
	if ( activity > 0 ) { activity--; }

	// wait one second until next cycle
	LowPower::sleepOneSecond();
}
