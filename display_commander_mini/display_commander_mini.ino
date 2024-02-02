#include <SPI.h>
#include <LoRa.h>
#include <TimerOne.h>
#include <avr/wdt.h>


#define node_count 6
#define alarm_duration 2
#define alarm_relay 3

// hrm@eurodistl.com.ng
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Serial.println("LoRa Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Timer1.initialize(1000000); // set timer for 1 second
  Timer1.attachInterrupt(timerIsr); // attach the timer interrupt

  pinMode(alarm_relay, OUTPUT);
  digitalWrite(alarm_relay, 0);

  // Serial.println(firstMsg);

  wdt_enable(WDTO_1S); // Enable the watchdog timer with an 1 second timeout
}



String offlineMsg = "Offline Nodes: ";
String allOnlineMsg = "All Nodes Online     ";
String alertMsg = "Husky ";
String firstMsg = "Line Monitor v0.0.2          ";
String loraMsg = "";
byte faultNodes = 0b00000000;
byte seenNodes = 0b00000000;
char loraChar;
byte okayCounter = 0; // count number of ping recieved from nodes
bool alarmOn = false;
byte comWatchdog = 0; // check if communication with lora has dropped for any reason
byte comCounter = 0;
byte x = 1; // starting point for led indicator of display_mini


byte secsCount = 0;

void timerIsr() {
  secsCount ++;
  if ((secsCount < alarm_duration) && (alarmOn)){
    digitalWrite(alarm_relay, 1); // power on speaker
  }

  if ((secsCount == 5)  && (digitalRead(alarm_relay))){
     digitalWrite(alarm_relay, 0); // power off speaker

    if (comWatchdog < 1){ // restart wdt if no communicaiton in 5 secs of alarm
      while(1);
    } else{
      comCounter = 0;
    }
  }

  if (secsCount > 10){
    secsCount = 0;
    comCounter ++;
    if ((comCounter > 3) && (comWatchdog < 1)){ // restart controller if communicaiton is lost 4 * 10 secs
      while(1);
    }
    else{
      comWatchdog = 0;
      comCounter = 0;
    }
  }

}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    // received a packet
    // Serial.print("Received packet '");

    // read packet
    loraMsg = LoRa.readStringUntil('\n');
    comWatchdog ++;
    // Serial.println(loraMsg);
    switch (loraMsg[0]){
      case 'N':
        digitalWrite((loraMsg[1] - 48 + x), 0); // mapped to the output pin to power on
        alarmOn = false;
        break;

      case 'F':
        digitalWrite((loraMsg[1] - 48 + x), 1); // mapped to the output pin to power on
        alarmOn = true;
        break;
      
      case 'T':
        digitalWrite((loraMsg[1] - 48 + x), 1); // mapped to the output pin to power on
        alarmOn = true;
        break;
    }
  }
  wdt_reset(); // Reset the watchdog timer
}
