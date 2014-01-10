/*
  mp3 player
*/

//Add the SdFat Libraries
#include <SPI.h>
#include <stdio.h>
#include <SdFat.h>
#include <SdFatUtil.h>

#define TRUE  0
#define FALSE  1

//Create the variables to be used by SdFat Library
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile track;

unsigned char sndVol = 100;
unsigned long fileSize = 0;
int progress = 0;

int previousTrigger = 255;
long lastCheck;

char errorMsg[100];

int trigger0 = A0;
int trigger1 = A1;
int trigger2 = A2;
int trigger3 = A3;
int trigger4 = A4;
int trigger5 = A5;

char name[13];

#define MP3_XCS 6
#define MP3_XDCS 7
#define MP3_DREQ 2
#define MP3_RESET 8

#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

void setup() {
  pinMode(trigger0, INPUT);
  pinMode(trigger1, INPUT);
  pinMode(trigger2, INPUT);
  pinMode(trigger3, INPUT);
  pinMode(trigger4, INPUT);
  pinMode(trigger5, INPUT);

  digitalWrite(trigger0, HIGH);
  digitalWrite(trigger1, HIGH);
  digitalWrite(trigger2, HIGH);
  digitalWrite(trigger3, HIGH);
  digitalWrite(trigger4, HIGH);
  digitalWrite(trigger5, HIGH);

  pinMode(MP3_DREQ, INPUT);
  pinMode(MP3_XCS, OUTPUT);
  pinMode(MP3_XDCS, OUTPUT);
  pinMode(MP3_RESET, OUTPUT);

  digitalWrite(MP3_XCS, HIGH);
  digitalWrite(MP3_XDCS, HIGH);
  digitalWrite(MP3_RESET, LOW);

  Serial.begin(57600);
  //Serial.println("MP3 Player Shield");

  pinMode(10, OUTPUT);
  if (!card.init(SPI_FULL_SPEED)) {
    Serial.println("Error: Card init");
    while(1);
  }
  if (!volume.init(&card)) {
    Serial.println("Error: Volume init");
    while(1);
  }

  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.transfer(0xFF);
  delay(10);
  digitalWrite(MP3_RESET, HIGH);

  // EarSpeaker setting
  //Mp3WriteRegister(SCI_MODE, 0x48, 0x80);
  //EQ setting
  //Mp3WriteRegister(SCI_BASS, 0x00, 0xf6);

  Mp3WriteRegister(SCI_CLOCKF, 0x60, 0x00);
  SPI.setClockDivider(SPI_CLOCK_DIV4);

  Mp3SetVolume(sndVol, sndVol);
  delay(1000);
}

void loop(){
  if (!root.openRoot(&volume)) {
    Serial.println("Error: Opening root");
    while(1);
  }
  while (track.openNext(&root, O_READ)) {
    fileSize = 0;
    track.getFilename(name);
    Serial.print("0");
    Serial.println(name);
    playMP3(track);
    ClearData();
    track.close();
  }
  root.close();
}

void playMP3(SdFile track) {
  uint8_t mp3DataBuffer[32];
  int need_data = TRUE;
  int trigger = 255;
  int nowTime = 0;
  int prevTime = 0;

  while(1) {
    while (!digitalRead(MP3_DREQ)) {
      if(need_data == TRUE) {
        if(!track.read(mp3DataBuffer, sizeof(mp3DataBuffer))) {
          break;
        } else {
          fileSize = fileSize + 32;
        }
        need_data = FALSE;
      }

      if (nowTime != prevTime) {
        progress = (float)fileSize / (float)track.fileSize() * 100;
        Serial.print("2");
        Serial.println(progress);
        nowTime = prevTime;
      }

      trigger = checkTriggers();
      if (trigger == 0) {
        if(sndVol < 250) {
          sndVol = sndVol + 10;
        }
        Mp3SetVolume(sndVol, sndVol);
        Serial.print("1");
        Serial.println(sndVol);
      } else if (trigger == 1) {
        if(sndVol > 0) {
          sndVol = sndVol - 10;
        }
        Mp3SetVolume(sndVol, sndVol);
        Serial.print("1");
        Serial.println(sndVol);
      } else if (trigger == 2) {
        return;
      }
    }

    if(need_data == TRUE) {
      if(!track.read(mp3DataBuffer, sizeof(mp3DataBuffer))) {
        break;
      } else {
        fileSize = fileSize + 32;
      }
      need_data = FALSE;
    }

    digitalWrite(MP3_XDCS, LOW);
     for(int y = 0 ; y < sizeof(mp3DataBuffer) ; y++) {
      SPI.transfer(mp3DataBuffer[y]);
    }
    digitalWrite(MP3_XDCS, HIGH);
    need_data = TRUE;
    prevTime = millis() / 1000;
  }
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XDCS, HIGH);
}

