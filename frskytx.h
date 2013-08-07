// FrSky emulation on TX side
//

// Interval at which SendFrame should get called
// FrSky frame is maximally 20bytes (all bytes stuffed)
// at 9600 baud we can send 960 bytes in second ->
// 20/960 == 20.8ms
#define FRSKY_INTERVAL 30000 // 30ms between frames 

void FrSkyInit()
{
  Serial.begin(9600);
}

uint8_t FrSkyUserBuf[16];
uint8_t FrSkyUserIdx = 0; // read/write indexes
#define FRSKY_RDI (FrSkyUserIdx>>4)
#define FRSKY_WRI (FrSkyUserIdx&0x0f)

uint8_t FrSkySchedule = 0;

void FrSkyUserData(uint8_t c)
{
  if ((FRSKY_WRI+1) != FRSKY_RDI) {
    FrSkyUserBuf[FRSKY_WRI]=c;
    FrSkyUserIdx = (FrSkyUserIdx & 0xf0) | ((FrSkyUserIdx + 1) & 0x0f);
  }
}

void FrSkySendStuffed(uint8_t frame[])
{
  Serial.write(0x7e);
  for (uint8_t i=0; i<9; i++) {
    if ((frame[i]==0x7e) || (frame[i]==0x7d)) {
      Serial.write(0x7d);
      frame[i]^=0x20;
    }
    Serial.write(frame[i]);
  }
  Serial.write(0x7e);
}


// Send frsky voltage/RSSI or userdata frame
// every 6th time we send the Ax/RSSI frame
void FrSkySendFrame(uint8_t a1, uint8_t a2, uint8_t rx, uint8_t tx)
{
  uint8_t frame[9];
  if (FrSkySchedule==0) {
    frame[0]=0xfe;
    frame[1]=a1;
    frame[2]=a2;
    frame[3]=(rx>>1); // this needs to be 0-127
    frame[4]=tx;      // this needs to be 0-255
    frame[5]=frame[6]=frame[7]=frame[8]=0;
    FrSkySendStuffed(frame);
  } else {
    if (FRSKY_RDI!=FRSKY_WRI) {
      uint8_t bytes = 0;
      frame[0] = 0xfd;
      frame[1] = 0;
      frame[2] = 0; // unused
      while ((bytes < 6) && (FRSKY_RDI!=FRSKY_WRI)) {
        frame[bytes + 3] = FrSkyUserBuf[FRSKY_RDI];
        FrSkyUserIdx+=0x10;
        bytes++;
      }
      frame[1] = bytes;
      FrSkySendStuffed(frame);
    }
  }
  FrSkySchedule=(FrSkySchedule+1) % 6;
}
