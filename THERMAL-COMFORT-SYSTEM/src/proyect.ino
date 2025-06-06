/**
*@mainpage THERMAL-COMFORT-SYSTEM
 * @file proyect.ino
 * @brief THERMAL COMFORT SYSTEM using Arduino.
 * 
 * This file contains the main code for a monitoring and security system
 * that uses an LCD, keypad, LEDs, light sensors, temperature and humidity sensors, and a Hall sensor.
 * It uses a state machine and asynchronous tasks.
 */

#include "StateMachineLib.h"
// include the library code:
#include <LiquidCrystal.h>
// Include the keypad code
#include <Keypad.h>
#include "AsyncTaskLib.h"
#include "DHT.h"

// Pin declarations and hardware configuration
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
 * @brief Matrix keyboard key mapping.
 */
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {22, 24, 26, 28}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#pragma endregion

#pragma region Configuration for the leds
#define LED_RED 10
#define LED_GREEN 9
#define LED_BLUE 8
#pragma endregion
#pragma region Button configuration
#define BUTTON_PIN 6

#pragma endregion

#pragma region Configuration for the Buzzer
int buzzer = 7;
#pragma endregion

#pragma region Configuration for the sensors
// Light
const int pinLight = A0;
int valueLight = 0;
//Temp
const int pinTemp=A4;
#define beta 4090
//InfraRed
#define INFRA 15

// Hall
const int pinHall = A1;
int valueHall = 0;

// DHT
#define DHTTYPE DHT11 // Simultor 22
#define DHTPIN 38
DHT dht(DHTPIN, DHTTYPE);
int hum = 0;
int sensorState = 0;
float temp = 0;
// Limits
int tempHigh = 29;
int tempLow = 15;
int luzHigh = 800;
int luzLow = 200;
int humHigh = 70;
int humLow = 30;
int hallHigh = 300;
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
int BloqueoMelody[] = {
    NOTE_E7, NOTE_E7, 0, NOTE_E7,
    0, NOTE_C7, NOTE_E7, 0,
    NOTE_G7, 0, 0, 0};
int correctDurations[] = {
    200, 200, 200, 200,
    200, 200, 200, 200,
    200, 200, 200, 200};
int correctMelodyLength = sizeof(BloqueoMelody) / sizeof(BloqueoMelody[0]);
int AlarmMelody[] = {
    NOTE_F3,
    NOTE_A3,
};
int incorrectDurations[] = {
    300, 300, 300, 300};
int incorrectMelodyLength = sizeof(AlarmMelody) / sizeof(AlarmMelody[0]);

int rightMelody[] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5
    };
int rightDurations[] = {
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500};
int rightMelodyLength = sizeof(rightMelody) / sizeof(rightMelody[0]);

int wrongMelody[] = {
    NOTE_C4, NOTE_B3, NOTE_A3, NOTE_G3, NOTE_F3, NOTE_E3, NOTE_D3, NOTE_C3, NOTE_B2, NOTE_A2
    };
int wrongDurations[] = {
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500};
int wrongMelodyLength = sizeof(wrongMelody) / sizeof(wrongMelody[0]);
#pragma endregion

#pragma region Configuration for the security
const char password[6] = {'1','1','2','2','3', '#'};
char buffer[6];
int counter = -1;
char tryCounter = 0;
#pragma endregion
#pragma region Configuration for the State Machine

// State Alias
enum State
{
  Inicio = 0,
  AlarmaRed = 1,
  MonitorAmbiental = 2,
  Bloqueo = 3,
  Alarma = 4,
  MonitorEventos = 5

};

// Input Alias
enum Input
{
  time = 0,
  claveCorrecta = 1,
  systemBlock = 2,
  btnPress = 3,
  hallExceeded = 4,
  tempLightExceeded = 5,
  infraZero = 6,
  sensores = 7,
  unknown = 8,
};

// Create new StateMachine
StateMachine stateMachine(6, 12);

// Stores last user input
Input input = Input::unknown;

