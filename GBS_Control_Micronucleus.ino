//#include <avr/pgmspace.h>
#include "I2CBitBanger.h"
#include "StartArray.h"
//#include "ProgramArray240p.h"
//#include "ProgramArray288p50.h"
//#include "ProgramArray480i.h"
#include "ProgramArray576i50.h"
//#include "ProgramArrayTEST.h"
//#include "pal_240p_fullscreen.h"

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

bool writeOneByte(uint8_t slaveRegister, uint8_t value) {
  return writeBytes(slaveRegister, &value, 1);
}

bool writeBytes(uint8_t slaveAddress, uint8_t slaveRegister, uint8_t* values, uint8_t numValues) {
  i2cObj.addByteForTransmission(slaveRegister);
  i2cObj.addBytesForTransmission(values, numValues);

  if (!i2cObj.transmitData()) {
    return false;
  }
  return true;
}


bool writeBytes(uint8_t slaveRegister, uint8_t* values, int numValues) {
  return writeBytes(GBS_I2C_ADDRESS, slaveRegister, values, numValues);
}


bool writeStartArray() {
  for (int z = 0; z < ((sizeof(startArray) / 2) - 1520); z++) {
    writeOneByte(pgm_read_byte(startArray + (z * 2)), pgm_read_byte(startArray + (z * 2) + 1));
    delay(1);
  }
  return true;
}

uint8_t getSingleByteFromPreset(const uint8_t* programArray, unsigned int offset) {
  return pgm_read_byte(programArray + offset);
}

