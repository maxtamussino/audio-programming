/***** FirstOrderFilterIIR.h *****/

#pragma once

class FirstOrderFilterIIR {
public:
	FirstOrderFilterIIR();
	
	void set_coefficients(float coeffB0, float coeffB1, float coeffA1);
	float process(float input);
	
	~FirstOrderFilterIIR() {}
	
private:
	float coeffB0_;
	float coeffB1_;
	float coeffA1_;
	
	float lastX_;
	float lastY_;
};