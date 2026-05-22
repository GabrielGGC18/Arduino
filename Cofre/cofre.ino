#include <Keypad.h>
#include <Servo.h>

String SENHA    = "1234";
String digitada = "";
int    erros    = 0;

Servo trava;
const int PINO_SERVO   = 10;
const int LED_VERDE    = 11;
const int LED_VERMELHO = 12;
const int BUZZER       = 13;

const byte LINHAS  = 4;
const byte COLUNAS = 4;

char teclas[LINHAS][COLUNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte pinosLinhas[LINHAS]   = {9, 8, 7, 6};
byte pinosColunas[COLUNAS] = {5, 4, 3, 2};

Keypad teclado = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  trava.attach(PINO_SERVO);
  trava.write(0);

  Serial.begin(9600);
  Serial.println("===== COFRE ELETRONICO =====");
  Serial.println("Digite a senha e pressione # para confirmar");
  Serial.println("(Pressione * para apagar)");
}

void loop() {
  char tecla = teclado.getKey();

  if (tecla) {
    if (tecla == '#') {
      verificarSenha();
    }
    else if (tecla == '*') {
      digitada = "";
      Serial.println("\n[Senha apagada]");
    }
    else {
      digitada += tecla;
      Serial.print("*");
    }
  }
}

void verificarSenha() {
  Serial.println();

  if (digitada == SENHA) {
    abrirCofre();
  } else {
    senhaErrada();
  }

  digitada = "";
}

void abrirCofre() {
  Serial.println(">> SENHA CORRETA! Cofre ABERTO.");
  erros = 0;

  digitalWrite(LED_VERDE, HIGH);
  trava.write(90);

  delay(3000);

  trava.write(0);
  digitalWrite(LED_VERDE, LOW);
  Serial.println("Cofre trancado. Aguardando nova senha...\n");
}

void senhaErrada() {
  erros++;
  Serial.print(">> SENHA ERRADA! Tentativas erradas: ");
  Serial.println(erros);

  digitalWrite(LED_VERMELHO, HIGH);
  tocarAlarme();
  delay(2000);
  digitalWrite(LED_VERMELHO, LOW);

  if (erros >= 3) {
    bloquearSistema();
  } else {
    Serial.println("Tente novamente.\n");
  }
}

void tocarAlarme() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, 1000);
    delay(200);
    noTone(BUZZER);
    delay(100);
  }
}

void bloquearSistema() {
  Serial.println("!!! SISTEMA BLOQUEADO POR 30 SEGUNDOS !!!");
  digitalWrite(LED_VERMELHO, HIGH);

  for (int i = 30; i > 0; i--) {
    Serial.print("Desbloqueio em ");
    Serial.print(i);
    Serial.println("s");
    delay(1000);
  }

  digitalWrite(LED_VERMELHO, LOW);
  erros = 0;
  Serial.println("Sistema LIBERADO. Pode tentar novamente.\n");
}