#include <SPI.h>
#include <LoRa.h>
#include <TimerOne.h>
#include <PinChangeInterrupt.h>
#include <avr/wdt.h>

#define push_button 3
#define probe 4
#define test_mode 3
#define reboot_mode 15
#define red_led 5
#define green_led 6


int counter = 0;
int stage = 1;
bool shoutTime = false;
bool buttonPressed = false;
bool trigger = false;
byte debounce = 0; // debounce checker
bool sec = false; // one second marker
bool threeSec = false; // marker for states like fault desable and test
byte buttonHoldCounter = 0; //count how long button remain pressed
bool resetMode = false; // reset cpu delibrately
bool testMode = false; // indicaate test mode
const char okay[]= "N3xxxxx\n";
const char fault[] = "F3xxxxx\n";
const char test[] = "T3xxxxx\n";
// const unsigned long mcNumber = 1;



void timerIsr() {
  // this function will be called every 1/2 second
  counter++;
  debounce++;

  if (!(counter % 2)){
    sec = true; // one second has passed
  }
  
  if (counter == 120){
    if (stage == 1)
      stage = 2;
    shoutTime = true;
    counter = 1; // reset counter every minute
  }

  if (counter % 6 == 0 ){ // triggered every 3 secs
    threeSec = true;
  }

  if (stage == 1 && counter % 10 == 0){  // shoutTime every 5 secs for first 60 secs
    shoutTime = true; // free to shout
  }
 
}

void myPushButton() {
  if (debounce < 2)
    return; // do nothing if debounce has not expired
  debounce = 0; // set debounce 
  buttonHoldCounter = 0; // clear button hold counter
  // Your function for pin D3 here
  buttonPressed = !digitalRead(push_button);
  trigger = false; // disable fault trigger
  testMode = false; // disable test mode
  sendPacket(okay);
}

void myProbe() {
  // Your function for pin D4 here
  if (debounce < 2)
    return; // do nothing if debounce has not expired
  debounce = 0; // set debounce 
  // Your function for pin D3 here
  trigger = true;
}

void setup() {
  // delay(mcNumber); //set random delay time to 
  Timer1.initialize(500000); // set timer for 1/2 second
  Timer1.attachInterrupt(timerIsr); // attach the timer interrupt
  pinMode(push_button, INPUT_PULLUP); // push button state
  pinMode(probe, INPUT_PULLUP); // machine status probe state
  pinMode(green_led, OUTPUT);
  pinMode(red_led, OUTPUT);

  digitalWrite(green_led, LOW); // reset green led to LOW
  digitalWrite(red_led, LOW); // reset gren


  // Attach the new PinChangeInterrupt and enable event function below
  // attachPCINT(digitalPinToPCINT(pinBlink), blinkLed, CHANGE);
  attachPCINT(digitalPinToPCINT(push_button), myPushButton, FALLING); 
  attachPCINT(digitalPinToPCINT(probe), myProbe, FALLING); // CHANGE oR FALLING
  // PCintPort::attachInterrupt(4, myProbe, CHANGE);

  wdt_enable(WDTO_8S); // Enable the watchdog timer with an 8 second timeout
  Serial.begin(9600); // for report only
  while (!Serial);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(433E6)) { // 433mhz transmision frequenct
    Serial.println("Starting LoRa failed!");
    while (1){
      digitalWrite(red_led, !digitalRead(red_led));
      digitalWrite(green_led, !digitalRead(green_led));
    }
  }
}

void sendPacket(char payload[]){
  // send packet
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
}

void loop() {
  if (shoutTime && !testMode && !trigger){
    Serial.print("shout out ");
    Serial.println(counter);

    sendPacket(okay); // Normal state
    shoutTime = false;
  }

  if(sec){ // every second routine
    sec = false;

    if (buttonPressed){
      buttonPressed = !digitalRead(push_button);
      Serial.println ("pushButton pressed");
      buttonHoldCounter ++;
    }

    if (buttonHoldCounter == test_mode && !testMode){
      // buttonHoldCounter = 0;
      Serial.println("test mode activated");
      testMode = true;
      trigger = false; // stop machine fault mode if active;
      sendPacket(test);
    }

    if (buttonHoldCounter == reboot_mode){
      Serial.println("restart mode activated");
      while(1);
    }

    if (!trigger){ // Normal mode -----------------------------------------------------
      digitalWrite(green_led, !digitalRead(green_led));
      digitalWrite(red_led, HIGH);
    }

    if (testMode){ // testmode --------------------------------------------------------
      digitalWrite(red_led, !digitalRead(red_led));
      digitalWrite(green_led, HIGH);
    }

    wdt_reset(); // Reset the watchdog timer
  }

  if (threeSec){ // execute every 3 seconds
    threeSec = false;

    if (trigger){
      trigger = !digitalRead(probe);
      if (!trigger){
        sendPacket(okay); // send signal for normal mode Issue resulved
        // best place to watchdog reset
      }
      Serial.println ("probe triggered");
      sendPacket(fault);
    }

    if (testMode){
      sendPacket(test);
    }
  }


}
