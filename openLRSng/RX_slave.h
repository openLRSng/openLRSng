#ifdef ENABLE_SLAVE_MODE
#ifndef _RX_SLAVE_H_
#define _RX_SLAVE_H_

//#define SLAVE_STATISTICS
#ifdef SLAVE_STATISTICS
uint16_t rxBoth   = 0;
uint16_t rxSlave  = 0;
uint16_t rxMaster = 0;
uint32_t rxStatsMs = 0;
#endif

uint8_t slaveAct = 0;
uint8_t slaveCnt = 0;
volatile uint8_t slaveState = 0; // 0 - no slave, 1 - slave initializing, 2 - slave running, 3- errored
uint32_t slaveFailedMs = 0;

void checkSlaveState(void);
void reinitSlave(void);
void slaveHop(void);
uint8_t readSlaveState(void);
void checkSlaveStatus(void);
uint8_t slaveHandler(uint8_t *data, uint8_t flags);
void slaveLoop(void);

extern uint8_t rx_buf[];
extern uint8_t RF_channel;

// this goes at end of main loop
void checkSlaveStatus(void)
{
  if ((slaveState == 255) && ((millis() - slaveFailedMs) > 1000)) {
    slaveFailedMs = millis();
    reinitSlave();
  }

#ifdef SLAVE_STATISTICS
  if ((millis() - rxStatsMs) > 5000) {
    rxStatsMs = millis();
    Serial.print(rxBoth);
    Serial.print(',');
    Serial.print(rxMaster);
    Serial.print(',');
    Serial.println(rxSlave);
    rxBoth = rxMaster = rxSlave = 0;
  }
#endif
}

uint8_t slaveHandler(uint8_t *data, uint8_t flags)
{
  if (flags & MYI2C_SLAVE_ISTX) {
    if (flags & MYI2C_SLAVE_ISFIRST) {
      *data = slaveState;
      slaveCnt = 0;
    } else {
      if (slaveCnt < getPacketSize(&bind_data)) {
        *data = rx_buf[slaveCnt++];
      } else {
        return 0;
      }
    }
  } else {
    if (flags & MYI2C_SLAVE_ISFIRST) {
      slaveAct = *data;
      slaveCnt = 0;
      if ((slaveAct & 0xe0) == 0x60) {
        if (slaveState >= 2) {
          RF_channel = (*data & 0x1f);
          slaveState = 3; // to RX mode
        }
        return 0;
      } else if (slaveAct == 0xfe) {
        // deinitialize
        slaveState = 0;
        return 0;
      }
    } else {
      if (slaveAct == 0xff) {
        // load bind_data
        if (slaveCnt < sizeof(bind_data)) {
          ((uint8_t *)(&bind_data))[slaveCnt++] = *data;
          if (slaveCnt == sizeof(bind_data)) {
            slaveState = 1;
            return 0;
          }
        } else {
          return 0;
        }
      }
    }
  }
  return 1;
}

void slaveLoop(void)
{
  myI2C_slaveSetup(32, 0, 0, slaveHandler);
  slaveState = 0;
  while(1) {
    if (slaveState == 1) {
      init_rfm(0);   // Configure the RFM22B's registers for normal operation
      slaveState = 2; // BIND applied
      Red_LED_OFF;
    } else if (slaveState == 3) {
      Green_LED_OFF;
      setHopChannel(RF_channel);
      rx_reset();
      slaveState = 4; // in RX mode
    } else if (slaveState == 4) {
      if (RF_Mode == RECEIVED) {
        rfmGetPacket(rx_buf, sizeof(bind_data));
        slaveState = 5;
        Green_LED_ON;
      }
    }
  }
}

void reinitSlave(void)
{
  uint8_t ret, buf[sizeof(bind_data)+1];
  buf[0] = 0xff;
  memcpy((buf + 1), &bind_data, sizeof(bind_data));
  ret = myI2C_writeTo(32, buf, (sizeof(bind_data) + 1), MYI2C_WAIT);

  if (ret == 0) {
    ret = myI2C_readFrom(32, buf, 1, MYI2C_WAIT);
    if ((ret == 0)) {
      slaveState = 2;
    } else {
      slaveState = 255;
    }
  } else {
    slaveState = 255;
  }

  if (slaveState == 2) {
  } else {
    slaveFailedMs = millis();
  }
}

void slaveHop(void)
{
  if (slaveState == 2) {
    uint8_t buf;
    buf = 0x60 + RF_channel;
    if (myI2C_writeTo(32, &buf, 1, MYI2C_WAIT)) {
      slaveState = 255;
      slaveFailedMs = millis();
    }
  }
}

// Return slave state or 255 in case of error
uint8_t readSlaveState(void)
{
  uint8_t ret = 255;
  uint8_t buf;
  if (slaveState == 2) {
    ret = myI2C_readFrom(32, &buf, 1, MYI2C_WAIT);
    if (ret) {
      slaveState = 255;
      slaveFailedMs = millis();
      ret=255;
    } else {
      ret=buf;
    }
  }
  return ret;
}

void checkSlaveState(void)
{
  if (slaveState) {
    reinitSlave();
  }
}
#endif
#endif
