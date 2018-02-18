# SPISRAM - Basic support for the 23LCV1024 chip
This library offers basic support to read and write data to a 23LCV1024 SRAM chip. 

## Summary

The 23LCV1024 chip is a serial SRAM memory device, which can store up to 1 megabit (128 kilobytes) of data. This device can be connected using a standard SPI interface so it requires 4 digital pins (apart from the power ones):

- MOSI
- MISO
- SCK
- CS

The atmega32x chips contain a hardware implementarion of the SPI protocol. Please check your Arduino documentation to see which Arduino Pins correspond to MOSI, MISO and SCK. You can choose whatever available pin to act as CS; the recommended one is PIN10, and it is also the default.

You can download the datasheet [here](https:ww1.microchip.com/downloads/en/DeviceDoc/25156A.pdf).

This library provides just basic support to initialize, read and write data from and to the device. You can use any of the three transfer modes available:

- **BYTE** mode, which allows the transfer of a byte per SPI transaction.
- **PAGE** mode, which allows the transfer of up to a page (32 bytes) per SPI transaction.
- **SEQUENTIAL** mode, which allows the transfer of arbitrary ammounts of bytes.

The defailt mode is SEQUENTIAL. Please read the datasheet to know more about the differences between PAGE and SEQUENTIAL mode. A summary could be in PAGE mode the address wrap up at page level, while in SEQUENTIAL mode it wraps at device level.


## Requirements

It uses the Arduino SPI library. 

## Reference

The library implements the class SPISRAM, which we will now describe.

### Enumerations

The class SPISRAM defines two public enumerations accessible to programmers.

#### Error

This enumeration covers the error codes found at runtime, available using the method ```getLastError()```. There is no message text available.

- OK: No error.
- BADSIZ: The size specified is invalid. In PAGE mode it's bigger than 32 bytes. In any mode, it is zero or negative.
- BADPAG: The page size specified is invalid. It is bigger than 32 or zero or negative.
- BADADDR: The address specified is outside the valid address range.
- BADOPT: Bad option specified.
- ERR: Unknown or unexpected error.

#### Mode

This enumeration covers the three access modes supported by the device:

- BYTE: Byte access mode. 
- PAGE: Page access mode.
- SEQ: Sequential access mode.
- NONE: Not set or wrong access mode. 

### Constructor
```
SPISRAM(SPIClass * spi, unsigned int kilobits=1024, unsigned int cspin=10);
```
- spi: the SPI device to use. The atmega328 has just one (SPI), other MCUs can have more. 
- kilobits: the capacity of the device. The 23LCV1024 has 1024 kilobits. Other similar devices have less capacity. At this time this library just supports the 23LCV1024, so put always 1024 here.
- cspin: the PIN you are going to use as chip-select for the 23LCV1024. Use Arduino IDE numbering.

Example:

```
SPISRAM ram(&SPI, 1024, 10);
```

### Management methods

#### begin()

```
bool begin(Mode mode=SEQ, unsigned int pageLen=32);
```

This method sets up the MCU to use the device.

- mode: Access mode. Can be SPISRAM::BYTE, SPISRAM::PAGE or SPISRAM::SEQ. The default is SEQ for sequential access mode. 
- pageLen: Page size for PAGE mode. This parameter is ignored if mode is not PAGE

Returns **False** if there has been any error. Use ```getLastError()``` to find the reason.


### setMode()

```
bool setMode(Mode mode=SEQ, unsigned int pageLen=32);
```

This method allows to change the access mode for the device. The parameters and return is the same as for the ```begin()``` method.

### getLastError

```
Error getLastError();
```

Returns the last error detected by the library, as a member of the ```Error``` enumeration.


### Data transfer methods

The library defines just two methods for data transfer, which handle arrays of bytes (```uint8_t```).

#### read()

```
bool read(uint32_t address, unsigned int size, byte *buffer);
```

This method reads up to ```size``` bytes from the device at address ```address``` into the byte array ```buffer```.

- address: memory address (insde the device). It must not be outside the device range. In the case of the 23LCV1024 that means its maximum is 0x00020000.
- size: number of bytes to transfer. If in PAGE mode it must be less than the declared page size.
- buffer: destination address (inside the MCU) of the read data.


#### write()

```
bool write(uint32_t address, unsigned int size, byte *buffer);
```

This method writes up to ```size``` bytes to the device at address ```address``` from the byte array ```buffer```.

- address: memory address (insde the device). It must not be outside the device range. In the case of the 23LCV1024 that means its maximum is 0x00020000.
- size: number of bytes to transfer. If in PAGE mode it must be less than the declared page size.
- buffer: source address (inside the MCU) of the data to be written.

