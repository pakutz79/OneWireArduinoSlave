/*
 * mcp455x.cpp
 *
 *  Created on: 27.02.2016
 *      Author: norbert
 */

#include "mcp455x.h"
#if (defined __AVR_ATtiny25__ || defined  __AVR_ATtiny45__ || defined  __AVR_ATtiny85__)
#define WI_ATTINY25
#else
#include "Wire.h"
#endif

const byte ADR_BASE  = 0x28;
const byte CMD_READ  = 0x0c;
const byte CMD_WRITE = 0x00;
const byte CMD_INC   = 0x04;
const byte CMD_DEC   = 0x08;

const byte REG_WIPER0 = 0x00;
const byte REG_WIPER1 = 0x10;
const byte REG_TCON   = 0x40;

const byte GCEN = 0x01; // General Call Enable bit;
const byte R1HW = 0x80; // Resistor 1 Hardware Configuration Control bit (1 = not shutdown, 0 = shutdown)
const byte R1A  = 0x40; // Resistor 1 Terminal A (P1A pin) Connect Control bit (1 = connected)
const byte R1W  = 0x20; // Resistor 1 Wiper      (P1W pin) Connect Control bit (1 = connected)
const byte R1B  = 0x10; // Resistor 1 Terminal B (P1B pin) Connect Control bit (1 = connected)
const byte R0HW = 0x08; // Resistor 0 Hardware Configuration Control bit (1 = not shutdown, 0 = shutdown)
const byte R0A  = 0x04; // Resistor 0 Terminal A (P0A pin) Connect Control bit (1 = connected)
const byte R0W  = 0x02; // Resistor 0 Wiper      (P0W pin) Connect Control bit (1 = connected)
const byte R0B  = 0x01; // Resistor 0 Terminal B (P0B pin) Connect Control bit (1 = connected)

MCP455X::MCP455X(byte address):address_(address) {
}

void MCP455X::begin() {
#ifndef WI_ATTINY25
	Wire.beginTransmission(ADR_BASE | address_);
	Wire.write(REG_TCON | CMD_WRITE); //GCEN not set
	Wire.write(R1HW | R1A | R1W | R1B | R0HW | R0A | R0W | R0B);
	Wire.endTransmission();
#endif
}

void MCP455X::writePosition(byte wiper, unsigned int pos) {
#ifndef WI_ATTINY25
        Wire.beginTransmission(ADR_BASE | address_);
        switch(wiper) {
	case 0:
		Wire.write(REG_WIPER0 | CMD_WRITE | ((pos >> 8) & 0x01) );
		break;
	case 1:
		Wire.write(REG_WIPER1 | CMD_WRITE | ((pos >> 8) & 0x01) );
		break;
	};
	Wire.write(pos & 0xFF);
	Wire.endTransmission();
#endif
}

unsigned int MCP455X::readPosition(byte wiper) {
#ifndef WI_ATTINY25
	Wire.beginTransmission(ADR_BASE | address_);
	switch(wiper) {
	case 0:
		Wire.write(REG_WIPER0 | CMD_READ);
		break;
	case 1:
		Wire.write(REG_WIPER1 | CMD_READ);
		break;
	};
	Wire.endTransmission(false);
	Wire.requestFrom(ADR_BASE | address_,2);
	Wire.read();
	byte ret = Wire.read();
	return ret;
#endif
}

unsigned int MCP455X::increment(byte wiper) {
#ifndef WI_ATTINY25
	Wire.beginTransmission(ADR_BASE | address_);
	switch(wiper) {
	case 0:
		Wire.write(REG_WIPER0 | CMD_INC);
		break;
	case 1:
		Wire.write(REG_WIPER1 | CMD_INC);
		break;
	};
	Wire.endTransmission();
	return readPosition(wiper);
#endif
}

unsigned int MCP455X::decrement(byte wiper) {
#ifndef WI_ATTINY25
	Wire.beginTransmission(ADR_BASE | address_);
	switch(wiper) {
	case 0:
		Wire.write(REG_WIPER0 | CMD_DEC);
		break;
	case 1:
		Wire.write(REG_WIPER1 | CMD_DEC);
		break;
	};
	Wire.endTransmission();
	return readPosition(wiper);
#endif WI_ATTINY25
}




