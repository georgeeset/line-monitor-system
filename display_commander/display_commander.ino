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


byte secsCount = 0;

void timerIsr() {
  secsCount ++;
  if ((secsCount < alarm_duration) && (alarmOn)){
    digitalWrite(alarm_relay, 1); // power on speaker
  }

  if ((secsCount == 5)  && (digitalRead(alarm_relay))){
     digitalWrite(alarm_relay, 0);
    //  alarmOn = false;
    //  faultNodes = 0; // clear everything
    if (comCounter < 1){
      while(1);
    } else{
      comCounter = 0;
    }
  }

  if (secsCount > 10){
    secsCount = 0;
    comCounter ++;
    if ((comCounter > 3) && (comWatchdog < 1)){ // restart controller if communicaiton is lost
      while(1);
    }
    else{
      comWatchdog = 0;
      comCounter = 0;
    }
  }

}

void updatePing(){
  bool titleSent = false;
  bool comFault = false; // indicates if there is communication fault on any node
  if (alarmOn)
    return;
  okayCounter++;
  int j = 0;
  if (okayCounter > node_count + 2 || comFault){
    for (j = 1; j <= node_count; j++){
      if (! bitRead(seenNodes, j)){
        comFault = true;
        if (!titleSent){
          Serial.print(offlineMsg);
          titleSent = true;
        }
        Serial.print(j);
        if (j < node_count){
           Serial.print(", ");
        }
       
      }
    }
    if (comFault){
      Serial.print("   ");
      Serial.print('\n');
    } else {
      if (!alarmOn) { // print this if no alarm and no communicaiton fault
        Serial.println(allOnlineMsg);
      }
    }

    okayCounter = 0; // clear counter
    seenNodes = 0; // clear seen
    
  }
}

void updateFault(){
  bool titleSent = false;
  bool alarmPresent = false;
  int j = 0;
  for (j = 1; j <= node_count; j++){
      if (bitRead(faultNodes, j)){
        alarmPresent = true;
        if (!titleSent){
          Serial.print(alertMsg);
          titleSent = true;
        }
        Serial.print(j);
        if (j < node_count){
           Serial.print(", ");
        }
      }
  }
  if (alarmPresent){
    Serial.print("   ");
    Serial.print('\n');
    alarmOn = true;
    comCounter = 0;
  }else{
    if (alarmOn)
      Serial.println(firstMsg);
    alarmOn = false;
    digitalWrite(alarm_relay, 0);
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
    comCounter ++;
    // Serial.println(loraMsg);
    switch (loraMsg[0]){
      case 'N':
        seenNodes |= (1 << loraMsg[1] - 48);
        // faultNodes &= ~(1 << loraMsg[1] - 48); // clear bit
        bitWrite(faultNodes, (loraMsg[1] - 48), 0); //repeatS
        updateFault();
        updatePing();
        break;
      case 'F':
        faultNodes |= (1 << loraMsg[1] - 48);
        updateFault();
        break;
      case 'T':
        faultNodes |= (1 << loraMsg[1] - 48);
        updateFault();
        break;
    }

    // print RSSI of packet
    // Serial.print("' with RSSI ");
    // Serial.println(LoRa.packetRssi());
  }
  wdt_reset(); // Reset the watchdog timer
}
