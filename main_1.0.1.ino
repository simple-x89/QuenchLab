/*
  ----------------Quench Lab v1.0.1-----------------------------------
  Keith Harris  2018
  purpose of firmware is to output sensor values to display and control stepper motors.
  the UTFT and URTouch libraries found at http://www.rinkydinkelectronics.com are used heavily
  to provide output to LCD and touchscreen capability. Future versions may use the SD standard
  library to record sensor data to an SD card.

  -----------------color scheme-------------------------------------
  adobe kuler theme:Dach3rColor
  background: (247,247,247);
  buttons: (0,96,113);
  graph body: (232,12,36);
  graph point, font: (38,38,38);
  button highlight: (0,123,166);

  ------------------ChangeLog---------------------------------------
  V1.0.1(9.21.2018):
  Deleted unnecessary code. Tweaked stepperProfile() positioning, and added setHome() to the end of the cycleScreen loop, this should prevent carrage drift over multiple tests. 
  Added flow and temp sensor capability.
*/

//#include <SD.h>
#include <EEPROM.h>
#include <stdlib.h>
#include <UTFT.h>
#include <URTouch.h>
#include <Adafruit_MAX31856.h>
#include <AccelStepper.h>

///////////////////Calling TFT Objects
UTFT myGLCD(TFT01_50, 38, 39, 40, 41); //Parameters should be adjusted to your Display/Schield model
URTouch myTouch( 6, 5, 4, 3, 2);

///////////////////Calling thermocouple objects
Adafruit_MAX31856 max = Adafruit_MAX31856(8, 9, 10, 11); //setting SPI for thermocouple (CS, DI, DO, CLK)

///////////////////Calling Stepper object
AccelStepper stepper(1, 14, 15);

///////////////////Global Variables
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
float pressureRaw;
int pressureMap;
int pressureSensorPin = A0;
int tempRaw;
int flow = 66;
const int homePin = A2;
int homePosition = 0;
int homeStatus = 0;
int homeState = digitalRead(homePin);
double cycleTime, cycleSpeed;
int x, y; // global int for touchscreen coordinates
char currentScreen;
String fversion = "1.0.1";

