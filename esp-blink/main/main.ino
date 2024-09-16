/*
ESP8266 Blink
Blink the blue LED on the ESP8266 module
*/

#define LED 1   // LED ESP-01
//#define LED 2  // LED NodeMCU / ESP-01s (LED_BUILTIN)


void setup() {
  pinMode(LED, OUTPUT); // Initialize the LED pin as an output
}
// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED, LOW); // Turn the LED on (Note that LOW is the voltage level)
  delay(1000); 
  
  digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
  delay(1000);
}
