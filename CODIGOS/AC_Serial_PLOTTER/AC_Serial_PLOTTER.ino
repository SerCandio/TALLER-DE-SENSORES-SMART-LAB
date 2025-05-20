void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.print("raw AC");
  Serial.println(analogRead(A0));
  delayMicroseconds(1000);
}