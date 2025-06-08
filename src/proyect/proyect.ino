/**
 * @mainpage THERMAL COMFORT & ACCESS CONTROL SYSTEM
 * 
 * @section intro_sec 📌 Overview
 * This project implements an environmental monitoring and security system using Arduino.
 * The system integrates multiple sensors and output devices to supervise ambient conditions 
 * and manage secure access through a state machine and asynchronous tasks.
 * 
 * @section features_sec ⚙️ Main Features
 * - LCD display for real-time sensor feedback and alerts.
 * - Matrix keypad for user password input.
 * - RFID reader for card-based identification and access control.
 * - Sensors for temperature, humidity, light (photoresistor), Hall effect (magnetic field), and infrared presence.
 * - Visual feedback through RGB LEDs.
 * - Audible feedback using a buzzer with custom melodies.
 * - Modular finite state machine to handle operational modes.
 * - Non-blocking behavior using asynchronous task scheduling.
 * 
 * @section fsm_sec 🔁 Finite State Machine (FSM)
 * The system operates through a set of well-defined states:
 * - Inicio: Password validation and access initiation.
 * - MonitorAmbiental: Environmental sensing and display.
 * - MonitorEventos: Event-based sensing (Hall, infrared).
 * - Alarma & AlarmaRed: Audible/visual alerts triggered by threshold exceedance.
 * - Bloqueo: System lockout after failed authentication.
 * 
 * Transitions are driven by user input, sensor thresholds, or elapsed time, managed via `StateMachineLib`.
 * 
 * @section hardware_sec 🔌 Hardware Components
 * - Arduino Mega or Uno
 * - LCD 16x2 with 6-pin interface
 * - 4x4 Keypad
 * - DHT11 sensor
 * - Photoresistor (LDR)
 * - Analog Hall effect sensor
 * - Infrared sensor
 * - RC522 RFID module
 * - RGB LEDs
 * - Passive Buzzer
 * 
 * @section structure_sec 🧩 Code Organization
 * The source code is modularized into functional regions using `#pragma region`, including:
 * - Setup and loop control
 * - Sensor reading and evaluation
 * - FSM transitions and actions
 * - Input management (keypad, RFID)
 * - Task scheduling (via `AsyncTaskLib`)
 * - Output functions: LCD display, LEDs, buzzer melodies
 * 
 * Each region and function is documented with Doxygen-compatible comments for clarity.
 * 
 * @section credits_sec 📣 Author
 * Developed by Julian,Katherin and Santiago for academic, experimental, or embedded deployment use.  
 * All logic is written in C++ and runs on standard Arduino-compatible boards.
 *
 * @file proyect.ino
 * @brief Main Arduino sketch for thermal comfort and access control.
 */

#include "StateMachineLib.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include "AsyncTaskLib.h"
#include "DHT.h"
#include "RFID.h"
#include <SPI.h>
#include <MFRC522.h>

#pragma region LCD configuration
/**
 * @brief Connection pins between the LCD and the Arduino.
 */
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
/**
 * @brief Object to control the LCD screen.
 */
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#pragma endregion

#pragma  region RFID definitions
/**
 * @brief Pin de reset para el módulo RC522.
 */
#define RST_PIN  32
/**
 * @brief Pin SS (SDA) para la comunicación con el módulo RC522.
 */
#define SS_PIN   53
/**
 * @brief Objeto para controlar el módulo RFID RC522.
 */
MFRC522 mfrc522(SS_PIN, RST_PIN);
#pragma endregion

/**
 * @brief Estructura que representa una tarjeta RFID registrada en el sistema.
 */
struct TarjetaRegistrada {
  byte uid[4];  /**< UID único de la tarjeta, asumimos que es de 4 bytes (MIFARE). */
  short valor;  /**< Valor asignado a la tarjeta, puede representar permisos o saldo. */
};

TarjetaRegistrada tarjetasConocidas[] = {
  {{0x5C, 0xF8, 0xD7, 0x73}, -2},
  {{0xF3, 0xAF, 0x2B, 0x27}, 2}
};
const int numTarjetas = sizeof(tarjetasConocidas) / sizeof(TarjetaRegistrada);
/**
 * @brief Inicializa el módulo RFID MFRC522.
 * 
 * Esta función configura la comunicación SPI y activa el lector RFID. 
 * Se debe llamar una vez al inicio del sistema para que el módulo funcione correctamente.
 */
void setupRFID() {
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Sistema RFID iniciado");
}
/**
 * @brief Lee una tarjeta RFID y retorna su valor asignado.
 * 
 * La función verifica si una tarjeta está presente y compara su UID con tarjetas registradas.
 * Si la tarjeta es reconocida, devuelve su valor asignado.
 * 
 * @return Valor de la tarjeta si es reconocida. Si no se detecta o no coincide, retorna -5.
 */
short readTarget() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("INSERTE TARJETA");
    return -5;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return -5;
  }

  for (int i = 0; i < numTarjetas; i++) {
    bool coincide = true;

    for (byte j = 0; j < 4; j++) {
      if (mfrc522.uid.uidByte[j] != tarjetasConocidas[i].uid[j]) {
        coincide = false;
        break;
      }
    }

    if (coincide) {
      Serial.print("Tarjeta reconocida. Valor: ");
      Serial.println(tarjetasConocidas[i].valor);
      mfrc522.PICC_HaltA();
      return tarjetasConocidas[i].valor;
    }
  }

  mfrc522.PICC_HaltA();
  return -5;
}
/**
 * @brief Registra una nueva tarjeta RFID en el sistema.
 * 
 * La función espera a que se acerque una tarjeta, lee su UID y muestra el bloque 
 * necesario para añadirla manualmente al array `tarjetasConocidas`.
 */
