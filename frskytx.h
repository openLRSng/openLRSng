// FrSky emulation on TX side
//

// Interval at which SendFrame should get called
// FrSky frame is maximally 20bytes (all bytes stuffed)
// at 9600 baud we can send 960 bytes in second ->
// 20/960 == 20.8ms
#define FRSKY_INTERVAL 30000
#define FRSKY_BAUDRATE 9600

#define SMARTPORT_INTERVAL 12000
#define SMARTPORT_BAUDRATE 57600

bool frskyIsSmartPort = 0;

uint8_t FrSkyUserBuf[16];
uint8_t FrSkyUserIdx = 0; // read/write indexes
#define FRSKY_RDI (FrSkyUserIdx >> 4)
#define FRSKY_WRI (FrSkyUserIdx & 0x0f)
uint32_t frskyLast = 0;

uint8_t frskySchedule = 0;

void frskyInit(bool isSmartPort)
{
  frskyLast = micros();
  frskyIsSmartPort = isSmartPort;
  TelemetrySerial.begin(isSmartPort ? SMARTPORT_BAUDRATE : FRSKY_BAUDRATE);
}

void frskyUserData(uint8_t c)
{
  if ((FRSKY_WRI + 1) != FRSKY_RDI) {
    FrSkyUserBuf[FRSKY_WRI] = c;
    FrSkyUserIdx = (FrSkyUserIdx & 0xf0) | ((FrSkyUserIdx + 1) & 0x0f);
  }
}

void frskySendStuffed(uint8_t frame[])
{
  TelemetrySerial.write(0x7e);
  for (uint8_t i = 0; i < 9; i++) {
    if ((frame[i] == 0x7e) || (frame[i] == 0x7d)) {
      TelemetrySerial.write(0x7d);
      frame[i] ^= 0x20;
    }
    TelemetrySerial.write(frame[i]);
  }
  TelemetrySerial.write(0x7e);
}


// Send frsky voltage/RSSI or userdata frame
// every 6th time we send the Ax/RSSI frame
void frskySendFrame(uint8_t a1, uint8_t a2, uint8_t rx, uint8_t tx)
{
  uint8_t frame[9];
  if (frskySchedule == 0) {
    frame[0] = 0xfe;
    frame[1] = a1;
    frame[2] = a2;
    frame[3] = (rx >> 1); // this needs to be 0-127
    frame[4] = tx;      // this needs to be 0-255
    frame[5] = frame[6] = frame[7] = frame[8] = 0;
    frskySendStuffed(frame);
  } else {
    if (FRSKY_RDI != FRSKY_WRI) {
      uint8_t bytes = 0;
      frame[0] = 0xfd;
      frame[1] = 0;
      frame[2] = 0; // unused
      while ((bytes < 6) && (FRSKY_RDI != FRSKY_WRI)) {
        frame[bytes + 3] = FrSkyUserBuf[FRSKY_RDI];
        FrSkyUserIdx += 0x10;
        bytes++;
      }
      frame[1] = bytes;
      frskySendStuffed(frame);
    }
  }
  frskySchedule = (frskySchedule + 1) % 6;
}

void smartportSend(uint8_t *p)
{
  uint16_t crc = 0;
  TelemetrySerial.write(0x7e);
  for (int i = 0; i < 9; i++) {
    if (i == 8) {
      p[i] = 0xff - crc;
    }
    if ((p[i] == 0x7e) || (p[i] == 0x7d)) {
      TelemetrySerial.write(0x7d);
      TelemetrySerial.write(0x20 ^ p[i]);
    } else {
      TelemetrySerial.write(p[i]);
    }
    if (i>0) {
      crc += p[i]; //0-1FF
      crc += crc >> 8; //0-100
      crc &= 0x00ff;
    }
  }
}

void smartportIdle()
{
  TelemetrySerial.write(0x7e);
}

void smartportSendFrame(uint8_t a1, uint8_t a2 ,uint8_t rssi, uint8_t prof)
{
  uint8_t buf[9];
  frskySchedule = (frskySchedule + 1) % 36;
  buf[0] = 0x98;
  buf[1] = 0x10;
  switch (frskySchedule) {
  case 0: // SWR
    buf[2] = 0x05;
    buf[3] = 0xf1;
    buf[4] = prof + 1;
    break;
  case 1: // RSSI
    buf[2] = 0x01;
    buf[3] = 0xf1;
    buf[4] = (uint16_t)(rssi) * 100 / 256;
    break;
  case 2: //BATT
    buf[2] = 0x04;
    buf[3] = 0xf1;
    buf[4] = a1;
    break;
  case 3: //A2
    buf[2] = 0x03;
    buf[3] = 0xf1;
    buf[4] = a2;
    break;
  default:
    smartportIdle();
    return;
  }
  smartportSend(buf);
}

void frskyUpdate(uint8_t a1, uint8_t a2, uint8_t rx, uint8_t tx, uint8_t prof)
{
  uint32_t now = micros();
  if (frskyIsSmartPort) {
    if ((now - frskyLast) > SMARTPORT_INTERVAL) {
      smartportSendFrame(a1, a2, (rx < tx) ? rx : tx, prof);
      frskyLast = now;
    }
  } else {
    if ((now - frskyLast) > FRSKY_INTERVAL) {
      frskySendFrame(a1, a2, rx, tx);
      frskyLast = now;
    }
  }
}
