 int MAINPIN = A0;
 int BLINKPIN = 13;

 long watertime = 30000;
 long waitTime = 120000;

void setup() {
  // put your setup code here, to run once:
  pinMode(MAINPIN, OUTPUT);
  pinMode(BLINKPIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(MAINPIN, HIGH);
  digitalWrite(BLINKPIN, HIGH);
  delay(watertime);
  digitalWrite(MAINPIN, LOW);
  digitalWrite(BLINKPIN, LOW);
  delay(waitTime);
}
