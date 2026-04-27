int RXLED = 17; // The RX LED has a defined Arduino pin
int TXLED = 30; // The TX LED has a defined Arduino pin

// the setup function runs once when you press reset or power the board
void setup() {
// initialize digital pin LED_BUILTIN as an output.
pinMode(LED_BUILTIN, OUTPUT);
pinMode(RXLED, OUTPUT); // Set RX LED as an output
pinMode(TXLED, OUTPUT); // Set TX LED as an output
}

// the loop function runs over and over again forever
void loop() {
digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
digitalWrite(RXLED, HIGH); // set the LED off
digitalWrite(TXLED, HIGH); // set the LED off
delay(1000); // wait for a second
digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
digitalWrite(RXLED, LOW); // set the LED on
digitalWrite(TXLED, LOW); // set the LED on
delay(1000); // wait for a second
}