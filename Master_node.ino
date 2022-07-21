
/*
  Froukje Temme
  09-05-2022
  Master node of program for the special olympics
  base / master node 00
  RF24 modules

  Inspiration source code:by Dejan, www.HowToMechatronics.com
*/

// include libraries for the network
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <RF24.h>
#include <SPI.h>

//define the buttons
#define button1 3
#define button2 2
#define button3 5

//setup for the network library
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;      // Address of the other node in Octal format
const uint16_t node02 = 02;
const uint16_t node03 = 03;

//timer for sending values once in a while
unsigned int timer = 0;

//variables for setting which program you;re in + detecting
int gameState;

//variables for mode 1 (remembering order in which you jump over it)
int order[20];            //empty placeholders for the order, better in arraylist but arduino doesn't have that
float orderPlace;         //remembers at which place you are jumping to give an example
int orderRetrieve = 0;    //remembers at which place you are jumping to retrieve the example
#define led1 9            //define leds that will show if the program is started or not (for feedback);
#define led2 10
bool setProgram;
int amountOfJumps = 0;

// for debouncing of the button
int val, val2, buttonState;

// for sending the data over the network
struct Data_Package {
  boolean jumpOver[3];
  int programState = 0;
};
Data_Package data;

//levels
int player = 3;
int playersLevel[3] = {0, 2, 3}; //player is the position in the array, the level is in the array
int level [5][4] = {{1, 3, 1, 3},
  {1, 3, 1, 2},
  {3, 1, 2, 3},
  {1, 2, 1, 2},
  {2, 3, 1, 2}
};
bool levelFailed = false;


void setup() {
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_1MBPS);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  timer = millis();
  orderPlace = 0;
}

void loop() {
  network.update();   //update the network
  runProgram();       // always has to happen, start the program by pushing the button2
  sending();          // sending also aways happens
  chooseMode();       // which mode you are in, changes gameState
  switch (gameState) {
    case 1:
      //code for: showing it, then having the person do it itself
      receivingMode1();
      break;
    case 2:
      //code for: putting everything on the same color
      receivingMode2();
      break;
    case 3:
      //code for: personalized with RFID tag
      receivingMode3();
      break;
  }
  unsigned long buttonState1 = digitalRead(button1);
  if (buttonState1) {
    reset(); //reset when changing game state? and when else, when clicking the button
  }
}

void chooseMode() {
  int buttonState3 = digitalRead(button3);
  if (buttonState3) {//stop button
    gameState ++;
    if (gameState == 4) gameState = 1; //loop around
    if (gameState == 1) Serial.println("Show and repeat");
    if (gameState == 2) Serial.println("Put all the lights on blue");
    if (gameState == 3) Serial.println("Instructions for mode 3");
    data.programState = gameState;
    reset();
    delay(1000);    //go out of the program for a second
  }
}

void runProgram() {
  val = digitalRead(button2);   //red input value, delay, read again, see if we have a consistant reading and not a bounce
  delay(10);
  val2 = digitalRead(button2);
  if (val == val2) {
    if (val != buttonState) {  // check if the button state has changed
      if (val == HIGH) {
        setProgram = !setProgram;  //so if it's really high, then change the program from setting to executing?
        amountOfJumps = int(orderPlace);
      }
    }
    buttonState = val;
  }

  //show the user that the program has started by putting the LED on
  if (setProgram) {
    digitalWrite(led1, HIGH);
    digitalWrite(led2, LOW);

  }
  else {
    digitalWrite(led2, HIGH);
    digitalWrite(led1, LOW);
  }
}

void sending() {
  //===== Sending =====// send the states of all the lights
  if (millis() > timer + 300) { // more acurate than delay(500);
    timer = millis();  // store timer for the next time
    sendData();
  }
}

void sendData() {
  RF24NetworkHeader header1(node01);    // (Address where the data is going)
  bool ok = network.write(header1, &data, sizeof(Data_Package)); // Send the data, ok is just a storage point, not really important

  // Button state at Node 02
  RF24NetworkHeader header2(node02);    // (Address where the data is going)
  bool ok2 = network.write(header2, &data, sizeof(Data_Package)); // Send the data

  // Button state at Node 03
  RF24NetworkHeader header3(node03);    // (Address where the data is going)
  bool ok3 = network.write(header3, &data, sizeof(Data_Package)); // Send the data, ok is just a storage point, not really important
}

void reset() {
  Serial.println("RESET");
  for (int i = 0; i < sizeof(order) / 2; i++) {
    order[i] = 0;
  }
  orderPlace = 0;
  orderRetrieve = 0;
}


