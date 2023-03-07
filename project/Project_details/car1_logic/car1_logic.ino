#include <SPI.h>
#include <Time.h>
#include <MFRC522.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CAR_ID '0'

#define START 's'
#define READY_STATE 'r'
#define FREE_MODE 'f'
#define GAME_MODE 'g'
#define OVER_STATE 'o'

#define RST_PIN 9  // Configurable, see typical pin layout above
#define SS_PIN 10  // Configurable, see typical pin layout above
#define LOCKED 'L'
#define FREE 'F'
#define OVER 'O'
#define READY 'R'
#define GAME 'G'
#define START_SEEN 'S'
#define MID_SEEN 'M'
#define END_SEEN 'E'

#define ON_DANGER (mfrc522.uid.uidByte[1] == 0xD3 || mfrc522.uid.uidByte[1] == 0x47 || mfrc522.uid.uidByte[1] == 0x57)
#define ON_START_LINE (mfrc522.uid.uidByte[1] < 0x68)
#define ON_MID_LINE (mfrc522.uid.uidByte[1] > 0xCB)


//L293 Connection
const int motorA1 = 3;
const int motorA2 = 4;
const int motorAspeed = 5;
const int motorB1 = 7;
const int motorB2 = 8;
const int motorBspeed = 6;

int vspeed = 255, halfSpeed = 127;


//Useful Variables
int signal = 0, timer_i = 0, timer_j = 0, timer_d = 0;
bool beep = false, danger = false, dangerCrossed = false;

char gameState[5] = "s";
char carState = LOCKED, lap = '0';
char carStateStr[5] = "...";


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance


RF24 radio(18, 19);  // CE, CSN
const byte addresses[][6] = { "00001", "00002" };

