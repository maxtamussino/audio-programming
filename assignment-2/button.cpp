/***** button.cpp *****/
/* Class implementation of a button
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#include "button.h"
#include <Bela.h>
#include <cmath>

// Standard constructor
Button::Button(int digital_pin) {
	// Set pin
	pin = digital_pin;
	
	// Other
	status = notpressed;
}

// To be called during setup
void Button::setup(BelaContext *context, int debounce_ms) {
	// Pin mode
	pinMode(context, 0, pin, INPUT);
	
	// Calculate timer interval in frames
	debounce_interval = debounce_ms * context->digitalSampleRate / 1000;
	debounce_counter = 0;
	
	// Finish
	setup_done = true;
}

// To be called once per frame
void Button::process(BelaContext *context, int frame) {
	if (!setup_done) return;
	
	int current_read = digitalRead(context, frame, pin);
	if (event_pressed) event_pressed = false;
	if (event_released) event_released = false;
	
	switch (status) {
		case pressed_bounce:
			if (--debounce_counter == 0) status = pressed;
    		break;
    	case pressed:
			if (current_read == HIGH) {
				event_released = true;
				status = notpressed_bounce;
				debounce_counter = debounce_interval;
			}
    		break;
    	case notpressed_bounce:
			if (--debounce_counter == 0) status = notpressed;
    		break;
    	case notpressed:
			if (current_read == LOW) {
				event_pressed = true;
				status = pressed_bounce;
				debounce_counter = debounce_interval;
			}
    		break;
	}
}

// Returns true in exactly one frame when button is pressed
bool Button::pressed_now() {
	return event_pressed;
}


// Returns true in exactly one frame when button is released
bool Button::released_now() {
	return event_released;
}