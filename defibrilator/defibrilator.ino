#include <EEPROM.h>
#include <Bounce2.h>

Bounce DEB_BUTTON = Bounce();
Bounce DEB_MAGNET = Bounce();
#define G3 1568
#define G4 3136

#define PIN_BUTTON 8
#define PIN_MAGNET 7
#define PIN_VIBRO_1 9
#define PIN_VIBRO_2 10

#define PIN_LED_RED_1 2
#define PIN_LED_RED_2 3
#define PIN_LED_RED_3 4
#define PIN_LED_RED_4 12
#define PIN_LED_GREEN 5

#define LED_LENGTH 4

int redLeds[LED_LENGTH] = {PIN_LED_RED_1, PIN_LED_RED_2, PIN_LED_RED_3, PIN_LED_RED_4};
int SECOND  = 1000;
unsigned long MINUTES = 60000;
long DEFAULT_CHARGE_TIME  = 5* SECOND;
unsigned long CHARGE_TIME;
unsigned long CHARGE_INTERVAL;

long SAVED_CHARGE_TIME;

#define  PIN_SOUND 13

const int SOUND_LENGTH = 200;
const int SOUND_BASE_FREQ = 1174;

String state;

bool blocked=false;


void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_RED_1, OUTPUT);
  pinMode(PIN_LED_RED_2, OUTPUT);
  pinMode(PIN_LED_RED_3, OUTPUT);
  pinMode(PIN_LED_RED_4, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  pinMode(PIN_SOUND, OUTPUT);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MAGNET, INPUT_PULLUP);
  pinMode(PIN_VIBRO_1, OUTPUT);
  pinMode(PIN_VIBRO_2, OUTPUT);

  DEB_BUTTON.attach(PIN_BUTTON);
  DEB_BUTTON.interval(5);

  DEB_MAGNET.attach(PIN_MAGNET);
  DEB_MAGNET.interval(5);

  SAVED_CHARGE_TIME = EEPROM.read(1);

   SAVED_CHARGE_TIME == 255
   ? _setChargeTime(DEFAULT_CHARGE_TIME)
   : _setChargeTime(SAVED_CHARGE_TIME* MINUTES);


  state="charge";
  Serial.println("Start");
}

