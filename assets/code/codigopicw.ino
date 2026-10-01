#include <SerialBT.h>

// ==================== PINES ====================
const int DIP1 = 16;
const int DIP2 = 17;
const int DIP3 = 18;
const int DIP4 = 19;
const int DIP5 = 20;
const int DIP6 = 21;
const int DIP7 = 22;
const int DIP8 = 26;

const int LED_ROJO     = 0;
const int LED_NARANJA  = 1;
const int LED_AMARILLO = 2;
const int LED_VERDE    = 3;
const int LED_AZUL     = 4;
const int LED_ONBOARD  = LED_BUILTIN;

const int BOMBA1_PIN = 7;   // pin físico 10
const int BOMBA2_PIN = 8;   // pin físico 11

const int SENSOR1_PIN = 27; // físico 32
const int SENSOR2_PIN = 28; // físico 34

// ==================== VARIABLES ====================
bool modoBluetooth = false;

bool bomba1Seleccionada = false;
bool bomba2Seleccionada = false;
int potenciaBomba1 = 0;
int potenciaBomba2 = 0;

bool modoValvs = false;
bool modoHume = false;
bool modoAuto = false;
bool autoRunning = false;

int potenciaAuto1 = 2;      // media por defecto
int potenciaAuto2 = 2;

// Umbrales de humedad (predeterminado: 0-4 → 8-10)
int umbralOn1  = 4;
int umbralOff1 = 8;
int umbralOn2  = 4;
int umbralOff2 = 8;

int sensorHumedadActivo = 0;   // 0=ninguno, 1 o 2

bool monitoreoH1 = false;
bool monitoreoH2 = false;
unsigned long ultimoMonitoreo = 0;

unsigned long ultimoParpadeo = 0;
bool estadoParpadeo = false;
unsigned long ultimoParpadeoOnboard = 0;
bool estadoOnboard = false;

unsigned long ultimoOla = 0;
int pasoOla = 0;
int ledsOla[] = {LED_ROJO, LED_NARANJA, LED_AMARILLO, LED_VERDE, LED_AZUL};

String bufferBT = "";

const int SECO   = 3100;
const int HUMEDO = 900;

// ==================== FUNCIONES ====================
void apagarTodo() {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_NARANJA, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);
  digitalWrite(LED_ONBOARD, LOW);
  analogWrite(BOMBA1_PIN, 0);
  analogWrite(BOMBA2_PIN, 0);
}

void setPotencia(int pin, int nivel) {
  if (nivel == 0) analogWrite(pin, 0);
  else if (nivel == 1) analogWrite(pin, 130);  // mínima
  else if (nivel == 2) analogWrite(pin, 180);  // media
  else if (nivel == 3) analogWrite(pin, 255);  // máxima
}

int leerHumedad(int pin) {
  long suma = 0;
  for (int i = 0; i < 12; i++) {
    suma += analogRead(pin);
    delay(2);
  }
  int valor = suma / 12;
  if (valor < 50) return -1;

  int nivel = map(valor, SECO, HUMEDO, 0, 10);
  if (valor > 2900) nivel = 0;
  else if (valor > 2700) nivel = 1;
  if (valor < 1300) nivel = 10;
  else if (valor < 1500) nivel = 9;
  else if (valor < 1700) nivel = 8;

  if (nivel < 0) nivel = 0;
  if (nivel > 10) nivel = 10;
  return nivel;
}

void mostrarBarraHumedad(int nivel) {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_NARANJA, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);

  if (nivel < 0) {
    if (millis() - ultimoParpadeo >= 300) {
      estadoParpadeo = !estadoParpadeo;
      ultimoParpadeo = millis();
    }
    digitalWrite(LED_ONBOARD, estadoParpadeo);
    return;
  }

  // Rojo (seco) → Azul (húmedo)
  if (nivel <= 2) {
    digitalWrite(LED_ROJO, HIGH);
  } else if (nivel <= 4) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_NARANJA, HIGH);
  } else if (nivel <= 6) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_NARANJA, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
  } else if (nivel <= 8) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_NARANJA, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
    digitalWrite(LED_VERDE, HIGH);
  } else {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_NARANJA, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_AZUL, HIGH);
  }
}

void ejecutarOla() {
  for (int i = 0; i < 5; i++) digitalWrite(ledsOla[i], LOW);
  digitalWrite(ledsOla[pasoOla], HIGH);

  if (millis() - ultimoOla >= 200) {
    pasoOla++;
    if (pasoOla > 4) pasoOla = 0;
    ultimoOla = millis();
  }
}