void setParametersSP() {
  writeOneByte(0xF0, 5);
  writeOneByte(0x20, 0x02); // was 0xd2 // keep jitter sync off, 0x02 is right (auto correct sog polarity, sog source = ADC)
  // H active detect control
  writeOneByte(0x21, 0x1b); // SP_SYNC_TGL_THD    H Sync toggle times threshold  0x20
  writeOneByte(0x22, 0x0f); // SP_L_DLT_REG       Sync pulse width different threshold (little than this as equal).
  writeOneByte(0x24, 0x40); // SP_T_DLT_REG       H total width different threshold rgbhv: b // try reducing to 0x0b again
  writeOneByte(0x25, 0x00); // SP_T_DLT_REG
  writeOneByte(0x26, 0x05); // SP_SYNC_PD_THD     H sync pulse width threshold // try increasing to ~ 0x50
  writeOneByte(0x27, 0x00); // SP_SYNC_PD_THD
  writeOneByte(0x2a, 0x0f); // SP_PRD_EQ_THD      How many continue legal line as valid
  // V active detect control
  writeOneByte(0x2d, 0x04); // SP_VSYNC_TGL_THD   V sync toggle times threshold
  writeOneByte(0x2e, 0x04); // SP_SYNC_WIDTH_DTHD V sync pulse width threshod // the 04 is a test
  writeOneByte(0x2f, 0x04); // SP_V_PRD_EQ_THD    How many continue legal v sync as valid  0x04
  writeOneByte(0x31, 0x2f); // SP_VT_DLT_REG      V total different threshold
  // Timer value control
  writeOneByte(0x33, 0x28); // SP_H_TIMER_VAL     H timer value for h detect (hpw 148 typical, need a little slack > 160/4 = 40 (0x28)) (was 0x28)
  writeOneByte(0x34, 0x03); // SP_V_TIMER_VAL     V timer for V detect       (?typical vtotal: 259. times 2 for 518. ntsc 525 - 518 = 7. so 0x08?)

  // Sync separation control
  writeOneByte(0x35, 0xb0); // SP_DLT_REG [7:0]   Sync pulse width difference threshold  (tweak point) (b0 seems best from experiments. above, no difference)
  writeOneByte(0x36, 0x00); // SP_DLT_REG [11:8]
  writeOneByte(0x37, 0x58); // SP_H_PULSE_IGNORE (tweak point) H pulse less than this will be ignored. (MD needs > 0x51) rgbhv: a
  writeOneByte(0x38, 0x07); // h coast pre (psx starts eq pulses around 4 hsyncs before vs pulse) rgbhv: 7
  writeOneByte(0x39, 0x03); // h coast post (psx stops eq pulses around 4 hsyncs after vs pulse) rgbhv: 12
  // note: the pre / post lines number probably depends on the vsync pulse delay, ie: sync stripper vsync delay

  writeOneByte(0x3a, 0x0a); // 0x0a rgbhv: 20
  //writeOneByte(0x3f, 0x03); // 0x03
  //writeOneByte(0x40, 0x0b); // 0x0b

  //writeOneByte(0x3e, 0x00); // problems with snes 239 line mode, use 0x00  0xc0 rgbhv: f0

  // clamp position
  // in RGB mode, should use sync tip clamping: s5s41s80 s5s43s90 s5s42s06 s5s44s06
  // in YUV mode, should use back porch clamping: s5s41s70 s5s43s98 s5s42s00 s5s44s00
  // tip: see clamp pulse in RGB signal with clamp start > clamp end (scope trigger on sync in, show one of the RGB lines)
  writeOneByte(0x41, 0x70); writeOneByte(0x43, 0x98); // newer GBS boards seem to float the inputs more??  0x32 0x45
  writeOneByte(0x44, 0x00); writeOneByte(0x42, 0x00); // 0xc0 0xc0

  // 0x45 to 0x48 set a HS position just for Mode Detect. it's fine at start = 0 and stop = 1 or above
  // Update: This is the retiming module. It can be used for SP processing with t5t57t6
  writeOneByte(0x45, 0x00); // 0x00 // retiming SOG HS start
  writeOneByte(0x46, 0x00); // 0xc0 // retiming SOG HS start
  writeOneByte(0x47, 0x02); // 0x05 // retiming SOG HS stop // align with 1_26 (same value) seems good for phase
  writeOneByte(0x48, 0x00); // 0xc0 // retiming SOG HS stop
  writeOneByte(0x49, 0x04); // 0x04 rgbhv: 20
  writeOneByte(0x4a, 0x00); // 0xc0
  writeOneByte(0x4b, 0x44); // 0x34 rgbhv: 50
  writeOneByte(0x4c, 0x00); // 0xc0

  // h coast start / stop positions
  // try these values and t5t3et2 when using cvid sync / no sync stripper
  // appears start should be around 0x70, stop should be htotal - 0x70
  //writeOneByte(0x4e, 0x00); writeOneByte(0x4d, 0x70); //  | rgbhv: 0 0
  //writeOneByte(0x50, 0x06); // rgbhv: 0 // is 0x06 for comment below
  writeOneByte(0x4f, 0x9a); // rgbhv: 0 // psx with 54mhz osc. > 0xa4 too much, 0xa0 barely ok, > 0x9a!

  writeOneByte(0x51, 0x02); // 0x00 rgbhv: 2
  writeOneByte(0x52, 0x00); // 0xc0
  writeOneByte(0x53, 0x06); // 0x05 rgbhv: 6
  writeOneByte(0x54, 0x00); // 0xc0

  //writeOneByte(0x55, 0x50); // auto coast off (on = d0, was default)  0xc0 rgbhv: 0 but 50 is fine
  //writeOneByte(0x56, 0x0d); // sog mode on, clamp source pixclk, no sync inversion (default was invert h sync?)  0x21 rgbhv: 36
  writeOneByte(0x56, 0x05); // update: one of the new bits causes clamp glitches, check with checkerboard pattern
  //writeOneByte(0x57, 0xc0); // 0xc0 rgbhv: 44 // set to 0x80 for retiming

  writeOneByte(0x58, 0x05); //rgbhv: 0
  writeOneByte(0x59, 0x00); //rgbhv: c0
  writeOneByte(0x5a, 0x05); //rgbhv: 0
  writeOneByte(0x5b, 0x00); //rgbhv: c8
  writeOneByte(0x5c, 0x06); //rgbhv: 0
  writeOneByte(0x5d, 0x00); //rgbhv: 0
}