// Setup the State Machine
void setupStateMachine()
{
  // Add transitions
  // State: Inicio
  stateMachine.AddTransition(Inicio, MonitorAmbiental, []()
                             { return input == claveCorrecta; });
  stateMachine.AddTransition(Inicio, Bloqueo, []()
                             { return input == systemBlock; });
  stateMachine.AddTransition(MonitorAmbiental, MonitorEventos, []()
                             { return input == time; });
  stateMachine.AddTransition(MonitorAmbiental, AlarmaRed, []()
                             { return input == tempLightExceeded; });
  // State: Monitor Eventos
  stateMachine.AddTransition(MonitorEventos, MonitorAmbiental, []()
                             { return input == time; });
  stateMachine.AddTransition(MonitorEventos, Alarma, []()
                             { return input == hallExceeded; });
  stateMachine.AddTransition(MonitorEventos, Alarma, []()
                             { return input == infraZero; });
  // State: Alarma

  stateMachine.AddTransition(Alarma, MonitorAmbiental, []()
                             { return input == time; });
  // State: Bloqueo
  stateMachine.AddTransition(Bloqueo, Inicio, []()
                             { return input == time; });
  stateMachine.AddTransition(MonitorEventos, AlarmaRed, []()
                             { return input == tempLightExceeded; });
  stateMachine.AddTransition(AlarmaRed, Inicio, []()
                            { return input == btnPress; });

  // Add actions
  // Entering functions
  stateMachine.SetOnEntering(Inicio, outputInicio);
  stateMachine.SetOnEntering(MonitorAmbiental, outputMAmbiental);
  stateMachine.SetOnEntering(Bloqueo, outputBloqueo);
  stateMachine.SetOnEntering(Alarma, outputAlarma);
  stateMachine.SetOnEntering(AlarmaRed, outputAlarmaRed);
  stateMachine.SetOnEntering(MonitorEventos, outputMEventos);
  // Leaving functions
  stateMachine.SetOnLeaving(Inicio, leavingInicio);
  stateMachine.SetOnLeaving(MonitorAmbiental, leavingAmbiental);
  stateMachine.SetOnLeaving(Bloqueo, leavingBloqueo);
  stateMachine.SetOnLeaving(Alarma, leavingAlarma);
  stateMachine.SetOnLeaving(AlarmaRed, leavingAlarmaRed);
  stateMachine.SetOnLeaving(MonitorEventos, leavingEventos);
}
#pragma endregion

#pragma region Methods
void readLight(void);
void readTemp(void);
void readHum(void);
void readTime(void);
void readHall(void);
void readInfra(void);
void printSensorsLcd(void);
void printEventosLcd(void);
void verifyTempLightLimits(void);
void verifyHallLimit(void);
void readBluelight(void);
void readRedlight(void);
void melodyExecutable(void);
void readButton(void);
void seguridad(void);
void failMelody(void);
void successMelody(void);
void sisBloqueado(void);
void verifyInfraLimit(void);
void resetAlarmCount(void);

#pragma endregion

#pragma region Tasks
AsyncTask taskReadLight(1000, true, readLight);
AsyncTask taskReadTemp(1000, true, readTemp);
AsyncTask taskReadHum(1000, true, readHum);
AsyncTask taskReadHall(1000, true, readHall);
AsyncTask taskReadInfra(1000, true, readInfra);
AsyncTask taskSetTime(10000, true, readTime);
AsyncTask taskPrintLcd(1000, true, printSensorsLcd);
AsyncTask taskPrintEventosLcd(1000, true, printEventosLcd);
AsyncTask taskTempLightLimits(1000, true, verifyTempLightLimits);
AsyncTask taskHallLimits(1000, true, verifyHallLimit);
AsyncTask taskInfraAct(1000, true, verifyInfraLimit);
AsyncTask taskBlueLight(400, true, readBluelight);
AsyncTask taskRedLight(400, true, readRedlight);
AsyncTask taskMelody(800, false, melodyExecutable);
AsyncTask taskReadButton(100, true, readButton);
AsyncTask taskSecurity(1000, false, seguridad);
AsyncTask taskMelodyFail(800, false, failMelody);
AsyncTask taskMelodySuccess(800, false, successMelody);
AsyncTask taskBloqueo(500, false, sisBloqueado);
#pragma endregion

