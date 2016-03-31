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

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
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
 * Start WiFi Set-Up
 ***********************************************************************************
 **********************************************************************************/

#include <SPI.h>
#include <WiFi101.h>

char ssid[] = "BPD-Surveillance-Van1";     //  your network SSID (name)
char pass[] = "Big_C1310";  // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

/**********************************************************************************
 ***********************************************************************************
 * End WiFi Set-Up
 ***********************************************************************************
 **********************************************************************************/
 
void setup() {
  Serial.begin(9600);

  /********Connect Via WiFi********/
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  /******** End WiFi Connect ********/

  
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

  
}

void loop() {
  
  /********** Lux Sensor Start **********/
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    Serial.print("Lux: ");
    Serial.println(event.light); 
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println("Sensor overload");
  }
  /********** End Lux Sensor **********/

  /********** Start Temp-Humid Sensor **********/
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);

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
  
  delay(2000);

}
