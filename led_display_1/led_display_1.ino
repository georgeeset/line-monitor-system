/*
  Use the Marquee function to make an LED-sign type display

  This marquee function scrolls in all directions, random distances for each
  direction. If you edit the code in the "switch" statement below then you can
  make a simpler marquee that only scrolls in one direction.
*/
#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial_Black_16.h>
// #include <fonts/Droid_Sans_16.h>

/* For "Hello World" as your message, leave the width at 4 even if you only have one display connected */
#define DISPLAYS_WIDE 2
#define DISPLAYS_HIGH 1

SoftDMD dmd(DISPLAYS_WIDE,DISPLAYS_HIGH);
DMD_TextBox box(dmd);
// DMD_TextBox box(dmd, 0, 0, 32, 16);

String inputString = "Line Monitor v0.0.2                    ";      // a String to hold incoming data
int msgLen = 40;
bool stringComplete = false;  // whether the string is complete


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial:
  Serial.begin(9600);

  dmd.setBrightness(255);
  // dmd.selectFont(Droid_Sans_16);
  dmd.selectFont(Arial_Black_16);
  dmd.begin();

  dmd.clearScreen();
  // dmd.drawString(0, 0, message1);
}

// the loop routine runs over and over again forever:

void loop() {
  for(int i = 0; i < msgLen; i++) {
    if (stringComplete)
      break;
    // Serial.print(*next);
    box.print(inputString[i]);
    delay(200);
  }

  if (stringComplete) {
    // Serial.println(inputString);
    // clear the string:
    // inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  int i = 0;
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString[i] = inChar;
    i++;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      msgLen = i;
    }
  }
}