void writeProgramArrayNew(const uint8_t* programArray) {
  int index = 0;
  uint8_t bank[16];

  // programs all valid registers (the register map has holes in it, so it's not straight forward)
  // 'index' keeps track of the current preset data location.
  writeOneByte(0xF0, 0);
  writeOneByte(0x46, 0x00); // reset controls 1
  writeOneByte(0x47, 0x00); // reset controls 2

  // 498 = s5_12, 499 = s5_13
  writeOneByte(0xF0, 5);
  writeOneByte(0x11, 0x11); // Initial VCO control voltage
  writeOneByte(0x13, getSingleByteFromPreset(programArray, 499)); // load PLLAD divider high bits first (tvp7002 manual)
  writeOneByte(0x12, getSingleByteFromPreset(programArray, 498));
  writeOneByte(0x16, getSingleByteFromPreset(programArray, 502)); // might as well
  writeOneByte(0x17, getSingleByteFromPreset(programArray, 503)); // charge pump current
  writeOneByte(0x18, 0); writeOneByte(0x19, 0); // adc / sp phase reset

  for (int y = 0; y < 6; y++) {
    writeOneByte(0xF0, (uint8_t)y );
    switch (y) {
      case 0:
        for (int j = 0; j <= 1; j++) { // 2 times
          for (int x = 0; x <= 15; x++) {
            // reset controls are at 0x46, 0x47
            if (j == 0 && (x == 6 || x == 7)) {
              // keep reset controls active
              bank[x] = 0;
            } else {
              // use preset values
              bank[x] = pgm_read_byte(programArray + index);
            }
            index++;
          }
          writeBytes(0x40 + (j * 16), bank, 16);
        }
        for (int x = 0; x <= 15; x++) {
          bank[x] = pgm_read_byte(programArray + index);
          index++;
        }
        writeBytes(0x90, bank, 16);
        break;
      case 1:
        for (int j = 0; j <= 8; j++) { // 9 times
          for (int x = 0; x <= 15; x++) {
            bank[x] = pgm_read_byte(programArray + index);
            index++;
          }
          writeBytes(j * 16, bank, 16);
        }
        break;
      case 2:
        for (int j = 0; j <= 3; j++) { // 4 times
          for (int x = 0; x <= 15; x++) {
            bank[x] = pgm_read_byte(programArray + index);
            index++;
          }
          writeBytes(j * 16, bank, 16);
        }
        break;
      case 3:
        for (int j = 0; j <= 7; j++) { // 8 times
          for (int x = 0; x <= 15; x++) {
            bank[x] = pgm_read_byte(programArray + index);
            index++;
          }
          writeBytes(j * 16, bank, 16);
        }
        break;
      case 4:
        for (int j = 0; j <= 5; j++) { // 6 times
          for (int x = 0; x <= 15; x++) {
            bank[x] = pgm_read_byte(programArray + index);
            index++;
          }
          writeBytes(j * 16, bank, 16);
        }
        break;
      case 5:
        for (int j = 0; j <= 6; j++) { // 7 times
          for (int x = 0; x <= 15; x++) {
            bank[x] = pgm_read_byte(programArray + index);
            if (index == 482) { // s5_02 bit 6+7 = input selector (only bit 6 is relevant)
              /*if (rto->inputIsYpBpR)bitClear(bank[x], 6);
              else*/ bitSet(bank[x], 6);
            }
            index++;
          }
          writeBytes(j * 16, bank, 16);
        }
        break;
    }
  }

  setParametersSP();

  writeOneByte(0xF0, 1);
  writeOneByte(0x60, 0x81); // MD H unlock / lock
  writeOneByte(0x61, 0x81); // MD V unlock / lock
  writeOneByte(0x80, 0xa9); // MD V nonsensical custom mode
  writeOneByte(0x81, 0x2e); // MD H nonsensical custom mode
  writeOneByte(0x82, 0x35); // MD H / V timer detect enable, auto detect enable
  writeOneByte(0x83, 0x10); // MD H / V unstable estimation lock value medium

  writeOneByte(0xF0, 0);
  writeOneByte(0x46, 0x3f); // reset controls 1 // everything on except VDS display output
  writeOneByte(0x47, 0x17); // all on except HD bypass
}

