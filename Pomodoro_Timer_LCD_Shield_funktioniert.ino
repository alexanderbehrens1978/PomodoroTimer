#include <LiquidCrystal.h>
#include <EEPROM.h> // EEPROM-Bibliothek

// LCD Initialisierung (Pins für RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Buzzer-Pin
const int buzzerPin = 50;

// Steuerungs-Tasten
#define KEY_RIGHT   0
#define KEY_UP      141
#define KEY_DOWN    340
#define KEY_LEFT    510
#define KEY_SELECT  730
#define KEY_NONE    1023

// Version des Programms
const char* VERSION = "1.2";

// Menü-Variablen
int menuIndex = 0;
const char* menuItems[] = {"Set Work Time", "Set Pause Time", "Set Beep Time", "Set Min Time", "Set Long Pause", "Set AutoStart Time", "Start Timer", "Reset Timer", "Help"};
const int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);

// Timer-Variablen (in Minuten und Sekunden)
int workTime = 25;        // Standard-Arbeitszeit (Minuten)
int pauseTime = 5;        // Standard-Pausenzeit (Minuten)
int longPauseTime = 15;   // Dauer der langen Pause (Minuten)
int beepTime = 5;         // Standard-Piepdauer (Sekunden)
int minTime = 1;          // Mindestzeit für Arbeits- und Pausenzeit (Minuten)
int autoStartTime = 15;   // Autostart-Zeit in Sekunden
int remainingTime = 0;    // Verbleibende Zeit in Sekunden
int cycleCount = 0;       // Anzahl abgeschlossener Arbeitszyklen
const int maxCycles = 4;  // Zyklen bis zur langen Pause
bool settingTime = false;
bool paused = true;       // Timer-Status

// Autostart-Variablen
unsigned long lastInteraction = 0; // Letzte Benutzerinteraktion

// Timer-Status
enum TimerState { STOPPED, WORK, PAUSE, LONG_PAUSE };
TimerState state = STOPPED;

// Zeitsteuerung für den Timer
unsigned long lastUpdate = 0; // Zeitpunkt der letzten Timer-Aktualisierung

// EEPROM Speicheradressen
#define EEPROM_WORK_TIME_ADDR       0
#define EEPROM_PAUSE_TIME_ADDR      2
#define EEPROM_LONG_PAUSE_TIME_ADDR 4
#define EEPROM_BEEP_TIME_ADDR       6
#define EEPROM_MIN_TIME_ADDR        8
#define EEPROM_AUTOSTART_TIME_ADDR  10
#define EEPROM_VALID_FLAG_ADDR      12 // Flag, um gültige Daten zu markieren

// Funktionsdeklarationen
void startTimer();
void resetTimer();
void saveSettings();
void loadSettings();
void displayMenu();
void executeMenuAction();
void updateLCD();
void endPhase();
void showHelp();
void adjustTime(int delta);

void setup() {
  lcd.begin(16, 2); // LCD initialisieren
  lcd.print("Pomodoro Timer");
  lcd.setCursor(0, 1);
  lcd.print("Version: ");
  lcd.print(VERSION);
  pinMode(buzzerPin, OUTPUT); // Buzzer-Pin setzen
  delay(2000);
  lcd.clear();

  loadSettings(); // Lade gespeicherte Einstellungen aus EEPROM
  displayMenu();
  lastInteraction = millis(); // Initialer Zeitstempel für Autostart
}

