
/**********************************************************************************
 ***********************************************************************************
 * Start Temp-Humid Set-Up
 ***********************************************************************************
 **********************************************************************************/
#include "DHT.h"
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
/***********************************************************************************
 ***********************************************************************************
 * End Temp-Humid Sensor Set-Up
 ***********************************************************************************
 ***********************************************************************************/

/**********************************************************************************
 ***********************************************************************************
 * Start Water Temp Set-Up
 ***********************************************************************************
 **********************************************************************************/
#include <OneWire.h>
int DS18S20_Pin = 4;
OneWire ds(DS18S20_Pin);
/***********************************************************************************
 ***********************************************************************************
 * End Water Temp Sensor Set-Up
 ***********************************************************************************
 ***********************************************************************************/

/**********************************************************************************
 ***********************************************************************************
 * Start Ethernet+PubNub Set-Up
 ***********************************************************************************
 **********************************************************************************/
#include <SPI.h>
#include <Ethernet.h>
#include <PubNub.h>
#include <aJSON.h>

// Some Ethernet shields have a MAC address printed on a sticker on the shield;
// fill in that address here, or choose your own at random:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

char pubkey[] = "pub-c-5bef0e0e-e177-4a94-b06f-ce3c1355e5cd";
char subkey[] = "sub-c-3c8b80da-de67-11e5-abb9-0619f8945a4f";
char channel[] = "tester";

aJsonObject *createMessage()
{
  aJsonObject *msg = aJson.createObject();

  aJsonObject *sender = aJson.createObject();
  aJson.addStringToObject(sender, "name", "Mario -- Arduino");
  aJson.addItemToObject(msg, "sender", sender);
  return msg;
}

/* Process message like: { "Mario": { "30": 0, "31": 128 } } */
void processActuatorInfo(aJsonObject *item)
{
  aJsonObject *actuators = aJson.getObjectItem(item, "Mario");
  if (!actuators) {
    Serial.println("not for me");
    return;
  }

  const static int pins[] = { 30, 31, 32, 33, 34, 35 };
  const static int pins_n = 6;
  for (int i = 0; i < pins_n; i++) {
    char pinstr[3];
    snprintf(pinstr, sizeof(pinstr), "%d", pins[i]);

    aJsonObject *actuatorsVal = aJson.getObjectItem(actuators, pinstr);
    if (!actuatorsVal) continue; /* Value not provided, ok. */
    if (actuatorsVal->type != aJson_Int) {
      Serial.print(" invalid data type ");
      Serial.print(actuatorsVal->type, DEC);
      Serial.print(" for pin ");
      Serial.println(pins[i], DEC);
      continue;
    }

    Serial.print(" setting pin ");
    Serial.print(pins[i], DEC);
    Serial.print(" to value ");
    Serial.println(actuatorsVal->valueint, DEC);
    analogWrite(pins[i], actuatorsVal->valueint);
  }
}

void dumpMessage(Stream &s, aJsonObject *msg)
{
  int msg_count = aJson.getArraySize(msg);
  for (int i = 0; i < msg_count; i++) {
    aJsonObject *item, *sender, *analog, *value;
    //s.print("Msg #");
    //s.println(i, DEC);

    item = aJson.getArrayItem(msg, i);
    if (!item) { s.println("item not acquired"); delay(1000); return; }

    processActuatorInfo(item);

    /* Below, we parse and dump messages from fellow Arduinos. */

    sender = aJson.getObjectItem(item, "sender");
    if (!sender) { s.println("sender not acquired"); delay(1000); return; }

    s.print(" A2: ");
    analog = aJson.getObjectItem(item, "analog");
    if (!analog) { s.println("analog not acquired"); delay(1000); return; }
    value = aJson.getArrayItem(analog, 2);
    if (!value) { s.println("analog[2] not acquired"); delay(1000); return; }
    s.print(value->valueint, DEC);

    s.println();
  }
}

/***********************************************************************************
 ***********************************************************************************
 * Actuator Pin Set-Up
 ***********************************************************************************
 **********************************************************************************/

int light1 = 30;
int light2 = 31;
int waterTrigger = 32;
int nutrientDoser = 33;
int phUP = 34;
int phDOWN = 35;

/***********************************************************************************
 ***********************************************************************************
 * End Pin Set-Up
 ***********************************************************************************
 **********************************************************************************/

 

/***********************************************************************************
 ***********************************************************************************
 * Lux Sensor Set-Up
 ***********************************************************************************
 **********************************************************************************/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

/*************************************************************************
    Configures the gain and integration time for the TSL2561
*************************************************************************/
void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above!   
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
  */
}

