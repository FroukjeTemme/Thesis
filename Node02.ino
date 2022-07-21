/*
  Froukje Temme
  09-05-2022
  Master node of program for the special olympics
  Node 02 (child of node 00)
  RF24 modules

  Inspiration source code :by Dejan, www.HowToMechatronics.com
*/


//code for wireless connection
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <RF24.h>
#include <SPI.h>

//code for putting the small LED on
int led = 4;
boolean button_state = 0;
int state = 0;

//code for the network
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 02;  // Address of our node in Octal format ( 04,031, etc)
const uint16_t master_node = 00;    // Address of the other node in Octal format, needs to go there

//code for LED strip
#include <FastLED.h>
#define LED_PIN     5 //pin number on arduino
#define NUM_LEDS    28 //amount of LED
#define BRIGHTNESS  255 //brithness
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS]; //to make a leds array
#define UPDATES_PER_SECOND 100 //for the delay
CRGBPalette16 currentPalette; //to fill the LED's with the current pallete
TBlendType    currentBlending; //type of belding: the colors overlap in each other or are hard cutf

//code for distance sensor
const int trigPin = 3;
const int echoPin = 2;
const int trigPin2 = 10;
const int echoPin2 = 9;
unsigned long duration, distance, duration2, distance2;

//code for determing the average over a running number
#include "RunningAverage.h" //to calculate the average over the running last 3 values
RunningAverage myRA(3);
RunningAverage myRA2(3);
int samples, samples2; //to clear the array after a while
unsigned long timer, timer2, timer3;
unsigned long averageDistance, averageDistance2;

//which state we are in
int gameState = 0;

//code for detecting signals
struct Data_Package {
  boolean jumpOver[3];
  int programState = 0;
};
Data_Package data;

//code for mode 3
bool state3;
int blinking3 = 1000;

int counterJump = 0;

void setup() {
  //for the connection
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_1MBPS);
  pinMode(led, OUTPUT);

  //for the LED strip
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip ); //add the leds on type, number, color order, etc
  FastLED.setBrightness(  BRIGHTNESS ); //set brightness

  //initialize a current pallete you want to start with + the type of blednign (linearblend or noblend);
  //currentPalette = RainbowStripeColors_p;
  currentBlending = NOBLEND;

  // for the distance sensor
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  myRA.clear(); // explicitly start the array clean
  myRA2.clear(); // explicitly start the array clean
  averageDistance = 300;
}

void loop() {
  //for connection
  network.update();
  receiving();

  //for the LED strip
  static uint8_t startIndex = 0;

  switch (gameState) {
    case 1:
      //code for displaying lights in mode 1
      if (state == 1) {
        currentPalette = RainbowStripeColors_p;
        startIndex += 1; //every loop set 1 to the left) this is 'speed'

      }
      else {
        stopColours();
      }

      break;
    case 2:
      //code for displaying lights in mode 1
      if (state == 1) {
        fill_solid( currentPalette, 16, CRGB::Green);
      }
      else {
        fill_solid( currentPalette, 16, CRGB::Red);
      }
      break;
    case 3:
      //code for displaying lights in mode 1
      if (state == 1) {
        countdownColor();
      }
      else {
        stopColours();
      }
      break;
  }
  //fill LED's
  FilltheLEDsFromPallete(startIndex);
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

  //for the distance sensor
  measureDistance();
}

void stopColours() {
  //puts everything on black
  fill_solid( currentPalette, 16, CRGB::Black);
}

//this fills the LED's with the colors from the pallete
void FilltheLEDsFromPallete(uint8_t colorIndex) {
  uint8_t brightness = 255;

  for ( int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 4; //if you do + 16, you do bit by bit, now the 16 is devided by 4 and every thing will now have 4 led's before it goes oto the next
    //so what you do, when colorIndex = 16, it goes to the next, LED, so if you have 8 (every color 2 LEDS< and if you have 4 = every color 4 LED's before it goes to the next
    // SO, i (i= nummer lED) = 0 & colorIndex =0 (LED1, kleur 1), i = 1 & colorIndex = 4 (LED2, kleur1), i = 2 & colorIndex = 8 (LED3, kleur1),i = 3 & colorIndex = 12 (LED4, kleur1), i = 4 & colorIndex = 16 (LED5, kleur 2) --> bij 16 begint de nieuwe!
  }
}