void registerTarget() {
  Serial.println("Esperando nueva tarjeta para registrar...");
  lcd.clear();
  lcd.print("Aproxime tarjeta");

  while (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Error leyendo UID.");
    lcd.clear();
    lcd.print("Error al leer");
    return;
  }

  byte nuevoUID[4];
  for (int i = 0; i < 4; i++) {
    nuevoUID[i] = mfrc522.uid.uidByte[i];
  }

  Serial.print("UID nuevo: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(nuevoUID[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  short valorAsignado = 0; // Puedes cambiar esto según lógica del sistema

  Serial.println("ATENCIÓN: Este registro solo se imprimió en consola.");
  Serial.println("Agrega el siguiente bloque manualmente al array tarjetasConocidas[]:");
  Serial.print("  {{");
  for (int i = 0; i < 4; i++) {
    Serial.print("0x");
    Serial.print(nuevoUID[i], HEX);
    if (i < 3) Serial.print(", ");
  }
  Serial.print("}, ");
  Serial.print(valorAsignado);
  Serial.println("},");

  lcd.clear();
  lcd.print("UID registrado");
  delay(1500);

  mfrc522.PICC_HaltA();
}
/**
 * @brief Lee el valor de una variable específica del sistema.
 * 
 * Actualmente, esta función devuelve 0. Puede expandirse para leer configuraciones almacenadas.
 * 
 * @return Valor leído (actualmente siempre es 0).
 */
byte readPMV() {
  return 0;
}
#pragma region Configuration for the Keypad
/**
 * @brief Number of rows in the matrix keyboard.
 */
const byte ROWS = 4;
/**
 * @brief Number of columns in the matrix keyboard.
 */
const byte COLS = 4;
/**
 * @brief Mapeo de teclas del teclado matricial.
 * 
 * Esta matriz define la disposición de los caracteres en el teclado físico.
 */
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
/**
 * @brief Pines conectados a las filas del teclado.
 */    
byte rowPins[ROWS] = {22, 24, 26, 28};
/**
 * @brief Pines conectados a las columnas del teclado.
 */
byte colPins[COLS] = {30, 32, 34, 36};
/**
 * @brief Objeto para manejar la lectura del teclado matricial.
 */
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#pragma endregion
#pragma region Configuration for the leds
/**
 * @brief Pines de los LEDs indicadores.
 * 
 * Estos pines controlan los LEDs rojo, verde y azul, que pueden usarse para 
 * mostrar estados del sistema.
 */
#define LED_RED 10 /**< Pin del LED rojo */
#define LED_GREEN 9 /**< Pin del LED verde */
#define LED_BLUE 8 /**< Pin del LED azul */
#pragma endregion
#pragma region Button configuration
/**
 * @brief Pin de entrada para el botón.
 * 
 * Este botón puede usarse para interacciones del usuario, como aceptar o cancelar.
 */
#define BUTTON_PIN 6
#pragma endregion
#pragma region Configuration for the Buzzer
/**
 * @brief Pin de salida del buzzer.
 * 
 * Este buzzer puede emitir sonidos de alerta o confirmación.
 */
int buzzer = 7;
#pragma endregion
#pragma region Configuration for the sensors
/**
 * @brief Pin analógico del sensor de luz.
 */
const int pinLight = A0;
/**
 * @brief Variable para almacenar la lectura del sensor de luz.
 */
int valueLight = 0;
/**
 * @brief Pin analógico del sensor de temperatura.
 */
const int pinTemp=A4;
/**
 * @brief Constante beta utilizada en el cálculo de temperatura del sensor NTC.
 */
#define beta 4090
/**
 * @brief Pin digital del sensor infrarrojo.
 */
#define INFRA 15
/**
 * @brief Pin analógico del sensor de efecto Hall.
 */
const int pinHall = A1;
/**
 * @brief Variable para almacenar la lectura del sensor de efecto Hall.
 */
int valueHall = 0;
/**
 * @brief Tipo de sensor de temperatura y humedad utilizado.
 */
#define DHTTYPE DHT11  /**< Se usa un DHT11, aunque se puede cambiar por otro modelo */
/**
 * @brief Pin del sensor DHT de temperatura y humedad.
 */
#define DHTPIN 38
/**
 * @brief Objeto del sensor DHT para obtener mediciones de temperatura y humedad.
 */
DHT dht(DHTPIN, DHTTYPE);
/**
 * @brief Variable que almacena la humedad medida por el sensor.
 */
int hum = 0;
/**
 * @brief Estado del sensor, utilizado para validaciones.
 */
int sensorState = 0;
/**
 * @brief Variable para almacenar la temperatura medida.
 */
float temp = 0;
// Limits
/**
 * @brief Umbrales de temperatura para generar alertas.
 */
int tempHigh = 29;  /**< Límite superior de temperatura */
int tempLow = 15;   /**< Límite inferior de temperatura */
/**
 * @brief Umbrales de luz para generar alertas.
 */
int luzHigh = 800;  /**< Límite superior de luz */
int luzLow = 200;   /**< Límite inferior de luz */
/**
 * @brief Umbrales de humedad para generar alertas.
 */
int humHigh = 70;   /**< Límite superior de humedad */
int humLow = 30;    /**< Límite inferior de humedad */
/**
 * @brief Umbral del sensor de efecto Hall para activación.
 */
int hallHigh = 300;
/**
 * @brief Contador de activaciones de alarma.
 */
int alarmCount = 0;
#pragma endregion
#pragma Buzzer notes
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_DB3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_EB3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0
#pragma endregion
#pragma region Melodies for the Buzzer
/**
 * @brief Conjunto de melodías utilizadas en el sistema.
 * 
 * Este módulo define arreglos de notas musicales que corresponden a diferentes eventos,
 * como desbloqueo exitoso, errores o alarmas.
 */
int BloqueoMelody[] = {
    NOTE_E7, NOTE_E7, 0, NOTE_E7,
    0, NOTE_C7, NOTE_E7, 0,
    NOTE_G7, 0, 0, 0};
/**
 * @brief Duraciones de cada nota en la melodía de bloqueo.
 */
int correctDurations[] = {
    200, 200, 200, 200,
    200, 200, 200, 200,
    200, 200, 200, 200};
    /**
 * @brief Longitud de la melodía de bloqueo.
 */
int correctMelodyLength = sizeof(BloqueoMelody) / sizeof(BloqueoMelody[0]);
/**
 * @brief Melodía utilizada para alertas o errores.
 */
int AlarmMelody[] = {
    NOTE_F3,
    NOTE_A3,
};
/**
 * @brief Duraciones de cada nota en la melodía de error.
 */
int incorrectDurations[] = {
    300, 300, 300, 300};
/**
 * @brief Longitud de la melodía de error.
 */
int incorrectMelodyLength = sizeof(AlarmMelody) / sizeof(AlarmMelody[0]);
/**
 * @brief Melodía utilizada para confirmaciones correctas.
 */
int rightMelody[] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4,
    NOTE_C5, NOTE_D5, NOTE_E5
};
/**
 * @brief Duraciones de cada nota en la melodía de éxito.
 */
