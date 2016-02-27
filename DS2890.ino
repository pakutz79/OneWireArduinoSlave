/**********************************************************************************/
/* The MIT License (MIT)                                                          */
/*                                                                                */
/* Copyright (c) 2015 Youen Toupin, aka neuoy                                     */
/* Copyright (c) 2016 Norbert Truchsess, <norbert.truchsess@t-online.de>          */
/*                                                                                */
/* Permission is hereby granted, free of charge, to any person obtaining a copy   */
/* of this software and associated documentation files (the "Software"), to deal  */
/* in the Software without restriction, including without limitation the rights   */
/* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/* copies of the Software, and to permit persons to whom the Software is          */
/* furnished to do so, subject to the following conditions:                       */
/*                                                                                */
/* The above copyright notice and this permission notice shall be included in all */
/* copies or substantial portions of the Software.                                */
/*                                                                                */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/* SOFTWARE.                                                                      */
/**********************************************************************************/

#include "Arduino.h"
#include "LowLevel.h"
#include "OneWireSlave.h"

// This is the pin that will be used for one-wire data (depending on your arduino model, you are limited to a few choices, because some pins don't have complete interrupt support)
// On Arduino Uno, you can use pin 2 or pin 3
Pin oneWireData(2);

const byte analogOutPins[4] = { 3, 5, 6, 9 };

// This is the ROM the arduino will respond to, make sure it doesn't conflict with another device
const byte owROM[7] = { 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };

// This sample emulates a DS2890 device (digital potentiometer)

const byte DS2890_READ_POSITION = 0xF0;
const byte DS2890_WRITE_POSITION = 0x0F;
const byte DS2890_READ_CONTROL_REGISTER = 0xAA;
const byte DS2890_WRITE_CONTROL_REGISTER = 0x55;
const byte DS2890_INCREMENT = 0xC3;
const byte DS2890_DECREMENT = 0x99;

const byte DS2890_WRITE_RELEASE = 0x96;

const byte DS2890_FEATURE_PC = 0b00000001; // potentiometer characteristic: 0 logarithmic, 1 linear
const byte DS2890_FEATURE_WSV = 0b00000010; // wiper setting volatility: 0 non-volatile, 1 volatile
const byte DS2890_FEATURE_NP = 0b00001100; // number of potentiometers
const byte DS2890_FEATURE_NWP = 0b00110000; // number of wiper positions 00 => 32, 01 => 64, 10 => 128, 11 => 256
const byte DS2890_FEATURE_PR = 0b11000000; // potentiometer resistance: 00 => 5k, 01 => 10k, 10 => 50k, 11 => 100k

const byte featureRegister = 0b11111111; // linear, volatile, 4 potentiometer, 256 positions, 100k

const byte DS2890_CONTROL_WN = 0b00000011; // wiper number to control
const byte DS2890_CONTROL_WN_INV = 0b00001100; // inverted wiper number to control
const byte DS2890_CONTROL_WN_MASK = DS2890_CONTROL_WN_INV | DS2890_CONTROL_WN;
const byte DS2890_CONTROL_CPC = 0b01000000; // charge pump control: 0 => off, 1 => on

enum DeviceState {
	DS_WaitingReset,
	DS_WaitingCommand,
	DS_WaitingNewPosition,
	DS_WaitingNewPositionRelease,
	DS_WaitingNewControlRegister,
	DS_WaitingNewControlRegisterRelease
};

volatile DeviceState state = DS_WaitingReset;

volatile byte wiper = 0;
volatile byte wiperPositions[4];
volatile byte newPosition;
volatile byte controlRegister = 0b00001100;
volatile byte newControlRegister;

// This function will be called each time the OneWire library has an event to notify (reset, error, byte received)
void owReceive(OneWireSlave::ReceiveEvent evt, byte data);

void owAfterReadPosition(bool error);
void owAfterReadControlRegister(bool error);
void owAfterReadFinish(bool error);
void owAfterNewPosition(bool error);
void owAfterNewControlRegister(bool error);
void owAfterPositionChange(bool error);

void setup() {
	byte i;
	for (i=0;i<4;i++) {
		pinMode(analogOutPins[i],OUTPUT);
	}
	// Setup the OneWire library
	OWSlave.setReceiveCallback(&owReceive);
	OWSlave.begin(owROM, oneWireData.getPinNumber());
}

