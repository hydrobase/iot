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
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
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
 * Start Ethernet+PubNub Set-Up
 ***********************************************************************************
 **********************************************************************************/
#include <SPI.h>
#include <Ethernet.h>
#include <PubNub.h>

// Some Ethernet shields have a MAC address printed on a sticker on the shield;
// fill in that address here, or choose your own at random:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

char pubkey[] = "pub-c-5bef0e0e-e177-4a94-b06f-ce3c1355e5cd";
char subkey[] = "sub-c-3c8b80da-de67-11e5-abb9-0619f8945a4f";
char channel[] = "tester";
/**********************************************************************************
 ***********************************************************************************
 * End Ethernet+PubNub Set-Up
 ***********************************************************************************
 **********************************************************************************/

int loopCounter = 0;
int light1 = 30;
int light2 = 31;
int waterTrigger = 24;
int waterLoop = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Serial set up");

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
  digitalWrite(light1, HIGH);
  digitalWrite(light2, HIGH);
}

void loop() {
  int lux = 0;

  /********** Lux Sensor Start **********/
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    Serial.print("Lux: ");
    Serial.println(event.light); 
    lux = event.light;
    Serial.println(lux);
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
  Serial.print("Humidity: ");
  Serial.println(h);
  Serial.print("Temp: ");
  Serial.print(f);
  Serial.println(" *F\t");
  Serial.println("\n");
  
  /********** End Temp-Humid Sensor **********/

  Ethernet.maintain();

  EthernetClient *client;
  
  char msg[64] = "{\"lux\":";
  sprintf(msg + strlen(msg), "%d", lux);
  strcat(msg, ",");
  strcat(msg, "\"humid\":");
  sprintf(msg + strlen(msg), "%d", h);
  strcat(msg, ",");
  strcat(msg, "\"temp\":");
  sprintf(msg + strlen(msg), "%d", f);
  strcat(msg, "}");

  Serial.print("publishing message: ");
  Serial.println(msg);
  client = PubNub.publish(channel, msg);
  if (!client) {
    Serial.println("publishing error");
  } else {
    client->stop();
  }

  if (loopCounter == 3) {
    digitalWrite(light1, LOW);
    digitalWrite(light2, LOW);
  }
  if (loopCounter == 6) {
    digitalWrite(light1, HIGH);
    digitalWrite(light2, HIGH);
    Serial.println("Lights HIGH");
    loopCounter = 0;
  }
  if (waterLoop == 1) {
    Serial.println("this is working");
    digitalWrite(waterTrigger, HIGH);
  }
  if (waterLoop == 2) {
    digitalWrite(waterTrigger, LOW);
    waterLoop = 0;
  }
  
  Serial.print("This is waterLoop: ");
  Serial.println(waterLoop);
  Serial.print("This is loopCounter: ");
  Serial.println(loopCounter);
  loopCounter += 1;
  waterLoop += 1;
  delay(900000);

}
