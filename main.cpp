#include <ESP32Servo.h>
#include <Arduino.h>

// Constante
#define CHOVENDO 2000
#define CLAREZA 2000

// entradas
#define buttonCloseAzul 34       // digital
#define buttonAutomaticoVerde 33 // digital
#define sensorChuva 12           // analogico
#define ldr 13                   // analogico

// saidas
#define servoMotor 21            // analogico
#define motorVaral 18            // digital
#define ledModoClosedAzul 4      // digital
#define ledModoAutomaticoVerde 2 ////digital

// Controle modo automatico
bool modoAutomatico = LOW;
bool btnAutomaticoState;
bool lastBtnAutomaticoState = LOW;
unsigned long lastDebounceTimeAuto = 0;
unsigned long debounceDelayAuto = 50;

// Controle mode fechado
bool modoClose = LOW;
bool btnCloseState;
bool lastBtnCloseState = LOW;
unsigned long lastDebounceTimeClose = 0;
unsigned long debounceDelayClose = 50;

// estados
bool fechado = LOW;
bool chovendo = LOW;
bool escuro = LOW;

Servo servo;

// Controle do modo automatico
void setModoAutomaticoAtual()
{
  btnAutomaticoState = digitalRead(buttonAutomaticoVerde);
  if (btnAutomaticoState == LOW)
  {
    modoAutomatico = !modoAutomatico;
  }
  if (btnAutomaticoState != lastBtnAutomaticoState)
  {
    lastDebounceTimeAuto = millis();
  }
  if ((millis() - lastDebounceTimeAuto) > debounceDelayAuto)
  {
    if (btnAutomaticoState == LOW)
    {
      modoAutomatico = !modoAutomatico;
    }
  }
  lastBtnAutomaticoState = btnAutomaticoState;
  delay(100);
  if (modoAutomatico)
  {
    digitalWrite(ledModoAutomaticoVerde, HIGH);
  }
  else
  {
    digitalWrite(ledModoAutomaticoVerde, LOW);
  }
}

// Controlle mode Close
void setModeoCloseAtual()
{
  btnCloseState = digitalRead(buttonCloseAzul);
  if (btnCloseState == LOW)
  {
    modoClose = !modoClose;
  }
  if (btnCloseState != lastBtnCloseState)
  {
    lastDebounceTimeClose = millis();
  }
  if ((millis() - lastDebounceTimeClose) > debounceDelayClose)
  {
    if (btnCloseState == LOW)
    {
      modoClose = !modoClose;
    }
  }
  lastBtnCloseState = btnCloseState;
}

// Fechar janela e recpçher roupa
void fechar()
{
  if (!fechado)
  {
    fechado = HIGH;
    digitalWrite(motorVaral, HIGH);
    delay(500);
    digitalWrite(motorVaral, LOW);
    servo.write(180);
    digitalWrite(ledModoClosedAzul, HIGH);
  }
}

// Abrir  janela e recpçher roupa
void abrir()
{
  if (fechado)
  {
    fechado = LOW;
    digitalWrite(motorVaral, HIGH);
    delay(500);
    digitalWrite(motorVaral, LOW);
    servo.write(0);
    digitalWrite(ledModoClosedAzul, LOW);
  }
}

// Verifica se esta chovendo agora
bool verificarChovendoAgora()
{
  int nivelChuva = analogRead(sensorChuva);
  if (nivelChuva > CHOVENDO)
  {
    return HIGH;
  }
  else
  {
    return LOW;
  }
}

// Verifica se esta escuro agora
bool verificarEscuroAgora()
{
  int clareza = analogRead(ldr);
  if (clareza > CLAREZA)
  {
    return HIGH;
  }
  else
  {
    return LOW;
  }
}

// Envia situação atual para servidor
void enviarDadosServidor(bool chovendo1, bool escuro2)
{
  String chovendostr = "\"Não\"";
  if (chovendo1)
  {
    chovendostr = "\"Sim\"";
  }
  String escurostr = "\"Não\"";
  if (escuro2)
  {
    escurostr = "\"Sim\"";
  }

  Serial.println("{");
  Serial.println("\"chovendo\":" + chovendostr + ",");
  Serial.println("\"escuro\":" + escurostr + ",");
  Serial.println("\"longitudo\": 100000,");
  Serial.println("\"latitude\": 100000,");
  Serial.println("}");
}

// Verifica o comando dado no Serial
void verificarComando()
{
  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');

    if (comando == "auto()")
    {
      modoAutomatico = !modoAutomatico;
    }
    else if (comando == "fechar()")
    {
      modoClose = HIGH;
    }
    else if (comando == "abrir()")
    {
      modoClose = LOW;
    }
  }
}

void setup()
{
  // entradas
  pinMode(buttonCloseAzul, INPUT_PULLUP);
  pinMode(buttonAutomaticoVerde, INPUT_PULLUP);
  pinMode(sensorChuva, INPUT);
  pinMode(ldr, INPUT);
  // saidas
  pinMode(motorVaral, OUTPUT);
  pinMode(ledModoClosedAzul, OUTPUT);
  pinMode(ledModoAutomaticoVerde, OUTPUT);
  servo.attach(servoMotor);

  // Define os modos atuais e led
  digitalWrite(ledModoAutomaticoVerde, modoAutomatico);
  digitalWrite(ledModoClosedAzul, modoClose);
  servo.write(0);
  Serial.begin(9600);
  Serial.println("Iniciando");
}

void loop()
{
  setModoAutomaticoAtual();
  setModeoCloseAtual();

  verificarComando();

  // Verificar estado atuais
  bool escuroAgora = verificarEscuroAgora();
  bool chovendoAgora = verificarChovendoAgora();

  if ((chovendoAgora && !chovendo) || (escuroAgora && !escuro))
  {
    if (chovendoAgora && !chovendo)
    {
      Serial.println("Chovendo Agora");
    }
    if (escuroAgora && !escuro)
    {
      Serial.println("Escuro Agora");
    }
    
    // Chovendo ou escuro agora
    escuro = escuroAgora;
    chovendo = chovendoAgora;
    enviarDadosServidor(chovendo, escuro);
    if (modoAutomatico)
    {
      modoClose = HIGH;
    }
  }
  else if (!chovendoAgora && !escuroAgora && (escuro || chovendo))
  {
    // Não chovendo nem escuro agora
    escuro = LOW;
    chovendo = LOW;
    enviarDadosServidor(chovendo, escuro);
    if (modoAutomatico)
    {
      modoClose = LOW;
    }
  }
  if (modoAutomatico)
  {
    if ((chovendoAgora && !chovendo) || (escuroAgora && !escuro))
    {
      modoClose = HIGH;
    }
    else if (!chovendoAgora && !escuroAgora && (escuro || chovendo))
    {
      modoClose = LOW;
    }
  }

  if (modoClose)
  {
    fechar();
  }
  else
  {
    abrir();
  }
  delay(100);
}