bool writeProgramArray(const uint8_t* programArray) {
  for (int y = 0; y < 6; y++) {
    writeOneByte(0xF0, (uint8_t)y );
    delay(1);

    for (int z = 0; z < 15; z++) {
      uint8_t bank[16];
      for (int w = 0; w < 16; w++) {
        bank[w] = pgm_read_byte(programArray + (y * 256 + z * 16 + w));
      }
      writeBytes(z * 16, bank, 16);
      delay(1);
    }
  }
  return true;
}


int readFromRegister(uint8_t segment, uint8_t reg, int bytesToRead, uint8_t* output) {
  if (!writeOneByte(0xF0, segment)) {
    return 0;
  }
  return readFromRegister(reg, bytesToRead, output);
}

int readFromRegister(uint8_t reg, int bytesToRead, uint8_t* output) {
  i2cObj.addByteForTransmission(reg);
  i2cObj.transmitData();
  return i2cObj.recvData(bytesToRead, output);
}


// dumps the current chip configuration in a format that's ready to use as new preset :)
void dumpRegisters(byte segment) {
  uint8_t readout = 0;
  if (segment > 5) return;
  writeOneByte(0xF0, segment);

  switch (segment) {
    case 0:
      for (int x = 0x40; x <= 0x5F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      for (int x = 0x90; x <= 0x9F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
    case 1:
      for (int x = 0x0; x <= 0x8F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
    case 2:
      for (int x = 0x0; x <= 0x3F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
    case 3:
      for (int x = 0x0; x <= 0x7F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
    case 4:
      for (int x = 0x0; x <= 0x5F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
    case 5:
      for (int x = 0x0; x <= 0x6F; x++) {
        readFromRegister(x, 1, &readout);
        Serial.print(readout); Serial.println(",");
      }
      break;
  }
}

void dumpRegisterPreset(byte segment) {
for (int segment = 0; segment <= 5; segment++) {
    dumpRegisters(segment);
  }
  Serial.println("};");
}


void shiftHorizontal(uint16_t amountToAdd, bool subtracting) {
  uint8_t hrstLow = 0x00;
  uint8_t hrstHigh = 0x00;
  uint16_t hrstValue = 0x0000;
  uint8_t hbstLow = 0x00;
  uint8_t hbstHigh = 0x00;
  uint16_t hbstValue = 0x0000;
  uint8_t hbspLow = 0x00;
  uint8_t hbspHigh = 0x00;
  uint16_t hbspValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, HRST_LOW, 1, &hrstLow) != 1) {
    return;
  }

  if (readFromRegister(HRST_HIGH, 1, &hrstHigh) != 1) {
    return;
  }

  hrstValue = ( ( ((uint16_t)hrstHigh) & 0x0007) << 8) | (uint16_t)hrstLow;

  if (readFromRegister(HBST_LOW, 1, &hbstLow) != 1) {
    return;
  }

  if (readFromRegister(HBST_HIGH, 1, &hbstHigh) != 1) {
    return;
  }

  hbstValue = ( ( ((uint16_t)hbstHigh) & 0x000f) << 8) | (uint16_t)hbstLow;

  hbspLow = hbstHigh;

  if (readFromRegister(HBSP_HIGH, 1, &hbspHigh) != 1) {
    return;
  }

  hbspValue = ( ( ((uint16_t)hbspHigh) & 0x00ff) << 4) | ( (((uint16_t)hbspLow) & 0x00f0) >> 4);

  if (subtracting){
    hbstValue -= amountToAdd;
    hbspValue -= amountToAdd;
  } else {
    hbstValue += amountToAdd;
    hbspValue += amountToAdd;
  }

  // handle the case where hbst or hbsp have been decremented below 0
  if (hbstValue & 0x8000) {
    hbstValue = hrstValue - 1;
  }

  if (hbspValue & 0x8000) {
    hbspValue = hrstValue - 1;
  }

  writeOneByte(HBST_LOW, (uint8_t)(hbstValue & 0x00ff));
  writeOneByte(HBST_HIGH, ((uint8_t)(hbspValue & 0x000f) << 4) | ((uint8_t)((hbstValue & 0x0f00) >> 8)) );
  writeOneByte(HBSP_HIGH, (uint8_t)((hbspValue & 0x0ff0) >> 4) );
}


void shiftHorizontalLeft() {
  shiftHorizontal(4, true);
}

void shiftHorizontalRight() {
  shiftHorizontal(4, false);
}

void scaleHorizontal(uint16_t amountToAdd, bool subtracting) {
  uint8_t high = 0x00;
  uint8_t newHigh = 0x00;
  uint8_t low = 0x00;
  uint8_t newLow = 0x00;
  uint16_t newValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, HSCALE_LOW, 1, &low) != 1) {
    return;
  }

  if (readFromRegister(HSCALE_HIGH, 1, &high) != 1) {
    return;
  }

  newValue = ( ( ((uint16_t)high) & 0x0003) * 256) + (uint16_t)low;

  if (subtracting) {
    newValue -= amountToAdd;
  } else {
    newValue += amountToAdd;
  }

  newHigh = (high & 0xfc) | (uint8_t)( (newValue / 256) & 0x0003);
  newLow = (uint8_t)(newValue & 0x00ff);

  writeOneByte(HSCALE_LOW, newLow);
  writeOneByte(HSCALE_HIGH, newHigh);
}

void scaleHorizontalSmaller() {
  scaleHorizontal(4, false);
}

void scaleHorizontalLarger() {
  scaleHorizontal(4, true);
}


void shiftVertical(uint16_t amountToAdd, bool subtracting) {
  uint8_t vrstLow = 0x00;
  uint8_t vrstHigh = 0x00;
  uint16_t vrstValue = 0x0000;
  uint8_t vbstLow = 0x00;
  uint8_t vbstHigh = 0x00;
  uint16_t vbstValue = 0x0000;
  uint8_t vbspLow = 0x00;
  uint8_t vbspHigh = 0x00;
  uint16_t vbspValue = 0x0000;

  if (readFromRegister(SCALING_SEGMENT, VRST_LOW, 1, &vrstLow) != 1) {
    return;
  }

  if (readFromRegister(VRST_HIGH, 1, &vrstHigh) != 1) {
    return;
  }

  vrstValue = ( (((uint16_t)vrstHigh) & 0x007f) << 4) | ( (((uint16_t)vrstLow) & 0x00f0) >> 4);

  if (readFromRegister(VBST_LOW, 1, &vbstLow) != 1) {
    return;
  }

  if (readFromRegister(VBST_HIGH, 1, &vbstHigh) != 1) {
    return;
  }

  vbstValue = ( ( ((uint16_t)vbstHigh) & 0x0007) << 8) | (uint16_t)vbstLow;

  vbspLow = vbstHigh;

  if (readFromRegister(VBSP_HIGH, 1, &vbspHigh) != 1) {
    return;
  }

  vbspValue = ( ( ((uint16_t)vbspHigh) & 0x007f) << 4) | ( (((uint16_t)vbspLow) & 0x00f0) >> 4);

  if (subtracting) {
    vbstValue -= amountToAdd;
    vbspValue -= amountToAdd;
  } else {
    vbstValue += amountToAdd;
    vbspValue += amountToAdd;
  }

  // handle the case where hbst or hbsp have been decremented below 0
  if (vbstValue & 0x8000) {
    vbstValue = vrstValue - 1;
  }

  if (vbspValue & 0x8000) {
    vbspValue = vrstValue - 1;
  }

  writeOneByte(VBST_LOW, (uint8_t)(vbstValue & 0x00ff));
  writeOneByte(VBST_HIGH, ((uint8_t)(vbspValue & 0x000f) << 4) | ((uint8_t)((vbstValue & 0x0700) >> 8)) );
  writeOneByte(VBSP_HIGH, (uint8_t)((vbspValue & 0x07f0) >> 4) );
}


void shiftVerticalUp() {
  shiftVertical(4, true);
}

void shiftVerticalDown() {
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


void scaleVerticalSmaller() {
  scaleVertical(4, false);
}

void scaleVerticalLarger() {
  scaleVertical(4, true);
}

void setup() {
  writeStartArray();
  writeProgramArray(programArray576i50);
  //writeProgramArrayNew(pal_240p);
}

void loop() {
  
}