void loop() {
  int key = analogRead(A0); // Tastenwert lesen

  // Autostart-Logik: Prüfen, ob Autostart-Zeit abgelaufen ist
  if (millis() - lastInteraction >= autoStartTime * 1000 && state == STOPPED && !settingTime) {
    startTimer(); // Timer automatisch starten
    return;
  }

  // Wenn eine Taste gedrückt wurde, Autostart-Zeitstempel aktualisieren
  if (key < KEY_NONE - 10) {
    lastInteraction = millis();
  }

  if (!settingTime) {
    // Navigation durch das Menü
    if (key > KEY_UP - 10 && key < KEY_UP + 10) { // UP-Taste
      menuIndex = (menuIndex + 1) % menuSize; // Nächstes Menü
      displayMenu();
      delay(300);
    } else if (key > KEY_DOWN - 10 && key < KEY_DOWN + 10) { // DOWN-Taste
      menuIndex = (menuIndex - 1 + menuSize) % menuSize; // Vorheriges Menü
      displayMenu();
      delay(300);
    } else if (key > KEY_SELECT - 10 && key < KEY_SELECT + 10) { // SELECT-Taste
      executeMenuAction();
      delay(300);
    } else if (!paused) {
      // Während einer Phase
      if (key > KEY_LEFT - 10 && key < KEY_LEFT + 10) { // LEFT-Taste
        resetTimer(); // Phase abbrechen und Timer zurücksetzen
        delay(300);
      } else if (key > KEY_RIGHT - 10 && key < KEY_RIGHT + 10) { // RIGHT-Taste
        endPhase(); // Phase überspringen
        delay(300);
      }
    }
  } else {
    // Zeit einstellen
    if (key > KEY_LEFT - 10 && key < KEY_LEFT + 10) { // LEFT-Taste
      adjustTime(-1); // Zeit verringern in 1-Schritten
      delay(300);
    } else if (key > KEY_RIGHT - 10 && key < KEY_RIGHT + 10) { // RIGHT-Taste
      adjustTime(1); // Zeit erhöhen in 1-Schritten
      delay(300);
    } else if (key > KEY_UP - 10 && key < KEY_UP + 10) { // UP-Taste
      adjustTime(5); // Zeit erhöhen in 5-Schritten
      delay(300);
    } else if (key > KEY_DOWN - 10 && key < KEY_DOWN + 10) { // DOWN-Taste
      adjustTime(-5); // Zeit verringern in 5-Schritten
      delay(300);
    } else if (key > KEY_SELECT - 10 && key < KEY_SELECT + 10) { // SELECT-Taste
      settingTime = false; // Einstellung abschließen
      saveSettings(); // Änderungen speichern
      displayMenu();
      delay(300);
    }
  }

  // Timer-Funktion: Alle 1000 ms aktualisieren
  if (!paused && (millis() - lastUpdate >= 1000)) {
    lastUpdate = millis(); // Zeitpunkt aktualisieren
    remainingTime--;

    updateLCD();

    if (remainingTime <= 0) {
      endPhase();
    }
  }
}

void adjustTime(int delta) {
  if (menuIndex == 0) { // Arbeitszeit
    workTime = max(minTime, workTime + delta);
    lcd.setCursor(0, 1);
    lcd.print(workTime);
    lcd.print(" min         ");
  } else if (menuIndex == 1) { // Pausenzeit
    pauseTime = max(minTime, pauseTime + delta);
    lcd.setCursor(0, 1);
    lcd.print(pauseTime);
    lcd.print(" min         ");
  } else if (menuIndex == 2) { // Piepdauer
    beepTime = max(1, beepTime + delta);
    lcd.setCursor(0, 1);
    lcd.print(beepTime);
    lcd.print(" sec         ");
  } else if (menuIndex == 3) { // Mindestzeit
    minTime = max(1, minTime + delta);
    lcd.setCursor(0, 1);
    lcd.print(minTime);
    lcd.print(" min         ");
  } else if (menuIndex == 4) { // Lange Pause
    longPauseTime = max(minTime, longPauseTime + delta);
    lcd.setCursor(0, 1);
    lcd.print(longPauseTime);
    lcd.print(" min         ");
  } else if (menuIndex == 5) { // Autostart-Zeit
    autoStartTime = max(5, autoStartTime + delta); // Mindestens 5 Sekunden
    lcd.setCursor(0, 1);
    lcd.print(autoStartTime);
    lcd.print(" sec         ");
  }
}

void startTimer() {
  paused = false;
  state = WORK;
  remainingTime = workTime * 60;
  lastUpdate = millis();
  updateLCD();
}

void resetTimer() {
  paused = true;
  state = STOPPED;
  remainingTime = 0;
  cycleCount = 0; // Zyklen zurücksetzen
  lcd.setCursor(0, 0);
  lcd.print("Timer Reset     ");
  delay(1000);
  displayMenu();
}

void saveSettings() {
  EEPROM.put(EEPROM_WORK_TIME_ADDR, workTime);
  EEPROM.put(EEPROM_PAUSE_TIME_ADDR, pauseTime);
  EEPROM.put(EEPROM_LONG_PAUSE_TIME_ADDR, longPauseTime);
  EEPROM.put(EEPROM_BEEP_TIME_ADDR, beepTime);
  EEPROM.put(EEPROM_MIN_TIME_ADDR, minTime);
  EEPROM.put(EEPROM_AUTOSTART_TIME_ADDR, autoStartTime);
  EEPROM.put(EEPROM_VALID_FLAG_ADDR, 0xAA); // Gültigkeitsflag setzen
}