void loop() {
  DEB_BUTTON .update();
  DEB_MAGNET.update();
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


unsigned long chargeTimestamp=0;
int ledToActivateIndex = 0;
bool tunedIn =false;
void charge(){
   unsigned long currentTime=millis();
   if (_longSignal(LOW, 2*SECOND, DEB_BUTTON) && !tunedIn){
       tunedIn = true;
       _setupEffects();
       state="postSetup";
   }
   else if ((currentTime - chargeTimestamp) > CHARGE_INTERVAL || !chargeTimestamp){
    Serial.println("=============");
    Serial.println(currentTime );
    Serial.println(chargeTimestamp);
    Serial.println(CHARGE_INTERVAL);
    Serial.println(currentTime - chargeTimestamp);
    
      chargeTimestamp = currentTime;
      digitalWrite(redLeds[ledToActivateIndex++], HIGH);
      tone(PIN_SOUND, SOUND_BASE_FREQ, SOUND_LENGTH);

      if (!redLeds[ledToActivateIndex]){
        ledToActivateIndex=0;
        chargeTimestamp=0;
        _magnetIsDisactivated();
        state = "charged";
        Serial.println("Im charged");
      }
   }
}

int pressedTime=0;
void postSetup(){
    if (_longSignal(LOW, 2*SECOND, DEB_BUTTON)){
        pressedTime--;
        Serial.println("pressedTime::");
        Serial.println(pressedTime);
        Serial.println("MINUTES::");
        Serial.println(MINUTES);
        if(pressedTime){
            _setChargeTime(pressedTime* MINUTES);
            _saveChargeTimeToMemory(pressedTime);
        }else{
            _setChargeTime(DEFAULT_CHARGE_TIME);
        }
        _setupEffects();
        state="charge";
    }
    else if(DEB_BUTTON.changed() && !DEB_BUTTON.read()){
        Serial.println("++");
        tone(PIN_SOUND, G3, 100);
        pressedTime++;
    }
}



int touchCount=0;
void charged() {
  // need to activate magnet
  const int TOUCH_LIMIT=6;
  if(DEB_MAGNET.changed() && !DEB_MAGNET.read()){
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
  if(_longSignal(HIGH, SECOND*0.4, DEB_MAGNET)){
    Serial.println("waitForReleaseMagnet -> waitForPressButton");
    state = "waitForPressButton";
  }
}


unsigned long timeoutStartTime = 0;
void waitForPressButton(){
  long TIMEOUT = 5*SECOND;
  if(DEB_BUTTON.changed() && !DEB_BUTTON.read()){
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
  if(DEB_BUTTON.changed() && DEB_BUTTON.read()){
    Serial.println("waitForReleaseButton -> discharge");
    state = "discharge";
  }
}


void discharge(){
  Serial.println("Discharge");
  const int DELAY_LENGTH = 1000;

  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_VIBRO_1, HIGH);
  digitalWrite(PIN_VIBRO_2, HIGH);

  tone(PIN_SOUND,G4);
  delay(800);
  noTone(PIN_SOUND);

  digitalWrite(PIN_VIBRO_1, LOW);
  digitalWrite(PIN_VIBRO_2, LOW);

  for(int i=G4; i>=0; i-=20){
      tone(PIN_SOUND,i);
      delay(4);
      noTone(PIN_SOUND);
  }

  delay(2000);

  state = "charge";
  Serial.println("I'm discharged!");
}


void _setChargeTime(unsigned long chargeTime){
    CHARGE_TIME  = chargeTime;
    CHARGE_INTERVAL   = CHARGE_TIME /LED_LENGTH;
    Serial.println("chargeTime::");
    Serial.println(chargeTime);
    Serial.println("CHARGE_INTERVAL::");
    Serial.println(CHARGE_INTERVAL);

}

void _saveChargeTimeToMemory(int timeInMinutes){
    SAVED_CHARGE_TIME == 255
         ? EEPROM.write(1, timeInMinutes)
         : EEPROM.update(1, timeInMinutes);
}

void _magnetIsDisactivated() {
  tone(PIN_SOUND, G3, SECOND);

  for(int i=0; i<3; i++){
    _clearAllLeds();
    delay(200);
    _allLedsRed();
    if (i<2) delay(200);
  }
}


void _magnetIsActivated() {

  for(int i=0; i<3; i++){
    tone(PIN_SOUND, G3, 100);
    _clearAllLeds();
    delay(100);
    _allLedsGreen();
    if (i<2) delay(100);
  }
}

void _allLedsGreen(){
  digitalWrite(PIN_LED_RED_1, LOW);
  digitalWrite(PIN_LED_RED_2, LOW);
  digitalWrite(PIN_LED_RED_3, LOW);
  digitalWrite(PIN_LED_RED_4, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);
}

void _allLedsRed(){
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED_1, HIGH);
  digitalWrite(PIN_LED_RED_2, HIGH);
  digitalWrite(PIN_LED_RED_3, HIGH);
  digitalWrite(PIN_LED_RED_4, HIGH);
}

void _clearAllLeds(){
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED_1, LOW);
  digitalWrite(PIN_LED_RED_2, LOW);
  digitalWrite(PIN_LED_RED_3, LOW);
  digitalWrite(PIN_LED_RED_4, LOW);
}

void _setupEffects(){
        tone(PIN_SOUND, G3, 200);
       _allLedsGreen();
       delay(400);


       tone(PIN_SOUND, G3, 200);
       _allLedsRed();
       delay(400);

       tone(PIN_SOUND, G3, 200);
       _allLedsGreen();
       delay(400);

       _clearAllLeds();
}

unsigned long startTime=0;
bool _longSignal(int expectedValue, long timeout, Bounce debouncer){
  debouncer.update();
  if(debouncer.read()==expectedValue){
    unsigned long currentTime=millis();
    if(!startTime) startTime = currentTime;
    if((currentTime - startTime)>timeout){
      startTime=0;
      return true;
    }
  }else{
   startTime=0;
  }
  return false;
}