int rightDurations[] = {
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};
/**
 * @brief Longitud de la melodía de éxito.
 */
int rightMelodyLength = sizeof(rightMelody) / sizeof(rightMelody[0]);
/**
 * @brief Melodía utilizada para indicar una acción incorrecta.
 */
int wrongMelody[] = {
    NOTE_C4, NOTE_B3, NOTE_A3, NOTE_G3, NOTE_F3, NOTE_E3, NOTE_D3,
    NOTE_C3, NOTE_B2, NOTE_A2
};
/**
 * @brief Duraciones de cada nota en la melodía de error.
 */
int wrongDurations[] = {
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};
/**
 * @brief Longitud de la melodía de error.
 */
int wrongMelodyLength = sizeof(wrongMelody) / sizeof(wrongMelody[0]);
#pragma endregion
#pragma region Configuration for the Security
/**
 * @brief Contraseña de acceso al sistema.
 * 
 * La contraseña está definida como un arreglo de caracteres y debe ser introducida 
 * en el teclado para verificar el acceso.
 */
const char password[6] = {'1', '1', '2', '2', '3', '#'};
/**
 * @brief Buffer temporal para almacenar la entrada de usuario al validar la contraseña.
 */
char buffer[6];
/**
 * @brief Contador utilizado para rastrear la posición en el buffer de la contraseña.
 */
int counter = -1;
/**
 * @brief Contador de intentos de acceso fallidos.
 */
char tryCounter = 0;
#pragma endregion
#pragma region Configuration for the State Machine
/**
 * @brief Estados definidos en la máquina de estados.
 * 
 * Cada estado representa una condición funcional del sistema.
 */
enum State {
  Inicio = 0,            /**< Estado inicial del sistema. */
  AlarmaRed = 1,         /**< Activación de alarma por condiciones ambientales. */
  MonitorAmbiental = 2,  /**< Monitoreo de temperatura, luz y humedad. */
  Bloqueo = 3,           /**< Estado de bloqueo por seguridad. */
  Alarma = 4,            /**< Alarma activada por sensores específicos. */
  MonitorEventos = 5     /**< Monitoreo de eventos adicionales como Hall o infrarrojo. */
};
/**
 * @brief Entradas posibles que controlan las transiciones entre estados.
 * 
 * Estas entradas representan eventos o condiciones leídas por el sistema.
 */
enum Input {
  time = 0,               /**< Tiempo transcurrido para cambiar de estado. */
  claveCorrecta = 1,      /**< Contraseña correctamente ingresada. */
  systemBlock = 2,        /**< Evento que indica solicitud de bloqueo del sistema. */
  btnPress = 3,           /**< Presión de botón físico. */
  hallExceeded = 4,       /**< Activación del sensor Hall por campo magnético. */
  tempLightExceeded = 5,  /**< Exceso de temperatura o luz. */
  infraZero = 6,          /**< Ausencia detectada por el sensor infrarrojo. */
  sensores = 7,           /**< Evento general de sensores. */
  unknown = 8             /**< Entrada desconocida o no inicializada. */
};
/**
 * @brief Objeto principal de la máquina de estados.
 * 
 * Se configura con 6 estados y hasta 12 entradas distintas.
 */