void loadSettings() {
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == 0xAA) {
    EEPROM.get(EEPROM_WORK_TIME_ADDR, workTime);
    EEPROM.get(EEPROM_PAUSE_TIME_ADDR, pauseTime);
    EEPROM.get(EEPROM_LONG_PAUSE_TIME_ADDR, longPauseTime);
    EEPROM.get(EEPROM_BEEP_TIME_ADDR, beepTime);
    EEPROM.get(EEPROM_MIN_TIME_ADDR, minTime);
    EEPROM.get(EEPROM_AUTOSTART_TIME_ADDR, autoStartTime);
  } else {
    // Standardwerte setzen, wenn kein gültiger Speicher vorhanden ist
    workTime = 25;
    pauseTime = 5;
    longPauseTime = 15;
    beepTime = 5;
    minTime = 1;
    autoStartTime = 15;
    saveSettings(); // Standardwerte speichern
  }
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu: ");
  lcd.print(VERSION); // Version im Menü anzeigen
  lcd.setCursor(0, 1);
  lcd.print(menuItems[menuIndex]);
}

void executeMenuAction() {
  lcd.clear();
  switch (menuIndex) {
    case 0: // Arbeitszeit einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("Work Time:");
      lcd.setCursor(0, 1);
      lcd.print(workTime);
      lcd.print(" min         ");
      break;
    case 1: // Pausenzeit einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("Pause Time:");
      lcd.setCursor(0, 1);
      lcd.print(pauseTime);
      lcd.print(" min         ");
      break;
    case 2: // Piepdauer einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("Beep Time:");
      lcd.setCursor(0, 1);
      lcd.print(beepTime);
      lcd.print(" sec         ");
      break;
    case 3: // Mindestzeit einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("Min Time:");
      lcd.setCursor(0, 1);
      lcd.print(minTime);
      lcd.print(" min         ");
      break;
    case 4: // Lange Pause einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("Long Pause:");
      lcd.setCursor(0, 1);
      lcd.print(longPauseTime);
      lcd.print(" min         ");
      break;
    case 5: // Autostart-Zeit einstellen
      settingTime = true;
      lcd.setCursor(0, 0);
      lcd.print("AutoStart Time:");
      lcd.setCursor(0, 1);
      lcd.print(autoStartTime);
      lcd.print(" sec         ");
      break;
    case 6: // Timer starten
      startTimer();
      break;
    case 7: // Timer zurücksetzen
      resetTimer();
      break;
    case 8: // Hilfe anzeigen
      showHelp();
      break;
  }
}

void updateLCD() {
  lcd.setCursor(0, 0);
  if (state == WORK) {
    lcd.print("Work Phase      ");
    lcd.setCursor(0, 1);
    int minutes = remainingTime / 60;
    int seconds = remainingTime % 60;
    int remainingCycles = maxCycles - cycleCount;

    lcd.print("T:");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0");
    lcd.print(seconds);
    lcd.print(" C:");
    lcd.print(remainingCycles);
  } else if (state == PAUSE) {
    lcd.print("Pause Phase     ");
    lcd.setCursor(0, 1);
    int minutes = remainingTime / 60;
    int seconds = remainingTime % 60;
    lcd.print("Time: ");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0");
    lcd.print(seconds);
  } else if (state == LONG_PAUSE) {
    lcd.print("Long Pause      ");
    lcd.setCursor(0, 1);
    int minutes = remainingTime / 60;
    int seconds = remainingTime % 60;
    lcd.print("Time: ");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) lcd.print("0");
    lcd.print(seconds);
  }
}

void endPhase() {
  for (int i = beepTime; i > 0; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Beeping...");
    lcd.setCursor(0, 1);
    lcd.print("Time left: ");
    lcd.print(i);
    lcd.print(" sec    ");
    tone(buzzerPin, 1000);
    delay(500);
    noTone(buzzerPin);
    delay(500);
  }

  lcd.clear(); // Anzeige für die neue Phase vorbereiten

  if (state == WORK) {
    cycleCount++;
    if (cycleCount >= maxCycles) {
      state = LONG_PAUSE;
      remainingTime = longPauseTime * 60;
      cycleCount = 0;
    } else {
      state = PAUSE;
      remainingTime = pauseTime * 60;
    }
  } else {
    state = WORK;
    remainingTime = workTime * 60;
  }

  updateLCD();
}

void showHelp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UP: Next Menu");
  lcd.setCursor(0, 1);
  lcd.print("DOWN: Prev Menu");
  delay(3000); // 3 Sekunden warten

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LEFT: Cancel");
  lcd.setCursor(0, 1);
  lcd.print("RIGHT: Skip");
  delay(3000); // 3 Sekunden warten

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SELECT: Confirm");
  lcd.setCursor(0, 1);
  lcd.print("Start Timer");
  delay(3000);

  displayMenu(); // Zurück zum Menü
}
