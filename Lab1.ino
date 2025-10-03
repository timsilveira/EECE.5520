#include <Keypad.h>  

// Pin out for LEDs and Buzzer//
const int MainDirectionRed    = 2; 
const int MainDirectionYellow = 3;
const int MainDirectionGreen  = 4;
const int CrossDirectionRed   = 6;
const int CrossDirectionYellow= 7;
const int CrossDirectionGreen = 8;
const int BUZZER = 14;

//Display//
const int dataPin  = 13;
const int latchPin = 52;
const int clockPin = 53;
const int First_digit = 9;   // Left to right
const int Second_digit = 10;
const int Third_digit = 11;
const int Fourth_digit = 12;  

//Keypad Mapping// 
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {23,25,27,29};
byte colPins[COLS] = {24,26,22,28};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
byte table[] = {0x5F,0x44,0x9D,0xD5,0xC6,0xD3,0xDB,0x45,0xDF,0xC7,0x00};
byte controlDigits[] = {Fourth_digit,Third_digit,Second_digit,First_digit};
byte displayDigits[4] = {10,10,10,10}; //Cleared

//Initialization of time for LEDs//
int MainRedInitialTime   = 0;
int MainGreenInitialTime = 0;
const int YellowConstantTime = 3;

//States//
enum State {MainDirectionRedLED, MainDirectionYellowLED, MainDirectionGreenLED, CrossDirectionYellowLED};
State state = MainDirectionRedLED;
bool running = false;
unsigned long LastState = 0;

//Controlling display//
void setDisplayNumber(int num){
  if(num<0) num=0;
  int d4 = num%10; //1s place
  int d3 = (num/10)%10;
  int d2 = (num/100)%10;
  int d1 = (num/1000)%10;
  displayDigits[0]= (num>0||d4>0)?d4:10; // display 1s digit if greater than 0 otherwise empty
  displayDigits[1]= (num>=10)?d3:10; //display 10s digit if greater than 10 otherwise empty 
  displayDigits[2]= (num>=100)?d2:10; 
  displayDigits[3]= (num>=1000)?d1:10; //Same logic as above
}

void displaySegments(){
  for(int x=0;x<4;x++){ // R --> L
    for(int i=0;i<4;i++) digitalWrite(controlDigits[i], LOW); //Set all digits off
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, (displayDigits[x]==10)?0x00:table[displayDigits[x]]); //Send data for current digit x update all at once MSB is processed 1st 
    digitalWrite(latchPin, HIGH);
    digitalWrite(controlDigits[x], HIGH); //Light current digit
    delayMicroseconds(1000); //hold digit lit
  }
  for(int i=0;i<4;i++) digitalWrite(controlDigits[i], LOW); //turn off cycle again
}

//Assembly//
extern "C" void Toggling(void);       
extern volatile uint8_t blink; //blinking control via assembly

// Blink & Buzz//
void BlinkingwBuzzer(int mainPin, int crossPin, unsigned long &LastBlink, unsigned long now, long remaining){
  if(remaining <= 3000){ //Last 3s
    if(now - LastBlink >= 500){ Toggling(); LastBlink = now; } //.5 s 
    digitalWrite(mainPin, blink ? HIGH : LOW);
    digitalWrite(crossPin, blink ? HIGH : LOW);
    digitalWrite(BUZZER, blink ? HIGH : LOW);
  } else {
    digitalWrite(mainPin, HIGH);
    digitalWrite(crossPin, HIGH);
    digitalWrite(BUZZER, LOW);
  }
}