const byte ledPin = LED_BUILTIN;
bool ledState = LOW;
char ledState_text[4];

char string_on[] = "ON";
char string_off[] = "OFF";

const byte analogPin = A5;
unsigned short analogValue = 0;



#pragma endregion

/**
 * @brief Initialize the system and its components.
 */
void setup()
{
  Serial.begin(9600); // Simulator 115200
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  dht.begin();
  // About leds
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(INFRA, INPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUTTON_PIN,INPUT);
  // Turn off the leds
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  Serial.println("Starting State Machine...");
  setupStateMachine();
  Serial.println("Start Machine Started");

  // Initial state
  stateMachine.SetState(Inicio, false, true);
}

/**
 * @brief Main loop of the system, executes repetitive logic.
 */

void loop()
{
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
  input = Input::unknown;
}

#pragma region Entering functions
void outputInicio()
{
 Serial.println("Inicio   Ambiental   Bloqueo   Alarma   Alerta   Eventos");
  Serial.println(" x                                                       ");
  Serial.println();
  tryCounter = 0;
  counter = -1;
  taskSecurity.Start();
}

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

void leavingInicio()
{
  taskSecurity.Stop();
  taskSetTime.Stop();
  taskMelodySuccess.Stop();
}

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
void leavingBloqueo()
{
  lcd.clear();
  taskSetTime.Stop();
  taskMelodyFail.Stop();
  taskBloqueo.Stop();
  digitalWrite(LED_RED, LOW);
}
void leavingAlarma()
{
  lcd.clear();
  taskMelody.Stop();
  taskBlueLight.Stop();
  digitalWrite(LED_BLUE, LOW);
  taskSetTime.Stop();
}
void leavingAlarmaRed()
{
  taskReadButton.Stop();
  lcd.clear();
  taskMelody.Stop();
  taskRedLight.Stop();
  digitalWrite(LED_RED, LOW);
  taskSetTime.Stop();


}
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
 * @brief Handles reading the password entered by the user.
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
      //taskSetTime.SetIntervalMillis(5000);
      //taskSetTime.Start();
    }
    if(startTime != 0){
      endTime = millis();
    }
    char key = keypad.getKey();
    if(!key && (endTime - startTime) >= 10000){
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
      if (counter < 6)
      {
        buffer[counter] = key;
      }
      counter++;
      if (key == '#')
      {
        if (comparar(password, buffer, 6) == true)
        {
          claCorrecta();
          input = Input::claveCorrecta;
          return;
        }
        else
        {
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
 * @brief Compare char vectors
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
 * @brief System blocked function
 */

void sisBloqueado()
{
  Serial.println("SystemBlock");
  lcd.print("SystemBlock");
  digitalWrite(LED_RED, HIGH);
  Input::time;
}

/**
 * @brief Visual function for correct password
 */

void claCorrecta()
{
  lcd.clear();
  Serial.println("Clave correcta");
  lcd.print("Clave correcta");
  digitalWrite(LED_GREEN, HIGH);
  successMelody();
  //delay(3000);
  digitalWrite(LED_GREEN, LOW);
}
#pragma endregion
#pragma region Reading sensors

/**
 * @brief Read photoresistor sensor
 */
void readLight()
{
  valueLight = analogRead(pinLight);
}

/**
 * @brief Read hall sensor
 */
void readHall()
{
  valueHall = analogRead(pinHall);
}

/**
 * @brief Read temp from DHT sensor
 */
void readTemp()
{
  long a = 1023 - analogRead(pinTemp);
  temp = beta / (log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;

}

/**
 * @brief Read humedity from DHT sensor
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
void readTime(void)
{
  input = Input::time;
  taskSetTime.Stop();
}

/**
 * @brief Read button input
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
 * @brief Read InfraRed input
 */
void readInfra()
{
  sensorState = digitalRead(INFRA); // Lee el estado del pin
    
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