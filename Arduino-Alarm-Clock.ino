#include <SPI.h>
#include <Wire.h>
#include <SD.h>

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <DS1307RTC.h>

s//char daysOfTheWeek[8][10] = {"", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

class NextAlarm {
  private:
    time_t prevAlarm;
    int interval;
    time_t newAlarm;
    char alarmName[30];
    char fileName[11];
    void makeName();
  public:
    NextAlarm();
    NextAlarm(int x);
    NextAlarm(int x, String y);
    time_t getNextAlarm();
    int getInterval();
    String getName();
    String getFileName();
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
  Serial.println("Next Alarm init");
  Serial.println(now());
  Serial.println(prevAlarm);
  Serial.println(interval);
  Serial.println(newAlarm);
  makeName();
  //alarmName+=char(interval);
  //itoa(interval,alarmName,10);
  // alarmName += char(interval);
}

NextAlarm::NextAlarm(int x, String y) {

  y.toCharArray(alarmName, 20);
  prevAlarm = now();
  interval = x;
  newAlarm = prevAlarm + interval;
  Serial.println("Next Alarm init");
  Serial.println(now());
  Serial.println(prevAlarm);
  Serial.println(interval);
  Serial.println(newAlarm);
}

time_t NextAlarm::getNextAlarm() {
  return newAlarm;
}

int NextAlarm::getInterval() {
  return interval;
}
String NextAlarm::getName() {
  return alarmName;
}

String NextAlarm::getFileName() {
  return fileName;
}

bool NextAlarm::alarmReached() {
  if (newAlarm < now()) {
    return true;
  }
  return false;
}

void NextAlarm::makeName() {
  itoa(hour(prevAlarm), fileName, 10);
  itoa(minute(prevAlarm), fileName + strlen(fileName), 10);
  strcat(fileName, "_");
  itoa(interval, fileName + strlen(fileName), 10);
  strcat(fileName, ".txt");

  sprintf(alarmName, "%li", prevAlarm);
  strcat(alarmName, "_");
  // strcat(alarmName,interval);
  itoa(interval, alarmName + strlen(alarmName), 10);
}

void NextAlarm::advanceToNext() {
  while (alarmReached()) {
    prevAlarm += interval;
    newAlarm = prevAlarm + interval;
    makeName();

  }
}
String NextAlarm::toString() {
  return String(prevAlarm) + "," + String(interval) + "," + String(fileName);;
}

time_t t;
NextAlarm asdf;
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;


// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
//11, 12, 13 used
const byte left = 5;
const byte down = 4;
const byte up = 3;
const byte right = 2;
const byte blue = 6;
const byte yellow = 7;
const byte red = 8;

const byte chipSelect = 10;
//11 used
//12 used
//13 used

//File dataFile;


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  oled.begin(&Adafruit128x64, I2C_ADDRESS);  // initialize with the I2C addr 0x3C (for the 128x64)
  oled.set400kHz();
  oled.setFont(Adafruit5x7);
  oled.clear();

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Cant sync RTC");
  else
    Serial.println("RTC has set time");

  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("SDcard failed check: card, wiring,chip select");
    return;
  } else {
    Serial.println("Wiring, card present");
  }
  if (!SD.begin(chipSelect)) {
    Serial.println("card failed");
    while (1);
  }
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  oled.clear();
  delay(2000);
  pinMode(left, INPUT);
  pinMode(up, INPUT);
  pinMode(down, INPUT);
  pinMode(right, INPUT);
  pinMode(blue, OUTPUT);
  pinMode(yellow, OUTPUT);


  asdf = NextAlarm(5);
  Serial.println(asdf.toString());
  //asdf.NextAlarm(60);
  //alarm = now();
  // alarm += 2 * SECS_PER_MIN;
}

void loop() {
  // put your main code here, to run repeatedly:
  t = now();
  // Serial.print(t);
  // Serial.print(" ");

  // Serial.println(asdf.getNextAlarm());

  //oled.clear();
  int tempTimeHolder;
  oled.home();

  //  oled.print(daysOfTheWeek[weekday(t)]);
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

  digitalWrite(blue, false);
  digitalWrite(yellow, false);
  digitalWrite(red, false);
  if (asdf.alarmReached())
    digitalWrite(blue, true);
  else
    digitalWrite(yellow, true);
  ///////create a new file for each alarm. use different time alarm made for filename
  //each line has different bit of info
  if (digitalRead(right)) {
    digitalWrite(red, true);
    if (asdf.alarmReached()) {
      //   holder += ".txt";
      SD.remove(asdf.getFileName());

      asdf.advanceToNext();

      File dataFile = SD.open(asdf.getFileName(), FILE_WRITE);
      if (dataFile) {
        dataFile.println(asdf.toString());
        
      } else {
        Serial.print("can't ");
        Serial.println(asdf.getFileName());

      }
      dataFile.close();
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

  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  }
*/
