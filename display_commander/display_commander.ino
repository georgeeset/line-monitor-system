#include <SPI.h>
#include <LoRa.h>
#include <TimerOne.h>


#define node_count 6
#define alarm_duration 4
#define alarm_relay 2

// hrm@eurodistl.com.ng
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Serial.println("LoRa Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Timer1.initialize(1000000); // set timer for 1/2 second
  Timer1.attachInterrupt(timerIsr); // attach the timer interrupt
}



String offlineMsg = "Offline Nodes: ";
String allOnlineMsg = "All Nodes Online     ";
String alertMsg = "Fault @ Node ";
String firstMsg = "Line Monitor v0.0.2          ";
String loraMsg = "";
byte faultNodes = 0b00000000;
byte seenNodes = 0b00000000;
char loraChar;
byte okayCounter = 0; // count number of ping recieved from nodes
bool alarmOn = false;


byte secsCount = 0;

void timerIsr() {
  secsCount ++;
  if ((secsCount < alarm_duration) && (alarmOn)){
    digitalWrite(alarm_relay, 1); // power on speaker
  }

  if ((secsCount == 7)  && (digitalRead(alarm_relay))){
     digitalWrite(alarm_relay, 0);
  }

  if (secsCount > 10){
    secsCount = 0;
  }

}

void updatePing(){
  bool titleSent = false;
  bool comFault = false; // indicates if there is communication fault on any node
  if (alarmOn)
    return;
  okayCounter++;
  int j = 0;
  if (okayCounter > node_count + 2){
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
    }
    if (!comFault && !alarmOn) { // print this if no alarm and no communicaiton fault
      Serial.println(allOnlineMsg);
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
  }else{
    if (alarmOn)
      Serial.println(firstMsg);
    alarmOn = false;
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
    // Serial.println(loraMsg);
    switch (loraMsg[0]){
      case 'N':
        seenNodes |= (1 << loraMsg[1]-48);
        faultNodes &= ~(1 << loraMsg[1]-48); // clear bit
        updateFault();
        updatePing();
        break;
      case 'F':
        faultNodes |= (1 << loraMsg[1]-48);
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
}
