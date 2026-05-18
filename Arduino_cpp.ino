// ============================================
// EXERCÍCIO 1 - Sensor de movimento aciona som
// Componentes: Arduino Uno R3, Sensor PIR, Buzzer Ativo
// ============================================

// --- Definição dos pinos ---
const int sensorPIR = 7;  // Pino digital do sensor PIR (OUT)
const int buzzer    = 8;  // Pino digital do buzzer ativo

void setup() {
  pinMode(sensorPIR, INPUT);   // PIR como ENTRADA (lê o sinal)
  pinMode(buzzer, OUTPUT);     // Buzzer como SAÍDA (recebe o comando)

  Serial.begin(9600);          // Inicia o monitor serial (opcional, para debug)
}

void loop() {
  int leitura = digitalRead(sensorPIR);  // Lê o sinal do PIR

  if (leitura == HIGH) {
    // Movimento detectado → liga o buzzer
    digitalWrite(buzzer, HIGH);
    Serial.println("Movimento detectado! Buzzer LIGADO.");
  } else {
    // Sem movimento → desliga o buzzer
    digitalWrite(buzzer, LOW);
    Serial.println("Sem movimento. Buzzer DESLIGADO.");
  }

  delay(100); // Pequena pausa para estabilizar a leitura
}