StateMachine stateMachine(6, 12);
/**
 * @brief Última entrada capturada por el sistema.
 * 
 * Se utiliza para evaluar condiciones de transición.
 */
Input input = Input::unknown;
/**
 * @brief Configura la máquina de estados del sistema.
 * 
 * Define las transiciones posibles entre estados en función de entradas,
 * y establece funciones para ejecutar al entrar o salir de cada estado.
 */
void setupStateMachine()
{
  // Transiciones desde el estado Inicio
  stateMachine.AddTransition(Inicio, MonitorAmbiental, []() { return input == claveCorrecta; });
  stateMachine.AddTransition(Inicio, Bloqueo, []() { return input == systemBlock; });
  // Transiciones desde MonitorAmbiental
  stateMachine.AddTransition(MonitorAmbiental, MonitorEventos, []() { return input == time; });
  stateMachine.AddTransition(MonitorAmbiental, AlarmaRed, []() { return input == tempLightExceeded; });
  // Transiciones desde MonitorEventos
  stateMachine.AddTransition(MonitorEventos, MonitorAmbiental, []() { return input == time; });
  stateMachine.AddTransition(MonitorEventos, Alarma, []() { return input == hallExceeded; });
  stateMachine.AddTransition(MonitorEventos, Alarma, []() { return input == infraZero; });
  stateMachine.AddTransition(MonitorEventos, AlarmaRed, []() { return input == tempLightExceeded; });
  // Transición desde Alarma
  stateMachine.AddTransition(Alarma, MonitorAmbiental, []() { return input == time; });
  // Transición desde Bloqueo
  stateMachine.AddTransition(Bloqueo, Inicio, []() { return input == time; });
  // Transición desde AlarmaRed
  stateMachine.AddTransition(AlarmaRed, Inicio, []() { return input == btnPress; });
  // Asignación de funciones al entrar a cada estado
  stateMachine.SetOnEntering(Inicio, outputInicio);
  stateMachine.SetOnEntering(MonitorAmbiental, outputMAmbiental);
  stateMachine.SetOnEntering(Bloqueo, outputBloqueo);
  stateMachine.SetOnEntering(Alarma, outputAlarma);
  stateMachine.SetOnEntering(AlarmaRed, outputAlarmaRed);
  stateMachine.SetOnEntering(MonitorEventos, outputMEventos);
  // Asignación de funciones al salir de cada estado
  stateMachine.SetOnLeaving(Inicio, leavingInicio);
  stateMachine.SetOnLeaving(MonitorAmbiental, leavingAmbiental);
  stateMachine.SetOnLeaving(Bloqueo, leavingBloqueo);
  stateMachine.SetOnLeaving(Alarma, leavingAlarma);
  stateMachine.SetOnLeaving(AlarmaRed, leavingAlarmaRed);
  stateMachine.SetOnLeaving(MonitorEventos, leavingEventos);
}
#pragma endregion
#pragma region Methods
/**
 * @brief Lee el valor del sensor de luz y actualiza la variable correspondiente.
 */
void readLight(void);
/**
 * @brief Lee el valor del sensor de temperatura (NTC o DHT) y lo almacena.
 */
void readTemp(void);
/**
 * @brief Lee el valor de humedad desde el sensor DHT.
 */
void readHum(void);
/**
 * @brief Actualiza el reloj interno o el control de tiempo de la FSM.
 */
void readTime(void);
/**
 * @brief Lee el valor del sensor de efecto Hall.
 */
void readHall(void);
/**
 * @brief Lee el estado del sensor infrarrojo.
 */
void readInfra(void);
/**
 * @brief Muestra en pantalla los datos de los sensores ambientales.
 */
void printSensorsLcd(void);
/**
 * @brief Muestra en pantalla información relacionada con eventos del sistema.
 */
void printEventosLcd(void);
/**
 * @brief Verifica si los valores de temperatura y luz exceden los umbrales.
 */
void verifyTempLightLimits(void);
/**
 * @brief Verifica si el valor del sensor Hall supera el límite permitido.
 */
void verifyHallLimit(void);
/**
 * @brief Controla la activación de la luz azul de retroalimentación.
 */
void readBluelight(void);
/**
 * @brief Controla la activación de la luz roja de advertencia.
 */
void readRedlight(void);
/**
 * @brief Ejecuta la reproducción de una melodía desde la cola de acciones.
 */
void melodyExecutable(void);
/**
 * @brief Lee el estado del botón físico del sistema.
 */
void readButton(void);
/**
 * @brief Control de seguridad para validar el acceso por contraseña.
 */
void seguridad(void);
/**
 * @brief Reproduce una melodía de error o fallo.
 */
void failMelody(void);
/**
 * @brief Reproduce una melodía de confirmación de éxito.
 */
void successMelody(void);
/**
 * @brief Ejecuta la rutina visual y sonora del estado de bloqueo.
 */
void sisBloqueado(void);
/**
 * @brief Verifica si la señal infrarroja indica un evento anómalo.
 */
void verifyInfraLimit(void);
/**
 * @brief Reinicia el contador de alarmas acumuladas.
 */
void resetAlarmCount(void);
#pragma endregion
#pragma region Tasks
/**
 * @brief Tareas asincrónicas que gestionan el sistema en tiempo real.
 * 
 * Cada tarea se ejecuta con un intervalo definido y llama a funciones específicas.
 * Algunas se reinician automáticamente (`true`) y otras son disparadas una sola vez (`false`).
 */
