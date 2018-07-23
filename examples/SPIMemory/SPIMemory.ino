#include "Arduino.h"

#include "SPISRAM.h"

SPISRAM ram = SPISRAM(&SPI, 1024, 10);
SPISRAM ram2= SPISRAM(&SPI, 1024,  9);

void setup() {
	Serial.begin(9600);
	Serial.println("Init SPIMemory");
	ram.begin();
	ram2.begin();
}

// The loop function is called in an endless loop
void loop()
{
	uint32_t addr = 128;
	byte buffer[32];
	char line[80];

	ram.setMode(SPISRAM::BYTE, 0);

	memset(buffer,32,' ');
	ram.read(addr, 32, buffer);
	sprintf(line, "1-ram: [%s]\n", (char*) buffer);
	Serial.print(line);

	delay(1000);
	memset(buffer,32,' ');
	sprintf((char*)buffer,"%s", "01234567890123456789");
	ram.write(addr,32,buffer);

	ram.setMode(SPISRAM::SEQ, 0);

	delay(1000);
	memset(buffer,32,' ');
	ram.read(addr,32,buffer);
	sprintf(line, "2-ram: [%s]\n", (char*) buffer);
	Serial.print(line);

	delay(1000);
	memset(buffer,32,' ');
	sprintf((char*)buffer,"%s", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	ram.write(addr,32,buffer);

	ram2.setMode(SPISRAM::BYTE, 0);

	delay(1000);
	memset(buffer,32,' ');
	ram2.read(addr, 32, buffer);
	sprintf(line, "3-ram2: [%s]\n", (char*) buffer);
	Serial.print(line);

	delay(1000);
	memset(buffer,32,' ');
	sprintf((char*)buffer,"%s", "98765432109876543210");
	ram2.write(addr,32,buffer);

	ram2.setMode(SPISRAM::SEQ, 0);

	delay(1000);
	memset(buffer,32,' ');
	ram2.read(addr,32,buffer);
	sprintf(line, "4-ram2: [%s]\n", (char*) buffer);
	Serial.print(line);

	delay(1000);
	memset(buffer,32,' ');
	sprintf((char*)buffer, "%s", "ZUXWVUSRQPONMLKJIHGFEDCBA");
	ram2.write(addr,32,buffer);

	delay(1000);
}
