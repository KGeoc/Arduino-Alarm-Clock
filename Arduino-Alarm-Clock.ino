#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <DS1307RTC.h>

class NextAlarm {
private:
  time_t prevAlarm;
  unsigned long interval;
  time_t newAlarm;
  char alarmName[30];
  char fileName[30];
  void makeFileName();
public:
  NextAlarm();
  NextAlarm(int x);
  NextAlarm(unsigned long x, char a[]);
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
}

NextAlarm::NextAlarm(unsigned long x, char a[]) {

  prevAlarm = now();
  interval = x;
  newAlarm = prevAlarm + interval;
  strcpy(alarmName, a);
  makeFileName();
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




//for oled
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

time_t t; //current time
NextAlarm asdf; //alarm class and time holder

//11, 12, 13 used
const byte rightDir = 2;
const byte upDir = 3;
const byte downDir = 4;
const byte leftDir = 5;
const byte sel = 6;
const byte ret = 7;
const byte LED = 8;
const byte chipSelect = 10;//for SD
//11 used
//12 used
//13 used

SdFat sd;
SdFile file;

unsigned long lastPressed = 0;
unsigned long pressDelay = 500;

//for creating new alarm
unsigned long newEventLength;
tmElements_t eventLengthTime;
char timePeriods[4][9] = {"", "Minutes ", "Hours ", "Days "};
char newAlarmName[25];
byte insertionPoint = 0;
boolean makingEvent = false;
boolean removingEvent = false;
byte stepNum = 0;


bool firstAlarmInstance = true;
byte totalNumFiles=0;

//removing alarm
byte pageNum = 0;
int8_t prevPageNum = -1;
const byte dispPerPage = 7;
byte fileSelection = 0;
bool queryDeletion=false;
byte numOnPage=0;
char fileSelected[30];

//line is temporary text holder for load file and delete
char line[30];


void loadFile() {

  int n;
  time_t lowestTime = 0 - 1;
  totalNumFiles=0;

  // Open next file in root.  The volume working directory, vwd, is root.
  // Warning, openNext starts at the current position of sd.vwd() so a
  // rewind may be neccessary in your application.
  sd.vwd()->rewind();
  while (file.openNext(sd.vwd(), O_READ)) {
    file.getName(line, 29);
    if (!file.isDir()) {
      totalNumFiles++;

      /*      if (!file.isOpen()) {
      Serial.println("No File Open");
    }*/
    file.fgets(line, sizeof(line));

    time_t tempLowest = atol(line);


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
      asdf = NextAlarm(lowestTime, hold, until, later, line);
    }
    else {
      //        Serial.println("this was not lower");
    }

  }

  file.close();
}
}

void makeNewFile() {
  if (sd.exists(asdf.getFileName())) {
    if (sd.remove(asdf.getFileName()))
    {
      //    Serial.println("File " + String(asdf.getFileName() ) + " removed");
    }
  }
  else {
    //    Serial.println("File " + String(asdf.getFileName()) + " file doesn't exist");
  }

  asdf.advanceToNext();

  File dataFile = sd.open(asdf.getFileName(), FILE_WRITE);
  if (dataFile) {
    dataFile.println(asdf.toString());
    //  Serial.println("File created");
  } else {
    //    Serial.print("can't ");
    //    Serial.println(asdf.getFileName());

  }
  dataFile.close();
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.set400kHz();
  oled.setFont(Adafruit5x7);
  oled.clear();

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  /*  if (timeStatus() != timeSet)
  Serial.println("Cant sync RTC");
  else
  Serial.println("RTC time set");
  */

  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }


  pinMode(leftDir, INPUT);
  pinMode(upDir, INPUT);
  pinMode(downDir, INPUT);
  pinMode(rightDir, INPUT);
  pinMode(sel, INPUT);
  pinMode(ret, INPUT);
  pinMode(LED, OUTPUT);

  newAlarmName[0] = 'A';
  eventLengthTime.Day = 1;
  eventLengthTime.Month = 1;

  loadFile();
  if(totalNumFiles==0){
    asdf=NextAlarm(30,"No alarms present");
  }

}


