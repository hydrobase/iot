
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
#include <aJSON.h>

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

int light1 = 30;
int light2 = 31;
int waterTrigger = 42;
int nutrientDoser = 43;
int phUP = 44;
int phDOWN = 45;

aJsonObject *createMessage()
{
  aJsonObject *msg = aJson.createObject();

  aJsonObject *sender = aJson.createObject();
  aJson.addStringToObject(sender, "name", "Arduino--Mario");
  aJson.addItemToObject(msg, "sender", sender);

  int analogValues[6];
  for (int i = 0; i < 6; i++) {
    analogValues[i] = analogRead(i);
  }
  aJsonObject *analog = aJson.createIntArray(analogValues, 6);
  aJson.addItemToObject(msg, "sensors", analog);
  return msg;
}

/* Process message like: { "pwm": { "8": 0, "9": 128 } } */
void processPwmInfo(aJsonObject *item)
{
  aJsonObject *pwm = aJson.getObjectItem(item, "actuators");
  if (!pwm) {
    Serial.println("no pwm data");
    return;
  }

  const static int pins[] = { 30, 31, 42, 43, 44, 45 };
  const static int pins_n = 6;
  for (int i = 0; i < pins_n; i++) {
    char pinstr[3];
    snprintf(pinstr, sizeof(pinstr), "%d", pins[i]);

    aJsonObject *pwmval = aJson.getObjectItem(pwm, pinstr);
    if (!pwmval) continue; /* Value not provided, ok. */
    if (pwmval->type != aJson_Int) {
      Serial.print(" invalid data type ");
      Serial.print(pwmval->type, DEC);
      Serial.print(" for pin ");
      Serial.println(pins[i], DEC);
      continue;
    }

    Serial.print(" setting pin ");
    Serial.print(pins[i], DEC);
    Serial.print(" to value ");
    Serial.println(pwmval->valueint, DEC);
    analogWrite(pins[i], pwmval->valueint);
  }
}

void dumpMessage(Stream &s, aJsonObject *msg)
{
  int msg_count = aJson.getArraySize(msg);
  for (int i = 0; i < msg_count; i++) {
    aJsonObject *item, *sender, *analog, *value;
    s.print("Msg #");
    s.println(i, DEC);

    item = aJson.getArrayItem(msg, i);
    if (!item) { s.println("item not acquired"); delay(1000); return; }

    processPwmInfo(item);

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

void setup() {
  // put your setup code here, to run once:
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

  /* Publish */

  Serial.print("publishing a message: ");
  aJsonObject *msg = createMessage();
  char *msgStr = aJson.print(msg);
  aJson.deleteItem(msg);

  // msgStr is returned in a buffer that can be potentially
  // needlessly large; this call will "tighten" it
  msgStr = (char *) realloc(msgStr, strlen(msgStr) + 1);

  Serial.println(msgStr);
  client = PubNub.publish(channel, msgStr);
  free(msgStr);
  if (!client) {
    Serial.println("publishing error");
    delay(1000);
    return;
  }
  client->stop();

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

  
  /*
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
  */
  delay(50);
}
