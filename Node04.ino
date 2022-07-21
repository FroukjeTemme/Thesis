/*
  Froukje Temme
  17-062022
  Node for detecting bracelets
  Node 04 (child of node 00)
  RF24 modules

  this node only has to send code

  Inspiration source code :by Dejan, www.HowToMechatronics.com
*/

//for the RFID scanner
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

//code for wireless connection
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <RF24.h>
#include <SPI.h>

//code for the network
RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 04;  // Address of our node in Octal format ( 04,031, etc)
const uint16_t master_node = 00;    // Address of the other node in Octal format, needs to go there

unsigned long player = 0;


void setup()
{
  Serial.begin(9600);   // Initiate a serial communication
  //the network
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_1MBPS);
  //the RFID tag
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
}
void loop()
{
  Serial.println(player);

  //for sending the values over the network
  network.update();

  //for the RFID tag readings

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Show UID on serial monitor
  // Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  //Serial.println();
  //Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "04 22 1B F2 43 5F 81" ) //change here the UID of the card/cards that you want to give access
  {
    player = 1;
    sending(player);

  }
  else if (content.substring(1) == "04 20 65 F2 43 5F 81" ) //change here the UID of the card/cards that you want to give access
  {
    player = 2;
    sending(player);
  }
  else if (content.substring(1) == "04 24 7A F2 43 5F 81" ) //change here the UID of the card/cards that you want to give access
  {
    player = 3;
    sending(player);
  }
}

void sending(unsigned long valuetosend) {
  Serial.println(valuetosend);
  RF24NetworkHeader header(master_node); //address where data is going
  bool ok = network.write(header, &valuetosend, sizeof(valuetosend)); //send data
  delay(1000);
}
