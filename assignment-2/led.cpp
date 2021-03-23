/***** Led.cpp *****/
/* Class implementation of a LED
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "Led.h"
#include <Bela.h>
#include <cmath>

// Standard constructor
LED::LED(int digital_pin) {
	// Set pin
	pin = digital_pin;
}

// To be called during setup
void LED::setup(BelaContext *context) {
	// Pin mode
	pinMode(context, 0, pin, OUTPUT);
	
	// Finish
	setup_done = true;
}

// To be called once per frame
void LED::process(BelaContext *context, int frame) {
	if (!setup_done) return;
	
	if (!remaining_ontime--) {
		digitalWrite(context, frame, pin, LOW);
	}
}

// Initiate LED flash
void LED::flash(BelaContext *context, int frame, int ontime_ms) {
	if (!setup_done) return;
	
	// Calculate timer interval in frames
	total_ontime = ontime_ms * context->digitalSampleRate / 1000;
	remaining_ontime = 0;
	
	// Set output
	digitalWrite(context, frame, pin, HIGH);
	remaining_ontime = total_ontime;
}