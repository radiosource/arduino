
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
 
#define SECOND 1000
#define MINUTES SECOND * 60;
#define CHARGE_TIME  5* SECOND 
#define CHARGE_INTERVAL  CHARGE_TIME /LED_LENGTH

#define  pinSound 6

const int SOUND_LENGTH = 200;
const int SOUND_BASE_FREQ = 1174;

const int CHARGE_FULL_TIME = 5*SECOND; 
const int CHARGE_INTERVAL_TIME = CHARGE_FULL_TIME / (LED_LENGTH-1);

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
  }
}

void charge(){
  Serial.println("Charging");
   tone(pinSound, SOUND_BASE_FREQ, SOUND_LENGTH); 
   digitalWrite(pinLedRed1, HIGH);
   delay(CHARGE_INTERVAL);
   
   digitalWrite(pinLedRed2, HIGH);
   tone(pinSound, SOUND_BASE_FREQ, SOUND_LENGTH); 
   delay(CHARGE_INTERVAL);
   
   digitalWrite(pinLedRed3, HIGH);
   tone(pinSound, SOUND_BASE_FREQ, SOUND_LENGTH);
   delay(CHARGE_INTERVAL);

   digitalWrite(pinLedRed4, HIGH);
  
   tone(pinSound, G3, SECOND);
   
   state = "charged";
   Serial.println("Im charged");
}

void charged() {
  // need to activate magnet 
  if(_longSignal(LOW, SECOND*0.4, debMagnet)){
    Serial.println("charged -> waitForRelease");

    digitalWrite(pinLedRed1, LOW);
    digitalWrite(pinLedRed2, LOW);
    digitalWrite(pinLedRed3, LOW);
    digitalWrite(pinLedRed4, LOW);
    digitalWrite(pinLedGreen, HIGH);
   
    tone(pinSound, G3, 100);
    delay(200);
    tone(pinSound, G3, 100);
    delay(200);
    tone(pinSound, G3, 100);
    delay(200);
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
  if(_longSignal(LOW, SECOND*0.4, debButton)){
    Serial.println("waitForPressButton-> waitForReleaseButton");
    timeoutStartTime=0;
    state = "waitForReleaseButton";
  }else{
    unsigned long currentTime=millis();
    if(!timeoutStartTime) timeoutStartTime = currentTime;
    if((currentTime - timeoutStartTime)>TIMEOUT){
      Serial.println("Wait for press timeout!");
      digitalWrite(pinLedGreen, LOW);
      digitalWrite(pinLedRed1, HIGH);
      digitalWrite(pinLedRed2, HIGH);
      digitalWrite(pinLedRed3, HIGH);
      digitalWrite(pinLedRed4, HIGH);
      timeoutStartTime=0;
      state = "charged";
    }
  }
}

void waitForReleaseButton(){
  if(_longSignal(HIGH, SECOND*0.4, debButton)){
    Serial.println("waitForReleaseButton -> discharge");
    state = "discharge";
  }
}


void discharge(){
  Serial.println("Discharge");
  const int DELAY_LENGTH = 1000;
  delay(200);
  
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
