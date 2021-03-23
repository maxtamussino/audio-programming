/***** Accelerometer.h *****/
/* Class implementation of an accelerometer
 *
 * ECS7012P - Queen Mary University of London
 * Assignment 2, Max Tamussino
 */

#pragma once
#include <Bela.h>
#include <libraries/Scope/Scope.h>

#include "Filter.h"

class Accelerometer {
public:
	// Constructor
	Accelerometer(int analog_in_pin_x, int analog_in_pin_y, int analog_in_pin_z, int digital_pin_sleep);
	
	// Setup
	void setup(BelaContext *context);
	
	// To be called once for each frame
	void process(BelaContext *context, int frame);
	
	// Calibration
	void calibrate();
	
	// Check if state is new this frame
	bool new_state_now() { return state_new; }
	
	// Status enum
	enum status_e { flat, over, front, back, left, right, intermediate };
	status_e get_status() { return state; }
	
	// Tap detection
	bool tap_detected_now() { return tap_detected; }
	
	// Destructor
	~Accelerometer() {}
	
private:
	// Info
	int pins[3], pin_sleep;
	bool setup_done = false;
	
	// Debug
	Scope scope;
	
	// Filters
	std::vector<Filter> lowpass_firststage, lowpass_secondstage;
	
	// State
	status_e state;
	bool state_new, tap_detected;
	float input[3];                  // Input [V]
	float accelerations_raw[3];      // Raw
	float accelerations_smooth[3];   // After initial LP filtering
	float accelerations_filtered[3]; // After LP, downsampling and LP again
	
	// Downsampling
	unsigned int downsample_rate = 32;
	unsigned int downsample_counter = 0;
	
	// Thresholding
	const float exit_threshold = 0.6;  // Threshold to enter a state [g]
	const float enter_threshold = 0.8; // Threshold to leave a state [g]
	
	// Voltage to g mapping
	float input_0g[3];
	float input_1g[3];
	
	// Analog frame handling
	int audioFramesPerAnalogFrame;
	
	// Utility
	void calculate_new_state();
	void set_new_state(Accelerometer::status_e new_state);
};
