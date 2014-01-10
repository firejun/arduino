#include <stdio.h>
#include <string.h>
#include <SD.h>
#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal.h>

OneWire  ds(9);  // on pin 10 (a 4.7K resistor is necessary)
int DS1307_ADDRESS=0x68;
const short Resolution = 12; //DS18B20 Resolution 9 - 12 Bit
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
File fds;
char r_sec[3];
char r_minute[3];
char r_hour[3];
char r_date[3];
char r_month[3];
char r_year[3];
char buffer[3] = {'0', '0', '\0'};
float celsius;
boolean flag = false;

void setup(void) {
  Wire.begin();
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.clear();

  // SD card initialize
  pinMode(10, OUTPUT);
  while (!SD.begin(8)) {
    lcd.setCursor(0,0);
    lcd.print("SD card failed!");
    delay(1000);
  }

  // Thermometer Resolution set (default 12 BITS)
  short Sw_value ;
  switch (Resolution){
  case 9:
    Sw_value = 0x1F;
    break;
  case 10:
    Sw_value = 0x3F;
    break;
  case 11:
    Sw_value = 0x5F;
    break;
  default:
    Sw_value = 0x7F;
    break;
  }
  ds.reset();
  ds.skip();
  ds.write(0x4E);         // Write Scratchpad 
  ds.write(0x00);         // User Byte 1 (not in use)
  ds.write(0x00);         // User Byte 2 (not in use)
  ds.write(Sw_value);     // set Thermometer Resolution
  lcd.clear();
}

void loop(void) {
  if(Serial.available()){
    if (Serial.read() == 0x74){ // "t"=0x74
      setTime();
    }
    if (Serial.read() == 0x64){ // "d"=0x64
      setDay();
    }
  }

  if (flag == false && (r_sec[0] == '0'
    && (r_sec[1] == '0'
    || r_sec[1] == '1'))) {
    writeSD();
    flag = true;
  } else {
    flag = false;
  }
  printTime();
  printTemp();
}

void setTime(){
  byte hour = (Serial.read() - 0x30) * 0x10;
  hour +=(Serial.read() - 0x30);
  byte minute=(Serial.read() - 0x30) * 0x10;
  minute += (Serial.read() - 0x30); 
  byte sec=(Serial.read() - 0x30) * 0x10;
  sec += (Serial.read() - 0x30);
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS,3);
  byte r_sec=Wire.read();
  byte r_minute=Wire.read();
  byte r_hour=Wire.read();
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.write((r_sec & 0x80)|sec);
  Wire.write(minute);
  Wire.write((r_hour & 0x40)|hour);
  Wire.endTransmission();
}

void setDay(){
  byte year = (Serial.read() - 0x30) * 0x10;
  year += (Serial.read() - 0x30);
  byte month = (Serial.read() - 0x30) * 0x10;
  month += (Serial.read() - 0x30); 
  byte date = (Serial.read() - 0x30) * 0x10;
  date += (Serial.read() - 0x30);
  byte day_of_week = (Serial.read() - 0x30);
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x03);
  Wire.write(day_of_week);
  Wire.write(date);
  Wire.write(month);
  Wire.write(year);
  Wire.endTransmission();
}

void printTime(){
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS,7);

  formatNum(Wire.read());
  strcpy(r_sec, buffer);
  formatNum(Wire.read());
  strcpy(r_minute, buffer);
  formatNum(Wire.read());
  strcpy(r_hour, buffer);
  Wire.read(); // Dummy r_day_of_week
  formatNum(Wire.read());
  strcpy(r_date, buffer);
  formatNum(Wire.read());
  strcpy(r_month, buffer);
  formatNum(Wire.read());
  strcpy(r_year, buffer);

  lcd.setCursor(0, 0);
  lcd.print(r_year);
  lcd.print("/");
  lcd.print(r_month);
  lcd.print("/");
  lcd.print(r_date);

  lcd.setCursor(0, 1);
  lcd.print(r_hour);
  lcd.print(":");
  lcd.print(r_minute);
  lcd.print(":");
  lcd.print(r_sec);
}

void printTemp() {
  byte i;
  byte data[12];
  
  ds.reset();
  ds.skip();
  ds.write(0x44,1);  // start conversion, with parasite power on at the end

  int Conv_time = 1000;
  switch (Resolution ){
  case 9 :
    Conv_time = 100;
    break;;
  case 10 :
    Conv_time = 200;
    break; 
  case 11 :
    Conv_time = 400;
    break;
  }
   delay(Conv_time); //  Resolution to 9 BITS  conversion time 93.75ms
                     // 200 : Resolution to 10 BITS conversion time 187.5 ms
                     // 400 : Resolution to 11 BITS conversion time 375 ms
                     // 1000 : Resolution to 12 BITS conversion time 750 ms
  // we might do a ds.depower() here, but the reset will take care of it.
   
  ds.reset();
  ds.skip();   
  ds.write(0xBE);         // Read Scratchpad
 
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
    if (OneWire::crc8(data,8) != data[8]) {
      Serial.println("CRC is not valid!");
      return;
  }
  // convert the data to actual temperature
 
  int raw = (data[1] << 8) | data[0];
 
  switch (Resolution){
  case 9:
    raw = raw & 0xFFF8;  // 9 bit resolution
    break;
  case 10:
    raw = raw & 0xFFFC; // 10 bit resolution
    break;
  case 11:
    raw = raw & 0xFFFE; // 11 bit resolution
    break;
  }
 
  celsius = (float)raw / 16.0 ;
  lcd.setCursor(9,1);
  lcd.print("      ");
  lcd.setCursor(9,1);
  lcd.print(celsius);
  lcd.print("C");
}

void formatNum(byte val) {
  val = (val >> 4) * 10 + (val & 0x0F);
  buffer[0] = (val / 10) + 0x30;
  buffer[1] = (val - val / 10 * 10) + 0x30;
}

void writeSD() {
  char val[6];
  dtostrf(celsius, 2, 2, val);
  fds = SD.open("temp.txt", FILE_WRITE);
  if (fds) {
    fds.write("20");
    fds.write(r_year);
    fds.write("/");
    fds.write(r_month);
    fds.write("/");
    fds.write(r_date);
    fds.write(",");
    fds.write(r_hour);
    fds.write(":");
    fds.write(r_minute);
    fds.write(":");
    fds.write(r_sec);
    fds.write(",");
    fds.write(val);
    fds.write("\r\n");
    fds.close();
  } else {
    lcd.clear();
    lcd.print("Write error.");
    delay(1000);
  }
}

