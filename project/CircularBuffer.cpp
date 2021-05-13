/***** CircularBuffer.cpp *****/
/* Class implementation of a circular buffer
 *
 * ECS7012P - Queen Mary University of London
 * Final project, Max Tamussino
 */

#include <Bela.h>
#include <vector>
#include "CircularBuffer.h"

// Constructor just setting the buffer length
template <typename T>
CircularBuffer<T>::CircularBuffer(unsigned int _length) : bufferLength_(_length) {
	// Empty
}

// Set up the buffer (NOT REAL-TIME SAFE, use at beginning)
template <typename T>
void CircularBuffer<T>::setup() {
	// Allocate enough memory during setup
	buffer_.resize(bufferLength_);
	writePointer_ = 0;
	setupDone_ = true;
}

// Write new element into buffer
template <typename T>
void CircularBuffer<T>::write_element(T _element) {
	// Write new element
	buffer_[writePointer_] = _element;
	
	// Increase and wrap the write pointer
	if (++writePointer_ == bufferLength_) writePointer_ = 0;
}

// This function returns a vector of N last elements, N being an arbitrary
// input to the function. It is NOT REAL-TIME SAFE! A new std::vector is 
// allocated and resized.
template <typename T>
std::vector<T> CircularBuffer<T>::get_last_N_elements(unsigned int _N) {
	// Create return buffer
	std::vector<T> returnBuffer;
	returnBuffer.resize(_N);
	
	// Check if N is valid (<= buffer size)
	if (_N > bufferLength_) return returnBuffer;
	
	// Fill return buffer with the last N samples
	for (int i = 0; i < _N; i++) {
		// Calculate new index, which is N-i samples back and needs to be wrapped
		int index = (writePointer_ - _N + bufferLength_ + i) % bufferLength_;
		
		// Write in buffer (starting from end)
		returnBuffer[i] = buffer_[index];
	}
	
	return returnBuffer;
}

// Returns one element which was put into the buffer N elements ago,
// N being an arbitrary number
template <typename T>
T CircularBuffer<T>::get_element_N_ago(unsigned int _N) {
	// Check if N is valid (<= buffer size)
	if (_N > bufferLength_) return 0;
	
	// Calculate wrapped index
	int index = (writePointer_ - _N + bufferLength_) % bufferLength_;
	
	return buffer_[index];
}

template class CircularBuffer<float>;