void setup() {// Initial setup
  pinMode(homePin, INPUT_PULLUP);//setting limit switch pin
  pinMode(pressureSensorPin, INPUT_PULLUP);
  Serial.begin(115200);
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  currentScreen = '0';
  cycleTime = EEPROM.read(0);
  cycleSpeed = EEPROM.read(1);
  max.begin();
  max.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() { // main event loop
  //0 = start-up screen,  1 = home screen, 2 = cycle screen, 3 = monitor screen, 4 = parameter screen
  if (currentScreen == '0') { //start up screen
    startScreen();
    delay(500);
    homingScreen();
    //setHome();
    myGLCD.clrScr();
    homeScreen();
    currentScreen = '1';

  }

  if (currentScreen == '1') { //home screen

    if (myTouch.dataAvailable()) {
      myTouch.read(); // touchscreen buffer
      x = myTouch.getX(); // X coordinate where the screen has been pressed
      y = myTouch.getY(); // Y coordinates where the screen has been pressed
      if ((x >= 40) && (x <= 253) && (y >= 120) && (y <= 370)) {
        highlight(40, 253, 120, 370);
        myGLCD.clrScr();
        cycleScreen(cycleTime, cycleSpeed);
        currentScreen = '2';
      }
      if ((x >= 293) && (x <= 506) && (y >= 120) && (y <= 370)) {
        highlight(293, 506, 120, 370);
        monitorScreen(pressureMap, tempRaw, flow);
        currentScreen = '3';
      }
      if ((x >= 546) && (x <= 759) && (y >= 120) && (y <= 370)) {
        highlight(546, 759, 120, 370);
        myGLCD.clrScr();
        parameterScreen(cycleTime, cycleSpeed);
        currentScreen = '4';
      }
    }
  }

  if (currentScreen == '2') { //cycle screen

    while (currentScreen == '2') {
      tempRaw = max.readThermocoupleTemperature();
      pressureRaw = analogRead(pressureSensorPin);
      cycleScreenRefresh(tempRaw, pressureRaw);
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x = myTouch.getX(); // X coordinate where the screen has been pressed
        y = myTouch.getY(); // Y coordinates where the screen has been pressed

        if ((x >= 20) && (x <= 220) && (y >= 150) && (y <= 350)) { //start button
          highlight(20, 220, 150, 350);
          testingScreen();
          stepperProfile(cycleTime, cycleSpeed);
          cycleScreen(cycleTime, cycleSpeed);
          stepperHome();
          setHome();
          currentScreen == '2';

        }

        if ((x >= 695) && (x <= 765) && (y >= 35) && (y <= 105)) { //back button
          highlight(695, 765, 35, 105);
          currentScreen = '1';
          myGLCD.clrScr();
          homeScreen();
        }
      }
    }
  }

  if (currentScreen == '3') { //monitor screen
    while (currentScreen == '3') {
      Serial.println(tempRaw);
      tempRaw = max.readThermocoupleTemperature();
      pressureRaw = analogRead(pressureSensorPin);
      monitorScreenRefresh(tempRaw, pressureRaw);
      Serial.println(tempRaw);
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x = myTouch.getX(); // X coordinate where the screen has been pressed
        y = myTouch.getY(); // Y coordinates where the screen has been pressed

        if ((x >= 695) && (x <= 765) && (y >= 35) && (y <= 105)) { //back button
          highlight(695, 765, 35, 105);
          currentScreen = '1';
          myGLCD.clrScr();
          homeScreen();
        }
      }
    }
  }

  if (currentScreen == '4') { //parameter screen

    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX(); // X coordinate where the screen has been pressed
      y = myTouch.getY(); // Y coordinates where the screen has been pressed

      if ((x >= 100) && (x <= 230) && (y >= 260) && (y <= 320)) { //Time +

        while (myTouch.dataAvailable() == true) {
          myTouch.read();
          cycleTime = timeMath(cycleTime, '+');
        }
      }

      if ((x >= 100) && (x <= 230) && (y >= 340) && (y <= 400)) { // Time -
        while (myTouch.dataAvailable() == true) {
          myTouch.read();
          cycleTime = timeMath(cycleTime, '-');
        }
      }

      if ((x >= 430) && (x <= 560) && (y >= 260) && (y <= 320)) {// Speed +
        while (myTouch.dataAvailable() == true) {
          myTouch.read();
          cycleSpeed = speedMath(cycleSpeed, '+');
        }
      }

      if ((x >= 430) && (x <= 560) && (y >= 340) && (y <= 400)) { //Speed -
        while (myTouch.dataAvailable() == true) {
          myTouch.read();
          cycleSpeed = speedMath(cycleSpeed, '-');
        }
      }

      if ((x >= 695) && (x <= 765) && (y >= 35) && (y <= 105)) {  //back button
        highlight(695, 765, 35, 105);
        currentScreen = '1';
        EEPROM.write(0, cycleTime);// writes parameters values to non-volatile memory
        EEPROM.write(1, cycleSpeed); // probably should write this to a random value. I'm going to burn out these mem spaces
        myGLCD.clrScr();
        homeScreen();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////class declaration////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class rec {
  public:
    int x1, x2;
    int y1, y2;

    void draw() {
      myGLCD.setColor(255, 61, 46);//button color
      myGLCD.drawRect(x1, y1, x2, y2);
    }
};

class container: public rec {
  public:
    String label;
    int xC, yC, offSet;

    void drawB() {
      xC = x1 - 5 + (x2 - x1) / 2;
      yC = y1 - 5 + (y2 - y1) / 2;
      myGLCD.setColor(0, 96, 113); //button color
      myGLCD.fillRoundRect(x1, y1, x2, y2);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 96, 113); //text background color
      myGLCD.print(label, xC - offSet, yC);
    }
    void drawR() {
      xC = x1 - 5 + (x2 - x1) / 2;
      yC = y1 - 5 + (y2 - y1) / 2;
      myGLCD.setColor(232, 12, 36); //button color
      myGLCD.fillRoundRect(x1, y1, x2, y2);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(232, 12, 36); //text background color
      myGLCD.print(label, xC - offSet, yC);
    }
};

class readOut: public container {
  public:

    int offSet2;
    void drawB(int Data) {
      xC = x1 - 5 + (x2 - x1) / 2;
      yC = y1 - 5 + (y2 - y1) / 2;
      myGLCD.setColor(0, 0, 0); //button color
      myGLCD.drawRect(x1, y1, x2, y2);
      myGLCD.setColor(0, 0, 0);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(247, 247, 247); //text background color
      myGLCD.print(label, xC - offSet2, yC - 55);
      myGLCD.printNumI(Data, xC - offSet, yC, 2, '0');//allows the print of interger with zero padding

    }
    void drawBlank() {
      myGLCD.setColor(0, 0, 0); //button color
      myGLCD.drawRect(x1, y1, x2, y2);
      myGLCD.setColor(0, 0, 0);
    }

