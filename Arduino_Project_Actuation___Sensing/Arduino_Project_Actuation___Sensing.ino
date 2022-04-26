// Subject: 48623 – Mechatronics 2
// Assessment 2 - Actuation & Sensing
// Student Name: Jeremy Myers
// Student Number: 

// include the library code:
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <avr/io.h>


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
volatile int counter = 0;
// define some values used by the panel and buttons
int lcd_key = 0;
#define btnUP 0
#define btnDOWN 1
#define btnRIGHT 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

#define modeStart 10
#define modeDebug 20
#define modeIR 30
#define modeCM 40
#define modePM 50
#define MOTOR_START 60
int currentMode = 10;

int MOTOR_SPEED = 1;
int MOTOR_DIR = 0;
int MOTOR_ANGLE = 0;
const int STEP_PER_REV = 2038; //full step of 28BYJ-48 Stepper Motor
Stepper myStepper = Stepper(STEP_PER_REV, 13, 11, 12, 3); //Third pin goes in 2nd spot

unsigned long previousCounter = counter;
bool blink = true;
int SELECTION = 1;
int CM_SELECT = 1;

void setup() {
  // TIMER 1 for interrupt frequency 1 Hz:
  cli(); // stop interrupts noInterrupts()
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // Set Compare Match register for 1 Hz increments
  OCR1A = 62499; // = 16000000 / (256 * 1) - 1 (must be <65536)
  // Turn on Clear Timer on Compare Match (CTC) mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 256 prescaler
  TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts interrupts()

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  Serial.begin(9600);
  lcd.setCursor(0,0);
  lcd.print("00:00");
}

ISR(TIMER1_COMPA_vect)        // interrupt service routine 
{
  counter++;
}

void loop() {
  lcd.setCursor(0,1);  
  lcd_key = read_LCD_buttons();
  switch(currentMode) {
    case modeStart:
      startMode(counter);
      delay(250);
      break;
    case modeDebug:
      debugMode();
      delay(250);
      break;
    case modeIR:
      irMode();
      delay(250);
      break;
    case modeCM:
      cmMode();
      delay(250);
      break;
     case MOTOR_START:
      cmStart();
      delay(250);
      break;
    case modePM:
      pmMode();
      delay(250);
      break;
  }
}



//// START MODE ////
void startMode(int counter) {
  printClock(counter);
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("13036379");
  if(read_LCD_buttons() == btnSELECT) {
    currentMode = modeDebug;
    debugMode();
    delay(100);
  }
}

void printClock(int counter){
    int seconds, minutes = 0;
    seconds = counter % 60;
    minutes = counter / 60;
    printMinutes(minutes);
    printSeconds(seconds);
}

void printSeconds(int seconds){
  lcd.setCursor(2,0);
  lcd.print(":");
  if(seconds < 10){
    lcd.print("0");
    lcd.print(seconds);  
  }
  else{
    lcd.print(seconds);
  }
}

void printMinutes(int minutes){
  lcd.setCursor(1,0);
  lcd.print(minutes);
  if(minutes > 10){
    lcd.setCursor(0,0);
    lcd.print(minutes);  
  }
}

/// DEBUG MODE ///
void debugMode(){
  lcd.setCursor(0,0);
  lcd.print("Debug Mode");
  lcd.setCursor(0,1);
  if (counter != previousCounter){
    blink = !blink;
    previousCounter = counter;
  }
  delay(50);//debounce
  
  switch(SELECTION) {
    case 1: 
      if (blink) {
        lcd.print("IR CM PM");
      } else {
        lcd.print("__ CM PM");
      }
      break;
    case 2: 
      if (blink) {
        lcd.print("IR CM PM");
      } else {
        lcd.print("IR __ PM");
      }
      break;
    case 3: 
      if (blink) {
        lcd.print("IR CM PM");
      } else {
        lcd.print("IR CM __");
      }
      break;
  }

  SELECTION = menuScroller(SELECTION, 3);
  if(read_LCD_buttons() == btnLEFT && SELECTION == 1) {
    SELECTION += 2;
    delay(100);
  }

  if(read_LCD_buttons() == btnRIGHT && SELECTION == 3) {
    SELECTION -= 2;
    delay(100);
  }
  if(read_LCD_buttons() == btnSELECT){
    currentMode = menuSelect(currentMode, SELECTION);
  }
}

