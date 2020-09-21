  
#include <Bounce2.h>
Bounce BUTTON_RED = Bounce();
Bounce BUTTON_GREEN = Bounce();
Bounce BUTTON_RESET = Bounce();
#define G3 1568
#define G4 3136

#define PIN_BUTTON_RED 5
#define PIN_BUTTON_GREEN 4
#define PIN_BUTTON_RESET 7

#define PIN_LED_RED 2
#define PIN_LED_GREEN 3
#define PIN_SOUND 9

#define DEFAULT_RETENTION_MIN 10

long SECOND = 1000;
long MINUTES = SECOND * 60;

unsigned long RETENTION_TIMOUT = DEFAULT_RETENTION_MIN * MINUTES;

String state = "waitForPress";
String activeColor;
int activeLedPin;
 
void setup() {
  Serial.begin(9600); 
  Serial.println("Start");
  pinMode(PIN_BUTTON_RED, INPUT_PULLUP);
  pinMode(PIN_BUTTON_GREEN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);
  
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_SOUND, OUTPUT);
  
  BUTTON_RED.attach(PIN_BUTTON_RED);
  BUTTON_RED.interval(5);

  BUTTON_GREEN.attach(PIN_BUTTON_GREEN);
  BUTTON_GREEN.interval(5);

  BUTTON_RESET.attach(PIN_BUTTON_RESET);
  BUTTON_RESET.interval(25);
  
}

void loop() {
 
  // put your main code here, to run repeatedly:
  BUTTON_RED.update();
  BUTTON_GREEN.update();
  BUTTON_RESET.update();
  changeState();  
}

void changeState(){
  if(state == "waitForPress"){
    waitForPress();
  } else if(state == "retention"){
    retention();
  } else if(state == "win"){
    win(); 
  }else if (state=="postSetup"){
    postSetup();
  }
}

int pressedTime=0;  
void postSetup(){
    
    if(BUTTON_RESET.changed() && !BUTTON_RESET.read()){
      Serial.println("++");
        tone(PIN_SOUND, G3, 100);
        pressedTime++;
    }
    if((BUTTON_GREEN.changed() && !BUTTON_GREEN.read()) || (BUTTON_RED.changed() && !BUTTON_RED.read())){
      Serial.println("pressedTime>>");
      Serial.println(pressedTime);
      if(!pressedTime)  pressedTime= DEFAULT_RETENTION_MIN;
      RETENTION_TIMOUT = MINUTES * pressedTime;

      if(pressedTime<=20){
        for(int i=0; i<pressedTime; i++){
              delay(200);
              tone(PIN_SOUND, G3, 200);
              digitalWrite(PIN_LED_GREEN, HIGH);
              delay(200);
              digitalWrite(PIN_LED_GREEN, LOW);
            }
      }else{
        tone(PIN_SOUND, G3, 800);
        digitalWrite(PIN_LED_GREEN, HIGH);
        delay(400);
        digitalWrite(PIN_LED_GREEN, LOW);
        
        digitalWrite(PIN_LED_RED, HIGH);
        delay(400);
        digitalWrite(PIN_LED_RED, LOW);
        
        digitalWrite(PIN_LED_GREEN, HIGH);
        delay(400);
        digitalWrite(PIN_LED_GREEN, LOW);
      }
      
      state="waitForPress";
    }
}

unsigned long timingLed;
bool played = false;
void win(){
  if(!played){
    tone(PIN_SOUND, G4, 3000);
    played =true;  
  }
  
  if(BUTTON_RESET.changed() && !BUTTON_RESET.read()){
      Serial.println("win -> waitForPress");
      state = "waitForPress";
      played =false;
   }
   unsigned long currentTime = millis();
   if (currentTime - timingLed > 1000){ 
    timingLed = currentTime;
    digitalWrite(activeLedPin, LOW);
   }else if (currentTime - timingLed > 500){
    digitalWrite(activeLedPin, HIGH);
   }
}

bool setuped = false;
unsigned long timing=0;
void waitForPress(){
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  
  if(BUTTON_RED.changed() && !BUTTON_RED.read()){
    digitalWrite(PIN_LED_RED, HIGH);
    activeColor = "red";
    activeLedPin=PIN_LED_RED;
    state = "retention";
    Serial.println("waitForPress->retention red");
    tone(PIN_SOUND, G3, 100);
    delay(200);
    tone(PIN_SOUND, G3, 100);
    delay(200);
    tone(PIN_SOUND, G3, 100);
    delay(200);
    timing=millis();
    
  }else if (BUTTON_GREEN.changed() && !BUTTON_GREEN.read()){
    digitalWrite(PIN_LED_GREEN, HIGH);
    activeLedPin=PIN_LED_GREEN;
    activeColor = "green";
    state = "retention";
    Serial.println("waitForPress->retention green");
    tone(PIN_SOUND, G3, 100);
    delay(200);
    tone(PIN_SOUND, G3, 100);
    delay(200);
    tone(PIN_SOUND, G3, 100);
    delay(200);
    timing=millis();
    
  }else if (!setuped  && _longSignal(LOW, 2*SECOND, BUTTON_RESET)){
    setuped = true;
    state="postSetup";
    
    tone(PIN_SOUND, G3, 200);
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(400);
    digitalWrite(PIN_LED_GREEN, LOW);
    
    tone(PIN_SOUND, G3, 200);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(400);
    digitalWrite(PIN_LED_RED, LOW);
    
    tone(PIN_SOUND, G3, 200);
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(400);
    digitalWrite(PIN_LED_GREEN, LOW);
  }
}



long retentionStartTime = 0;
void retention(){  
   unsigned long currentTime=millis();
    if(!retentionStartTime) retentionStartTime = currentTime;
    if((currentTime - retentionStartTime)>RETENTION_TIMOUT){
      Serial.println("retention -> win");
      retentionStartTime=0;
      state = "win";
    }else{
      if(BUTTON_RESET.changed() && !BUTTON_RESET.read()){
        retentionStartTime=0;
        Serial.println("retention -> waitForPress");
        state = "waitForPress";
      }else{
        if (currentTime - timing > 10000){ 
          timing = currentTime;
          tone(PIN_SOUND, G3, 800);
        }
      }
    }
    
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
