#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>

int wakeHour;
int wakeMinute;
bool wakeSet;
bool wakeSent;

char serialBuffer[10];
int bufferIndex;

char timeBuffer[10];

RTC_DS1307 RTC;

void setup() {

  // Open serial port, don't wait until it's available (blocking!)
  Serial.begin(9600);
  //while (!Serial);
  
  Serial.println("WakeMeUP");
  Serial.println();
   
  // Initialize RTC library
  Wire.begin();
  RTC.begin();
  DateTime now = RTC.now();
  Serial.print("- RTC library initialized, current time ");
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  Serial.println(timeBuffer);
  
  // A wake up time was set? EEPROM address 0 = 0x01
  byte setByte = EEPROM.read(0);
  if(setByte == 0x01) {
    wakeHour = EEPROM.read(1);
    wakeMinute = EEPROM.read(2);
    Serial.print("- wake time read from EEPROM: ");
    sprintf(timeBuffer, "%02d:%02d", wakeHour, wakeMinute);
    Serial.println(timeBuffer);
    wakeSet = true;
  }
  else {
    Serial.println("- No wake time saved in EEPROM");
    wakeSet = false;  
  }
  Serial.println();
  
  // Initialize variables
  bufferIndex = 0;
  wakeSent = false;
  
}

void loop() {
  
  // Check for incoming serial data
  if (Serial.available() > 0) {
    
    // read incoming char
    char inChar = Serial.read();
    
    // a new line (= end of command) was received?
    // If so, parse the buffer (command syntax: WHHMM<LF> ot T<LF>)
    // else save the char in the buffer and increment the index
    if(inChar == 0x0A) parseCommand();
    else serialBuffer[bufferIndex++] = inChar;
    
    // Limit of buffer reached? Restart
    if(bufferIndex == 10) bufferIndex = 0;
  }

  // Check if it's time to send the wake char
  if(wakeSet) {
    
    DateTime now = RTC.now();
    if(now.hour() == wakeHour && now.minute() == wakeMinute && !wakeSent) {
      USBDevice.wakeupHost();
      Serial.println("Time to wake up!");
      Serial.println();
      wakeSent = true;
    } 
    
    // if it's not wake time, reset wakeSent
    if(now.hour() != wakeHour && now.minute() != wakeMinute) wakeSent = false;
  }
}


// Parse the receivingBuffer looking for a valid command
void parseCommand() {
  
  // W command, change the wake time
  if(serialBuffer[0] == 'W') {
    
    int newWakeHour = (serialBuffer[1] - '0') * 10 + (serialBuffer[2] - '0');
    int newWakeMinute = (serialBuffer[3] - '0') * 10 + (serialBuffer[4] - '0');
    
    if(newWakeHour > -1 && newWakeHour < 24 && newWakeMinute > -1 && newWakeMinute < 60) {
      
      wakeSet = true;
      wakeSent = false;
      wakeHour = newWakeHour;
      wakeMinute = newWakeMinute;
      
      Serial.print("New wake time set: ");
      sprintf(timeBuffer, "%02d:%02d", wakeHour, wakeMinute);
      Serial.println(timeBuffer);
      
      EEPROM.write(0, 0x01);
      EEPROM.write(1, wakeHour);
      EEPROM.write(2, wakeMinute);
      Serial.println("Wake time saved in EEPROM");
    } 
    else {
      Serial.print("Invalid wake time specified: ");
      Serial.println(serialBuffer);
    }    
  }
  
  // T command, send the current time
  else if(serialBuffer[0] == 'T') {
    DateTime now = RTC.now();
    Serial.print("Current time: ");
    sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    Serial.println(timeBuffer);      
  }
  
  else {
    Serial.print("Invalid command: ");
    Serial.println(serialBuffer);
  }
  
  Serial.println();
  bufferIndex = 0; 
}
