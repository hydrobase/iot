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
int channel;                             //INT pointer for channel switching - 0-7 serial, 8-127 I2C addresses
char *cmd;                               //Char pointer used in string parsing
int retries;                             // com-check functions store number of retries here
boolean answerReceived;                  // com-functions store here if a connection-attempt was successful
byte error;                              // error-byte to store result of Wire.transmissionEnd()

String stamp_type;                       // hold the name / type of the stamp
char stamp_version[4];                   // hold the version of the stamp

char computerdata[20];             //we make a 20 byte character array to hold incoming data from a pc/mac/other.

byte i2c_response_code = 0;              //used to hold the I2C response code.
byte in_char = 0;                    //used as a 1 byte buffer to store in bound bytes from an I2C stamp.

const long validBaudrates[] = {          // list of baudrates to try when connecting to a stamp (they're ordered by probability to speed up things a bit)
  38400, 19200, 9600, 115200, 57600
};
long channelBaudrate[] = {               // store for the determined baudrates for every stamp
  0, 0, 0, 0, 0, 0, 0, 0
};

boolean I2C_mode = false;    //bool switch for serial/I2C

void setup() {
  pinMode(s1, OUTPUT);                   //Set the digital pin as output.
  pinMode(s0, OUTPUT);                   //Set the digital pin as output.
  pinMode(enable_1, OUTPUT);             //Set the digital pin as output.
  pinMode(enable_2, OUTPUT);             //Set the digital pin as output.

  Serial.begin(9600);                    // Set the hardware serial port to 38400
  while (!Serial) ;                      // Leonardo-type arduinos need this to be able to write to the serial port in setup()
  sSerial.begin(38400);                  // Set the soft serial port to 38400
  Wire.begin();                    // enable I2C port.

  stamp_type.reserve(16);                // reserve string buffer to save some SRAM

}

void loop() {
  channel = 99;
  cmd = "r";
  // put your main code here, to run repeatedly:
  I2C_call();  // send i2c command and wait for answer
  if (sensor_bytes_received > 0) {
    Serial.print("Current pH: ");
    Serial.println(sensordata);       //print the data.
  }
  delay(3000);
  channel = 100;
  I2C_call();  // send i2c command and wait for answer
  if (sensor_bytes_received > 0) {
    Serial.print("Current EC: ");
    Serial.println(sensordata);       //print the data.
  }
  delay(3000);
}

byte I2C_call() {           //function to parse and call I2C commands.
  sensor_bytes_received = 0;                            // reset data counter
  memset(sensordata, 0, sizeof(sensordata));            // clear sensordata array;

  Wire.beginTransmission(channel);                  //call the circuit by its ID number.
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

    Wire.requestFrom(channel, 48, 1);     //call the circuit and request 48 bytes (this is more then we need).
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
    Wire.beginTransmission(channel);      // just do a short connection attempt without command to scan i2c for devices
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
