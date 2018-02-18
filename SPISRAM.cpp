/*
 * SPISRAM.cpp
 *
 *  Created on: Feb 16, 2018
 *      Author: jguillaumes
 */

#include "SPISRAM.h"

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
       Serial.print("0x");
       for (int i=0; i<length; i++) {
         if (data[i]<0x10) {Serial.print("0");}
         Serial.print(data[i],HEX);
         Serial.print(" ");
       }
}

SPISRAM::SPISRAM(SPIClass * spi, unsigned int kilobits, unsigned int cspin) {
	_cspin = cspin;
	_bytes = ((uint32_t) kilobits) * 128;
	_mode  = NONE;
	_pageLen = 0;
	_lastError = OK;
	_spi = spi;
	_settings = SPISettings(1000000,MSBFIRST,SPI_MODE0);
}

SPISRAM::~SPISRAM() {
	pinMode(_cspin,INPUT);
}

void SPISRAM::startOp() {
	_spi->beginTransaction(_settings);
	digitalWrite(_cspin, LOW);
}

void SPISRAM::finishOp() {
	digitalWrite(_cspin, HIGH);
	_spi->endTransaction();
}

bool SPISRAM::sendCommand(SPISRAM::Op cmd, uint32_t addr) {
/*
	if (addr > _bytes) {
		_lastError = BADADDR;
		Serial.println("Error BADADDR - sendCommand");
		char msg[80];
		sprintf(msg,"addr: %08lX, _bytes: %08lX\n", addr, _bytes);
		Serial.print(msg);
		// return false;
	}
*/
	uint8_t a1 = (addr & 0x00ff0000) >> 16;
	uint8_t a2 = (addr & 0x0000ff00) >> 8;
	uint8_t a3 = (addr & 0x000000ff);
	uint8_t cmdB = (uint8_t) cmd & 0xff;
	_spi->transfer(cmdB);
	_spi->transfer(a1);
	_spi->transfer(a2);
	_spi->transfer(a3);
	_lastError = OK;
	return true;
}

void SPISRAM::sendBuffer(unsigned int size, byte *buffer) {
	for (unsigned int i=0; i<size; i++) {
		_spi->transfer(buffer[i]);
	}
}

void SPISRAM::readBuffer(unsigned int size, byte *buffer) {
	for (unsigned int i=0; i<size; i++) {
		byte b = _spi->transfer(0xff);
		buffer[i] = b;
	}
}

bool SPISRAM::begin(Mode mode, unsigned int pageLen) {
	char msg[80];

	sprintf(msg,"Starting SRAM CS: %d, _bytes: %08lX\n", _cspin, _bytes);
	Serial.print(msg);

	bool result = false;
	byte modeReg;
	digitalWrite(_cspin,HIGH);
	pinMode(_cspin, OUTPUT);
	_spi->begin();
	modeReg = mode;
	_mode = mode;
	switch (mode) {
	    case BYTE:
	    	_pageLen = 0;
	    	result = true;
	    	break;
		case PAGE:
			if (pageLen > 32) {
				_lastError = BADPAG;
				return false;
			}
			/* no break */
		case SEQ:
			_pageLen = pageLen;
			result = true;
			break;
		default:
			_lastError = BADOPT;
			return false;
			break;
	}
	_spi->beginTransaction(_settings);
	digitalWrite(_cspin, LOW);
	_spi->transfer(WRMR);
	_spi->transfer(modeReg);
	digitalWrite(_cspin, HIGH);
	_spi->endTransaction();
	return result;
}

bool SPISRAM::setMode(Mode mode, unsigned int pageLen) {
	bool result = false;
	byte modeReg;
	modeReg = mode;
	_mode = mode;
	switch (mode) {
	    case BYTE:
	    	_pageLen = 0;
	    	result = true;
	    	break;
		case PAGE:
			if (pageLen > 32) {
				_lastError = BADPAG;
				return false;
			}
			/* no break */
		case SEQ:
			_pageLen = pageLen;
			result = true;
			break;
		default:
			_lastError = BADOPT;
			return false;
			break;
	}
	_spi->beginTransaction(_settings);
	digitalWrite(_cspin, LOW);
	_spi->transfer(WRMR);
	_spi->transfer(modeReg);
	digitalWrite(_cspin, HIGH);
	_spi->endTransaction();
	return result;
}


bool SPISRAM::write(uint32_t address, unsigned int size, byte *buffer) {
	bool outcome = false;
	switch (_mode) {
		case BYTE:
			for (unsigned int i=0; i<size; i++) {
				startOp();
				if (sendCommand(WRITE,address)) {
					sendBuffer(1, &(buffer[i]));
					address += 1;
					outcome = true;
				}
				finishOp();
			}
			break;
		case PAGE:
			if (size > _pageLen) {
				_lastError = BADSIZ;
				Serial.println("Error BADSIZ - write");
			}
			/* no break */
		case SEQ:
			if (size < 1) {
				_lastError = BADSIZ;
				Serial.println("Error BADSIZ - write");
			} else {
				startOp();
				if (sendCommand(WRITE,address)) {
					sendBuffer(size, buffer);
					outcome = true;
				}
				finishOp();
			}
			break;
		default:
			_lastError = ERR;
			Serial.println("Error ERR - write");
			break;
	}
	return outcome;
}

bool SPISRAM::read(uint32_t address, unsigned int size, byte *buffer) {
	bool outcome = false;
	switch (_mode) {
		case BYTE:
			for (unsigned int i=0; i<size; i++) {
				startOp();
				if (sendCommand(READ,address)) {
					readBuffer(1, &(buffer[i]));
					address += 1;
					outcome = true;
				}
				finishOp();
			}
			break;
		case PAGE:
			if (size > _pageLen) {
				_lastError = BADSIZ;
				Serial.println("Error BADSIZ - read");
				return false;
			}
			/* no break */
		case SEQ:
			if (size<1) {
				_lastError = BADSIZ;
				Serial.println("Error BADSIZ - read");
			} else {
				startOp();
				if (sendCommand(READ,address)) {
					readBuffer(size, buffer);
					outcome = true;
				}
				finishOp();
			}
			break;
		default:
			_lastError = ERR;
			Serial.println("Error ERR - read");
			break;
	}
	return outcome;
}