void setup() {
  setTime(0);
  // Set pins as outputs:
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(17, LOW);
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  radio.begin();
  radio.openWritingPipe(addresses[0]);     // 00001
  radio.openReadingPipe(1, addresses[1]);  // 00002
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  delay(10);  // Optional delay. Some board do need more time after init to be ready, see Readme

  //SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522
  delay(4);                           // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop() {

  if (Serial.available() > 0) {
    signal = Serial.read();
  }

  if (carState != LOCKED && carState != READY) {
      /***********************Forward****************************/
      //If signal is equal with letter 'F', car will go forward!
      if (signal == 'F') {
        digitalWrite(motorA1, LOW);
        digitalWrite(motorA2, HIGH);
        digitalWrite(motorB1, LOW);
        digitalWrite(motorB2, HIGH);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, vspeed);
      }
      /**********************Forward Left************************/
      //If signal is equal with letter 'G', car will go forward left
      else if (signal == 'G') {
        digitalWrite(motorA2, LOW);
        digitalWrite(motorA1, HIGH);
        digitalWrite(motorB2, HIGH);
        digitalWrite(motorB1, LOW);
        analogWrite(motorAspeed, halfSpeed);
        analogWrite(motorBspeed, vspeed);
      }
      /**********************Forward Right************************/
      //If signal is equal with letter 'I', car will go forward right
      else if (signal == 'I') {
        digitalWrite(motorA2, HIGH);
        digitalWrite(motorA1, LOW);
        digitalWrite(motorB2, LOW);
        digitalWrite(motorB1, HIGH);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, halfSpeed);
      }
      /***********************Backward****************************/
      //If signal is equal with letter 'B', car will go backward
      else if (signal == 'B') {
        digitalWrite(motorA1, HIGH);
        digitalWrite(motorA2, LOW);
        digitalWrite(motorB1, HIGH);
        digitalWrite(motorB2, LOW);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, vspeed);
      }
      /**********************Backward Left************************/
      //If signal is equal with letter 'H', car will go backward left
      else if (signal == 'H') {
        digitalWrite(motorA1, HIGH);
        digitalWrite(motorA2, LOW);
        digitalWrite(motorB1, HIGH);
        digitalWrite(motorB2, LOW);
        analogWrite(motorAspeed, 0);
        analogWrite(motorBspeed, vspeed);
      }
      /**********************Backward Right************************/
      //If signal is equal with letter 'J', car will go backward right
      else if (signal == 'J') {
        digitalWrite(motorA1, HIGH);
        digitalWrite(motorA2, LOW);
        digitalWrite(motorB1, HIGH);
        digitalWrite(motorB2, LOW);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, 0);
      }
      /***************************Left*****************************/
      //If signal is equal with letter 'L', wheels will turn left
      else if (signal == 'L') {
        digitalWrite(motorA2, LOW);
        digitalWrite(motorA1, HIGH);
        digitalWrite(motorB2, HIGH);
        digitalWrite(motorB1, LOW);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, vspeed);
      }
      /***************************Right*****************************/
      //If signal is equal with letter 'R', wheels will turn right
      else if (signal == 'R') {
        digitalWrite(motorA2, HIGH);
        digitalWrite(motorA1, LOW);
        digitalWrite(motorB2, LOW);
        digitalWrite(motorB1, HIGH);
        analogWrite(motorAspeed, vspeed);
        analogWrite(motorBspeed, vspeed);
      }
    
      /************************Stop*****************************/
      //If signal is equal with letter 'S', stop the car
      else if (signal == 'S') {
        analogWrite(motorA1, 0);
        analogWrite(motorA2, 0);
        analogWrite(motorB1, 0);
        analogWrite(motorB2, 0);
      }
  } else {
    analogWrite(motorA1, 0);
    analogWrite(motorA2, 0);
    analogWrite(motorB1, 0);
    analogWrite(motorB2, 0);
  }

  if (radio.available()) {
    // Variable for the received timestamp
    while (radio.available()) {                   // While there is data ready
      radio.read(&gameState, sizeof(gameState));  // Get the payload
    }

    radio.stopListening();  // First, stop listening so we can talk
    carStateStr[0] = CAR_ID;
    carStateStr[1] = carState;
    carStateStr[2] = lap;
    radio.write(&carStateStr, sizeof(carStateStr));  // Send the final one back.
    radio.startListening();                          // Now, resume listening so we catch the next packets.

    Serial.print(F("Got message '"));
    Serial.print(gameState);
    Serial.print(F("', Sent response '"));
    Serial.print(carStateStr);
    Serial.println(F("'"));
  }

  //    delay(4);
  if (beep) {
    digitalWrite(14, HIGH);
    if (timer_j < 60) timer_j++;
    else {
      digitalWrite(14, LOW);
      timer_j = 0;
      beep = false;
    }
  }

  if (danger) {
    if (timer_d < 350) timer_d++;
    else {
      timer_d = 0;
      vspeed = 255;
      halfSpeed = 127;
      danger = false;
      dangerCrossed = true;
    }
  }

  if (gameState[0] == GAME_MODE && carState == READY) carState = GAME, lap = '0';
  else if (gameState[0] == FREE_MODE && carState == LOCKED) carState = FREE;
  else if (gameState[0] == READY_STATE && carState == FREE) carState = LOCKED; 
  else if (gameState[0] == OVER_STATE && (carState==GAME || carState==START_SEEN || carState==MID_SEEN || carState==END_SEEN)) carState = OVER;

  else if (carState == OVER) {
    if (timer_i < 120) timer_i++;
    else {
      carState = LOCKED;
      lap = '0';
      timer_i = 0;
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent()) {
    digitalWrite(2, LOW);
    //if (carState == READY) carState = LOCKED;
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    digitalWrite(2, LOW);
    if (carState == READY) carState = LOCKED;
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  digitalWrite(2, HIGH);
  if (gameState[0] == GAME_MODE || gameState[0] == READY_STATE) {
    if (ON_DANGER) {
      if (!dangerCrossed && !danger && gameState[0] == GAME_MODE && lap == '1') {
        vspeed = (second()%2)? 127 : 0;
        halfSpeed = vspeed? 63 : 0;
        danger = true;
      }
    } else if (ON_START_LINE) {
      if (carState == GAME) carState = START_SEEN, beep = true;
      else if (carState == END_SEEN) carState = START_SEEN, beep = true;
      else if (carState == LOCKED) carState = READY;
    } else if (ON_MID_LINE) {
      if (carState == START_SEEN) carState = MID_SEEN;
    } else /*ON_END_LINE*/ {
      if (carState == MID_SEEN) {
        carState = END_SEEN;
        beep = true;
        lap++;
      }
    }
  }
}
