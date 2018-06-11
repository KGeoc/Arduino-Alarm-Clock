#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
#include <MemoryFree.h>

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <DS1307RTC.h>

//char daysOfTheWeek[8][10] = {"", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

class NextAlarm {
  private:
    time_t prevAlarm;
    unsigned long interval;
    time_t newAlarm;
    char alarmName[30];
    char fileName[12];
    void makeFileName();
  public:
    NextAlarm();
    NextAlarm(int x);
    NextAlarm(int x, String y);
    NextAlarm(time_t x, time_t y, unsigned long z, char a[], char b[]);
    time_t getNextAlarm();
    time_t getInterval();
    char * getAlarmName();
    char * getFileName();
    bool alarmReached();
    void advanceToNext();
    String toString();

};

NextAlarm::NextAlarm() {
  prevAlarm = now();
  interval = 60;
  newAlarm = prevAlarm + interval;
}
NextAlarm::NextAlarm(int x) {

  prevAlarm = now();
  interval = x;
  newAlarm = prevAlarm + interval;
  makeFileName();
  //alarmName+=char(interval);
  //itoa(interval,alarmName,10);
  // alarmName += char(interval);
}

NextAlarm::NextAlarm(int x, String y) {

  y.toCharArray(alarmName, 30);
  prevAlarm = now();
  interval = x;
  newAlarm = prevAlarm + interval;
}
NextAlarm::NextAlarm(time_t x, time_t y, unsigned long z, char a[], char b[]) {
  newAlarm = x;
  prevAlarm = y;
  interval = z;
  //  a.toCharArray(alarmName, 30);
  strcpy(alarmName, a);
  strcpy(fileName, b);
}

time_t NextAlarm::getNextAlarm() {
  return newAlarm;
}

unsigned long NextAlarm::getInterval() {
  return interval;
}
char * NextAlarm::getAlarmName() {
  return alarmName;
}

char * NextAlarm::getFileName() {
  return fileName;
}

bool NextAlarm::alarmReached() {
  if (newAlarm <= now()) {
    return true;
  }
  return false;
}

void NextAlarm::makeFileName() {

  itoa(hour(prevAlarm), fileName, 10);
  itoa(minute(prevAlarm), fileName + strlen(fileName), 10);
  strcat(fileName, "_");
  itoa(interval, fileName + strlen(fileName), 10);
  strcat(fileName, ".txt");


}

void NextAlarm::advanceToNext() {
  while (alarmReached()) {
    prevAlarm += interval;
    newAlarm += interval;
  }
  makeFileName();
}
String NextAlarm::toString() {
  return (String)newAlarm + "\r\n" + (String)prevAlarm + "\r\n" + (String)interval + "\r\n" + String(alarmName);
}

time_t t;
NextAlarm asdf;
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;


// set up variables using the SD utility library functions:



//11, 12, 13 used
const byte leftDir = 5;
const byte downDir = 4;
const byte upDir = 3;
const byte rightDir = 2;
const byte blue = 6;
const byte yellow = 7;
const byte red = 8;

const uint8_t chipSelect = 10;
//11 used
//12 used
//13 used

SdFat sd;
SdFile file;

void printMemory() {

  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
}

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  printMemory();

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  oled.begin(&Adafruit128x64, I2C_ADDRESS);  // initialize with the I2C addr 0x3C (for the 128x64)
  oled.set400kHz();
  oled.setFont(Adafruit5x7);
  oled.clear();

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Cant sync RTC");
  else
    Serial.println("RTC time set");


  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  oled.clear();
  delay(2000);
  pinMode(leftDir, INPUT);
  pinMode(upDir, INPUT);
  pinMode(downDir, INPUT);
  pinMode(rightDir, INPUT);
  pinMode(blue, OUTPUT);
  pinMode(yellow, OUTPUT);

  asdf = NextAlarm(13, "didn't load any");
  loadFile();
  printMemory();

  //asdf.NextAlarm(60);
  //alarm = now();
  // alarm += 2 * SECS_PER_MIN;
}

