
const int sensorPIR = 7; 
const int buzzer    = 8;  

void setup() {
  pinMode(sensorPIR, INPUT); 
  pinMode(buzzer, OUTPUT);     

  Serial.begin(9600);         
}

void loop() {
  int leitura = digitalRead(sensorPIR);  

  if (leitura == HIGH) {
 
    digitalWrite(buzzer, HIGH);
    Serial.println("Movimento detectado! Buzzer LIGADO.");
  } else {
  
    digitalWrite(buzzer, LOW);
    Serial.println("Sem movimento. Buzzer DESLIGADO.");
  }

  delay(100);
}