void enviarAyudaAuto() {
  SerialBT.println("======= MODO AUTO =======");
  SerialBT.println("POTENCIA:");
  SerialBT.println("val1.1  val1.2  val1.3");
  SerialBT.println("val2.1  val2.2  val2.3");
  SerialBT.println("valv.pre -> Ambas media");
  SerialBT.println("");
  SerialBT.println("UMBRAL HUMEDAD (1-9):");
  SerialBT.println("s1mi.1 ... s1mi.9");
  SerialBT.println("s2mi.1 ... s2mi.9");
  SerialBT.println("");
  SerialBT.println("endcon  -> Iniciar / Detener");
  SerialBT.println("h1 / h2 -> Ver humedad");
  SerialBT.println("auto    -> Salir");
  SerialBT.println("=========================");
}

void enviarAyudaValvs() {
  SerialBT.println("----- MODO VALVS -----");
  SerialBT.println("val1 / val2 / valv");
  SerialBT.println("val1.1 val1.2 val1.3");
  SerialBT.println("val2.1 val2.2 val2.3");
  SerialBT.println("valv.1 valv.2 valv.3");
  SerialBT.println("valvs -> Salir");
  SerialBT.println("----------------------");
}

void enviarAyudaHume() {
  SerialBT.println("----- MODO HUME -----");
  SerialBT.println("h1  h2  h1l  h2l");
  SerialBT.println("h1lc  h2lc");
  SerialBT.println("hume -> Salir");
  SerialBT.println("---------------------");
}

void setup() {
  SerialBT.setName("PicoRiego");
  SerialBT.begin();
  Serial.begin(115200);

  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);
  pinMode(DIP4, INPUT_PULLUP);
  pinMode(DIP5, INPUT_PULLUP);
  pinMode(DIP6, INPUT_PULLUP);
  pinMode(DIP7, INPUT_PULLUP);
  pinMode(DIP8, INPUT_PULLUP);

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_NARANJA, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_ONBOARD, OUTPUT);

  pinMode(BOMBA1_PIN, OUTPUT);
  pinMode(BOMBA2_PIN, OUTPUT);

  analogWrite(BOMBA1_PIN, 0);
  analogWrite(BOMBA2_PIN, 0);

  analogReadResolution(12);
  apagarTodo();
}

