byte relON[] = {0xA0, 0x01, 0x01, 0xA2}; 
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};

int ledPin = 1;
int relayPin = 0;

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  delay(5000);
  digitalWrite(ledPin, HIGH);
}

void serialMode() {
  Serial.begin(9600);
  Serial.write(relON, sizeof(relON));
  Serial.flush();
  delay(2000);

  Serial.write(relOFF, sizeof(relOFF));
  Serial.flush();
  Serial.end();
  delay(2000);
}

void digitalMode() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  delay(2000);

  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, HIGH);
  delay(2000);
}

void loop(void){
  digitalMode();
}