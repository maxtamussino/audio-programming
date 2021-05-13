/***** CircularBuffer.h *****/
/* Class implementation of a circular buffer
 *
 * ECS7012P - Queen Mary University of London
 * Final project, Max Tamussino
 */

#pragma once
#include <Bela.h>
#include <vector>

template <typename T>
class CircularBuffer {
public:
	// Constructor
	CircularBuffer(unsigned int _length);
	
	// Setup (must be called during Bela setup)
	void setup();
	
	// Add new value
	void write_element(T _element);
	T get_element_N_ago(unsigned int _N);
	
	// Get last N elements
	std::vector<T> get_last_N_elements(unsigned int _N);
	
	// Destructor
	~CircularBuffer() {}
	
protected:
	// Info
	unsigned int bufferLength_, writePointer_;
	bool setupDone_ = false;
	
	// Buffer
	std::vector<T> buffer_;
};
