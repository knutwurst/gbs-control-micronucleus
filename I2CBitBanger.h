/*
  mybook4's I2C bit banging library for the Digispark Pro
  Only supports writing at the moment
  Developed specifically to write register values to the GBS 8220
  Timing is based on the timing observed with the GBS 8220's onboard microcontroller

  This is (somewhat) based off of the TinyWireM and USI_TWI_Master interface
*/

#define F_CPU 8000000UL

#ifndef I2CBitBanger_h
#define I2CBitBanger_h

#include <avr/io.h>
#include <inttypes.h>

#define I2CBB_RW_BIT_POSITION 0x01
#define I2CBB_BUF_SIZE    33             // bytes in message buffer (holds a slave address and 32 bytes

#define DDR_I2CBB             DDRB
#define PORT_I2CBB            PORTB
#define PIN_I2CBB             PINB
#define SDA_BIT        0
#define SCL_BIT        2

#define I2C_ACK 0
#define I2C_NACK 1

class I2CBitBanger
{  
  public:
    I2CBitBanger(uint8_t sevenBitAddressArg);
    
    void setSlaveAddress(uint8_t sevenBitAddressArg); // 7-bit address
    
    // Adds a byte to an internally managed transmission buffer (preping for a write)
    void addByteForTransmission(uint8_t data);
    
    // Adds a buffer to an internally managed transmission buffer (preping for a write)
    void addBytesForTransmission(uint8_t* buffer, uint8_t bufferSize);
    
    bool transmitData();  // returns true if transmission was successful (everything ACKed by the slave device)
    int recvData(int numBytesToRead, uint8_t* outputBuffer);  // returns the number of bytes that were read
    

  private:
    static uint8_t I2CBB_Buffer[];           // holds I2C send data
    static uint8_t I2CBB_BufferIndex;        // current number of bytes in the send buff

    void initializePins();
    
    bool sendDataOverI2c(uint8_t* buffer, uint8_t bufferSize);
    void sendI2cStartSignal();
    bool sendI2cByte(uint8_t dataByte);
    void sendI2cStopSignal();
    
    void receiveI2cByte(bool sendAcknowledge, uint8_t* output);
};

#endif