AsyncTask taskReadLight(1000, true, readLight);               /**< Lee la luz ambiental cada 1s. */
AsyncTask taskReadTemp(1000, true, readTemp);                 /**< Lee la temperatura cada 1s. */
AsyncTask taskReadHum(1000, true, readHum);                   /**< Lee la humedad cada 1s. */
AsyncTask taskReadHall(1000, true, readHall);                 /**< Lee el sensor Hall cada 1s. */
AsyncTask taskReadInfra(1000, true, readInfra);               /**< Lee el sensor infrarrojo cada 1s. */
AsyncTask taskSetTime(10000, true, readTime);                 /**< Actualiza el reloj cada 10s. */
AsyncTask taskPrintLcd(1000, true, printSensorsLcd);          /**< Muestra datos de sensores. */
AsyncTask taskPrintEventosLcd(1000, true, printEventosLcd);   /**< Muestra eventos en LCD. */
AsyncTask taskTempLightLimits(1000, true, verifyTempLightLimits); /**< Verifica límites ambientales. */
AsyncTask taskHallLimits(1000, true, verifyHallLimit);        /**< Verifica límite del sensor Hall. */
AsyncTask taskInfraAct(1000, true, verifyInfraLimit);         /**< Verifica actividad del IR. */
AsyncTask taskBlueLight(400, true, readBluelight);            /**< Controla luz azul. */
AsyncTask taskRedLight(400, true, readRedlight);              /**< Controla luz roja. */
AsyncTask taskMelody(800, false, melodyExecutable);           /**< Ejecuta melodía programada. */
AsyncTask taskReadButton(100, true, readButton);              /**< Lee botón cada 100ms. */
AsyncTask taskSecurity(1000, false, seguridad);               /**< Activa seguridad una vez. */
AsyncTask taskMelodyFail(800, false, failMelody);             /**< Reproduce melodía de error. */
AsyncTask taskMelodySuccess(800, false, successMelody);       /**< Reproduce melodía de éxito. */
AsyncTask taskBloqueo(500, false, sisBloqueado);              /**< Ejecuta rutina de bloqueo. */
#pragma endregion
#pragma region Miscellaneous Configuration
/**
 * @brief Pin del LED integrado del microcontrolador (usualmente LED_BUILTIN).
 */
const byte ledPin = LED_BUILTIN;
/**
 * @brief Estado actual del LED principal.
 */
bool ledState = LOW;
/**
 * @brief Representación en texto del estado del LED ("ON"/"OFF").
 */
char ledState_text[4];
/**
 * @brief Texto para indicar encendido.
 */
char string_on[] = "ON";
/**
 * @brief Texto para indicar apagado.
 */
char string_off[] = "OFF";
/**
 * @brief Pin analógico para lectura general.
 * 
 * Puede usarse para depurar señales analógicas u otros sensores.
 */
const byte analogPin = A5;
/**
 * @brief Valor leído desde el pin analógico.
 */
unsigned short analogValue = 0;
#pragma endregion
/**
 * @brief Inicializa el sistema y todos sus componentes.
 * 
 * Configura los pines de entrada/salida, inicia la comunicación serial, 
 * configura el LCD, inicializa el sensor DHT, el lector RFID, 
 * y establece el estado inicial de la máquina de estados.
 */
void setup()
{
  Serial.begin(9600); // Simulator 115200
  // Inicialización de la pantalla LCD
  lcd.begin(16, 2);
  // Inicialización del sensor DHT
  dht.begin();
  // Inicialización del lector RFID
  setupRFID(); // <- corregido el ":" por ";"
  // Configuración de pines
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(INFRA, INPUT);
  pinMode(BUTTON_PIN, INPUT);
  // Apagar todos los LEDs inicialmente
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  Serial.println("Starting State Machine...");
  setupStateMachine();
  Serial.println("State Machine Started");
  // Establecer el estado inicial
  stateMachine.SetState(Inicio, false, true);
}
/**
 * @brief Bucle principal del sistema, ejecuta la lógica repetitiva.
 * 
 * - Lee tarjetas RFID y determina el acceso autorizado o denegado.
 * - Actualiza tareas asincrónicas relacionadas con sensores, seguridad, y alarmas.
 * - Actualiza la máquina de estados y restablece la entrada de control.
 */
void loop()
{
  
  //Read RFID card
  short rfidValor = readTarget();
  if (rfidValor != -5) {
    Serial.print("RFID card detected. Value: ");
    Serial.println(rfidValor);
    lcd.clear();
    lcd.print("RFID value: ");
    lcd.setCursor(0, 1);
    lcd.print(rfidValor);
    delay(1000);

    if (rfidValor == 2) {
      lcd.print("Acceso autorizado");
      lcd.setCursor(0, 1);
      lcd.print("Bienvenido!");
      input = Input::claveCorrecta;
      digitalWrite(LED_GREEN, HIGH);
      delay(1000);
      digitalWrite(LED_GREEN, LOW);
    } else if (rfidValor == -2) {
      lcd.print("Acceso denegado");
      lcd.setCursor(0, 1);
      lcd.print("Tarjeta no valida");
      input = Input::systemBlock;
      digitalWrite(LED_RED, HIGH);
      delay(1000);
      digitalWrite(LED_RED, LOW);
    }
  }
  //General tasks
  taskSetTime.Update();
  taskReadButton.Update();
  // Updating tasks from Ambiental State
  taskReadLight.Update();
  taskTempLightLimits.Update();
  taskReadTemp.Update();
  taskReadHum.Update();
  taskPrintLcd.Update();
  // Updating tasks from Eventos State
  taskReadHall.Update();
  taskReadInfra.Update();
  taskPrintEventosLcd.Update();
  taskHallLimits.Update();
  taskInfraAct.Update();
  // Updating security stuff
  taskSecurity.Update();
  taskMelodyFail.Update();
  taskMelodySuccess.Update();
  taskBloqueo.Update();
  // updating task for Alarma State
  taskBlueLight.Update();
  taskRedLight.Update();
  taskMelody.Update();
  // Update State Machine
  stateMachine.Update();
  // Resetear la entrada
  input = Input::unknown;
}