void loop() {
  bool d1 = (digitalRead(DIP1) == LOW);
  bool d2 = (digitalRead(DIP2) == LOW);
  bool d3 = (digitalRead(DIP3) == LOW);
  bool d4 = (digitalRead(DIP4) == LOW);
  bool d5 = (digitalRead(DIP5) == LOW);
  bool d6 = (digitalRead(DIP6) == LOW);
  bool d7 = (digitalRead(DIP7) == LOW);
  bool d8 = (digitalRead(DIP8) == LOW);

  // ===== PARADA TOTAL (DIP8) =====
  if (d8) {
    apagarTodo();
    bomba1Seleccionada = false;
    bomba2Seleccionada = false;
    potenciaBomba1 = 0;
    potenciaBomba2 = 0;
    modoValvs = false;
    modoHume = false;
    modoAuto = false;
    autoRunning = false;
    sensorHumedadActivo = 0;
    monitoreoH1 = false;
    monitoreoH2 = false;
    delay(20);
    return;
  }

  modoBluetooth = d7;

  // LED Integrado
  if (modoBluetooth) {
    if (millis() - ultimoParpadeoOnboard >= 600) {
      estadoOnboard = !estadoOnboard;
      ultimoParpadeoOnboard = millis();
    }
    digitalWrite(LED_ONBOARD, estadoOnboard);
  } else {
    digitalWrite(LED_ONBOARD, HIGH);  // fijo en modo manual
  }

  if (modoBluetooth) {
    // ==================== MODO BLUETOOTH ====================
    procesarBluetooth();

    // Monitoreo continuo
    if (monitoreoH1 || monitoreoH2) {
      if (millis() - ultimoMonitoreo >= 500) {
        if (monitoreoH1) {
          int n = leerHumedad(SENSOR1_PIN);
          SerialBT.print("H1: ");
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        if (monitoreoH2) {
          int n = leerHumedad(SENSOR2_PIN);
          SerialBT.print("H2: ");
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        ultimoMonitoreo = millis();
      }
    }

    // Visualización (prioridad: humedad > ola)
    if (sensorHumedadActivo == 1) {
      mostrarBarraHumedad(leerHumedad(SENSOR1_PIN));
    }
    else if (sensorHumedadActivo == 2) {
      mostrarBarraHumedad(leerHumedad(SENSOR2_PIN));
    }
    else if (modoAuto) {
      ejecutarOla();
    }

    // ===== LÓGICA AUTOMÁTICA =====
    if (modoAuto && autoRunning) {
      int h1 = leerHumedad(SENSOR1_PIN);
      int h2 = leerHumedad(SENSOR2_PIN);

      // Bomba 1
      if (h1 >= 0 && h1 <= umbralOn1) {
        setPotencia(BOMBA1_PIN, potenciaAuto1);
      } else if (h1 > umbralOn1 || h1 < 0) {
        analogWrite(BOMBA1_PIN, 0);
      }

      // Bomba 2
      if (h2 >= 0 && h2 <= umbralOn2) {
        setPotencia(BOMBA2_PIN, potenciaAuto2);
      } else if (h2 > umbralOn2 || h2 < 0) {
        analogWrite(BOMBA2_PIN, 0);
      }
    }
    else if (modoValvs) {
      digitalWrite(LED_VERDE, bomba1Seleccionada);
      digitalWrite(LED_AZUL,  bomba2Seleccionada);

      digitalWrite(LED_AMARILLO, (potenciaBomba1 == 1 || potenciaBomba2 == 1));
      digitalWrite(LED_NARANJA,  (potenciaBomba1 == 2 || potenciaBomba2 == 2));
      digitalWrite(LED_ROJO,     (potenciaBomba1 == 3 || potenciaBomba2 == 3));

      setPotencia(BOMBA1_PIN, potenciaBomba1);
      setPotencia(BOMBA2_PIN, potenciaBomba2);
    }
    else if (!modoAuto) {
      analogWrite(BOMBA1_PIN, 0);
      analogWrite(BOMBA2_PIN, 0);
    }
  }
  else {
    // ==================== MODO MANUAL ====================
    modoValvs = false;
    modoHume = false;
    modoAuto = false;
    autoRunning = false;
    sensorHumedadActivo = 0;
    monitoreoH1 = false;
    monitoreoH2 = false;
    bomba1Seleccionada = false;
    bomba2Seleccionada = false;
    potenciaBomba1 = 0;
    potenciaBomba2 = 0;

    int potenciasActivas = 0;
    if (d1) potenciasActivas++;
    if (d2) potenciasActivas++;
    if (d3) potenciasActivas++;

    if (d4 && d5) {
      // Ambos sensores → parpadean todos
      if (millis() - ultimoParpadeo >= 250) {
        estadoParpadeo = !estadoParpadeo;
        ultimoParpadeo = millis();
      }
      digitalWrite(LED_ROJO, estadoParpadeo);
      digitalWrite(LED_NARANJA, estadoParpadeo);
      digitalWrite(LED_AMARILLO, estadoParpadeo);
      digitalWrite(LED_VERDE, estadoParpadeo);
      digitalWrite(LED_AZUL, estadoParpadeo);
      analogWrite(BOMBA1_PIN, 0);
      analogWrite(BOMBA2_PIN, 0);
    }
    else if (d4) {
      mostrarBarraHumedad(leerHumedad(SENSOR1_PIN));
      analogWrite(BOMBA1_PIN, 0);
      analogWrite(BOMBA2_PIN, 0);
    }
    else if (d5) {
      mostrarBarraHumedad(leerHumedad(SENSOR2_PIN));
      analogWrite(BOMBA1_PIN, 0);
      analogWrite(BOMBA2_PIN, 0);
    }
    else {
      if (potenciasActivas > 1) {
        // Conflicto de potencias
        if (millis() - ultimoParpadeo >= 250) {
          estadoParpadeo = !estadoParpadeo;
          ultimoParpadeo = millis();
        }
        digitalWrite(LED_ROJO, estadoParpadeo);
        digitalWrite(LED_NARANJA, estadoParpadeo);
        digitalWrite(LED_AMARILLO, estadoParpadeo);
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_AZUL, LOW);
        analogWrite(BOMBA1_PIN, 0);
        analogWrite(BOMBA2_PIN, 0);
      }
      else {
        // Selector de bomba
        digitalWrite(LED_VERDE, !d6);  // Bomba 1
        digitalWrite(LED_AZUL, d6);    // Bomba 2

        int pot = 0;
        if (d1) {          // Mínima
          pot = 1;
          digitalWrite(LED_AMARILLO, HIGH);
          digitalWrite(LED_NARANJA, LOW);
          digitalWrite(LED_ROJO, LOW);
        } 
        else if (d2) {     // Media
          pot = 2;
          digitalWrite(LED_AMARILLO, LOW);
          digitalWrite(LED_NARANJA, HIGH);
          digitalWrite(LED_ROJO, LOW);
        } 
        else if (d3) {     // Máxima
          pot = 3;
          digitalWrite(LED_AMARILLO, LOW);
          digitalWrite(LED_NARANJA, LOW);
          digitalWrite(LED_ROJO, HIGH);
        } 
        else {
          digitalWrite(LED_AMARILLO, LOW);
          digitalWrite(LED_NARANJA, LOW);
          digitalWrite(LED_ROJO, LOW);
        }

        if (!d6) {
          setPotencia(BOMBA1_PIN, pot);
          analogWrite(BOMBA2_PIN, 0);
        } else {
          setPotencia(BOMBA2_PIN, pot);
          analogWrite(BOMBA1_PIN, 0);
        }
      }
    }
  }

  delay(10);
}

void procesarBluetooth() {
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n' || c == '\r') {
      bufferBT.trim();
      bufferBT.toLowerCase();

      // ===== MODO AUTO =====
      if (bufferBT == "auto") {
        modoAuto = !modoAuto;
        autoRunning = false;
        sensorHumedadActivo = 0;

        if (modoAuto) {
          SerialBT.println("AUTO ON - Configura primero");
          enviarAyudaAuto();
        } else {
          SerialBT.println("AUTO OFF");
          analogWrite(BOMBA1_PIN, 0);
          analogWrite(BOMBA2_PIN, 0);
        }
      }
      else if (modoAuto) {
        // Potencia
        if (bufferBT == "val1.1") { potenciaAuto1 = 1; autoRunning = false; SerialBT.println("Bomba1 = Minima"); }
        else if (bufferBT == "val1.2") { potenciaAuto1 = 2; autoRunning = false; SerialBT.println("Bomba1 = Media"); }
        else if (bufferBT == "val1.3") { potenciaAuto1 = 3; autoRunning = false; SerialBT.println("Bomba1 = Maxima"); }
        else if (bufferBT == "val2.1") { potenciaAuto2 = 1; autoRunning = false; SerialBT.println("Bomba2 = Minima"); }
        else if (bufferBT == "val2.2") { potenciaAuto2 = 2; autoRunning = false; SerialBT.println("Bomba2 = Media"); }
        else if (bufferBT == "val2.3") { potenciaAuto2 = 3; autoRunning = false; SerialBT.println("Bomba2 = Maxima"); }
        else if (bufferBT == "valv.pre") {
          potenciaAuto1 = 2;
          potenciaAuto2 = 2;
          umbralOn1 = 4; umbralOff1 = 8;
          umbralOn2 = 4; umbralOff2 = 8;
          autoRunning = false;
          SerialBT.println("Predeterminado: 0-4 -> 8-10 | Media");
        }
        // Umbrales de humedad (1 a 9)
        else if (bufferBT.startsWith("s1mi.")) {
          int valor = bufferBT.substring(5).toInt();
          if (valor >= 1 && valor <= 9) {
            umbralOn1 = valor;
            umbralOff1 = valor;
            autoRunning = false;
            SerialBT.print("Sensor1 umbral = ");
            SerialBT.println(valor);
          }
        }
        else if (bufferBT.startsWith("s2mi.")) {
          int valor = bufferBT.substring(5).toInt();
          if (valor >= 1 && valor <= 9) {
            umbralOn2 = valor;
            umbralOff2 = valor;
            autoRunning = false;
            SerialBT.print("Sensor2 umbral = ");
            SerialBT.println(valor);
          }
        }
        else if (bufferBT == "endcon") {
          autoRunning = !autoRunning;
          if (autoRunning) {
            SerialBT.println("SISTEMA AUTOMATICO INICIADO");
          } else {
            SerialBT.println("SISTEMA AUTOMATICO DETENIDO");
            analogWrite(BOMBA1_PIN, 0);
            analogWrite(BOMBA2_PIN, 0);
          }
        }
        // Ver humedad (se puede usar dentro de Auto)
        else if (bufferBT == "h1") sensorHumedadActivo = 1;
        else if (bufferBT == "h2") sensorHumedadActivo = 2;
        else if (bufferBT == "h1l") {
          int n = leerHumedad(SENSOR1_PIN);
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        else if (bufferBT == "h2l") {
          int n = leerHumedad(SENSOR2_PIN);
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        else if (bufferBT == "h1lc") {
          monitoreoH1 = !monitoreoH1;
          SerialBT.println(monitoreoH1 ? "MONITOREO H1 ON" : "MONITOREO H1 OFF");
        }
        else if (bufferBT == "h2lc") {
          monitoreoH2 = !monitoreoH2;
          SerialBT.println(monitoreoH2 ? "MONITOREO H2 ON" : "MONITOREO H2 OFF");
        }
      }

      // ===== MODO VALVS =====
      else if (bufferBT == "valvs") {
        modoValvs = !modoValvs;
        modoHume = false;
        modoAuto = false;
        autoRunning = false;
        sensorHumedadActivo = 0;
        bomba1Seleccionada = false;
        bomba2Seleccionada = false;
        potenciaBomba1 = 0;
        potenciaBomba2 = 0;
        apagarTodo();

        if (modoValvs) {
          SerialBT.println("VALVS ON");
          enviarAyudaValvs();
        } else {
          SerialBT.println("VALVS OFF");
        }
      }
      else if (modoValvs) {
        if (bufferBT == "val1") {
          if (bomba1Seleccionada && !bomba2Seleccionada) {
            bomba1Seleccionada = false;
            potenciaBomba1 = 0;
          } else {
            bomba1Seleccionada = true;
            bomba2Seleccionada = false;
            potenciaBomba2 = 0;
          }
        }
        else if (bufferBT == "val2") {
          if (bomba2Seleccionada && !bomba1Seleccionada) {
            bomba2Seleccionada = false;
            potenciaBomba2 = 0;
          } else {
            bomba2Seleccionada = true;
            bomba1Seleccionada = false;
            potenciaBomba1 = 0;
          }
        }
        else if (bufferBT == "valv") {
          if (bomba1Seleccionada && bomba2Seleccionada) {
            bomba1Seleccionada = false;
            bomba2Seleccionada = false;
            potenciaBomba1 = 0;
            potenciaBomba2 = 0;
          } else {
            bomba1Seleccionada = true;
            bomba2Seleccionada = true;
          }
        }
        else if (bomba1Seleccionada && !bomba2Seleccionada) {
          if (bufferBT == "val1.1") potenciaBomba1 = 1;
          else if (bufferBT == "val1.2") potenciaBomba1 = 2;
          else if (bufferBT == "val1.3") potenciaBomba1 = 3;
        }
        else if (bomba2Seleccionada && !bomba1Seleccionada) {
          if (bufferBT == "val2.1") potenciaBomba2 = 1;
          else if (bufferBT == "val2.2") potenciaBomba2 = 2;
          else if (bufferBT == "val2.3") potenciaBomba2 = 3;
        }
        else if (bomba1Seleccionada && bomba2Seleccionada) {
          if (bufferBT == "valv.1") { potenciaBomba1 = 1; potenciaBomba2 = 1; }
          else if (bufferBT == "valv.2") { potenciaBomba1 = 2; potenciaBomba2 = 2; }
          else if (bufferBT == "valv.3") { potenciaBomba1 = 3; potenciaBomba2 = 3; }
        }
      }

      // ===== MODO HUME =====
      else if (bufferBT == "hume") {
        modoHume = !modoHume;
        modoValvs = false;
        modoAuto = false;
        autoRunning = false;
        sensorHumedadActivo = 0;
        bomba1Seleccionada = false;
        bomba2Seleccionada = false;
        potenciaBomba1 = 0;
        potenciaBomba2 = 0;
        monitoreoH1 = false;
        monitoreoH2 = false;
        apagarTodo();

        if (modoHume) {
          SerialBT.println("HUME ON");
          enviarAyudaHume();
        } else {
          SerialBT.println("HUME OFF");
        }
      }
      else if (modoHume) {
        if (bufferBT == "h1") sensorHumedadActivo = 1;
        else if (bufferBT == "h2") sensorHumedadActivo = 2;
        else if (bufferBT == "h1l") {
          int n = leerHumedad(SENSOR1_PIN);
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        else if (bufferBT == "h2l") {
          int n = leerHumedad(SENSOR2_PIN);
          SerialBT.println(n == -1 ? "-" : String(n));
        }
        else if (bufferBT == "h1lc") {
          monitoreoH1 = !monitoreoH1;
          SerialBT.println(monitoreoH1 ? "MONITOREO H1 ON" : "MONITOREO H1 OFF");
        }
        else if (bufferBT == "h2lc") {
          monitoreoH2 = !monitoreoH2;
          SerialBT.println(monitoreoH2 ? "MONITOREO H2 ON" : "MONITOREO H2 OFF");
        }
      }

      bufferBT = "";
    } else {
      if (bufferBT.length() < 20) bufferBT += c;
    }
  }
}