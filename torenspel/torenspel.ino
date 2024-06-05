#include <IRremote.h>
// code based on https://projecthub.arduino.cc/SAnwandter1/programming-8x8-led-matrix-a3b852

// pin setup
const int RECV_PIN = 11;
const int SPEAKER_PIN = 9;
const int POWER_LED_PIN = 7; // LED power-indicator

//update from SAnwandter

#define ROW_1 A4
#define ROW_2 3
#define ROW_3 4
#define ROW_4 5
#define ROW_5 6
#define ROW_6 7
#define ROW_7 8
#define ROW_8 9

#define COL_1 10
#define COL_2 11
#define COL_3 12
#define COL_4 13
#define COL_5 A0
#define COL_6 A1
#define COL_7 A2
#define COL_8 A3

int S1 = 2;

int s1State = 0;
bool flag = false;
int grootte = 5;
int row = 0;

int i = 0;
bool systemOn = true; // System state
int volume = 5; // initiële volume level (assumed range 0-10)

// IR Afstandsbediening codes
const unsigned long POWER_BUTTON_CODE = 0xBD42FF00; // power on/off
const unsigned long VOLUME_UP_BUTTON_CODE = 0xB946FF00; // volume up
const unsigned long VOLUME_DOWN_BUTTON_CODE = 0xEA15FF00; // volume down

byte display_row[8] = { B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111 };
const byte rows[] = {
  ROW_1, ROW_2, ROW_3, ROW_4, ROW_5, ROW_6, ROW_7, ROW_8
};
const byte col[] = {
  COL_1, COL_2, COL_3, COL_4, COL_5, COL_6, COL_7, COL_8
};

float timeCount = 0;
long time = millis();
int delayTime = 10;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 

void setup() {
  // Open serial port
  Serial.begin(9600);
  
  // Start IR receiver
  IrReceiver.begin(RECV_PIN);

  // Set all used pins to OUTPUT
  // This is very important! If   the pins are set to input
  // the display will be very dim.
  for (byte i = 0; i <= 8; i++)
    pinMode(rows[i], OUTPUT);
  for (byte i = 0; i <= 8; i++)
    pinMode(col[i], OUTPUT);

  pinMode(S1, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(S1), buttonPressed, FALLING);  // trigger when button pressed, but not when released.

  // Optional: Indicate system is on at startup
  digitalWrite(POWER_LED_PIN, systemOn ? HIGH : LOW);
}

void loop() {
  // IR afstandsbediening inputs
  if (IrReceiver.decode()) {
    unsigned long receivedCode = IrReceiver.decodedIRData.decodedRawData;

    if (receivedCode == POWER_BUTTON_CODE) {
      systemOn = !systemOn; // Toggle system state
      Serial.println(systemOn ? "System ON" : "System OFF");

      // Optional: Update power LED state
      digitalWrite(POWER_LED_PIN, systemOn ? HIGH : LOW);

      if (!systemOn) {
        // toevoegen code system OFF (uitzetten van: LEDs, speaker, LED matrix)
        powerDownSystem();
      } else {
        // toevoegen code system ON (aanzetten van: LEDs, speaker, LED matrix)
        powerUpSystem();
      }
    } else if (receivedCode == VOLUME_UP_BUTTON_CODE) {
      if (systemOn && volume < 10) {
        volume++;
        Serial.print("Volume Up: ");
        Serial.println(volume);
        // Code om speaker volume te verhogen
        analogWrite(SPEAKER_PIN, map(volume, 0, 10, 0, 255));
      }
    } else if (receivedCode == VOLUME_DOWN_BUTTON_CODE) {
      if (systemOn && volume > 0) {
        volume--;
        Serial.print("Volume Down: ");
        Serial.println(volume);
        // Code om speaker volume te verlagen
        analogWrite(SPEAKER_PIN, map(volume, 0, 10, 0, 255));
      }
    }

    IrReceiver.resume(); // Receive next value
  }

  // Ga door met de bestaande functionaliteit als het systeem is ingeschakeld
  if (systemOn) {
    flag = false;
    for (int i = 0; i < grootte + 8; i++) {
      if (!flag) {
        if (i < grootte) {
          display_row[row] = display_row[row] << 1;
        } else {
          byte x = B11111111;
          display_row[row] = ((display_row[row] << 1) | (x >> (7 - (i - grootte))));
        }
        time = millis();
        while (millis() - time < delayTime) {
          drawScreen();
        }
      }
    }

    for (int i = grootte + 8; i > 0; i--) {
      if (!flag) {
        if (i > 8) {
          display_row[row] = display_row[row] >> 1;
        } else {
          byte x = B11111111;
          display_row[row] = ((display_row[row] >> 1) | (x << (i - 1)));
        }
        Serial.println(flag); //niet verwijderen, ik heb geen idee waarom, maar zonder dit werkt het niet lol
        time = millis();
        while (millis() - time < delayTime) {
          drawScreen();
        }
      }
    }
  }
}

void drawScreen() {
  // Turn on each   row in series
  for (byte o = 0; o < 8; o++)  // count next row
  {
    digitalWrite(rows[o], HIGH);  //initiate whole row
    for (byte a = 0; a < 8; a++)  // count next row
    {
      // if You set   (~buffer2[i] >> a) then You will have positive
      digitalWrite(col[a], (display_row[o] >> a) & 0x01);  // initiate whole column

      delayMicroseconds(500);  // uncoment deley for diferent speed of display
      //delayMicroseconds(1000);
      //  delay(1);

      digitalWrite(col[a], 1);  // reset whole column
    }

    digitalWrite(rows[o], LOW);  // reset whole row
                                 // otherwise last row will intersect with next   row
  }
}

void buttonPressed() {
  if (millis() - lastDebounceTime > debounceDelay){
    lastDebounceTime = millis();
    flag = true;
    
    row++;

    if (row>1){
      display_row[row-1] = (display_row[row-2] | display_row[row-1]);
      grootte = 0;
      for (int i=0; i<8; i++){
        if(((display_row[row-1] >> i) & 0x01) == 0){
          grootte++;
        }
      }
    }
  }
}

// Functie om het systeem uit te zetten (Uitzetten van alle pins, behalve de IR receiver pin)
void powerDownSystem() {
  // Turn off speaker
  noTone(SPEAKER_PIN);

  // Uitzetten alle digital pins, behalve RECV_PIN
  for (int pin = 2; pin <= 13; pin++) {
    if (pin != RECV_PIN) {
      pinMode(pin, INPUT);
    }
  }

  // Uitzetten alle analog pins
  for (int pin = A0; pin <= A5; pin++) {
    pinMode(pin, INPUT);
  }

  Serial.println("System is powered down.");
}

// Functie om systeem aan te zetten
void powerUpSystem() {
  // Aanzetten speaker (if needed, you can initialize it to a default state)
  analogWrite(SPEAKER_PIN, map(volume, 0, 10, 0, 255));

  // Aanzetten alle digital pins, behalve RECV_PIN
  for (int pin = 2; pin <= 13; pin++) {
    if (pin != RECV_PIN) {
      pinMode(pin, OUTPUT);
    }
  }

  // Aanzetten alle analog pins
  for (int pin = A0; pin <= A5; pin++) {
    pinMode(pin, OUTPUT);
  }

  Serial.println("System is powered up.");
}