void loop() {

  if (makingEvent) {
    newEvent();
  }
  else if (removingEvent) {
    removeEvent();
  }

  else {
    t = now();

    oled.home();
    oled.print(hourFormat12(t));
    oled.print(":");
    appendDigit(minute(t));

    oled.print(":");
    appendDigit(second(t));

    oled.clearToEOL();

    oled.setCursor(0, 1);
    oled.print("Alarm ");
    oled.print(hourFormat12(asdf.getNextAlarm()));
    oled.print(":");
    appendDigit(minute(asdf.getNextAlarm()));
    oled.print(":");
    appendDigit(second(asdf.getNextAlarm()));
    oled.clearToEOL();
    oled.println();

    digitalWrite(LED, false);

    if (asdf.alarmReached()) {
      if (firstAlarmInstance) {
        oled.clear(0, oled.displayHeight(), oled.row() + 1, oled.displayRows());
        firstAlarmInstance = false;
      }
      digitalWrite(LED, true);
      oled.setCursor(0, 3);

      oled.print(asdf.toString());

    }
    else {
      oled.clear(0, oled.displayHeight(), oled.row(), oled.displayRows());


    }


    if (!pressedRecent()) {

      if (digitalRead(rightDir)) {
        lastPressed = millis();

        if (asdf.alarmReached()) {
          makeNewFile();

          loadFile();
          firstAlarmInstance = true;
        }
      }
      if (digitalRead(sel)) {
        lastPressed = millis();
        makingEvent = true;
        oled.clear();
        oled.println("Enter name");
        newEvent();
      }
      if (digitalRead(ret)) {
        lastPressed = millis();
        removingEvent = true;
        oled.clear();
        removeEvent();
      }
    }
  }

}



void newEvent() {
  //creation of a new alarm
  if (!pressedRecent()) {
    if (digitalRead(ret)) {
      leaveCreation();
      lastPressed = millis();
      return;
    }
    else if (stepNum == 0) {
      ///inputting name for new alarm
      if (digitalRead(upDir)) {
        lastPressed = millis();

        if (newAlarmName[insertionPoint] > 32)
        newAlarmName[insertionPoint] -= 1;
      }
      else if (digitalRead(downDir)) {
        lastPressed = millis();

        if (newAlarmName[insertionPoint] < 126)
        newAlarmName[insertionPoint] += 1;

      }

      if (digitalRead(rightDir)) {
        lastPressed = millis();

        if (insertionPoint < 9)
        insertionPoint++;
        if (newAlarmName[insertionPoint] == '\0') {
          newAlarmName[insertionPoint] = 'A';
          newAlarmName[insertionPoint + 1] = '\0';
        }
      }
      else if (digitalRead(leftDir)) {
        lastPressed = millis();

        if (insertionPoint > 0)
        insertionPoint--;
        if (newAlarmName[insertionPoint] == '\0')
        newAlarmName[insertionPoint] = 'A';
      }


      if (digitalRead(sel)) {
        lastPressed = millis();

        oled.println();
        insertionPoint = 0;
        stepNum++;

      }

    }
    else if (stepNum >= 1) {
      //inputting numbers for length of time
      if (digitalRead(upDir)) {
        lastPressed = millis();

        newEventLength++;
      }
      else if (digitalRead(downDir)) {
        lastPressed = millis();
        newEventLength--;

      }

      if (digitalRead(rightDir)) {
        lastPressed = millis();
        /*
        if (insertionPoint < 9)
        insertionPoint++;
        if (line[insertionPoint] == '\0')
        line[insertionPoint] = 'A';*/
      }
      else if (digitalRead(leftDir)) {
        lastPressed = millis();
        /*
        if (insertionPoint > 0)
        insertionPoint--;
        if (line[insertionPoint] == '\0')
        line[insertionPoint] = 'A';*/
      }


      if (digitalRead(sel)) {
        oled.println();
        lastPressed = millis();
        insertionPoint = 0;

        if (stepNum == 1)
        eventLengthTime.Minute += newEventLength;
        else if (stepNum == 2)
        eventLengthTime.Hour += newEventLength;
        else if (stepNum == 3)
        eventLengthTime.Day += newEventLength;

        stepNum++;
        newEventLength = 0;
      }
    }
  }

  if (stepNum == 0) {
    //prints name
    for (int i = 0; i < strlen(newAlarmName); i++) {
      if (i == insertionPoint) {
        oled.setInvertMode(true);
      }
      oled.print(newAlarmName[i]);
      oled.setInvertMode(false);
    }
    oled.clearToEOL();
    oled.setCol(0);
  }
  else if (stepNum >= 1) {
    //prints numbers
    oled.print(timePeriods[stepNum]);
    oled.print(newEventLength);
    oled.clearToEOL();
    oled.setCol(0);
    /*
    Serial.print(makeTime(eventLengthTime));
    Serial.print(" ");
    Serial.println(stepNum);
    */
  }
  if (stepNum == 4) {
    //finishes up creation of alarm
    oled.println("Finished");
    asdf = NextAlarm(makeTime(eventLengthTime), newAlarmName);
    makeNewFile();
    loadFile();
    delay(2000);
    leaveCreation();
    return;

  }

}
void leaveCreation() {
  //resets all previous names/times
  memset(newAlarmName, 0, sizeof newAlarmName);
  newAlarmName[0] = 'A';
  oled.clear();
  makingEvent = false;
  stepNum = 0;
  return;
}



