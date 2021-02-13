/***** FirstOrderFilterIIR.h *****/

#pragma once

class FirstOrderFilterIIR {
public:
	FirstOrderFilterIIR();
	
	void calculate_coefficients(float sampleRate, float frequencyHz, float resonance);
	float process(float input);
	
	~FirstOrderFilterIIR() {}
	
private:
	float coeffB0_;
	float coeffB1_;
	float coeffA1_;
	
	float lastX_;
	float lastY_;
};