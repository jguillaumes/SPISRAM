/*
 * SPISRAM.h
 *
 * Serial SRAM device support for AVR attmega MCUs
 * Requires the Arduino libraries.
 *
 * See LICENSE.md and README.md for details
 *
 *  Created on: Feb 16, 2018
 *      Author: jguillaumes
 */

#ifndef SPISRAM_H_
#define SPISRAM_H_

#include <SPI.h>

#define SPISRAM_VERSION "01.00.00"

class SPISRAM {
public:
	enum Error { OK=0, BADSIZ, BADPAG, BADADDR, BADOPT, ERR };
	enum Mode {
		BYTE = 0b00000000,
		PAGE = 0b10000000,
		SEQ  = 0b01000000,
		NONE = 0b11000000
	};
	SPISRAM(SPIClass * spi, unsigned int kilobits=1024, unsigned int cspin=10);
	virtual ~SPISRAM();
	bool begin(Mode mode=SEQ, unsigned int pageLen=32);
	bool setMode(Mode mode=SEQ, unsigned int pageLen=32);
	bool write(uint32_t address, unsigned int size, byte *buffer);
	bool read(uint32_t address, unsigned int size, byte *buffer);
	byte readByte(unsigned int address);
	Error getLastError() {
		return _lastError;
	}


private:
	enum Op {
		READ  = 0b00000011,
		WRITE = 0b00000010,
		EDIO  = 0b00111011,
		RSTIO = 0b11111111,
		RMDR  = 0b00000101,
		WRMR  = 0b00000001
	};
	uint32_t  _bytes;
	unsigned int _cspin;
	unsigned int _pageLen;
	Mode _mode;
	Error _lastError;
	SPISettings _settings;
	SPIClass * _spi;
	void startOp();
	void finishOp();
	bool sendCommand(Op cmd, uint32_t addr);
	void sendBuffer(unsigned int size, byte *buffer);
	void readBuffer(unsigned int size, byte *buffer);
	void endCommand();
};

#endif /* SPISRAM_H_ */
