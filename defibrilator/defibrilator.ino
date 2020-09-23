#include <EEPROM.h>
#include <Bounce2.h>

Bounce debButton = Bounce();
Bounce debMagnet = Bounce();
#define G3 1568
#define G4 3136

#define pinButton 8
#define pinMagnet 7
#define pinVibro1 9
#define pinVibro2 10

#define pinLedRed1 2 
#define pinLedRed2 3
#define pinLedRed3 4
#define pinLedRed4 12
#define pinLedGreen 5

#define LED_LENGTH 4

int redLeds[LED_LENGTH] = {pinLedRed1, pinLedRed2, pinLedRed3, pinLedRed4}; 
int SECOND  = 1000;
long MINUTES = SECOND * 60;
long CHARGE_TIME  = 5* SECOND;
long CHARGE_INTERVAL   = CHARGE_TIME /LED_LENGTH;

long SAVED_CHARGE_TIME;

#define  pinSound 13

const int SOUND_LENGTH = 200;
const int SOUND_BASE_FREQ = 1174;

String state;

bool blocked=false;


void setup() {
  Serial.begin(9600);

  pinMode(pinLedRed1, OUTPUT);
  pinMode(pinLedRed2, OUTPUT);
  pinMode(pinLedRed3, OUTPUT);
  pinMode(pinLedRed4, OUTPUT);
  pinMode(pinLedGreen, OUTPUT);

  pinMode(pinSound, OUTPUT);

  pinMode(pinButton, INPUT_PULLUP);
  pinMode(pinMagnet, INPUT_PULLUP);
  pinMode(pinVibro1, OUTPUT);
  pinMode(pinVibro2, OUTPUT);
  
  debButton.attach(pinButton);
  debButton.interval(5);

  debMagnet.attach(pinMagnet);
  debMagnet.interval(5);

  SAVED_CHARGE_TIME = EEPROM.read(0); 

  
  if(SAVED_CHARGE_TIME != 255){
    CHARGE_TIME  = SAVED_CHARGE_TIME* MINUTES;
    CHARGE_INTERVAL   = CHARGE_TIME /LED_LENGTH;
  }
 
  state="charge";
  Serial.println("Start");
}

void loop() {
  debButton .update();
  debMagnet.update();
  changeState();  
}

void changeState(){
  if (blocked) return;
  
  if(state=="charge"){
    charge();
  }else if (state == "charged"){
    charged();
  }else if (state == "waitForReleaseMagnet"){
    waitForReleaseMagnet();
  }else if (state == "waitForPressButton"){
     waitForPressButton();
  }else if (state == "waitForReleaseButton"){
    waitForReleaseButton();
  }else if (state == "discharge"){
     discharge();
  } else if(state == "postSetup"){
    postSetup(); 
  }
}


int pressedTime=0;  
void postSetup(){
//    
//    if(BUTTON_RESET.changed() && !BUTTON_RESET.read()){
//      Serial.println("++");
//        tone(PIN_SOUND, G3, 100);
//        pressedTime++;
//    }
//    if(BUTTON_GREEN.changed() && !BUTTON_GREEN.read()){
//      Serial.println("pressedTime>>");
//      Serial.println(pressedTime);
//      if(!pressedTime)  pressedTime= DEFAULT_RETENTION_MIN;
//      RETENTION_TIMOUT = MINUTES * pressedTime;
//      if(pressedTime<=20){
//        for(int i=0; i<pressedTime; i++){
//              delay(200);
//              tone(PIN_SOUND, G3, 200);
//              digitalWrite(PIN_LED_GREEN, HIGH);
//              delay(200);
//              digitalWrite(PIN_LED_GREEN, LOW);
//            }
//      }else{
//        tone(PIN_SOUND, G3, 800);
//        digitalWrite(PIN_LED_GREEN, HIGH);
//        delay(400);
//        digitalWrite(PIN_LED_GREEN, LOW);
//        
//        digitalWrite(PIN_LED_RED, HIGH);
//        delay(400);
//        digitalWrite(PIN_LED_RED, LOW);
//        
//        digitalWrite(PIN_LED_GREEN, HIGH);
//        delay(400);
//        digitalWrite(PIN_LED_GREEN, LOW);
//      }
//      
//      
      state="charge";
//    }
}

