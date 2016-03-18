/*
 * mcp455x.h
 *
 *  Created on: 27.02.2016
 *      Author: norbert
 */

#ifndef MCP455X_H_
#define MCP455X_H_

#include "Potentiometer.h"

class MCP455X:public Potentiometer
{
public:
	MCP455X(byte address);
	void begin();
	void writePosition(byte wiper, unsigned int pos);
	unsigned int readPosition(byte wiper);
	unsigned int increment(byte wiper);
	unsigned int decrement(byte wiper);
	static const byte steps = 0xFF;
private:
	const byte address_;
};

#endif /* MCP455X_H_ */