/***********************************************************************************
 ***********************************************************************************
 * End Lux Sensor Set-Up
 ***********************************************************************************
 **********************************************************************************/


/***********************************************************************************
 ***********************************************************************************
 * Tentacle Set-Up
 ***********************************************************************************
 **********************************************************************************/

#include <SoftwareSerial.h>              //Include the software serial library  
#include <Wire.h>                   //enable I2C.

SoftwareSerial sSerial(11, 10);          // RX, TX  - Name the software serial library sSerial (this cannot be omitted)
                                         // assigned to pins 10 and 11 for maximum compatibility

const int s0 = 7;                        //Arduino pin 7 to control pin S0
const int s1 = 6;                        //Arduino pin 6 to control pin S1
const int enable_1 = 5;                //Arduino pin 5 to control pin E on shield 1
const int enable_2 = 4;                  //Arduino pin 4 to control pin E on shield 2

char sensordata[30];                     //A 30 byte character array to hold incoming data from the sensors
byte computer_bytes_received = 0;        //We need to know how many characters bytes have been received
byte sensor_bytes_received = 0;          //We need to know how many characters bytes have been received
int tenticleChannel;                             //INT pointer for channel switching - 0-7 serial, 8-127 I2C addresses
char *cmd;                               //Char pointer used in string parsing
int retries;                             // com-check functions store number of retries here
boolean answerReceived;                  // com-functions store here if a connection-attempt was successful
byte error;                              // error-byte to store result of Wire.transmissionEnd()

String stamp_type;                       // hold the name / type of the stamp
char stamp_version[4];                   // hold the version of the stamp

char computerdata[20];             //we make a 20 byte character array to hold incoming data from a pc/mac/other.

byte i2c_response_code = 0;              //used to hold the I2C response code.
byte in_char = 0;                    //used as a 1 byte buffer to store in bound bytes from an I2C stamp.

/***********************************************************************************
 ***********************************************************************************
 * End Tentacle Sensor Set-Up
 ***********************************************************************************
 **********************************************************************************/

int loopCounter = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Serial set up");

  pinMode(s1, OUTPUT);                   //Set the digital pin as output.
  pinMode(s0, OUTPUT);                   //Set the digital pin as output.
  pinMode(enable_1, OUTPUT);             //Set the digital pin as output.
  pinMode(enable_2, OUTPUT);             //Set the digital pin as output.
  sSerial.begin(38400);                  // Set the soft serial port to 38400
  Wire.begin();                          // enable I2C port.
  

  while (!Ethernet.begin(mac)) {
    Serial.println("Ethernet setup error");
    delay(1000);
  }
  Serial.println("Ethernet set up");

  PubNub.begin(pubkey, subkey);
  Serial.println("PubNub set up");

   /******** Initialise the Lux Sensor ********/
  if(!tsl.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  configureSensor();
  /******** End Lux initialization ********/

  /******** Temp-Humid Sensor ********/
  dht.begin();
  /******** End Temp-Humid init ********/
  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);
  pinMode(waterTrigger, OUTPUT);
  pinMode(nutrientDoser, OUTPUT);
  pinMode(phUP, OUTPUT);
  pinMode(phDOWN, OUTPUT);
  digitalWrite(light1, HIGH);
  digitalWrite(light2, HIGH);
  digitalWrite(waterTrigger, HIGH);
  digitalWrite(nutrientDoser, HIGH);
  digitalWrite(phUP, HIGH);
  digitalWrite(phDOWN, HIGH);
}

/**********************************************************************************
 ***********************************************************************************
 * End Ethernet+PubNub Set-Up
 ***********************************************************************************
 **********************************************************************************/