void measureDistance() {
  // Clears the trigPin
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  distance2 = rdPulseIn(echoPin2, HIGH, 5) / 5.82; // distance in mm

  // Clears the trigPin1
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  distance = rdPulseIn(echoPin, HIGH, 5) / 5.82; // distance in mm


  //calculating the running average over the last 4 values for #1
  if (distance < 900) { //don't add values that are way out of bounce
    myRA.addValue(distance);
    samples++;
  }
  unsigned long average = myRA.getAverage();


  //calculating the running average over the last 4 values for #2
  if (distance2 < 900) { //don't add values that are way out of bounce
    myRA2.addValue(distance2);
    samples2++;
  }
  unsigned long average2 = myRA2.getAverage();


  if ((average > 0 && average < 900) || (average2 > 0 && average2 < 900)) {
    if (millis() > timer + 999) { //more acurate than delay (500);
      timer = millis();
      //sending this over the network:
      if (gameState == 1 || gameState == 3) {
        unsigned long sent = 10;
        RF24NetworkHeader header(master_node); //address where data is going
        bool ok = network.write(header, &sent, sizeof(sent)); //send data
        blinking3 = 1000; //reset
      }
      if (gameState == 2) {
        counterJump++;
        if (counterJump > 4) { //als je 5x hebt gesprongen, dan pas aan
          unsigned long sent = 10;
          RF24NetworkHeader header(master_node); //address where data is going
          bool ok = network.write(header, &sent, sizeof(sent)); //send data
          counterJump = 0;
        }
      }
    }
  }

  if (samples == 300)
  {
    samples = 0;
    myRA.clear();
  }

  if (samples2 == 300)
  {
    samples2 = 0;
    myRA2.clear();
  }
}

void receiving() {
  //===== Receiving =====//
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header5;
    network.read(header5, &data, sizeof(Data_Package)); // Read the incoming data
    button_state = data.jumpOver[1];


    gameState = data.programState;

    // Turn on the LEDs as depending on the incoming value from the button
    if (button_state == HIGH) {
      digitalWrite(led, HIGH);
      state = 1;
    }
    else {
      digitalWrite(led, LOW);
      state = 0;
    }
  }
}

unsigned long rdPulseIn(int pin, int value2, int timeout) { // the following comments assume that we're passing HIGH as value. timeout is in milliseconds

  unsigned long now = micros();
  while (digitalRead(pin) == value2) { // wait if pin is already HIGH when the function is called, but timeout if it never goes LOW
    if (micros() - now > (timeout * 1000)) {
      return 0;
    }
  }

  now = micros(); // could delete this line if you want only one timeout period from the start until the actual pulse width timing starts
  while (digitalRead(pin) != value2) { // pin is LOW, wait for it to go HIGH befor we start timing, but timeout if it never goes HIGH within the timeout period
    if (micros() - now > (timeout * 1000)) {
      return 0;
    }
  }

  now = micros();
  while (digitalRead(pin) == value2) { // start timing the HIGH pulse width, but time out if over timeout milliseconds
    if (micros() - now > (timeout * 1000)) {
      return 0;
    }
  }
  return micros() - now;
}

void countdownColor() {
  switch (state3) {
    case true:
      fill_solid( currentPalette, 16, CRGB::Cyan);
      break;
    case false:
      stopColours();
      break;
  }

  if (millis() > timer3 + blinking3) { //more acurate than delay (500);
    timer3 = millis();
    state3 = !state3;
  }
  blinking3--;
  Serial.println(blinking3);
  if (blinking3 < -300) {
    unsigned long sendt = 20;
    RF24NetworkHeader header(master_node); //address where data is going
    bool ok = network.write(header, &sendt, sizeof(sendt)); //send data
    blinking3 = 1000;
  }
}
