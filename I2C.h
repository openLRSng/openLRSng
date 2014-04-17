#include <util/twi.h>
#include <Arduino.h>
#define I2C_FREQ 200000


// API

void myI2C_init(uint8_t enable_pullup);

// Initialize slave operation
void myI2C_slaveSetup(uint8_t address, uint8_t mask, uint8_t enableGCall,
                      uint8_t (*handler)(uint8_t *data, uint8_t flags));

// Flag values given to slaveHandler
#define MYI2C_SLAVE_ISTX    0x01 // slave handler should 'send' a byte
#define MYI2C_SLAVE_ISFIRST 0x02 // first byte after addressing (normally register number)

// Master mode transfer functions
uint8_t myI2C_writeTo(uint8_t address, uint8_t* data, uint8_t length, uint8_t flags);
uint8_t myI2C_readFrom(uint8_t address, uint8_t* data, uint8_t length, uint8_t flags);

// flag values used by master
#define MYI2C_WAIT      0x01 // do not return until transfer is done
#define MYI2C_NOSTOP    0x02 // do not release bus after transaction
#define MYI2C_NOTIMEOUT 0x04

// slave handler routine, should return desired ACK status
// *data  - pointter to fetch/store data
// flags  - see above
// RETVAL 0 - no more data wanted/available
//        1 - OK to continue transfer
uint8_t (*myI2C_slaveHandler)(uint8_t *data, uint8_t flags) = NULL;

// Internal state data
volatile uint8_t myI2C_slaRw;    // slave address & RW bit, used in master mode
volatile uint8_t *myI2C_dataPtr; // TX/RX data ptr
volatile uint8_t myI2C_dataCnt;  // data countter
#define MYI2C_DONTSTOP 0x01 // do not release bus after operation
#define MYI2C_REPSTART 0x02 // going to use repeated start on next transfer
#define MYI2C_BUSY     0x04 // transfer ongoing
volatile uint8_t myI2C_flags;
volatile uint8_t myI2C_error;
uint16_t myI2C_timeout = 5000; // default 2ms

void myI2C_init(uint8_t enablePullup)
{
  digitalWrite(SDA, enablePullup?1:0);
  digitalWrite(SCL, enablePullup?1:0);

  myI2C_dataCnt=0;
  myI2C_flags=0;

  TWSR |= ~(_BV(TWPS0)|_BV(TWPS1));
  TWBR = ((F_CPU / I2C_FREQ) - 16) / 2;

  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

void myI2C_slaveSetup(uint8_t address, uint8_t mask, uint8_t enableGCall,
                      uint8_t (*handler)(uint8_t *data, uint8_t flags))
{
  TWAR  = (address << 1) | (enableGCall?1:0);
  TWAMR = (mask << 1);
  myI2C_slaveHandler = handler;
}

void myI2C_reply(uint8_t ack)
{
  if(ack) {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  } else {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
  }
}

void myI2C_stop(void)
{
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);
  while(TWCR & _BV(TWSTO)) {
    continue;
  }
}

void myI2C_releaseBus(void)
{
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);
}

