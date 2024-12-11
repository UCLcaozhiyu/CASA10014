#define analogPin A0
#define ledPin 3
#define Vcc 5.0 //Board voltage

void setup() {
  Serial.begin(9600);
}

void loop() {
  int analogValue = analogRead(analogPin);
  float voltage = Vcc * analogValue / 1024.0;
  Serial.print("Voltage is: ");
  Serial.println(voltage);
  int dutyCycle = map(analogValue, 0, 1023, 0, 255);
  analogWrite(ledPin, dutyCycle);
  
  delay(100);
}
