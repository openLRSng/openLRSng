#include<math.h>
#include<stdio.h>
#include<stdlib.h>

// INPUT INFO

float Rb = 9.6;  // kbps
float Fd      = 50.0; // kHz Frequency deviation

int   enmanch = 0;   // manchester enabled = 1 else 0


// internal


float bandwidth;
int ndec_exp, dwn3_bypass, filset;
int rxosr,ncoff,crgain;



void calc_if_filt()
{

  bandwidth = Rb * (enmanch?2:1) + 2 * Fd;

  if (bandwidth <= 2.6) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 2.8) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 3.1) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 3.2) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 3.7) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 4.2) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 4.5) {
    ndec_exp=5;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 4.9) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 5.4) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 5.9) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 6.1) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 7.2) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 8.2) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 8.8) {
    ndec_exp=4;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 9.5) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 10.6) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 11.5) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 12.1) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 14.2) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 16.2) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 17.5) {
    ndec_exp=3;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 18.9) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 21.0) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 22.7) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 24.0) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 28.2) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 32.2) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 34.7) {
    ndec_exp=2;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 37.7) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 41.7) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 45.2) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 47.9) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 56.2) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 64.1) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 69.2) {
    ndec_exp=1;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 75.2) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=1;
  } else if (bandwidth <= 83.2) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=2;
  } else if (bandwidth <= 90.0) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=3;
  } else if (bandwidth <= 95.3) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=4;
  } else if (bandwidth <= 112.1) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=5;
  } else if (bandwidth <= 127.9) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=6;
  } else if (bandwidth <= 137.9) {
    ndec_exp=0;
    dwn3_bypass=0;
    filset=7;
  } else if (bandwidth <= 142.8) {
    ndec_exp=1;
    dwn3_bypass=1;
    filset=4;
  } else if (bandwidth <= 167.8) {
    ndec_exp=1;
    dwn3_bypass=1;
    filset=5;
  } else if (bandwidth <= 181.1) {
    ndec_exp=1;
    dwn3_bypass=1;
    filset=9;
  } else if (bandwidth <= 191.5) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=15;
  } else if (bandwidth <= 225.1) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=1;
  } else if (bandwidth <= 248.8) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=2;
  } else if (bandwidth <= 269.3) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=3;
  } else if (bandwidth <= 284.9) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=4;
  } else if (bandwidth <= 335.5) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=8;
  } else if (bandwidth <= 361.8) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=9;
  } else if (bandwidth <= 420.2) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=10;
  } else if (bandwidth <= 468.4) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=11;
  } else if (bandwidth <= 518.8) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=12;
  } else if (bandwidth <= 577.0) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=13;
  } else if (bandwidth <= 620.7) {
    ndec_exp=0;
    dwn3_bypass=1;
    filset=14;
  } else {
    exit(1);
  }
}

void calc_rxosr()
{
  double rxosr_d = (500.0 * (1.0 + 2.0 * dwn3_bypass)) /
                   (pow(2.0,ndec_exp-3) * Rb * (enmanch?2:1));
  rxosr = (int)round(rxosr_d);
}

void calc_ncoff()
{
  double ncoff_d = (Rb * (enmanch?2:1) * pow(2.0,20+ndec_exp)) /
                   (500 * (1 + 2 * dwn3_bypass));
  ncoff = (int)round(ncoff_d);
}

void calc_crgain()
{
  double crgain_d = 2.0 + (65536 * (enmanch?2:1) * Rb) / (rxosr*Fd);
  crgain = (int)crgain_d;
}
int main(int argc, char **argv)
{

  printf("input data\n");
  printf("bitrate %.1f kpbs manch %s deviation %.1f kHz\n",Rb,enmanch?"ON":"OFF", Fd);

  calc_if_filt();
  calc_rxosr();
  calc_ncoff();
  calc_crgain();

  printf("Calculated parameters:\n");

  printf("bandwidth %.1f\n", bandwidth);
  printf("ndec_exp %d dwn3_bypass %d filset %d\n",
         ndec_exp, dwn3_bypass, filset);
  printf("rxosr %d\n", rxosr);
  printf("ncoff %d\n", ncoff);
  printf("crgain %d\n", crgain);


  printf("REGISTERS\n");
  printf("1C: %02x\n", (dwn3_bypass<<7) | (ndec_exp<<4) | filset);
  printf("20: %02x\n", rxosr&0xff);
  printf("21: %02x\n", (((rxosr>>8)&7)<<5) | ((ncoff>>16)&0x0f));
  printf("22: %02x\n", ((ncoff>>8)&0xff));
  printf("23: %02x\n", (ncoff&0xff));

  printf("24: %02x\n", ((crgain>>8)&7));
  printf("25: %02x\n", (crgain&0xff));


}
