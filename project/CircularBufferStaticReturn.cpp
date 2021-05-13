/***** CircularBufferStaticReturn.cpp *****/
/* Class implementation of a circular buffer with
 * static pre-initialised return vector
 *
 * ECS7012P - Queen Mary University of London
 * Final project, Max Tamussino
 */
 
#include <Bela.h>
#include <vector>
#include "CircularBufferStaticReturn.h"

// Constructor setting return buffer length and also calling parent constructor
template <typename T>
CircularBufferStaticReturn<T>::CircularBufferStaticReturn(unsigned int _length, unsigned int _returnLength) : 
	CircularBuffer<T>(_length), returnBufferLength_(_returnLength)  {
	// Empty
}

// Set up the buffers (NOT REAL-TIME SAFE, use at beginning)
// This overridden setup function also resizes the return buffer
template <typename T>
void CircularBufferStaticReturn<T>::setup() {
	// Allocate enough memory during setup
	returnBuffer_.resize(returnBufferLength_);
	CircularBuffer<T>::setup();
}

// This function returns a vector of last elements, the number of which being
// specified at the construction of the class. It is real-time safe, as the
// memory needed is already allocated when the buffer is set up.
template <typename T>
std::vector<T> CircularBufferStaticReturn<T>::get_last_elements() {
	// Fill return buffer with the last N samples
	for (int i = 0; i < returnBufferLength_; i++) {
		// Calculate new index, which is N-i samples back and needs to be wrapped
		int index = (this->writePointer_ - returnBufferLength_ + this->bufferLength_ + i) % this->bufferLength_;
		
		// Write in buffer (starting from end)
		returnBuffer_[i] = this->buffer_[index];
	}
	
	// This deliberately uses "return by value", which is specified preferred
	// since the C++11 standard, because most compilers support "copy elision"
	return returnBuffer_;
}

template class CircularBufferStaticReturn<float>;