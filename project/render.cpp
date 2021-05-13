/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

http://bela.io

C++ Real-Time Audio Programming with Bela - Lecture 17: Block-based processing
fft-circular-buffer: show spectrum of signal from overlapping FFT windows
*/

#include <cmath>
#include <cstring>
#include <vector>

#include <Bela.h>
#include <libraries/Fft/Fft.h>
#include <libraries/Gui/Gui.h>
#include <libraries/math_neon/math_neon.h>

#include "MonoFilePlayer.h"
#include "CircularBuffer.h"
#include "CircularBufferStaticReturn.h"

// System parameters
#define DETECT_DOMINANT_IS_HARMONIC true
#define USE_WAV_FILE false

// FFT-related variables
const int kDownsample = 16;  // Downsampling factor before FFT
Fft gFft;					// FFT processing object
const int kFftSize = 2048;	// FFT window size in samples
const int kHopSize = 512;	// How often we calculate a window

// Circular buffer for assembling a window of samples
const int kBufferSize = 16384;
CircularBufferStaticReturn<float> gInputBuffer(kBufferSize, kFftSize);
std::vector<float> gUnwrappedBuffer(kFftSize);
int gHopCounter = 0;

// Name of the sound file (in project folder)
std::string gFilename = "guitar-c3.wav"; 

// Object that handles playing sound from a buffer
MonoFilePlayer gPlayer;

// GUI to display the spectrum
Gui gSpectrumGui;


bool setup(BelaContext *context, void *userData)
{
	#if USE_WAV_FILE
	// Load the audio file
	if(!gPlayer.setup(gFilename)) {
    	rt_printf("Error loading audio file '%s'\n", gFilename.c_str());
    	return false;
	}

	// Print some useful info
    rt_printf("Loaded the audio file '%s' with %d frames (%.1f seconds)\n", 
    			gFilename.c_str(), gPlayer.size(),
    			gPlayer.size() / context->audioSampleRate);
    #endif
	
	// Set up the FFT and its input buffer
	gFft.setup(kFftSize);
	gInputBuffer.setup();
	
	// GUI to show the spectrum
	gSpectrumGui.setup(context->projectName);

	return true;
}


// This function is used to find local maxima at multiples of an index in a given vector.
// More specifically for this project, this function is used to find local maxima in
// the spectrum at multiples of the dominant frequency in order to determine if the 
// dominant frequency is actually a harmonic
unsigned int findMaximumAroundIndex(std::vector<float> *vec, unsigned int idx, float divisor, float deviation) {
	// Define search area
	int minIdx = floor((1.0 - deviation) / divisor * idx);
	int maxIdx = ceil((1.0 + deviation) / divisor * idx);
	
	// Limit to array bounds
	if (minIdx < 0) minIdx = 0;
	if (maxIdx > (*vec).size()) maxIdx = (*vec).size();
	
	// Initialise maximum
	float current, maximum = 0.0;
	unsigned int maximumIdx;
	
	// Search
	for (int i = minIdx; i < maxIdx; i++) {
		current = (*vec)[i];
		if (current > maximum) {
			maximum = current;
			maximumIdx = i;
		}
	}
	
	return maximumIdx;
}


// This function handles the FFT processing in this example once the buffer has
// been assembled.
void process_fft(float sampleRate)
{
	static std::vector<float> fftCurrentOut(kFftSize / 2);	// Current FFT energy
	static float dominantMag;
	static unsigned int dominantFreqIdx, fundamentalFreqIdx;
	
	// Process the FFT based on the time domain input
	gFft.fft(gUnwrappedBuffer);
		
	// Copy the lower half of the FFT bins to a buffer and find the global maximum
	dominantMag = 0;
	float current;
	for (int n = 0; n < kFftSize / 2; n++) {
		current = gFft.fda(n);
		fftCurrentOut[n] = current;
		if (current > dominantMag) {
			dominantMag = current;
			dominantFreqIdx = n;
		}
	}
	
	// Check if the dominant frequency is actually a harmonic
	#if DETECT_DOMINANT_IS_HARMONIC
	
	// Check if maximum is actually second harmonic
	bool dominantFreqIsSecondHarmonic = false;
	unsigned int halfFreqMaxIdx = findMaximumAroundIndex(&fftCurrentOut, dominantFreqIdx, 2.0, 0.03);
	if (fftCurrentOut[halfFreqMaxIdx] > 0.7 * dominantMag) {
		dominantFreqIsSecondHarmonic = true;
		fundamentalFreqIdx = halfFreqMaxIdx;
	}
	
	// Check if maximum is actually third harmonic
	bool dominantFreqIsThirdHarmonic = false;
	unsigned int thirdFreqMaxIdx = findMaximumAroundIndex(&fftCurrentOut, dominantFreqIdx, 3.0, 0.03);
	if (fftCurrentOut[thirdFreqMaxIdx] > 0.7 * dominantMag) {
		dominantFreqIsThirdHarmonic = true;
		fundamentalFreqIdx = thirdFreqMaxIdx;
	}
	
	// If the dominant frequency is discovered to be either both or neither 
	// a second and third harmonic, the index is invalid and the same will
	// be sent twice for dominant and suspected fundamental
	if (dominantFreqIsSecondHarmonic == dominantFreqIsThirdHarmonic) {
		fundamentalFreqIdx = dominantFreqIdx;
	}
	
	// Dont check if the dominant frequency is actually a harmonic
	#else
	fundamentalFreqIdx = dominantFreqIdx;
	#endif
	
	// Calculate accurate frequency using parabolic interpolation
	// Get maximum and its neighbours
	float left =   fftCurrentOut[fundamentalFreqIdx-1];
	float middle = fftCurrentOut[fundamentalFreqIdx];
	float right =  fftCurrentOut[fundamentalFreqIdx+1];
	
	// Calculate frequency deviation from bin
	float deltaFreq = (right - left) / (2 * (2 * middle - left - right));
	
	// Convert index + deviation to frequency
	float fundamentalFreq = (fundamentalFreqIdx + deltaFreq) * sampleRate / (kFftSize * kDownsample);
	
	// Calculate MIDI note number (log2(x) is ln(x)/ln(2))
	float midiNoteNumber = 12.0 / M_LN2 * logf_neon(fundamentalFreq / 440.0) + 69.0;
	
	// Send the values to the GUI
	gSpectrumGui.sendBuffer(0, fftCurrentOut);
	gSpectrumGui.sendBuffer(1, fundamentalFreq);
	gSpectrumGui.sendBuffer(2, midiNoteNumber);
}


void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		// Read the next sample from the wav-file buffer
		#if USE_WAV_FILE
        float in = gPlayer.process();
        
        // Read microphone input
        #else
        float in = audioRead(context, n, 0);
        #endif
        
        // Downsampling
        if (n % kDownsample == 0) {
			// Store the sample in input buffer for the FFT
			gInputBuffer.write_element(in);
			
			// Increment the hop counter and start a new FFT if we've reached the hop size
			if (++gHopCounter == kHopSize) {
				gUnwrappedBuffer = gInputBuffer.get_last_elements();
				process_fft(context->audioSampleRate);
				gHopCounter = 0;
			}
        }
		
		// Write the audio to the output if file is played
		#if USE_WAV_FILE
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, in);
		}
		#endif
	}
}

void cleanup(BelaContext *context, void *userData)
{

}