void loadFile() {
  char line[25];
  int n;
  time_t lowestTime = 0 - 1;

  // Open next file in root.  The volume working directory, vwd, is root.
  // Warning, openNext starts at the current position of sd.vwd() so a
  // rewind may be neccessary in your application.
  sd.vwd()->rewind();
  while (file.openNext(sd.vwd(), O_READ)) {
    file.getName(line, 12);
    if (file.isDir()) {
      // do nothing for directories
    }
    else {

      if (!file.isOpen()) {
        Serial.println("No File Open");
      }
      file.fgets(line, sizeof(line));

      time_t tempLowest = atol(line);
      /*     Serial.print("tempLowest: ");
           Serial.println(tempLowest);
           Serial.println(line);*/

      if (lowestTime >  tempLowest)
      {
        lowestTime = tempLowest;
        //     Serial.println("New low");
        //      Serial.println(lowestTime);

        file.fgets(line, sizeof(line));

        time_t hold = atol(line);
        file.fgets(line, sizeof(line));
        unsigned long until = atol(line);
        char later[30];
        file.fgets(later, sizeof(later));
        file.getName(line, 25);
        /*       Serial.println(line);
               Serial.println(later);
               Serial.println(hold);
               Serial.println(until);
               Serial.println();
        */     asdf = NextAlarm(lowestTime, hold, until, later, line);
      }
      else {
        //        Serial.println("this was not lower");
      }
      /*   while ((n = file.fgets(line, sizeof(line))) > 0) {
           Serial.print(line);

         }*/
    }

    file.close();
  }
}

void makeNewFile() {
  if (sd.exists(asdf.getFileName())) {
    if (sd.remove(asdf.getFileName()))
    {
      Serial.println("File " + String(asdf.getFileName() ) + " removed");
    }

  }
  else {
    Serial.println("File " + String(asdf.getFileName()) + " file doesn't exist");
  }

  asdf.advanceToNext();

  File dataFile = sd.open(asdf.getFileName(), FILE_WRITE);
  if (dataFile) {
    dataFile.println(asdf.toString());

  } else {
    Serial.print("can't ");
    Serial.println(asdf.getFileName());

  }
  dataFile.close();
}
///////////////



bool firstAlarmInstance = true;

void loop() {
  // put your main code here, to run repeatedly:

  t = now();

  int tempTimeHolder;
  oled.home();



  oled.print(hourFormat12(t));
  oled.print(":");
  tempTimeHolder = minute(t);
  if (tempTimeHolder < 10)
    oled.print("0");
  oled.print(minute(t));
  oled.print(":");
  tempTimeHolder = second(t);
  if (tempTimeHolder < 10)
    oled.print("0");
  oled.print(second(t));
  oled.clearToEOL();

  oled.setCursor(0, 1);
  oled.print("Alarm ");
  oled.print(hourFormat12(asdf.getNextAlarm()));
  oled.print(":");
  tempTimeHolder = minute(asdf.getNextAlarm());
  if (tempTimeHolder < 10)
    oled.print("0");
  oled.print(minute(asdf.getNextAlarm()));
  oled.print(":");
  tempTimeHolder = second(asdf.getNextAlarm());
  if (tempTimeHolder < 10)
    oled.print("0");
  oled.print(second(asdf.getNextAlarm()));
  oled.clearToEOL();
  oled.println();

  digitalWrite(blue, false);
  digitalWrite(yellow, false);
  digitalWrite(red, false);
  if (asdf.alarmReached()) {
    if (firstAlarmInstance) {
      oled.clear(0, oled.displayHeight(), oled.row() + 1, oled.displayRows());



      firstAlarmInstance = false;
    }
    digitalWrite(blue, true);
    oled.setCursor(0, 3);

    oled.print(asdf.toString());

  }
  else {
    digitalWrite(yellow, true);
    oled.clear(0, oled.displayHeight(), oled.row(), oled.displayRows());


  }

  ///////create a new file for each alarm. use different time alarm made for filename
  //each line has different bit of info
  if (digitalRead(rightDir)) {
    printMemory();

    digitalWrite(red, true);
    if (asdf.alarmReached()) {
      makeNewFile();

      loadFile();
      firstAlarmInstance = true;
    }
  }




}



/*
  void testscrolltext(void) {
  DateTime now = rtc.now();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.clearDisplay();
  display.print(daysOfTheWeek[now.dayOfTheWeek()]);
  display.println(now.minute(),DEC);
  display.display();

  display.startscrollrightDir(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleftDir(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagrightDir(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleftDir(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  }
*/
