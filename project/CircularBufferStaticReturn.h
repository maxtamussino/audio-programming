/***** CircularBufferStaticReturn.h *****/
/* Class implementation of a circular buffer with
 * static pre-initialised return vector
 *
 * ECS7012P - Queen Mary University of London
 * Final project, Max Tamussino
 */

#pragma once
#include <Bela.h>
#include <vector>
#include "CircularBuffer.h"

template <typename T>
class CircularBufferStaticReturn : public CircularBuffer<T> {
public:
	// Constructor adding a return vector of fixed size
	CircularBufferStaticReturn(unsigned int _length, unsigned int _returnLength);
	
	// Setup (must be called during Bela setup), which also allocates memory for return buffer
	void setup();
	
	// Get last elements, the number of which is specified in the constructor
	std::vector<T> get_last_elements();
	
	// Destructor
	~CircularBufferStaticReturn() {}
private:
	// Static return buffer
	std::vector<T> returnBuffer_;
	unsigned int returnBufferLength_;
};