int menuSelect(int currentMode, int slection){
  //Selecting specific mode
  if(SELECTION == 1) {
    currentMode = modeIR;
    delay(100);
  }

  if(SELECTION == 2) {
    currentMode = modeCM;
    SELECTION -= 1;
    delay(100);
  }
  
  if(SELECTION == 3) {
    currentMode = modePM;
    SELECTION -= 2;
    lcd.clear();
    delay(100);
  }
    
   return currentMode;
}

//// IR MODE ////
void irMode() {
  int val;
  val = analogRead(1);
  delay(500);
  lcd.setCursor(0,0);
  lcd.print("IR Mode    ");
  lcd.setCursor(0,1);
  lcd.print("0.00 mm     ");
  if(distConversion(val) >20 && distConversion(val) < 150){
    lcd.setCursor(0,1);
    lcd.print(distConversion(val));
    lcd.print("cm    ");
  }
  if(distConversion(val) < 20){
    lcd.setCursor(0,1);
    lcd.print("To Close    ");  
  }
  else if(distConversion(val) > 150){
    lcd.setCursor(0,1);
    lcd.print("Out Of Range    ");
  }

  if(read_LCD_buttons() == btnSELECT){
    currentMode = modeDebug;
    lcd.clear();
    delay(100);
  }
}

int distConversion(int value){
    int irDistance;
    float b = -1.001;
    irDistance = 12814 * pow(value, b);
    return irDistance;
}

//// CM MODE ////
void cmMode() {
  lcd.setCursor(0,0);
  lcd.print("CM Mode    ");
  lcd.setCursor(0,1);
  if (counter != previousCounter){
    blink = !blink;
    previousCounter = counter;
  }
  delay(50);//debounce
  
  switch(CM_SELECT) {
    case 1: 
      if (blink) {
        lcd.print("Start Exit");
      } else {
        lcd.print("_____ Exit");
      }
      break;
    case 2: 
      if (blink) {
        lcd.print("Start Exit");
      } else {
        lcd.print("Start ____");
      }
      break;
  }

  CM_SELECT = menuScroller(CM_SELECT, 2);
  if(read_LCD_buttons() == btnLEFT && CM_SELECT == 1){
    CM_SELECT += 1; 
  }
  if(read_LCD_buttons() == btnRIGHT && CM_SELECT == 2){
    CM_SELECT -= 1; 
  }
  //SELECTION
  if(read_LCD_buttons() == btnSELECT && CM_SELECT == 1){
    lcd.clear();
    currentMode = MOTOR_START;
  }
  
  if(read_LCD_buttons() == btnSELECT && CM_SELECT == 2){
    currentMode = modeDebug;
    CM_SELECT -= 1;
    lcd.clear();
    delay(200);
  }
}

//// cmStart ///
void cmStart(){
    lcd.home();
    lcd.print("CM Mode    ");
    switch(MOTOR_DIR) {
    case 0: 
      lcd.setCursor(0,1);
      lcd.print("CW ");
      break;
    case 1: 
      lcd.setCursor(0,1);
      lcd.print("CCW ");
      break;
    }
    
    if(read_LCD_buttons() == btnLEFT && MOTOR_DIR == 0){
      MOTOR_DIR++;
    }
    if(read_LCD_buttons() == btnRIGHT && MOTOR_DIR == 1){
      MOTOR_DIR--;
    }
    
    switch(MOTOR_SPEED){
    case 0:
      //slow
      lcd.setCursor(4,1);
      lcd.print("Slow  ");
      delay(150);
      break;
    case 1:
      //medium
      lcd.setCursor(4,1);
      lcd.print("Medium  ");
      delay(150);
      break;
    case 2:
      //fast
      lcd.setCursor(4,1);
      lcd.print("Fast  ");
      delay(150);
      break;
    }
    if(read_LCD_buttons() == btnUP && MOTOR_SPEED < 3){
      MOTOR_SPEED++;  
    }
    if(read_LCD_buttons() == btnDOWN && MOTOR_SPEED > 0){
      MOTOR_SPEED--;  
    }

    if(read_LCD_buttons() == btnSELECT){
      currentMode = modeCM;
      MOTOR_SPEED = 1;
      MOTOR_DIR = 0;
      lcd.clear();  
    }
    motorStart(MOTOR_SPEED, MOTOR_DIR);
}

