/***** Button.h *****/
/* Class implementation of a button
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>

class Button {
public:
	// Constructor
	Button(int digital_pin);
	
	// Setup
	void setup(BelaContext *context, int debounce_ms);
	
	// Status enum
	enum status_e { pressed_bounce, pressed, notpressed_bounce, notpressed };
	
	// To be called once for each sample
	void process(BelaContext *context, int frame);
	
	// Get events
	bool pressed_now();
	bool released_now();
	
	// Destructor
	~Button() {}
	
private:
	// Info
	int pin;
	bool setup_done = false;
	
	// State
	status_e status;
	bool event_pressed, event_released;
	
	// Debouncing interval
	int debounce_interval, debounce_counter;
};