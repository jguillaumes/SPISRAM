/*
 * SPISRAM.cpp
 *
 * Serial SRAM device support for AVR attmega MCUs
 * Requires the Arduino libraries.
 *
 * See LICENSE.md and README.md for details
 *
 *  Created on: Feb 16, 2018
 *      Author: jguillaumes
 */

#include "SPISRAM.h"

#ifdef DEBUG
void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
       Serial.print("0x");
       for (int i=0; i<length; i++) {
         if (data[i]<0x10) {Serial.print("0");}
         Serial.print(data[i],HEX);
         Serial.print(" ");
       }
}
#endif

/*
 * Constructor.
 * spi: reference to the SPI interface to be used. Usually you'll specify just &SPI here
 * kilobits: capacity of the device. 1024 for the 23LCV1024
 * cspin: pin to be used as CS. Use Arduino numbering. The default is 10.
 */
SPISRAM::SPISRAM(SPIClass * spi, unsigned int kilobits, unsigned int cspin) {
	_cspin = cspin;
	_bytes = ((uint32_t) kilobits) * 128;
	_mode  = NONE;
	_pageLen = 0;
	_lastError = OK;
	_spi = spi;
	_settings = SPISettings(1000000,MSBFIRST,SPI_MODE0);
}

/*
 * Destructor. Returns the CS PIN to input state.
 */
SPISRAM::~SPISRAM() {
	pinMode(_cspin,INPUT);
}

/*
 * Private method to start up a SPI transaction and mark the device as active
 * The CS signal is active-low.
 */
void SPISRAM::startOp() {
	_spi->beginTransaction(_settings);
	digitalWrite(_cspin, LOW);
}

/*
 * Private method to end the SPI transaction and mark the device as inactive
 * The CS signal is unactive high.
 */
void SPISRAM::finishOp() {
	digitalWrite(_cspin, HIGH);
	_spi->endTransaction();
}

/*
 * Private method: sends a byte command followed by the three bytes (24 bits) of an address.
 * Please notice this method will not work with smalled devices which use 16 bit memory addresses.
 */
bool SPISRAM::sendCommand(SPISRAM::Op cmd, uint32_t addr) {
	if (addr > _bytes) {
#ifdef DEBUG
		Serial.println("Error BADADDR - sendCommand");
		char msg[80];
		sprintf(msg,"addr: %08lX, _bytes: %08lX\n", addr, _bytes);
		Serial.print(msg);
#endif
		_lastError = BADADDR;
		return false;
	}
	// Extract the three address bytes
	uint8_t a1 = (addr & 0x00ff0000) >> 16;
	uint8_t a2 = (addr & 0x0000ff00) >> 8;
	uint8_t a3 = (addr & 0x000000ff);
	uint8_t cmdB = (uint8_t) cmd & 0xff;
	// Send the command followed by the address, MSB first
	_spi->transfer(cmdB);
	_spi->transfer(a1);
	_spi->transfer(a2);
	_spi->transfer(a3);
	_lastError = OK;
	return true;
}

/*
 * Private method: send an array of bytes
 */
void SPISRAM::sendBuffer(unsigned int size, byte *buffer) {
	for (unsigned int i=0; i<size; i++) {
		_spi->transfer(buffer[i]);
	}
}

/*
 * Private method: read an array of bytes
 */
void SPISRAM::readBuffer(unsigned int size, byte *buffer) {
	for (unsigned int i=0; i<size; i++) {
		// SPI requires to send something to read something. It will be ignored by the device
		byte b = _spi->transfer(0xff);
		buffer[i] = b;
	}
}

/*
 * Set up the device and the CS pin.
 * It also sets the initial mode
 */
bool SPISRAM::begin(Mode mode, unsigned int pageLen) {

#ifdef DEBUG
	char msg[80];
	sprintf(msg,"Starting SRAM CS: %d, _bytes: %08lX\n", _cspin, _bytes);
	Serial.print(msg);
#endif

	bool result = false;
	byte modeReg;

	// Set up the CS PIN and set the device to disabled (CS is active low)
	digitalWrite(_cspin,HIGH);
	pinMode(_cspin, OUTPUT);
	// Set up SPI
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
			_pageLen = 0;
			result = true;
			break;
		default:
			_lastError = BADOPT;
			return false;
			break;
	}
	// Set up the initial mode
	_spi->beginTransaction(_settings);
	digitalWrite(_cspin, LOW);
	_spi->transfer(WRMR);
	_spi->transfer(modeReg);
	digitalWrite(_cspin, HIGH);
	_spi->endTransaction();
	return result;
}

// Set the access mode. It's the same as begin() without the SPI initialization
// TO-DO: Refactor this method and begin()
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

/*
 * Write an array of bytes into the device
 */
bool SPISRAM::write(uint32_t address, unsigned int size, byte *buffer) {
	bool outcome = false;
	switch (_mode) {
		case BYTE:
			// In BYTE mode we need to send the bytes one by one
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
			// In PAGE mode the size must be less than the page size
			if (size > _pageLen) {
				_lastError = BADSIZ;
#ifdef DEBUG
				Serial.println("Error BADSIZ - write");
#endif
			}
			/* no break */
		case SEQ:
			// In SEQ mode we just check the size is not negative
			if (size < 1) {
				_lastError = BADSIZ;
#ifdef DEBUG
				Serial.println("Error BADSIZ - write");
#endif
			} else {
				// In SEQ and PAGE modes we can send multiple bytes in just one transaction
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
#ifdef DEBUG
			Serial.println("Error ERR - write");
#endif
			break;
	}
	return outcome;
}

/*
 * Read an array of bytes from the device
 * See the write() comments for details, since both methods are basically equal
 */
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
#ifdef DEBUG
				Serial.println("Error BADSIZ - read");
#endif
				return false;
			}
			/* no break */
		case SEQ:
			if (size<1) {
				_lastError = BADSIZ;
#ifdef DEBUG
				Serial.println("Error BADSIZ - read");
#endif
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
#ifdef DEBUG
			Serial.println("Error ERR - read");
#endif
			break;
	}
	return outcome;
}