void motorStart(int stepperSpeed, int stepperDir){
  int mSpeed;
  switch(stepperSpeed){
    case 0:
      mSpeed = 2;
      break;
    case 1:
      mSpeed = 5;
      break;
    case 2:
      mSpeed = 10;
      break;
  }
  int stepping = 1;
   while(read_LCD_buttons() == btnNONE){
    myStepper.setSpeed(mSpeed);
    myStepper.step(stepping);
    stepping += (getStepDir()*1);
  }
  
}

// To change the direction of rotation
int getStepDir(){
  if(MOTOR_DIR == 0) return 1;
  if(MOTOR_DIR == 1) return -1; 
}

//// PM Mode ////
void pmMode(){
    printDefault();
    if(read_LCD_buttons() == btnUP && MOTOR_ANGLE < 360){
      MOTOR_ANGLE += 15;
      delay(500);
    }
    if(read_LCD_buttons() == btnDOWN && MOTOR_ANGLE > 0){
      MOTOR_ANGLE -= 15;
      delay(500);
    }
    if(read_LCD_buttons() == btnLEFT){
      MOTOR_ANGLE = 0;
      lcd.clear();
      printDefault();
        
    }
    if(read_LCD_buttons() == btnRIGHT){
      startPM(MOTOR_ANGLE);  
      MOTOR_ANGLE = 0;
      lcd.clear();
      printDefault();  
    }
    if(read_LCD_buttons() == btnSELECT){
      currentMode = modeDebug;
      lcd.clear();
    }
}

void startPM(int MOTOR_ANGLE){
    // Eqn: 2038/360 = 5.66 step per degree
    float STEPS_PER_DEGREE = 5.66;
    int angleLeft = MOTOR_ANGLE; 
    bool isStepping = true;
    while(isStepping == true){
      myStepper.setSpeed(3);
      myStepper.step(STEPS_PER_DEGREE);
      if(angleLeft == 0){
        isStepping = false;
      }
      //
      if(angleLeft%1 == 0){
        printValue(angleLeft);
      }
      angleLeft--;
      delay(150);
    }
      lcd.clear();
      printDefault();
}

void printDefault(){
    lcd.setCursor(0,0);
    lcd.print("PM Mode    ");
    lcd.setCursor(0,1);
    lcd.print("SA: ");
    lcd.print(MOTOR_ANGLE);
    lcd.print("  ");
    lcd.setCursor(8,1);
    lcd.print("RA: ");
    lcd.print(MOTOR_ANGLE);
    lcd.print("  ");
}

void printValue(int remainder){
    lcd.setCursor(0,0);
    lcd.print("PM Mode    ");
    lcd.setCursor(0,1);
    lcd.print("SA: ");
    lcd.print(MOTOR_ANGLE);
    lcd.setCursor(8,1);
    lcd.print("RA: ");
    lcd.print(remainder);
    lcd.print("  ");
}

/// Frequently Used Functions ///
int read_LCD_buttons() {
  int adc = analogRead(A0);
  if (adc > 1000) return btnNONE; 
  else if (adc < 50) return btnRIGHT;
  else if (adc < 140) return btnUP;
  else if (adc < 290) return btnDOWN;
  else if (adc < 440) return btnLEFT;
  else if (adc < 690){
    delay(50);
    return btnSELECT; 
  }
  return btnNONE; 
}

int menuScroller(int SELECTION, int maxLen){
  //Scrolling through SELECTIONs
  if(read_LCD_buttons() == btnRIGHT && SELECTION < maxLen) {
    SELECTION++;
    delay(250);
  }
  if(read_LCD_buttons() == btnLEFT && SELECTION > 1) {
    SELECTION--;
    delay(250);
  }
   return SELECTION;
}