#pragma region Entering functions
/**
 * @brief Acción al entrar en el estado Inicio.
 * 
 * Reinicia contadores de intentos de acceso, lanza la tarea de seguridad
 * y muestra el indicador de estado en la consola serial.
 */
void outputInicio()
{
 Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println(" x                                                       ");
  Serial.println();
  tryCounter = 0;
  counter = -1;
  taskSecurity.Start();
}
/**
 * @brief Acción al entrar en el estado MonitorAmbiental.
 * 
 * Inicia tareas relacionadas con sensores ambientales y visualización
 * en el LCD. Ajusta la frecuencia de lectura de tiempo.
 */
void outputMAmbiental()
{
  Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println("             x                                          ");
  Serial.println();
  taskReadLight.Start();
  taskReadTemp.Start();
  taskReadHum.Start();
  taskPrintLcd.Start();
  taskTempLightLimits.Start();
  taskSetTime.SetIntervalMillis(7000);
  taskSetTime.Start();
    temp = 0;
}
/**
 * @brief Acción al entrar en el estado Bloqueo.
 * 
 * Activa las tareas asociadas al bloqueo, melodía de fallo
 * y validación de condiciones ambientales críticas.
 */
void outputBloqueo()
{
  Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println("                        x                               ");
  Serial.println();
  taskSetTime.SetIntervalMillis(7000);
  taskSetTime.Start();
  taskBloqueo.Start();
  taskTempLightLimits.Start();
  taskMelodyFail.Start();
}
/**
 * @brief Acción al entrar en el estado Alarma.
 * 
 * Ejecuta las tareas de notificación sonora y visual mediante LED azul,
 * y vigila condiciones ambientales mientras espera tiempo para volver.
 */
void outputAlarma()
{
  Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println("                                          x             ");
  Serial.println();
  taskMelody.Start();
  taskBlueLight.Start();
  taskTempLightLimits.Start();
  taskSetTime.SetIntervalMillis(8000);
  taskSetTime.Start();
}
/**
 * @brief Acción al entrar en el estado AlarmaRed.
 * 
 * Activa alarmas visuales y sonoras específicas de alerta roja,
 * así como monitoreo por botón para salir de la condición.
 */
void outputAlarmaRed()
{
  Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println("                                 x                      ");
  Serial.println();
  taskMelody.Start();
  taskRedLight.Start();
  taskReadButton.Start();
  taskSetTime.SetIntervalMillis(4000);
  taskSetTime.Start();
}
/**
 * @brief Acción al entrar en el estado MonitorEventos.
 * 
 * Inicia las tareas que supervisan eventos relacionados con sensores como Hall e infrarrojo.
 * También muestra la sección correspondiente en consola e inicia validaciones de límites.
 */
void outputMEventos()
{
  temp = 0;
  Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println("                                                    x   ");
  Serial.println();
  taskReadHall.Start();
  taskPrintEventosLcd.Start();
  taskTempLightLimits.Start();
  taskHallLimits.Start();
  taskReadInfra.Start();
  taskInfraAct.Start();
  taskSetTime.SetIntervalMillis(3000);
  taskSetTime.Start();
}
#pragma endregion

#pragma region Leaving States
/**
 * @brief Acción al salir del estado Inicio.
 * 
 * Detiene tareas relacionadas con la seguridad y el temporizador de transición inicial.
 */
void leavingInicio()
{
  taskSecurity.Stop();
  taskSetTime.Stop();
  taskMelodySuccess.Stop();
}
/**
 * @brief Acción al salir del estado MonitorAmbiental.
 * 
 * Detiene todas las tareas asociadas a sensores ambientales y borra el LCD.
 */
void leavingAmbiental()
{
  taskReadButton.Stop();
  taskReadLight.Stop();
  taskReadTemp.Stop();
  taskReadHum.Stop();
  taskPrintLcd.Stop();
  taskTempLightLimits.Stop();
  taskSetTime.Stop();
  lcd.clear();
}
/**
 * @brief Acción al salir del estado Bloqueo.
 * 
 * Detiene las tareas de bloqueo y la melodía de fallo, limpia la pantalla y apaga el LED rojo.
 */
void leavingBloqueo()
{
  lcd.clear();
  taskSetTime.Stop();
  taskMelodyFail.Stop();
  taskBloqueo.Stop();
  digitalWrite(LED_RED, LOW);
}
/**
 * @brief Acción al salir del estado Alarma.
 * 
 * Detiene la melodía y la señal visual azul, limpia el LCD y detiene el temporizador.
 */
