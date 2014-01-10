#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);   // Enable -> GND
 
const int ledPin = 13;
const int thmPin =  4;
const int pseudoGND = 19;
const int pseudoVCC = 17;
const float thmPitch = 0.01;
const float thmOffset = 0.6;
        
int aValue = 0;
float temp = 0;
float aRef ;
float gain ;

//int pickDecimal(float a, int digit) {
//  float d = pow(10, -1 - digit);
//  return int((abs(a * d) - abs(int(a * d))) * 10);
//}

void setup() {                
  Serial.begin(9600);
  //lcd.begin(16,2);
  pinMode(ledPin, OUTPUT); 
  pinMode(pseudoGND, OUTPUT);
  pinMode(pseudoVCC, OUTPUT); 
  
  analogReference(INTERNAL);
  aRef = 1.1;
  gain = 1.03;
  
  digitalWrite(pseudoVCC, HIGH);
  digitalWrite(pseudoGND,  LOW);
  
  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(temp);
  aValue = analogRead(thmPin);
  temp = ((float)aValue / gain / 1024 * aRef - thmOffset) / thmPitch;

}

void loop() {
  digitalWrite(ledPin, HIGH);
  delay(200);
    
  aValue = analogRead(thmPin);
  temp = ((float)aValue / gain / 1024 * aRef - thmOffset) / thmPitch;
  
  digitalWrite(ledPin, LOW);
    
//  Serial.print  (aValue);
//  Serial.print  ("   ");
  Serial.println(temp); 

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(temp);
//  lcd.print((long)temp);
//  lcd.print(".");
//  lcd.print(pickDecimal(temp + 0.05 * temp / abs(temp), -1));
//  lcd.write("0");
//  lcd.print("C");

  lcd.setCursor(8, 0);
  lcd.print("Gain");
  lcd.print(gain);
  lcd.setCursor(8, 1);
  lcd.print("Ref");
  lcd.print(aRef);
  lcd.print("V");

  delay(59800);
}
