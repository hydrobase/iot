int light1 = 4; 
int waterPump = 2;
int waterLoop = 1;
int lightLoop = 0;

void setup() {
  //Serial.begin(9600);
  pinMode(light1, OUTPUT);
  pinMode(waterPump, OUTPUT);
}



void loop() {
  if (lightLoop <= 12) {
      digitalWrite(light1, HIGH);
  } else if (lightLoop < 24) {
      digitalWrite(light1, LOW);
  } else if (lightLoop == 24) {
      lightLoop = 0;
  }
  
  if (waterLoop == 1) {
      digitalWrite(waterPump, HIGH);
  } else if (waterLoop == 2) {
      digitalWrite(waterPump, LOW);
      waterLoop = 0;
  }
  // We'll leave it on for 4 Hours
  waterLoop += 1;
  lightLoop += 1;
  delay(3000);
}


