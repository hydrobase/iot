int light1 = 4; 
int waterPump = 2;
int waterLoop = 0;
int lightLoop = 0;

void setup() {
  Serial.begin(9600);
  pinMode(light1, OUTPUT);
  pinMode(waterPump, OUTPUT);
}



void loop() {
  if (lightLoop <= 48) {
      digitalWrite(light1, HIGH);
      Serial.println("-------------------------");
      Serial.print("this is light loop  nested in HIGH: ");
      Serial.println(lightLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  } else if (lightLoop < 96) {
      digitalWrite(light1, LOW);
      Serial.println("-------------------------");
      Serial.print("this is light loop  nested in LOW: ");
      Serial.println(lightLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  } else if (lightLoop == 96) {
      lightLoop = 0;
      Serial.println("-------------------------");
      Serial.print("this is light loop  nested in RESET: ");
      Serial.println(lightLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  }
  
  if (waterLoop == 1) {
      digitalWrite(waterPump, HIGH);
      Serial.println("-------------------------");
      Serial.print("this is water loop  nested in HIGH: ");
      Serial.println(waterLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  } else if (waterLoop == 2) {
      digitalWrite(waterPump, LOW);
      waterLoop = 0;
      Serial.println("-------------------------");
      Serial.print("this is water loop  nested in RESET: ");
      Serial.println(waterLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  }
  // We'll leave it on for 4 Hours
  waterLoop += 1;
  lightLoop += 1;
  Serial.println("-------------------------");
      Serial.print("this is light loop  nested in ADD ONE: ");
      Serial.println(lightLoop);
      Serial.println("-------------------------");
      Serial.println(" ");
      Serial.println(" ");
  delay(3000);
}


