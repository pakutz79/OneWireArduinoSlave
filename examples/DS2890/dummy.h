/*
 * dummy.h
 *
 *  Created on: 27.02.2016
 *      Author: norbert
 */

#ifndef DUMMY_H_
#define DUMMY_H_

#include "Potentiometer.h"

class Dummy:public Potentiometer
{
public:
	Dummy() { position_ = 0; };
	void begin() {};
	void writePosition(byte wiper, unsigned int pos) { position_ = pos; };
	unsigned int readPosition(byte wiper) { return position_; };
	unsigned int increment(byte wiper) { if (position_ < steps) position_++; return position_; };
	unsigned int decrement(byte wiper) { if (position_ > 0) position_--; return position_; };

	static const byte steps = 0xFF;
private:
	unsigned int position_;
};

#endif /* DUMMY_H_ */
