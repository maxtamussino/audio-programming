/***** led.h *****/
/* Class implementation of a LED
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>

class LED {
public:
	// Constructor
	LED(int digital_pin);
	
	// Setup
	void setup(BelaContext *context);
	
	// To be called once for each frame
	void process(BelaContext *context, int frame);
	
	// Initiate one flash
	void flash(BelaContext *context, int frame, int ontime_ms);
	
	// Destructor
	~LED() {}
	
private:
	// Info
	int pin;
	bool setup_done = false;
	
	// LED timing (in frames)
	int remaining_ontime, total_ontime;
};