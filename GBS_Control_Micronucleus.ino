//#include <avr/pgmspace.h>
#include "I2CBitBanger.h"
#include "StartArray.h"
//#include "ProgramArray240p.h"
//#include "ProgramArray288p50.h"
//#include "ProgramArray480i.h"
#include "ProgramArray576i50.h"
//#include "ProgramArrayTEST.h"

#define GBS_I2C_ADDRESS 0x17 // 0x2E
I2CBitBanger i2cObj(GBS_I2C_ADDRESS);

#define VALID_BANK 0x7e
#define SCALING_SEGMENT 0x03
#define HRST_LOW  0x01
#define HRST_HIGH 0x02
#define HBST_LOW  0x04
#define HBST_HIGH 0x05
#define HBSP_LOW  0x05
#define HBSP_HIGH 0x06
#define HSCALE_LOW  0x16
#define HSCALE_HIGH 0x17
#define VRST_LOW  0x02
#define VRST_HIGH 0x03
#define VBST_LOW  0x07
#define VBST_HIGH 0x08
#define VBSP_LOW  0x08
#define VBSP_HIGH 0x09

#define LED_BIT 1

bool writeOneByte(uint8_t slaveRegister, uint8_t value)
{
  return writeBytes(slaveRegister, &value, 1);
}

bool writeBytes(uint8_t slaveAddress, uint8_t slaveRegister, uint8_t* values, uint8_t numValues)
{
  i2cObj.addByteForTransmission(slaveRegister);
  i2cObj.addBytesForTransmission(values, numValues);

  if (!i2cObj.transmitData())
  {
    return false;
  }

  return true;
}


bool writeBytes(uint8_t slaveRegister, uint8_t* values, int numValues)
{
  return writeBytes(GBS_I2C_ADDRESS, slaveRegister, values, numValues);
}


bool writeStartArray()
{
  for (int z = 0; z < ((sizeof(startArray) / 2) - 1520); z++)
  {
    writeOneByte(pgm_read_byte(startArray + (z * 2)), pgm_read_byte(startArray + (z * 2) + 1));
    delay(1);
  }
  return true;
}


bool writeProgramArray(const uint8_t* programArray)
{
  for (int y = 0; y < 6; y++)
  {
    writeOneByte(0xF0, (uint8_t)y );
    delay(1);

    for (int z = 0; z < 15; z++)
    {
      uint8_t bank[16];
      for (int w = 0; w < 16; w++)
      {
        bank[w] = pgm_read_byte(programArray + (y * 256 + z * 16 + w));
      }
      writeBytes(z * 16, bank, 16);
      delay(1);
    }
  }
  return true;
}


int readFromRegister(uint8_t segment, uint8_t reg, int bytesToRead, uint8_t* output)
{
  if (!writeOneByte(0xF0, segment))
  {
    return 0;
  }
  return readFromRegister(reg, bytesToRead, output);
}

int readFromRegister(uint8_t reg, int bytesToRead, uint8_t* output)
{
  i2cObj.addByteForTransmission(reg);
  i2cObj.transmitData();
  return i2cObj.recvData(bytesToRead, output);
}

// This variable holds the last known state of the Resolution switch
// true = 240p, false = 480i
//bool lastKnownResolutionSwitchState = true;

void returnToDefaultSettings()
{
  writeProgramArray(programArray576i50);
}

void shiftHorizontal(uint16_t amountToAdd, bool subtracting)
{
  uint8_t hrstLow = 0x00;
  uint8_t hrstHigh = 0x00;
  uint16_t hrstValue = 0x0000;
  uint8_t hbstLow = 0x00;
  uint8_t hbstHigh = 0x00;
  uint16_t hbstValue = 0x0000;
  uint8_t hbspLow = 0x00;
  uint8_t hbspHigh = 0x00;
  uint16_t hbspValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, HRST_LOW, 1, &hrstLow) != 1)
  {
    return;
  }

  if (readFromRegister(HRST_HIGH, 1, &hrstHigh) != 1)
  {
    return;
  }

  hrstValue = ( ( ((uint16_t)hrstHigh) & 0x0007) << 8) | (uint16_t)hrstLow;

  if (readFromRegister(HBST_LOW, 1, &hbstLow) != 1)
  {
    return;
  }

  if (readFromRegister(HBST_HIGH, 1, &hbstHigh) != 1)
  {
    return;
  }

  hbstValue = ( ( ((uint16_t)hbstHigh) & 0x000f) << 8) | (uint16_t)hbstLow;

  hbspLow = hbstHigh;

  if (readFromRegister(HBSP_HIGH, 1, &hbspHigh) != 1)
  {
    return;
  }

  hbspValue = ( ( ((uint16_t)hbspHigh) & 0x00ff) << 4) | ( (((uint16_t)hbspLow) & 0x00f0) >> 4);

  if (subtracting)
  {
    hbstValue -= amountToAdd;
    hbspValue -= amountToAdd;
  } else {
    hbstValue += amountToAdd;
    hbspValue += amountToAdd;
  }

  // handle the case where hbst or hbsp have been decremented below 0
  if (hbstValue & 0x8000)
  {
    hbstValue = hrstValue - 1;
  }

  if (hbspValue & 0x8000)
  {
    hbspValue = hrstValue - 1;
  }

  writeOneByte(HBST_LOW, (uint8_t)(hbstValue & 0x00ff));
  writeOneByte(HBST_HIGH, ((uint8_t)(hbspValue & 0x000f) << 4) | ((uint8_t)((hbstValue & 0x0f00) >> 8)) );
  writeOneByte(HBSP_HIGH, (uint8_t)((hbspValue & 0x0ff0) >> 4) );
}