//General Setup//
void setup() {
  Serial.begin(9600);
  pinMode(MainDirectionRed, OUTPUT); pinMode(MainDirectionYellow, OUTPUT); pinMode(MainDirectionGreen, OUTPUT);
  pinMode(CrossDirectionRed, OUTPUT); pinMode(CrossDirectionYellow, OUTPUT); pinMode(CrossDirectionGreen, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(latchPin, OUTPUT); pinMode(clockPin, OUTPUT); pinMode(dataPin, OUTPUT);
  for(int i=0;i<4;i++){ pinMode(controlDigits[i], OUTPUT); digitalWrite(controlDigits[i], LOW); }
}

//Keypad Input Loop//
void loop() {
  displaySegments();
  char key = customKeypad.getKey();
  static String inputBuffer = "";
  static bool settingRed = false, settingGreen = false;

  if(key){
    if(key=='A'){ 
      inputBuffer=""; settingRed=true; settingGreen=false; 
    }
    else if(key=='B'){ 
      inputBuffer=""; settingGreen=true; settingRed=false; 
    }
    else if(key=='#'){
      if(settingRed && inputBuffer.length()>0){ 
        MainRedInitialTime=inputBuffer.toInt(); 
        Serial.print("A "); Serial.print(MainRedInitialTime); Serial.println(" Seconds");
      }
      if(settingGreen && inputBuffer.length()>0){ 
        MainGreenInitialTime=inputBuffer.toInt(); 
        Serial.print("B "); Serial.print(MainGreenInitialTime); Serial.println(" Seconds");
      }
      inputBuffer=""; settingRed=false; settingGreen=false;
    }
    else if(key=='*'){
      if(!running && MainRedInitialTime>0 && MainGreenInitialTime>0){ 
        running=true; state=MainDirectionRedLED; LastState=millis(); 
        Serial.println("Running");
      } else if(running){ 
        running=false; setDisplayNumber(0); 
        Serial.println("Stopped");
      }
    }
    else{ if(settingRed || settingGreen) inputBuffer+=key; }
  }

 //Initial Blinking//
  static unsigned long LastBlink = 0;
  if(!running){
    if(millis() - LastBlink >= 1000){ Toggling(); LastBlink = millis(); } //1s blink
    digitalWrite(MainDirectionRed, blink ? HIGH : LOW);
    digitalWrite(MainDirectionYellow, LOW); digitalWrite(MainDirectionGreen, LOW);
    digitalWrite(CrossDirectionRed, LOW); digitalWrite(CrossDirectionYellow, LOW); digitalWrite(CrossDirectionGreen, LOW);
    digitalWrite(BUZZER, LOW);
    return;
  }

  //Switches for controlling light//
  unsigned long now = millis();
  unsigned long elapsed = now - LastState;
  long remaining;

  switch(state){
    case MainDirectionRedLED:
      remaining = MainRedInitialTime*1000 - elapsed;
      setDisplayNumber(remaining/1000);
      BlinkingwBuzzer(MainDirectionRed, CrossDirectionGreen, LastBlink, now, remaining);
      digitalWrite(MainDirectionYellow, LOW); digitalWrite(CrossDirectionRed, LOW);
      digitalWrite(MainDirectionGreen, LOW); digitalWrite(CrossDirectionYellow, LOW);
      if(elapsed >= MainRedInitialTime*1000){ state=MainDirectionYellowLED; LastState=now; digitalWrite(BUZZER, LOW);}
      break;

    case MainDirectionYellowLED:
      setDisplayNumber(YellowConstantTime - (elapsed/1000));
      digitalWrite(MainDirectionYellow, HIGH); digitalWrite(CrossDirectionYellow, HIGH);
      digitalWrite(MainDirectionRed, LOW); digitalWrite(CrossDirectionGreen, LOW);
      digitalWrite(MainDirectionGreen, LOW); digitalWrite(CrossDirectionRed, LOW);
      digitalWrite(BUZZER, LOW);
      if(elapsed >= YellowConstantTime*1000){ state=MainDirectionGreenLED; LastState=now; }
      break;

    case MainDirectionGreenLED:
      remaining = MainGreenInitialTime*1000 - elapsed;
      setDisplayNumber(remaining/1000);
      BlinkingwBuzzer(MainDirectionGreen, CrossDirectionRed, LastBlink, now, remaining);
      digitalWrite(MainDirectionRed, LOW); digitalWrite(MainDirectionYellow, LOW);
      digitalWrite(CrossDirectionGreen, LOW); digitalWrite(CrossDirectionYellow, LOW);
      if(elapsed >= MainGreenInitialTime*1000){ state=CrossDirectionYellowLED; LastState=now; digitalWrite(BUZZER, LOW);}
      break;

    case CrossDirectionYellowLED:
      setDisplayNumber(YellowConstantTime - (elapsed/1000));
      digitalWrite(MainDirectionYellow, HIGH); digitalWrite(CrossDirectionYellow, HIGH);
      digitalWrite(MainDirectionRed, LOW); digitalWrite(CrossDirectionGreen, LOW);
      digitalWrite(MainDirectionGreen, LOW); digitalWrite(CrossDirectionRed, LOW);
      digitalWrite(BUZZER, LOW);
      if(elapsed >= YellowConstantTime*1000){ state=MainDirectionRedLED; LastState=now; }
      break;
  }
}