void receivingMode1() {
  //===== Receiving =====//
  while ( network.available() ) {       // Is there any incoming data?
    RF24NetworkHeader header;
    unsigned long incomingData;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    uint16_t headers = header.from_node;     //store 01, 02 or 03 in header

    if (setProgram) {
      //remembering the order
      order[int(orderPlace)] = headers;       //so in the arraylist order: at the current spot (so first hurdle, or afterwards), put which light it is 01, 02, 03 etc
      orderPlace++;                           //say that something was added, so next one in arraylist
    }

    if (!setProgram) {                                  // putting the lights back on false + increase the order when someone jumped over a light
      if ((int) headers == order[orderRetrieve]) {       //have to jump over the right bar. So: you get a header which is has to be, which should match the one from order[orderretrieve]
        data.jumpOver[headers - 1] = false;                    // so e.g. jump over node 03: data.jumpOver[2] (light 3) goes to false again
        orderRetrieve ++;

        if (orderRetrieve == amountOfJumps && orderRetrieve != 0) {
          sendData();
          delay(2000);                                      //wait for a second to indicate that the player has succeeded
          orderRetrieve = 0;                                // start from the beginning again
        }
      }
    }
  }

  // put the status of the lights on the right position
  if (!setProgram) {                                  //so when the program is already set (when you first start, every thing is 0 0 0)
    int positionNow = order[orderRetrieve];           //read at which position you are now (start with order[0])
    data.jumpOver[positionNow - 1] = true;                 //put that particular light on (e.g. if the first thing in array is 3: set data.jumpOver[2](=light 3) on true
  }
}

void receivingMode2() {
  //this mode: start with all lights LOW --> put in the slave nodes that then the lights should be on
  //when you jump over it (only when started), put the lights on HIGH (which indicates low)

  //===== Receiving =====//
  while ( network.available() ) {       // Is there any incoming data?
    RF24NetworkHeader header;
    bool incomingData;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    uint16_t headers = header.from_node;     //store 01, 02 or 03 in header

    //putting lights on
    if (setProgram) {
      data.jumpOver[headers - 1] = true;         // read headers (is 1, 2 or 3) --> put it in high if jumped over, turn red


      int finished = 0;
      for (int i = 0; i < sizeof(data.jumpOver); i++) {    //check if they are all on true, then the program is done
        if (data.jumpOver[i] == true) {
          finished ++;
        }
        else {
        }
      }

      if (finished == sizeof(data.jumpOver)) {
        for (int i = 0; i < sizeof(data.jumpOver); i++) {    //check if they are all on true, then the program is done
          data.jumpOver[i] = data.jumpOver[i];
        }
        sendData();
        delay(3000);
        setProgram = !setProgram;
        reset();
      }
    }
  }

  //setting:
  if (!setProgram) {                                  //putting everything on again (on green)
    for (int i = 0; i < sizeof(data.jumpOver); i++) {
      data.jumpOver[i] = false;
    }
  }
}


void receivingMode3() {
  //===== Receiving =====//
  while ( network.available() ) {       // Is there any incoming data?
    RF24NetworkHeader header;
    unsigned long incomingData;
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    uint16_t headers = header.from_node;     //store 01, 02 or 03 in header or 04

    if (!setProgram) { //when you haven't started yet, note which player it is
      if ((int) headers == 4) {
        player = incomingData;            //variable player is set to the incoming data, which is send to all nodes
        reset();
      }
    }

    if (setProgram) {                                  // putting the lights back on false + increase the order when someone jumped over a light
      if ((int) headers == order[orderRetrieve]) {       //have to jump over the right bar. So: you get a header which is has to be, which should match the one from order[orderretrieve]
        data.jumpOver[headers - 1] = false;                    // so e.g. jump over node 03: data.jumpOver[2] (light 3) goes to false again
        orderRetrieve ++;
      }
      if (incomingData == 20) { //meaning: when ran out of time
        //say: you cannot upgrade your evel anymore
        setProgram = !setProgram;
        reset();
      }
    }
  }

  if (!setProgram) {                          //set the order of the lights
    int tempLevel = playersLevel[player - 1];   //dit kijkt op de plek van playerslevel (0 1 of 2,bij player 1 2 of 3) wat het level is)

    //read which level --> 0 1 2 3 or 4, put and loop then in this level trough what order, put it in order
    for (int i = 0; i < sizeof(level[tempLevel]) / 2; i++) {   //loop through level
      order[i] = level[tempLevel][i];
    }
  }

  // start the program: put the status of the lights on the right position (also based on where you are at the moment)
  if (setProgram) {                                   //so when the program is already set (when you first start, every thing is 0 0 0)
    int positionNow = order[orderRetrieve];           //read at which position you are now (start with order[0])
    data.jumpOver[positionNow - 1] = true;            //put that particular light on (e.g. if the first thing in array is 3: set data.jumpOver[2](=light 3) on true

    //if everything is reached
    //go back to !setProgram and upp the level
    if (orderRetrieve > 3) {
      if (levelFailed == false) {
        playersLevel[player - 1]++;
      }
      reset();
      setProgram = !setProgram;
    }
  }
}