void loop() {
	delay(10);

	cli();
	//disable interrupts
	// Be sure to not block interrupts for too long, OneWire timing is very tight for some operations. 1 or 2 microseconds (yes, microseconds, not milliseconds) can be too much depending on your master controller, but then it's equally unlikely that you block exactly at the moment where it matters.
	// This can be mitigated by using error checking and retry in your high-level communication protocol. A good thing to do anyway.
	byte localWiper = wiper;
	byte localPosition = wiperPositions[localWiper];
	sei();

	//enable interrupts
	analogWrite(analogOutPins[localWiper], localPosition);
}

void owReceive(OneWireSlave::ReceiveEvent evt, byte data) {
	switch (evt) {
	case OneWireSlave::RE_Byte:
		switch (state) {
		case DS_WaitingCommand:
			switch (data) {
			case DS2890_READ_POSITION:
				OWSlave.write((const byte*)&controlRegister,1,&owAfterReadPosition);
				break;
			case DS2890_WRITE_POSITION:
				state = DS_WaitingNewPosition;
				break;
			case DS2890_READ_CONTROL_REGISTER:
				OWSlave.write((const byte*)&featureRegister,1,&owAfterReadControlRegister);
				break;
			case DS2890_WRITE_CONTROL_REGISTER:
				state = DS_WaitingNewControlRegister;
				break;
			case DS2890_INCREMENT:
				if (wiperPositions[wiper] < 0xFF) {
					wiperPositions[wiper]++;
				}
				OWSlave.write((const byte*)&wiperPositions[wiper],1,&owAfterPositionChange);
				break;
			case DS2890_DECREMENT:
				if (wiperPositions[wiper] > 0x00) {
					wiperPositions[wiper]--;
				}
				OWSlave.write((const byte*)&wiperPositions[wiper],1,&owAfterPositionChange);
				break;
			default:
				OWSlave.stopWrite();
				state = DS_WaitingReset;
				break;
			}
			break;

		case DS_WaitingNewPosition:
			newPosition = data;
			OWSlave.write((const byte*)&newPosition,1,&owAfterNewPosition);
			break;

		case DS_WaitingNewPositionRelease:
			if (data == DS2890_WRITE_RELEASE) {
				wiperPositions[wiper] = newPosition;
				OWSlave.writeBit(0,true);
			} else {
				OWSlave.writeBit(1,true);
			}
			state = DS_WaitingReset;
			break;

		case DS_WaitingNewControlRegister:
			if (((data & DS2890_CONTROL_WN_MASK) == 0b00001100) ||
				((data & DS2890_CONTROL_WN_MASK) == 0b00001001) ||
				((data & DS2890_CONTROL_WN_MASK) == 0b00000110) ||
				((data & DS2890_CONTROL_WN_MASK) == 0b00000011)) {
				newControlRegister = data;
				OWSlave.write((const byte*)&newControlRegister,1,&owAfterNewControlRegister);
			} else {
				OWSlave.writeBit(1,true);
				state = DS_WaitingReset;
			}
			break;

		case DS_WaitingNewControlRegisterRelease:
			if (data == DS2890_WRITE_RELEASE) {
				controlRegister = newControlRegister;
				wiper = newControlRegister & DS2890_CONTROL_WN;
				OWSlave.writeBit(0,true);
			} else {
				OWSlave.writeBit(1,true);
			}
			state = DS_WaitingReset;
			break;

		default:
			OWSlave.stopWrite();
			state = DS_WaitingReset;
			break;
		}
		break;

	case OneWireSlave::RE_Reset:
		state = DS_WaitingCommand;
		break;

	case OneWireSlave::RE_Error:
		OWSlave.stopWrite();
		state = DS_WaitingReset;
		break;
	}
}

void owAfterReadPosition(bool error) {
	if (error) {
		OWSlave.stopWrite();
		state = DS_WaitingReset;
	} else {
		OWSlave.write((const byte*)&wiperPositions[wiper],1,&owAfterReadFinish);
	}
}

void owAfterReadControlRegister(bool error) {
	if (error) {
		OWSlave.stopWrite();
		state = DS_WaitingReset;
	} else {
		OWSlave.write((const byte*)&controlRegister,1,&owAfterReadFinish);
	}
}

void owAfterReadFinish(bool error) {
	if (error) {
		OWSlave.stopWrite();
	} else {
		OWSlave.writeBit(0,true);
	}
	state = DS_WaitingReset;
}

void owAfterNewPosition(bool error) {
	OWSlave.stopWrite();
	state = error ? DS_WaitingReset : DS_WaitingNewPositionRelease;
}

void owAfterNewControlRegister(bool error) {
	OWSlave.stopWrite();
	state = error ? DS_WaitingReset : DS_WaitingNewControlRegisterRelease;
}

void owAfterPositionChange(bool error) {
	OWSlave.stopWrite();
	state = error ? DS_WaitingReset : DS_WaitingCommand;
}