void loop() {
  int lux = 0;
  /********** Lux Sensor Start **********/
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    lux = event.light;
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println("Sensor overload");
  }
  /********** End Lux Sensor **********/

  
  /********** Start Temp-Humid Sensor **********/
  int h = dht.readHumidity();
  int f = dht.readTemperature(true);

  //Validate Temp-Humid Readings
  if (isnan(h) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  /********** End Temp-Humid Sensor **********/

  float waterTemp = getWaterTemp();
  waterTemp = (waterTemp*1.8)+32.0;

  Ethernet.maintain();

  EthernetClient *client;

  /* Publish */

  Serial.print("publishing a message: ");
  aJsonObject *msg = createMessage();

 /*Add to message aJsonObject */
  aJsonObject *luxData = aJson.createItem(lux);
  aJson.addItemToObject(msg, "lux", luxData);
  aJsonObject *humidityData = aJson.createItem(h);
  aJson.addItemToObject(msg, "humidity", humidityData);
  aJsonObject *tempData = aJson.createItem(f);
  aJson.addItemToObject(msg, "airTemp", tempData);
  aJsonObject *watertempData = aJson.createItem(waterTemp);
  aJson.addItemToObject(msg, "waterTemp", watertempData);

  tenticleChannel = 99;
  cmd = "r";
  I2C_call();  // send i2c command and wait for answer
  if (sensor_bytes_received > 0) {
    Serial.print("Current pH: ");
    Serial.println(sensordata);       //print the data.
    aJsonObject *phData = aJson.createItem(sensordata);
    aJson.addItemToObject(msg, "pH", phData);
  }

  tenticleChannel = 100;
  cmd = "r";
  I2C_call();  // send i2c command and wait for answer
  if (sensor_bytes_received > 0) {
    Serial.print("Current pH: ");
    Serial.println(sensordata);       //print the data.
    aJsonObject *ecData = aJson.createItem(sensordata);
    aJson.addItemToObject(msg, "EC", ecData);
  }
  

  
  char *msgStr = aJson.print(msg);
  aJson.deleteItem(msg);

  // msgStr is returned in a buffer that can be potentially
  // needlessly large; this call will "tighten" it
  msgStr = (char *) realloc(msgStr, strlen(msgStr) + 1);
  if (loopCounter == 233) {
      Serial.println(msgStr);
      client = PubNub.publish(channel, msgStr);
      free(msgStr);
      if (!client) {
        Serial.println("publishing error");
        delay(1000);
        return;
      }
      client->stop();
      loopCounter = 0;
    }

  /* Subscribe and load reply */

  Serial.println("waiting for a message (subscribe)");
  client = PubNub.subscribe(channel);
  if (!client) {
    Serial.println("subscription error");
    delay(1000);
    return;
  }

  /* Parse */

  aJsonClientStream stream(client);
  msg = aJson.parse(&stream);
  client->stop();
  if (!msg) { Serial.println("parse error"); delay(1000); return; }
  dumpMessage(Serial, msg);
  aJson.deleteItem(msg);

  loopCounter += 1;
}


/*tentacle shield stuffems*/

byte I2C_call() {           //function to parse and call I2C commands.
  sensor_bytes_received = 0;                            // reset data counter
  memset(sensordata, 0, sizeof(sensordata));            // clear sensordata array;

  Wire.beginTransmission(tenticleChannel);                  //call the circuit by its ID number.
  Wire.write(cmd);                      //transmit the command that was sent through the serial port.
  Wire.endTransmission();                           //end the I2C data transmission.

  i2c_response_code = 254;
  while (i2c_response_code == 254) {      // in case the cammand takes longer to process, we keep looping here until we get a success or an error

    if (String(cmd).startsWith(F("cal")) || String(cmd).startsWith(F("Cal")) ) {
      delay(1400);                        // cal-commands take 1300ms or more
    } else if (String(cmd) == F("r") || String(cmd) == F("R")) {
      delay(1000);                        // reading command takes about a second
    }
    else {
      delay(300);                         // all other commands: wait 300ms
    }

    Wire.requestFrom(tenticleChannel, 48, 1);     //call the circuit and request 48 bytes (this is more then we need).
    i2c_response_code = Wire.read();      //the first byte is the response code, we read this separately.

    while (Wire.available()) {            //are there bytes to receive.
      in_char = Wire.read();              //receive a byte.

      if (in_char == 0) {                 //if we see that we have been sent a null command.
        Wire.endTransmission();           //end the I2C data transmission.
        break;                            //exit the while loop.
      }
      else {
        sensordata[sensor_bytes_received] = in_char;        //load this byte into our array.
        sensor_bytes_received++;
      }
    }

    switch (i2c_response_code) {         //switch case based on what the response code is.
      case 1:                          //decimal 1.
        Serial.println( F("< success"));     //means the command was successful.
        break;                           //exits the switch case.

      case 2:                          //decimal 2.
        Serial.println( F("< command failed"));     //means the command has failed.
        break;                           //exits the switch case.

      case 254:                        //decimal 254.
        Serial.println( F("< command pending"));    //means the command has not yet been finished calculating.
        break;                           //exits the switch case.

      case 255:                        //decimal 255.
        Serial.println( F("No Data"));    //means there is no further data to send.
        break;                           //exits the switch case.
    }
  }
}

boolean check_i2c_connection() {                      // check selected i2c channel/address. verify that it's working by requesting info about the stamp

  retries = 0;

  while (retries < 3) {
    retries++;
    Wire.beginTransmission(tenticleChannel);      // just do a short connection attempt without command to scan i2c for devices
    error = Wire.endTransmission();

    if (error == 0)                       // if error is 0, there's a device
    {

      int r_retries = 0;
      while (r_retries < 3) {
        cmd = "i";                          // set cmd to request info (in I2C_call())
        I2C_call();

        if (parseInfo()) {
          return true;
        }
      }
      return false;
    }
    else
    {
      return false;                      // no device at this address
    }
  }

}