void myI2C_recover(void)
{
  TWCR = 0;
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
  myI2C_flags = 0;
}
// ISR //
SIGNAL(TWI_vect)
{
  uint8_t slack=0;
  //  Serial.println(TW_STATUS,16);
  switch(TW_STATUS) {
    // All Master
  case TW_START:     // sent start condition
  case TW_REP_START: // sent repeated start condition
    // copy device address and r/w bit to output register and ack
    TWDR = myI2C_slaRw;
    myI2C_reply(1);
    break;

    // Master Transmitter
  case TW_MT_SLA_ACK:  // slave receiver acked address
  case TW_MT_DATA_ACK: // slave receiver acked data
    // if there is data to send, send it, otherwise stop
    if(myI2C_dataCnt--) {
      // copy data to output register and ack
      TWDR = *(myI2C_dataPtr++);
      myI2C_reply(1);
    } else {
      myI2C_flags &= ~MYI2C_BUSY;
      if (myI2C_flags&MYI2C_DONTSTOP) {
        myI2C_flags &= ~MYI2C_DONTSTOP;
        myI2C_flags |= MYI2C_REPSTART;
        TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN);
      } else {
        myI2C_stop();
      }
    }
    break;
  case TW_MT_SLA_NACK:  // address sent, nack received
    myI2C_error = 2;
    myI2C_flags &= ~MYI2C_BUSY;
    myI2C_stop();
    break;
  case TW_MT_DATA_NACK: // data sent, nack received
    myI2C_error = 3;
    myI2C_flags &= ~MYI2C_BUSY;
    myI2C_stop();
    break;
  case TW_MT_ARB_LOST: // lost bus arbitration
    myI2C_error = 4;
    myI2C_flags &= ~MYI2C_BUSY;
    myI2C_releaseBus();
    break;

    // Master Receiver
  case TW_MR_DATA_ACK: // data received, ack sent
    // put byte into buffer
    *(myI2C_dataPtr++) = TWDR;
    myI2C_dataCnt--;
  case TW_MR_SLA_ACK:  // address sent, ack received
    // ack if more bytes are expected, otherwise nack
    if (myI2C_dataCnt>1) {
      myI2C_reply(1);
    } else {
      myI2C_reply(0);
    }
    break;
  case TW_MR_DATA_NACK: // data received, nack sent
    // put final byte into buffer
    *(myI2C_dataPtr++) = TWDR;
    myI2C_dataCnt--;
    myI2C_flags &= ~MYI2C_BUSY;
    if (myI2C_flags&MYI2C_DONTSTOP) {
      myI2C_flags &= ~MYI2C_DONTSTOP;
      myI2C_flags |= MYI2C_REPSTART;
      TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN);
    } else {
      myI2C_stop();
    }
    break;
  case TW_MR_SLA_NACK: // address sent, nack received
    myI2C_error = 5;
    myI2C_flags &= ~MYI2C_BUSY;
    myI2C_stop();
    break;
    // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

    // Slave Receiver
  case TW_SR_SLA_ACK:   // addressed, returned ack
  case TW_SR_GCALL_ACK: // addressed generally, returned ack
  case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
  case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
    myI2C_dataCnt = 0;
    myI2C_reply(1);
    break;
  case TW_SR_DATA_ACK:       // data received, returned ack
  case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
    if (myI2C_slaveHandler) {
      uint8_t data=TWDR;
      myI2C_reply(myI2C_slaveHandler(&data, (myI2C_dataCnt++==0)?MYI2C_SLAVE_ISFIRST:0));
    } else {
      myI2C_reply(0);
    }
    break;
  case TW_SR_STOP: // stop or repeated start condition received
    myI2C_stop();
    myI2C_releaseBus();
    break;
  case TW_SR_DATA_NACK:       // data received, returned nack
  case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
    // nack back at master
    myI2C_reply(0);
    break;

    // Slave Transmitter
  case TW_ST_SLA_ACK:          // addressed, returned ack
  case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
    slack=1;
    // just fall thru to data sending
  case TW_ST_DATA_ACK: // byte sent, ack returned

    if (myI2C_slaveHandler) {
      uint8_t data,ret;
      ret = myI2C_slaveHandler(&data, (slack?MYI2C_SLAVE_ISFIRST:0)|MYI2C_SLAVE_ISTX);
      TWDR = data;
      myI2C_reply(ret); // reply with ACK or NACK depending on slave callback
    } else {
      myI2C_reply(0);
    }
    break;
  case TW_ST_DATA_NACK: // received nack, we are done
  case TW_ST_LAST_DATA: // received ack, but we are done already!
    // ack future responses
    myI2C_reply(1);
    break;

    // All
  case TW_NO_INFO:   // no state information
    break;
  case TW_BUS_ERROR: // bus error, illegal stop/start
    //twi_error = TW_BUS_ERROR;
    myI2C_stop();
    break;
  }
}

uint8_t myI2C_wait(uint16_t timeout)
{
  uint8_t ret;
  uint32_t start = micros();
  while (myI2C_flags & MYI2C_BUSY) {
    if ((timeout) && ((micros() - start) > timeout)) {
      myI2C_recover();
      return 1;
    }
  }
  ret = myI2C_error;
  myI2C_error=0;
  return ret;
}

uint8_t myI2C_writeTo(uint8_t address, uint8_t* data, uint8_t length, uint8_t flags)
{
  uint8_t ret;
  if ((ret = myI2C_wait((flags & MYI2C_NOTIMEOUT) ? 0 : myI2C_timeout))) {
    return ret;
  }
  myI2C_error = 0;
  myI2C_dataCnt = length;
  myI2C_dataPtr = data;
  myI2C_slaRw = TW_WRITE | (address << 1);

  myI2C_flags |= MYI2C_BUSY | ((flags&MYI2C_NOSTOP)?MYI2C_DONTSTOP:0);

  if (myI2C_flags & MYI2C_REPSTART) {
    myI2C_flags &= ~MYI2C_REPSTART;
    TWDR = myI2C_slaRw;
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
  } else {
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
  }
  if (flags & MYI2C_WAIT) {
    if ((ret = myI2C_wait((flags & MYI2C_NOTIMEOUT) ? 0 : myI2C_timeout))) {
      return ret;
    }
  }
  return 0;
}


uint8_t myI2C_readFrom(uint8_t address, uint8_t* data, uint8_t length, uint8_t flags)
{
  uint8_t ret;
  if ((ret=myI2C_wait((flags & MYI2C_NOTIMEOUT) ? 0 : myI2C_timeout))) {
    return ret;
  }

  myI2C_error = 0;
  myI2C_dataCnt = length;
  myI2C_dataPtr = data;
  myI2C_slaRw = TW_READ | (address << 1);

  myI2C_flags |= MYI2C_BUSY | ((flags&MYI2C_NOSTOP)?MYI2C_DONTSTOP:0);

  if (myI2C_flags & MYI2C_REPSTART) {
    myI2C_flags &= ~MYI2C_REPSTART;
    TWDR = myI2C_slaRw;
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
  } else {
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
  }

  if (flags & MYI2C_WAIT) {
    if ((ret=myI2C_wait((flags & MYI2C_NOTIMEOUT) ? 0 : myI2C_timeout))) {
      return ret;
    }
  }
  return 0;
}