void shiftHorizontalLeft()
{
  shiftHorizontal(4, true);
}

void shiftHorizontalRight()
{
  shiftHorizontal(4, false);
}

void scaleHorizontal(uint16_t amountToAdd, bool subtracting)
{
  uint8_t high = 0x00;
  uint8_t newHigh = 0x00;
  uint8_t low = 0x00;
  uint8_t newLow = 0x00;
  uint16_t newValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, HSCALE_LOW, 1, &low) != 1)
  {
    return;
  }

  if (readFromRegister(HSCALE_HIGH, 1, &high) != 1)
  {
    return;
  }

  newValue = ( ( ((uint16_t)high) & 0x0003) * 256) + (uint16_t)low;

  if (subtracting)
  {
    newValue -= amountToAdd;
  } else {
    newValue += amountToAdd;
  }

  newHigh = (high & 0xfc) | (uint8_t)( (newValue / 256) & 0x0003);
  newLow = (uint8_t)(newValue & 0x00ff);

  writeOneByte(HSCALE_LOW, newLow);
  writeOneByte(HSCALE_HIGH, newHigh);
}

void scaleHorizontalSmaller()
{
  scaleHorizontal(4, false);
}

void scaleHorizontalLarger()
{
  scaleHorizontal(4, true);
}


void shiftVertical(uint16_t amountToAdd, bool subtracting)
{
  uint8_t vrstLow = 0x00;
  uint8_t vrstHigh = 0x00;
  uint16_t vrstValue = 0x0000;
  uint8_t vbstLow = 0x00;
  uint8_t vbstHigh = 0x00;
  uint16_t vbstValue = 0x0000;
  uint8_t vbspLow = 0x00;
  uint8_t vbspHigh = 0x00;
  uint16_t vbspValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, VRST_LOW, 1, &vrstLow) != 1)
  {
    return;
  }

  if (readFromRegister(VRST_HIGH, 1, &vrstHigh) != 1)
  {
    return;
  }

  vrstValue = ( (((uint16_t)vrstHigh) & 0x007f) << 4) | ( (((uint16_t)vrstLow) & 0x00f0) >> 4);

  if (readFromRegister(VBST_LOW, 1, &vbstLow) != 1)
  {
    return;
  }

  if (readFromRegister(VBST_HIGH, 1, &vbstHigh) != 1)
  {
    return;
  }

  vbstValue = ( ( ((uint16_t)vbstHigh) & 0x0007) << 8) | (uint16_t)vbstLow;

  vbspLow = vbstHigh;

  if (readFromRegister(VBSP_HIGH, 1, &vbspHigh) != 1)
  {
    return;
  }

  vbspValue = ( ( ((uint16_t)vbspHigh) & 0x007f) << 4) | ( (((uint16_t)vbspLow) & 0x00f0) >> 4);

  if (subtracting)
  {
    vbstValue -= amountToAdd;
    vbspValue -= amountToAdd;
  } else {
    vbstValue += amountToAdd;
    vbspValue += amountToAdd;
  }

  // handle the case where hbst or hbsp have been decremented below 0
  if (vbstValue & 0x8000)
  {
    vbstValue = vrstValue - 1;
  }

  if (vbspValue & 0x8000)
  {
    vbspValue = vrstValue - 1;
  }

  writeOneByte(VBST_LOW, (uint8_t)(vbstValue & 0x00ff));
  writeOneByte(VBST_HIGH, ((uint8_t)(vbspValue & 0x000f) << 4) | ((uint8_t)((vbstValue & 0x0700) >> 8)) );
  writeOneByte(VBSP_HIGH, (uint8_t)((vbspValue & 0x07f0) >> 4) );
}


void shiftVerticalUp()
{
  shiftVertical(4, true);
}

void shiftVerticalDown()
{
  shiftVertical(4, false);
}


void scaleVertical(uint16_t amountToAdd, bool subtracting) {
  uint8_t high = 0x00;
  uint8_t newHigh = 0x00;
  uint8_t low = 0x00;
  uint8_t newLow = 0x00;
  uint16_t newValue = 0x0000;

  readFromRegister(0x03, 0x18, 1, &high);
  readFromRegister(0x03, 0x17, 1, &low);
  newValue = ( (((uint16_t)high) & 0x007f) << 4) | ( (((uint16_t)low) & 0x00f0) >> 4);

  if (subtracting && ((newValue - amountToAdd) >= 0)) {
    newValue -= amountToAdd;
  } else if ((newValue + amountToAdd) <= 1023) {
    newValue += amountToAdd;
  }

  newHigh = (uint8_t)(newValue >> 4);
  newLow = (low & 0x0f) | (((uint8_t)(newValue & 0x00ff)) << 4) ;

  writeOneByte(0x17, newLow);
  writeOneByte(0x18, newHigh);
}


void scaleVerticalSmaller()
{
  scaleVertical(4, false);
}

void scaleVerticalLarger()
{
  scaleVertical(4, true);
}

void setup()
{
  //pinMode(LED_BIT, OUTPUT);
  writeStartArray();
  writeProgramArray(programArray576i50);
  //digitalWrite(LED_BIT, HIGH);
}

void loop()
{

}