void leavingAlarma()
{
  lcd.clear();
  taskMelody.Stop();
  taskBlueLight.Stop();
  digitalWrite(LED_BLUE, LOW);
  taskSetTime.Stop();
}
/**
 * @brief Acción al salir del estado AlarmaRed.
 * 
 * Detiene tareas de alerta por sensor, apaga el LED rojo y limpia la pantalla.
 */
void leavingAlarmaRed()
{
  taskReadButton.Stop();
  lcd.clear();
  taskMelody.Stop();
  taskRedLight.Stop();
  digitalWrite(LED_RED, LOW);
  taskSetTime.Stop();
}
/**
 * @brief Acción al salir del estado MonitorEventos.
 * 
 * Finaliza todas las tareas relacionadas con eventos externos y borra el temporizador.
 */
void leavingEventos()
{
  taskReadButton.Stop();
  taskReadHall.Stop();
  taskPrintEventosLcd.Stop();
  taskHallLimits.Stop();
  taskInfraAct.Stop();  
  taskReadInfra.Stop();
  taskSetTime.Stop();
}
#pragma endregion

#pragma region Functions in Inicio state
/**
 * @brief Maneja el ingreso de la contraseña por parte del usuario.
 * 
 * Lee la entrada desde el teclado matricial, la compara con la contraseña almacenada
 * y determina si el acceso es válido. Si hay tres intentos fallidos, se bloquea el sistema.
 * Además, gestiona el tiempo límite para la entrada del usuario y muestra asteriscos como feedback.
 */
void seguridad()
{
  memset(buffer, 0, sizeof(buffer));
  long startTime = 0;
  long endTime = 0;

  while (tryCounter < 3)
  {
    if (counter == -1)
    {
      lcd.clear();
      lcd.print("Clave:");
      counter++;
    }

    if (startTime != 0) {
      endTime = millis();
    }

    char key = keypad.getKey();

    if (!key && (endTime - startTime) >= 10000) {
      Serial.print("tiempo expirado.");
      buffer[0] = 'w';
      key = '#';
    }
    if (key)
    {
      startTime = millis();
      Serial.print("tarea de tiempo iniciada...");
      Serial.println(key);
      lcd.setCursor(counter, 2);
      lcd.print("*");

      if (counter < 6) {
        buffer[counter] = key;
      }
      counter++;

      if (key == '#')
      {
        if (comparar(password, buffer, 6)) {
          claCorrecta();
          input = Input::claveCorrecta;
          return;
        } else {
          tryCounter++;
          digitalWrite(LED_BLUE, HIGH);
          lcd.clear();
          lcd.print("Clave incorrecta");
          delay(1000);
          digitalWrite(LED_BLUE, LOW);
          counter = -1;
          lcd.clear();
          startTime = 0;
          endTime = 0;
        }
      }
    }
  }
  input = Input::systemBlock;
}
/**
 * @brief Compara dos vectores de caracteres para verificar igualdad.
 * 
 * @param vector1 Primer vector a comparar.
 * @param vector2 Segundo vector a comparar.
 * @param longitud Número de caracteres a comparar.
 * @return true si todos los caracteres coinciden, false en caso contrario.
 */
bool comparar(char vector1[], char vector2[], int longitud)
{
  for (int i = 0; i < longitud; i++)
  {
    if (vector1[i] != vector2[i])
    {
      return false;
    }
  }
  return true;
}
/**
 * @brief Función de bloqueo del sistema.
 * 
 * Muestra en pantalla y consola que el sistema está bloqueado y enciende el LED rojo.
 */
void sisBloqueado()
{
  Serial.println("SystemBlock");
  lcd.print("SystemBlock");
  digitalWrite(LED_RED, HIGH);
  Input::time; // <-- Parece que esta línea no tiene efecto. Tal vez quieras revisarla.
}
/**
 * @brief Indicación visual y sonora de contraseña correcta.
 * 
 * Limpia el LCD, muestra mensajes de éxito, enciende el LED verde y reproduce la melodía de validación.
 */
void claCorrecta()
{
  lcd.clear();
  Serial.println("Clave correcta");
  lcd.print("Clave correcta");
  digitalWrite(LED_GREEN, HIGH);
  successMelody();
  digitalWrite(LED_GREEN, LOW);
}
#pragma endregion

#pragma region Reading sensors
/**
 * @brief Lee el valor del sensor de luz (fotoresistor).
 * 
 * Actualiza la variable global `valueLight` con el valor analógico leído del pin definido como `pinLight`.
 */
void readLight()
{
  valueLight = analogRead(pinLight);
}
/**
 * @brief Lee el valor del sensor de efecto Hall.
 * 
 * Actualiza la variable global `valueHall` con el valor analógico leído del pin definido como `pinHall`.
 */
void readHall()
{
  valueHall = analogRead(pinHall);
}
/**
 * @brief Lee la temperatura a partir de un NTC conectado al pin `pinTemp`.
 * 
 * Calcula la temperatura en grados Celsius utilizando la fórmula de la ecuación de Steinhart-Hart
 * y almacena el valor en la variable global `temp`.
 */
