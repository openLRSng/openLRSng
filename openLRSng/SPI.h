#ifndef _SPI_H_
#define _SPI_H_

// **** bit-banged SPI routines

#define NOP() __asm__ __volatile__("nop")

uint8_t spiReadBit(void);
uint8_t spiReadData(void);
uint8_t spiReadRegister(uint8_t address);

void spiWriteBit(uint8_t b);
void spiWriteData(uint8_t i);
void spiWriteRegister(uint8_t address, uint8_t data);

void spiSendAddress(uint8_t i);
void spiSendCommand(uint8_t command);

uint8_t spiReadBit(void)
{
  uint8_t r = 0;
  SCK_on;
  NOP();
  if (SDO_1) {
    r = 1;
  }
  SCK_off;
  NOP();
  return r;
}

uint8_t spiReadData(void)
{
  uint8_t Result = 0;
  SCK_off;
  for (uint8_t i = 0; i < 8; i++) {   //read fifo data byte
    Result = (Result << 1) + spiReadBit();
  }
  return(Result);
}

uint8_t spiReadRegister(uint8_t address)
{
  uint8_t result;
  spiSendAddress(address);
  result = spiReadData();
  nSEL_on;
  return(result);
}

void spiWriteBit(uint8_t b)
{
  if (b) {
    SCK_off;
    NOP();
    SDI_on;
    NOP();
    SCK_on;
    NOP();
  } else {
    SCK_off;
    NOP();
    SDI_off;
    NOP();
    SCK_on;
    NOP();
  }
}

void spiWriteData(uint8_t i)
{
  for (uint8_t n = 0; n < 8; n++) {
    spiWriteBit(i & 0x80);
    i = i << 1;
  }
  SCK_off;
}

void spiWriteRegister(uint8_t address, uint8_t data)
{
  address |= 0x80;
  spiSendCommand(address);
  spiWriteData(data);
  nSEL_on;
}

void spiSendAddress(uint8_t i)
{
  spiSendCommand(i & 0x7f);
}

void spiSendCommand(uint8_t command)
{
  nSEL_on;
  SCK_off;
  nSEL_off;
  for (uint8_t n = 0; n < 8 ; n++) {
    spiWriteBit(command & 0x80);
    command = command << 1;
  }
  SCK_off;
}

#endif