void Mp3SetVolume(unsigned char leftchannel, unsigned char rightchannel){
  Mp3WriteRegister(SCI_VOL, leftchannel, rightchannel);
}

void ClearData() {
  uint8_t endFillByte = Mp3ReadRegister8(0x1e06);
  for (int i = 0; i < 2052; i++) {
    while(!digitalRead(MP3_DREQ));
    digitalWrite(MP3_XDCS, LOW);
    SPI.transfer(endFillByte);
    digitalWrite(MP3_XDCS, HIGH);
  }
  Mp3WriteRegister(SCI_MODE, 0x48, 0x08);
  for (int i = 0; i < 2048; i++) {
    while(!digitalRead(MP3_DREQ));
    digitalWrite(MP3_XDCS, LOW);
    SPI.transfer(endFillByte);
    digitalWrite(MP3_XDCS, HIGH);
  }
  if (Mp3ReadRegister(SCI_HDAT0) != 0 || Mp3ReadRegister(SCI_HDAT1) != 0) {
    Mp3WriteRegister(SCI_MODE, 0x48, 0x04);
  }
  delay(100);
}

int checkTriggers(void) {
#define DEBOUNCE  100
  int foundTrigger = 255;
  if( (previousTrigger != 255) && (millis() - lastCheck) > 3000) {
    previousTrigger = 255;
    Serial.println("Previous trigger reset");
  }
  if(digitalRead(trigger0) == LOW){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger0) == LOW) foundTrigger = 0;
  }
  else if(digitalRead(trigger1) == LOW ){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger1) == LOW) foundTrigger = 1;
  }
  else if(digitalRead(trigger2) == LOW){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger2) == LOW) foundTrigger = 2;
  }
  else if(digitalRead(trigger3) == LOW){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger3) == LOW) foundTrigger = 3;
  }
  else if(digitalRead(trigger4) == LOW){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger4) == LOW) foundTrigger = 4;
  }
  else if(digitalRead(trigger5) == LOW){ 
    delay(DEBOUNCE); 
    if(digitalRead(trigger5) == LOW) foundTrigger = 5;
  }
  lastCheck = millis();
  if(foundTrigger != previousTrigger){
    previousTrigger = foundTrigger; 
    return(foundTrigger);
  }
  else
    return(255);
}

void Mp3WriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte){
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, LOW);
  SPI.transfer(0x02);
  SPI.transfer(addressbyte);
  SPI.transfer(highbyte);
  SPI.transfer(lowbyte);
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, HIGH);
}

unsigned int Mp3ReadRegister (unsigned char addressbyte){
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, LOW);
  SPI.transfer(0x03);
  SPI.transfer(addressbyte);
  byte response1 = SPI.transfer(0xFF);
  while(!digitalRead(MP3_DREQ));
  byte response2 = SPI.transfer(0xFF);
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, HIGH);
  unsigned int resultvalue = response1 << 8;
  resultvalue |= response2;
  return resultvalue;
}

unsigned int Mp3ReadRegister8 (unsigned char addressbyte){
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, LOW);
  SPI.transfer(0x03);
  SPI.transfer(addressbyte);
  byte response = SPI.transfer(0xFF);
  while(!digitalRead(MP3_DREQ));
  digitalWrite(MP3_XCS, HIGH);
  unsigned int resultvalue = response;
  return resultvalue;
}

