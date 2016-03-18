/*
 * Potentiometer.h
 *
 *  Created on: 27.02.2016
 *      Author: norbert
 */

#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#include "Arduino.h"

class Potentiometer
{
public:
	virtual void begin() = 0;
	virtual void writePosition(byte wiper, unsigned int pos) = 0;
	virtual unsigned int readPosition(byte wiper) = 0;
	virtual unsigned int increment(byte wiper) = 0;
	virtual unsigned int decrement(byte wiper) = 0;
};

#endif /* POTENTIOMETER_H_ */