unsigned long chargeTimestamp=0;
int ledToActivateIndex = 0;
void charge(){
   unsigned long currentTime=millis();
   
   if ((currentTime - chargeTimestamp) > CHARGE_INTERVAL){      
      chargeTimestamp = currentTime;
      digitalWrite(redLeds[ledToActivateIndex++], HIGH);
      tone(pinSound, SOUND_BASE_FREQ, SOUND_LENGTH); 
      
      if (!redLeds[ledToActivateIndex]){
        ledToActivateIndex=0;
        chargeTimestamp=0;
        _magnetIsDisactivated();
        state = "charged";
        Serial.println("Im charged");
      }
   }
}

int touchCount=0;
void charged() {
  // need to activate magnet 
  const int TOUCH_LIMIT=6;
  if(debMagnet.changed() && !debMagnet.read()){
    ++touchCount;
  }
  if(touchCount>TOUCH_LIMIT){
    touchCount=0;
    _magnetIsActivated();
    Serial.println("charged -> waitForReleaseMagnet");
    state = "waitForReleaseMagnet";
  }
}

void waitForReleaseMagnet(){
  if(_longSignal(HIGH, SECOND*0.4, debMagnet)){
    Serial.println("waitForReleaseMagnet -> waitForPressButton");
    state = "waitForPressButton";
  }
}


unsigned long timeoutStartTime = 0;
void waitForPressButton(){
  long TIMEOUT = 5*SECOND;
  if(debButton.changed() && !debButton.read()){
    Serial.println("waitForPressButton-> waitForReleaseButton");
    timeoutStartTime=0;
    state = "waitForReleaseButton";
  }else{
    unsigned long currentTime=millis();
    if(!timeoutStartTime) timeoutStartTime = currentTime;
    if((currentTime - timeoutStartTime)>TIMEOUT){
      Serial.println("Wait for press timeout!");
      _magnetIsDisactivated();
      timeoutStartTime=0;
      state = "charged";
    }
  }
}

void waitForReleaseButton(){
  if(debButton.changed() && debButton.read()){
    Serial.println("waitForReleaseButton -> discharge");
    state = "discharge";
  }
}


void discharge(){
  Serial.println("Discharge");
  const int DELAY_LENGTH = 1000;
  
  digitalWrite(pinLedGreen, LOW);
  digitalWrite(pinVibro1, HIGH);
  digitalWrite(pinVibro2, HIGH);
  
  tone(pinSound,G4);
  delay(800);
  noTone(pinSound);

  digitalWrite(pinVibro1, LOW);
  digitalWrite(pinVibro2, LOW);
  
  for(int i=G4; i>=0; i-=20){
      tone(pinSound,i);
      delay(4);
      noTone(pinSound);
  }
  
  delay(2000);

  state = "charge";
  Serial.println("I'm discharged!");
}


void _magnetIsDisactivated() {
  tone(pinSound, G3, SECOND);

  for(int i=0; i<3; i++){
    _clearAllLeds();
    delay(200);
    _allLedsRed();
    if (i<2) delay(200); 
  }
}


void _magnetIsActivated() {

  for(int i=0; i<3; i++){
    tone(pinSound, G3, 100);
    _clearAllLeds();
    delay(100);
    _allLedsGreen();
    if (i<2) delay(100); 
  }
}

void _allLedsGreen(){
  digitalWrite(pinLedRed1, LOW);
  digitalWrite(pinLedRed2, LOW);
  digitalWrite(pinLedRed3, LOW);
  digitalWrite(pinLedRed4, LOW);
  digitalWrite(pinLedGreen, HIGH);
}

void _allLedsRed(){
  digitalWrite(pinLedGreen, LOW);
  digitalWrite(pinLedRed1, HIGH);
  digitalWrite(pinLedRed2, HIGH);
  digitalWrite(pinLedRed3, HIGH);
  digitalWrite(pinLedRed4, HIGH);
}

void _clearAllLeds(){
  digitalWrite(pinLedGreen, LOW);
  digitalWrite(pinLedRed1, LOW);
  digitalWrite(pinLedRed2, LOW);
  digitalWrite(pinLedRed3, LOW);
  digitalWrite(pinLedRed4, LOW);
}

unsigned long startTime=0;
bool _longSignal(int expectedValue, long timeout, Bounce debouncer){
  debouncer.update();
  if(debouncer.read()==expectedValue){
    unsigned long currentTime=millis();
    if(!startTime) startTime = currentTime;
    if((currentTime - startTime)>timeout){
      Serial.println("_longSignal!");
      Serial.println(currentTime - startTime);
      startTime=0;
      return true;
    }
  }else{
   startTime=0; 
  }  
  return false;
}