void readTemp()
{
  long a = 1023 - analogRead(pinTemp);
  temp = beta / (log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;
}
/**
 * @brief Lee el valor de humedad relativa desde el sensor DHT.
 * 
 * Actualiza la variable global `hum`. Si falla la lectura, imprime un mensaje de error por consola.
 */
void readHum()
{
  hum = dht.readHumidity();
  if (isnan(hum))
  {
    Serial.println("Failed to read temperature");
  }
}
#pragma endregion

#pragma region General tasks
/**
 * @brief Genera una entrada de tiempo para la máquina de estados.
 * 
 * Esta función marca la entrada como `Input::time` para permitir transiciones temporizadas
 * y detiene la tarea asociada al temporizador principal.
 */
void readTime(void)
{
  input = Input::time;
  taskSetTime.Stop();
}
/**
 * @brief Lee la entrada desde el teclado para detectar una interacción del usuario.
 * 
 * Si se presiona la tecla '*', se considera una señal de botón físico y
 * se actualiza la entrada como `Input::btnPress`.
 */
void readButton()
{
  char key = keypad.getKey();
  if (key == '*')
  {
    input = Input::btnPress;
  }
}
/**
 * @brief Lee el estado del sensor infrarrojo digital.
 * 
 * Almacena el valor lógico del pin asociado al sensor en la variable global `sensorState`.
 */
void readInfra()
{
  sensorState = digitalRead(INFRA);
}
#pragma endregion

#pragma region Printing on LCD
/**
 * @brief Prints the sensor values on the LCD screen.
 * 
 * This function clears the LCD screen and prints the temperature, humidity, and light sensor values.
 */
void printSensorsLcd()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEM:");
  lcd.print(temp);
  lcd.setCursor(8, 0);
  lcd.print("HUM:");
  lcd.print(hum);
  lcd.setCursor(0, 1);
  lcd.print("LUZ:");
  lcd.print(valueLight);
}
/**
 * @brief Prints the hall and InfraRed sensor value on the LCD screen.
 * 
 * This function clears the LCD screen and prints the hall and InfraRed sensor value.
 */
void printEventosLcd()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MAG:");
  lcd.print(valueHall);
  lcd.setCursor(0, 1);
  lcd.print("Infra:");
  lcd.print(sensorState);
  
}
/**
 * @brief Verifies if the temperature limits are exceeded.
 * 
 * This function checks if the current temperature exceed predefined limits and sets the input state accordingly.
 */
void verifyTempLightLimits()
{
  if (temp > tempHigh)
  {
    input = Input::tempLightExceeded;
  }
}
/**
 * @brief Reset alarm Count.
 * 
 * This function reset the alarm Count.
 */
void resetAlarmCount() {
  alarmCount = 0;
}
/**
 * @brief Verifies if the Infrared sensor  is activated.
 * 
 * This function checks if the current Infrared is activated and sets the input state accordingly.
 */
void verifyInfraLimit()
{
  if (sensorState == 0)
  {
    alarmCount++;
    if(alarmCount<3){
    input = Input::infraZero;
    }
  }
  if (alarmCount>=3){
       alarmCount==0;
   input = Input::tempLightExceeded;
resetAlarmCount();
  }
}
/**
 * @brief Verifies if the hall sensor limit is exceeded.
 * 
 * This function checks if the current hall sensor value exceeds a predefined limit and sets the input state accordingly.
 */
void verifyHallLimit()
{
  if (valueHall > hallHigh)
  {
    alarmCount++;
    input = Input::hallExceeded;
  }  if (alarmCount>=3){
   input = Input::tempLightExceeded;
   
  }
}
/**
 * @brief Activates the blue light and displays an alarm message on the LCD screen.
 * 
 * This function clears the LCD screen, displays an alarm message, and turns on the blue LED.
 */
void readBluelight()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ALERTA ACTIVADA:");
  digitalWrite(LED_BLUE, HIGH);
}
/**
 * @brief Activates the blue light and displays an alarm message on the LCD screen.
 * 
 * This function clears the LCD screen, displays an alarm message, and turns on the Red LED.
 */
void readRedlight()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ALARMA ACTIVADA:");
  digitalWrite(LED_RED, HIGH);
}
/**
 * @brief Plays the melody for a successful operation.
 * 
 * This function plays a series of tones on the buzzer representing a success melody.
 */
void melodyExecutable()
{
  int *melody = AlarmMelody;
  int *durations = correctDurations;
  int length = correctMelodyLength;
  for (int i = 0; i < length; i++)
  {
    tone(buzzer, melody[i], durations[i]);
    delay(durations[i] * 1.20); // Delay between notes
    noTone(buzzer);             // Ensure the buzzer is off
  }
}
/**
 * @brief Plays the melody for a failed operation.
 * 
 * This function plays a series of tones on the buzzer representing a failure melody.
 */
void failMelody()
{
  int *melody = wrongMelody;
  int *durations = wrongDurations;
  int length = wrongMelodyLength;
  for (int i = 0; i < length; i++)
  {
    tone(buzzer, melody[i], durations[i]);
    delay(durations[i] * 1.20); // Delay between notes
    noTone(buzzer);             // Ensure the buzzer is off
  }
}
/**
 * @brief Plays the melody for a successful operation.
 * 
 * This function plays a series of tones on the buzzer representing a success melody.
 */
void successMelody()
{
  int *melody = rightMelody;
  int *durations = rightDurations;
  int length = rightMelodyLength;
  for (int i = 0; i < length; i++)
  {
    tone(buzzer, melody[i], durations[i]);
    delay(durations[i] * 1.20); // Delay between notes
    noTone(buzzer);             // Ensure the buzzer is off
  }
}
#pragma endregion