    void drawBig(int Data) {
      xC = x1 - 5 + (x2 - x1) / 2;
      yC = y1 - 5 + (y2 - y1) / 2;
      myGLCD.setColor(0, 0, 0); //button color
      myGLCD.drawRect(x1, y1, x2, y2);
      myGLCD.setColor(0, 0, 0);
      myGLCD.setFont(SevenSegNumFont);
      myGLCD.setBackColor(247, 247, 247);
      myGLCD.printNumI(Data, xC - offSet, yC - 20, 3, '0'); //allows the print of interger with zero padding
      myGLCD.setFont(BigFont);
      myGLCD.print(label, xC - offSet2, yC - 55);
    }
};

class graph: public rec { //class for making sick ass graphs
  public:
    void drawBody() {
      myGLCD.setColor(232, 12, 36); //graph color
      myGLCD.fillRoundRect (x1, y1, x2, y2);

    }
    void drawPoint(int y3) {
      myGLCD.setColor(38, 38, 38); //point color
      myGLCD.fillRect (x1, y3, x2, y3 + 5);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////Function declaration/////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void startScreen() { // kick ass startup
  myGLCD.fillScr(2, 2, 2);
  myGLCD.setBackColor(2, 2, 2);
  myGLCD.setFont(BigFont);
  myGLCD.print(fversion, 350, 250);

}

void homingScreen() { // kick ass screen while motors home
  myGLCD.fillScr(2, 2, 2);
  myGLCD.setBackColor(2, 2, 2);
  myGLCD.setFont(BigFont);
  myGLCD.print("homing...", 350, 250);

}

void homeScreen() { // main menu. Three menue items in big colored boxes. Button spacing 40px button width 213px
  myGLCD.fillScr(247, 247, 247);
  container cycleButton;
  cycleButton.x1 = 40;
  cycleButton.x2 = 253;
  cycleButton.y1 = 120;
  cycleButton.y2 = 370;
  cycleButton.offSet = 30;
  cycleButton.label = "CYCLE";
  cycleButton.drawB();

  container monitorButton;
  monitorButton.x1 = 293;
  monitorButton.x2 = 506;
  monitorButton.y1 = 120;
  monitorButton.y2 = 370;
  monitorButton.offSet = 50;
  monitorButton.label = "MONITOR";
  monitorButton.drawB();

  container parameterButton;
  parameterButton.x1 = 546;
  parameterButton.x2 = 759;
  parameterButton.y1 = 120;
  parameterButton.y2 = 370;
  parameterButton.offSet = 75;
  parameterButton.label = "PARAMETERS";
  parameterButton.drawB();
}

void cycleScreen(int ct, int Cs) { //where cycle for tests are started. Has 3 value readouts for Pressure, temp and flow. Also displays user set test parameter cycle speed and cycle time
  myGLCD.fillScr(247, 247, 247);

  timeReadout(ct, 260, 390, 80, 150);
  speedReadout(Cs, 260, 390, 190, 260);

  container startButton;
  startButton.x1 = 20;
  startButton.x2 = 220;
  startButton.y1 = 150;
  startButton.y2 = 350;
  startButton.label = "START";
  startButton.offSet = 40;
  startButton.drawB();

  readOut parRead;
  parRead.x1 = 240;
  parRead.x2 = 675;
  parRead.y1 = 20;
  parRead.y2 = 470;
  parRead.offSet = 20;
  parRead.offSet2 = 60;
  //parRead.label = "PARAMETERS";
  parRead.drawBlank();
  backButton();
}

void monitorScreen(int p, int t, int f) { // a screen to monitor pressure, temp and flow. Has 3 animated graphs with labels value readouts and back button
  myGLCD.fillScr(247, 247, 247);

  //flow
  //////////////////////////////////////////
  graph fGraph;
  fGraph.x1 = 560;
  fGraph.x2 = 620;
  fGraph.y1 = 40;
  fGraph.y2 = 460;
  fGraph.drawBody();
  fGraph.drawPoint(f);

  readOut fRead;
  fRead.x1 = 660;
  fRead.x2 = 720;
  fRead.y1 = 300;
  fRead.y2 = 360;
  fRead.offSet = 20;
  fRead.offSet2 = 30;
  fRead.label = "FLOW";
  fRead.drawB(f);
  //////////////////////////////////////////

  backButton();
}

void monitorScreenRefresh(int t, float p) { // this function rewrites only the variables in the readout and graph with out having to rewrite the entire screen
  int tempF = (t * 1.8) + 32;
  int tempMap = map(tempF, 32, 150, 460, 40); // maps temp values to y coordinates for graph

  //temperature
  //////////////////////////////////////////
  graph tGraph;
  tGraph.x1 = 300;
  tGraph.x2 = 360;
  tGraph.y1 = 40;
  tGraph.y2 = 460;
  tGraph.drawBody();
  tGraph.drawPoint(tempMap);

  readOut tRead;
  tRead.x1 = 400;
  tRead.x2 = 460;
  tRead.y1 = 300;
  tRead.y2 = 360;
  tRead.offSet = 20;
  tRead.offSet2 = 60;
  tRead.label = "TEMPERATURE";
  tRead.drawB(tempF);
  //////////////////////////////////////////

  pressureMap = mapF(p, 0, 1023, 0, 200);
  int pressureGraph = map(pressureRaw, 0, 1023, 460, 40);

  //pressure
  ////////////////////////////////////////
  graph pGraph;
  pGraph.x1 = 40;
  pGraph.x2 = 100;
  pGraph.y1 = 40;
  pGraph.y2 = 460;
  pGraph.drawBody();
  pGraph.drawPoint(pressureGraph);

  readOut pRead;
  pRead.x1 = 140;
  pRead.x2 = 200;
  pRead.y1 = 300;
  pRead.y2 = 360;
  pRead.offSet = 20;
  pRead.offSet2 = 50;
  pRead.label = "PRESSURE";
  pRead.drawB(pressureMap);

  ///////////////////////////////////////////

}

void testingScreen() {
  myGLCD.fillScr(2, 2, 2);
  myGLCD.setBackColor(2, 2, 2);
  myGLCD.setFont(BigFont);
  myGLCD.print("TESTING...", 350, 250);
}

void cycleScreenRefresh(int t, float p) {
  int tempF = (t * 1.8) + 32;
  pressureReadout(p, 260, 340, 300, 360);
  tempReadout(t, 260, 340, 400, 460);
}

void pressureReadout(int p, int x1, int x2, int y1, int y2) {

  pressureMap = mapF(p, 0, 1023, 0, 200);
  readOut pRead;
  pRead.x1 = x1;
  pRead.x2 = x2;
  pRead.y1 = y1;
  pRead.y2 = y2;
  pRead.offSet = 20;
  pRead.offSet2 = 50;
  pRead.label = "PRESSURE (PSI)";
  pRead.drawB(pressureMap);

}

void tempReadout(int t, int x1, int x2, int y1, int y2) {
  int tempF = (t * 1.8) + 32;
  readOut tRead;
  tRead.x1 = x1;
  tRead.x2 = x2;
  tRead.y1 = y1;
  tRead.y2 = y2;
  tRead.offSet = 20;
  tRead.offSet2 = 50;
  tRead.label = "TEMPERATURE(F)";
  tRead.drawB(tempF);
}

void parameterScreen(int t, int s) { // screeen to change test parameters (time, and speed). Parameters get saved to eeprom. Possibly add more in the future (frequency???)
  myGLCD.fillScr(247, 247, 247);

  //TIME
  //////////////////////////////////////////
  timeReadout(t, 100, 230, 150, 220);

  container timePlus;
  timePlus.x1 = 100;
  timePlus.x2 = 230;
  timePlus.y1 = 260;
  timePlus.y2 = 320;
  timePlus.offSet = 5;
  timePlus.label = "+";
  timePlus.drawR();

  container timeMinus;
  timeMinus.x1 = 100;
  timeMinus.x2 = 230;
  timeMinus.y1 = 340;
  timeMinus.y2 = 400;
  timeMinus.offSet = 5;
  timeMinus.label = "-";
  timeMinus.drawR();

  //SPEED
  //////////////////////////////////////////
  speedReadout(s, 430, 560, 150, 220);

  container speedPlus;
  speedPlus.x1 = 430;
  speedPlus.x2 = 560;
  speedPlus.y1 = 260;
  speedPlus.y2 = 320;
  speedPlus.offSet = 5;
  speedPlus.label = "+";
  speedPlus.drawR();

  container speedMinus;
  speedMinus.x1 = 430;
  speedMinus.x2 = 560;
  speedMinus.y1 = 340;
  speedMinus.y2 = 400;
  speedMinus.offSet = 5;
  speedMinus.label = "-";
  speedMinus.drawR();

  backButton();
}

void backButton() { // menu navigation back button
  container backButton;
  backButton.x1 = 695;
  backButton.y1 = 35;
  backButton.x2 = 765;
  backButton.y2 = 105;
  backButton.offSet = 28;
  backButton.label = "BACK";
  backButton.drawB();
}

void timeReadout(int t, int x1, int x2, int y1, int y2) {//parameter screen  readouts for time
  readOut cTime;
  cTime.x1 = x1;
  cTime.x2 = x2;
  cTime.y1 = y1;
  cTime.y2 = y2;
  cTime.offSet = 50;
  cTime.offSet2 = 70;
  cTime.label = "CYCLE TIME (s)";
  cTime.drawBig(t);
}

void speedReadout(int s, int x1, int x2, int y1, int y2) { //parameter screen  readouts for speed
  readOut cSpeed;
  cSpeed.x1 = x1;
  cSpeed.x2 = x2;
  cSpeed.y1 = y1;
  cSpeed.y2 = y2;
  cSpeed.offSet = 50;
  cSpeed.offSet2 = 80;
  cSpeed.label = "CYCLE SPEED (ft/min)";
  cSpeed.drawBig(s);
}

void highlight(int x1, int x2, int y1, int y2) { // function highlights buttons when pressed
  myGLCD.setColor(232, 12, 36);
  myGLCD.drawRoundRect (x1 - 2, y1 - 2, x2 + 2, y2 + 2);
  while (myTouch.dataAvailable()) {
    myTouch.read();
    myGLCD.setColor(0, 123, 166);
    myGLCD.drawRoundRect (x1 - 2, y1 - 2, x2 + 2, y2 + 2);
  }
}

int timeMath (int timeCounter, char function) { // the time + and - buttons on the parameter page
  while (myTouch.dataAvailable() == true) {
    myTouch.read();

    if (function == '+') { //time +
      timeCounter ++;
      timeReadout(timeCounter, 100, 230, 150, 220);
    }
    if (function == '-') { //time -
      timeCounter --;
      timeReadout(timeCounter, 100, 230, 150, 220);
    }
    return timeCounter;
  }
}

int speedMath (int speedCounter, char function) { // the speed + and - buttons on the parameter page
  while (myTouch.dataAvailable() == true) {
    myTouch.read();

    if (function == '+') { //speed +
      speedCounter ++;
      speedReadout(speedCounter, 430, 560, 150, 220);
    }

    if (function == '-') { //Speed -
      speedCounter --;
      speedReadout(speedCounter, 430, 560, 150, 220);
    }
    return speedCounter;
  }
}

float mapF(float x, float in_min, float in_max, float out_min, float out_max) { // Custom function that maps float values (AVR compiler builtin map function doesnt accept floats)
  float result;
  result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return result;
}

int setHome() {// sets the 0 location for the motor. Walks motor back until limit switch is hit and then writes this as the 0 point

  stepper.setMaxSpeed(18000.0);
  stepper.setAcceleration(5000.0);
  if (homeState == 1) {

    stepper.runToNewPosition(-1000);
    stepper.runToNewPosition(5);

    while (homeState == 1) {

      homeState = digitalRead(homePin);

      stepper.runToNewPosition(homePosition);
      homePosition = homePosition + 20; // Decrease by 10 for next move if needed
      stepper.run();  // Start moving the stepper

    }
  }
  if (homeState == 0) {
    stepper.setCurrentPosition(0);
    stepper.runToNewPosition(-400);
    delay(800);
    stepper.runToNewPosition(0);
    delay(800);
    stepper.runToNewPosition(-100);
    delay(800);
    stepper.setCurrentPosition(0);


  }
  return 1;
}

void stepperProfile(double cT, double cS) { // translates user speed input inch/minute to motor steps
  long cycleTimeMS = 1000L * cT;
  float cycleSpeedSTP = ((cS * 25600L * 12L) / (60L * 256L)); //converting inch/min to steps/ms
  stepper.setMaxSpeed(30000.0);
  stepper.setAcceleration(20000); //jerk. no not you. this part actually jerks the sample to get it into the sprays as quickly as possible
  stepper.runToNewPosition(-7000);

  long currentTime = millis();
  long startTime = 0;



  while (startTime - currentTime <= cycleTimeMS) { //run loop until user define time has been reached

    stepper.setMaxSpeed(cycleSpeedSTP);
    stepper.setAcceleration(15000.0);
    stepper.runToNewPosition(-11000); // old value was -9000 and -7000
    stepper.runToNewPosition(-7000);
    startTime = millis();
  }

}

void stepperHome() { // returns carrage to home position
  stepper.setMaxSpeed(2000.0);
  stepper.setAcceleration(1000.0);
  stepper.runToNewPosition(15);
}