boolean parseInfo() {                  // parses the answer to a "i" command. returns true if answer was parseable, false if not.

  // example:
  // PH EZO  -> '?I,pH,1.1'
  // ORP EZO -> '?I,OR,1.0'   (-> wrong in documentation 'OR' instead of 'ORP')
  // DO EZO  -> '?I,D.O.,1.0' || '?I,DO,1.7' (-> exists in D.O. and DO form)
  // EC EZO  -> '?I,EC,1.0 '

  // Legazy PH  -> 'P,V5.0,5/13'
  // Legazy ORP -> 'O,V4.4,2/13'
  // Legazy DO  -> 'D,V5.0,1/13'
  // Legazy EC  -> 'E,V3.1,5/13'

  if (sensordata[0] == '?' && sensordata[1] == 'I') {          // seems to be an EZO stamp

    // PH EZO
    if (sensordata[3] == 'p' && sensordata[4] == 'H') {
      stamp_type = F("pH EZO");
      stamp_version[0] = sensordata[6];
      stamp_version[1] = sensordata[7];
      stamp_version[2] = sensordata[8];
      stamp_version[3] = 0;

      return true;

      // ORP EZO
    }
    else if (sensordata[3] == 'O' && sensordata[4] == 'R') {
      stamp_type = F("ORP EZO");
      stamp_version[0] = sensordata[6];
      stamp_version[1] = sensordata[7];
      stamp_version[2] = sensordata[8];
      stamp_version[3] = 0;
      return true;

      // DO EZO
    }
    else if (sensordata[3] == 'D' && sensordata[4] == 'O') {
      stamp_type = F("D.O. EZO");
      stamp_version[0] = sensordata[6];
      stamp_version[1] = sensordata[7];
      stamp_version[2] = sensordata[8];
      stamp_version[3] = 0;
      return true;

      // D.O. EZO
    }
    else if (sensordata[3] == 'D' && sensordata[4] == '.' && sensordata[5] == 'O' && sensordata[6] == '.') {
      stamp_type = F("D.O. EZO");
      stamp_version[0] = sensordata[8];
      stamp_version[1] = sensordata[9];
      stamp_version[2] = sensordata[10];
      stamp_version[3] = 0;
      return true;

      // EC EZO
    }
    else if (sensordata[3] == 'E' && sensordata[4] == 'C') {
      stamp_type = F("EC EZO");
      stamp_version[0] = sensordata[6];
      stamp_version[1] = sensordata[7];
      stamp_version[2] = sensordata[8];
      stamp_version[3] = 0;
      return true;

      // unknown EZO stamp
    }
    else {
      stamp_type = F("unknown EZO stamp");
      return true;
    }

  }

  // it's a legacy stamp (non-EZO)
  else
  {
    // Legacy pH
    if ( sensordata[0] == 'P') {
      stamp_type = F("pH (legacy)");
      stamp_version[0] = sensordata[3];
      stamp_version[1] = sensordata[4];
      stamp_version[2] = sensordata[5];
      stamp_version[3] = 0;
      return true;

      // legacy ORP
    }
    else if ( sensordata[0] == 'O') {
      stamp_type = F("ORP (legacy)");
      stamp_version[0] = sensordata[3];
      stamp_version[1] = sensordata[4];
      stamp_version[2] = sensordata[5];
      stamp_version[3] = 0;
      return true;

      // Legacy D.O.
    }
    else if ( sensordata[0] == 'D') {
      stamp_type = F("D.O. (legacy)");
      stamp_version[0] = sensordata[3];
      stamp_version[1] = sensordata[4];
      stamp_version[2] = sensordata[5];
      stamp_version[3] = 0;
      return true;

      // Lecagy EC
    }
    else if ( sensordata[0] == 'E') {
      stamp_type = F("EC (legacy)");
      stamp_version[0] = sensordata[3];
      stamp_version[1] = sensordata[4];
      stamp_version[2] = sensordata[5];
      stamp_version[3] = 0;
      return true;
    }
  }

  /*
  Serial.println("can not parse data: ");
   Serial.print("'");
   Serial.print(sensordata);
   Serial.println("'");
   */
  return false;        // can not parse this info-string
}



void clearIncomingBuffer() {          // "clears" the incoming soft-serial buffer
  while (sSerial.available() ) {
    //Serial.print((char)sSerial.read());
    sSerial.read();
  }
}

/*end of tenticle stuffems*/


/*Start of waterTemp stuffems*/
float getWaterTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
  
}