void removeEvent() {

  if(queryDeletion){
    deleteQuery();
  }
  else {
    if (!pressedRecent()) {

      oled.home();

      displayFiles();


      if (digitalRead(leftDir)) {
        lastPressed = millis();
        if (pageNum > 0) {
          pageNum--;
        }
      }
      if (digitalRead(rightDir)) {
        lastPressed = millis();
        if(pageNum<(totalNumFiles-1)/dispPerPage)
        pageNum++;
      }

      if (digitalRead(upDir)) {
        lastPressed = millis();
        if (fileSelection > 0) {
          fileSelection--;
          prevPageNum = -1;
        }
      }
      if (digitalRead(downDir)) {
        lastPressed = millis();
        if (fileSelection < numOnPage-1) {
          fileSelection++;
          prevPageNum = -1;
        }
      }

      if(digitalRead(sel)){
        lastPressed=millis();
        oled.clear();
        oled.home();
        oled.println("Delete?");
        oled.println(fileSelected);
        queryDeletion=true;
      }

      if (digitalRead(ret)) {
        lastPressed = millis();
        leaveDeletion();
      }
    }
  }
}



void deleteQuery(){

  if(!pressedRecent())
  if(digitalRead(sel)){
    lastPressed=millis();
    if(sd.remove(fileSelected)){
      oled.println("File deleted");
    }
    else{
      oled.println("Something went wrong");
    }
    loadFile();
    delay(2000);
    leaveDeletion();
  }
  if(digitalRead(ret)){
    lastPressed=millis();
    leaveDeletion();
  }
}


void displayFiles() {
  if (pageNum != prevPageNum) {
    oled.clear();

    numOnPage=0;
    sd.vwd()->rewind();
    for (int i = 0; file.openNext(sd.vwd(), O_READ) || i>((pageNum+1)*dispPerPage); )
    {
      if (oled.row() == fileSelection%dispPerPage) {
        oled.setInvertMode(true);
        file.getName(fileSelected, 25);
      }

      if (!file.isDir()) {
        //object is not a directory

        if (i >= (pageNum * dispPerPage) && i < ((pageNum + 1)*dispPerPage)) {
          /*          if (!file.isOpen()) {
          Serial.println("No File Open");
        }*/
        //first 3 lines are useless
        file.fgets(line, sizeof(line));
        file.fgets(line, sizeof(line));
        file.fgets(line, sizeof(line));

        //line has alarm name
        file.fgets(line, sizeof(line));
        oled.print(line);
        numOnPage++;
      }
      i++;
    }
    file.close();
    oled.setInvertMode(false);
  }
  prevPageNum = pageNum;
}
}

void leaveDeletion() {
  removingEvent = false;
  pageNum = 0;
  prevPageNum = -1;
  fileSelection = 0;
  oled.setInvertMode(false);
  oled.clear();
  numOnPage=0;
  queryDeletion=false;
  return;
}

void appendDigit(int number) {
  //appends 0 to 1 digit numbers
  if (number < 10) {
    oled.print("0");
  }
  oled.print(number);
}

bool pressedRecent() {
  //chescks if any buttons were recently pressed
  if ((millis() - lastPressed) > pressDelay )
  return false;
  return true;
}
