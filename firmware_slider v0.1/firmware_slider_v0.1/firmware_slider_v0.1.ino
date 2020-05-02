#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// LCD Display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pins
const int upButtonPin = 13;
const int middleButtonPin = 12;
const int downButtonPin = 11;

// Motorverbindungen für A4988
const int dirPin = 4;       // Richtung
const int stepPin = 3;      // Schrittweite
const byte MS1 = 8; 
const byte MS2 = 9;
const byte MS3 = 10;
bool terminated = false;
#define home_switch 2

// Button-Variablen
int upButtonState = 0;
int middleButtonState = 0;
int downButtonState = 0;
int lastUpButtonState = 0;
int lastMiddleButtonState = 0;
int lastDownButtonState = 0;

// Kontroll-Variablen
bool LCDChanged = true;
int menuLayer = 0;                          // Wert für das aktuelle Menülayer
int menuCursor = 1;                         // Pointer für die 4 Zeilen des LCD Displays
int timelapseDistance = 100;                // Distanz, die der Schlitten fahren muss in Prozent
int timelapseFrameratePointer = 2;          // Pointer auf framerateOptions; Zielframerate des fertigen Videos
int timelapseLength = 10;                   // Länge des fertigen Videos
int timelapseInterval = 8;                  // Intervall zwischen Bildern in Sekunden
int timelapseImageCount = 0;                // Anzahl der Bilder, die aufgenommen werden müssen4
unsigned int timelapseDuration = 600;       // Dauer des Zeitraffers in Sekunden
int timelapseMinuimumInterval = 1;          // Mindestintervall bei der Aufnahme
unsigned int timelapseStepsPerRev = 3200;   // Motorschritte pro Rotation
long timelapseDollyMotorSpeed = 60;         // Motorgeschwindigkeit im Dollymodus (min. 60; 312.5ms * 3200steps = 1rps)
int shotImages = 0;                         // Anzahl der bereits aufgenommenen Bilder im Intervalmode
int startTime = 0;                          // Variable zum Einhalten des Intervalls
unsigned int intervalDelay = 0;             // Wartezeit, bis der Durchlauf fertig ist

// Konstante Variablen
const String sliderVersion = "v0.0.1";
const String blankLine = "                   ";
const int sliderLength = 314;               // Länge des Sliders in mm
const int framerateOptions[8] = {24, 25, 30, 48, 50, 60, 100, 120};
const int menuLinks[16][4] = {
  {-1, 1, 10, 14},                        // 0  Hautpmenü
  {-1, 2, 0, 0},                          // 1  Distanz einstellen Übersicht
  {-1, -1, -1, 3},                        // 2  Distanz einstellen Eingabe
  {-1, 4, 1, 0},                          // 3  Framerate einstellen Übersicht
  {-1, -1, -1, 5},                        // 4  Framerate einstellen Eingabe
  {-1, 6, 3, 0},                          // 5  Videolänge einstellen Übersicht
  {-1, -1, -1, 7},                        // 6  Videolänge einstellen Eingabe
  {-1, 8, 5, 0},                          // 7  Intvervall einstellen Übersicht
  {-1, -1, -1, 9},                        // 8  Intvervall einstellen Eingabe
  {-1, 15, 7, 0},                         // 9  Aufnahme starten
  {-1, 11, 0, 0},                         // 10 Distanz einstellen Übersicht
  {-1, -1, -1, 12},                       // 11 Distanz einstellen Eingabe
  {-1, 13, 10, 0},                        // 12 Dauer einstellen Übersicht
  {-1, -1, -1, 15},                       // 13 Dauer einstellen Eingabe
  {-1, -1, -1, 0},                        // 14 Slider zurücksetzen
  {-1, -1, -1, 0}                         // 15 Aufnahme
};
const int menuLength = int(sizeof(menuLinks)/sizeof(menuLinks[0]));  // Anzahl der Menülayer


// Initialisierung
void setup() {
  // Buttons als Input initialisieren
  pinMode(upButtonPin, INPUT);
  pinMode(middleButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  // LCD initialisieren
  lcd.init();
  lcd.backlight();
  // Willkommensscreen
  printLCD(2, 1, "Timelapse Slider", 0, true);
  printLCD(7, 2, sliderVersion, 0, false);
  // Motorparameter konfigurieren
  Serial.begin(9600);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(MS1,OUTPUT);
  pinMode(MS2,OUTPUT);
  pinMode(MS3,OUTPUT);
  pinMode (home_switch, INPUT);
  digitalWrite(MS1,HIGH);                 // Definiert die Schrittweite
  digitalWrite(MS2,HIGH);                 // 
  digitalWrite(MS3,HIGH);                 // 3x HIGH = 0.1125°
  delay(1000);
  // LCD für Menü vorbereiten
  printLCD(0, 0, "", 0, true);
}


// Hauptschleife
void loop() {
  // Wenn man im vorletzten Menüeintrag ist (Slider initialisieren)
  if(menuLayer == menuLength - 2){
      delay(1000);
      // Slider auf Startposition fahren
      moveSliderToStartPosition();
      // Menülayer und -cursor auf Ausgangsposition setzen
      menuLayer = 0;
      menuCursor = 1;
      // LCD updaten
      LCDChanged = true;
  }
  // Wenn man im letzten Menüeintrag ist (Aufnahme)
  if(menuLayer >= menuLength - 1){
    // Wenn initialisierung abgeschlossen wurde, Aufnahme ermöglichen
    if(middleButtonState == 0){
      terminated = false;
    }
    // Aufnahme wurde in der Initiailisierungsphase abgebrochen
    else{
      // Aufnahme verhindern
      terminated = true;
      // Menülayer und -cursor auf Ausgangsposition setzen
      menuLayer = 0;
      menuCursor = 1;
      // LCD updaten
      LCDChanged = true;
    }
    // Anzahl der aufgenommenen Bilder zurücksetzen
    shotImages = 0;
    // Aufnahmeparamter berechnen
    calculateRecordingParameters();
    // Aufnhametimer auf 0 setzen
    setTime(0);
    // Starttime für Intervall festlegen
    startTime = millis();
    // Solange der Schlitten nicht am Ende angekommen ist
    // Intervallaufnahme
    if(timelapseInterval > 0){
      // Solange der Schlitten nicht am Ende angekommen ist
      while(!terminated){
          // Aufnahmeinformationen anzeigen
          displayRecordingStatus();
          // Motorsteuerung - Im Uhrzeigersinn drehen = HIGH; Gegen den Uhrzeigersinn drehen = LOW
          intervalMode(LOW);
          // Starttime für Intervall festlegen
          startTime = millis();
          // Aufnahmeinformationen anzeigen
          displayRecordingStatus();
          // Stopbedingungen (middleButton gedrückt, Anzahl Bilder erreicht)
          checkStopCondition();
        }
    }
    // Dolly-Aufnahme
    else{
      // Anzeige updaten
      printLCD(1, 1, "Aufnahme gestartet", 0, false);
      printLCD(1, 2, "Dauer: " + String(timelapseDuration) + " s", 0, false);
      // Gewünschte Sliderlänge fahren
      for(int i = 0; i < (sliderLength*(100/timelapseDistance)); i++){
        // Solange der Schlitten nicht am Ende angekommen ist
        if(!terminated){
          // Motorsteuerung - Im Uhrzeigersinn drehen = HIGH; Gegen den Uhrzeigersinn drehen = LOW
          dollyMode(LOW);
          // Stopbedingungen (middleButton gedrückt, Anzahl Bilder erreicht)
          checkStopCondition();
        }
        // Schleife abbrechen
        else{
          i = (sliderLength*(100/timelapseDistance));
        }
      }
    }
  }
  // Menü
  else{
    // Benutzereingabe verwalten
    displayControl();
  }
}



///////////////////////////////////////////// Helferfunktionen /////////////////////////////////////////////
// Interval-Mode Motorrotation
void intervalMode(bool direction){
  // Motorrichtung festlegen
  digitalWrite(dirPin,direction);
  // Motor langsam eine Umdrehung fahren lassen
  for(int x = 0; x < timelapseStepsPerRev ; x++) {
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(50);
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(50);
  }
  // Anzahl der aufgenommenen Bilder updaten
  shotImages += 1;
  // Pausieren, um Intervall einzuhalten
  intervalDelay = long(long(timelapseInterval)*1000) - ((millis() - startTime));
  if(intervalDelay > 0){
    delay(intervalDelay);
  }
}


// Dolly-Mode Motorrotation
void dollyMode(bool direction){
  // Motorrichtung festlegen
  digitalWrite(dirPin,direction);
  // Motor langsam eine Umdrehung fahren lassen
  for(int x = 0; x < timelapseStepsPerRev ; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(timelapseDollyMotorSpeed/2);
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(timelapseDollyMotorSpeed/2);
  }
}


// Überprüfung, ob der Schlitten am Ende angekommen ist
void check_home_switch(){
    // Wenn Endschalter aktiv ist
    if(digitalRead(home_switch)){
      digitalWrite(dirPin, HIGH);    // Richtungsänderung
      digitalWrite(stepPin, HIGH);   // Motor an
      delay(5);
      digitalWrite(stepPin, LOW);    // Motor aus
      delay(5);
      terminated = true;
    }
}


// Slider auf Ausgangsposition setzen
void moveSliderToStartPosition(){
  // Endschalter zurücksetzen
  terminated = false;
  // Für den Zeitraffer berechnete Motorschrittweite speichern
  unsigned int oldValue = timelapseStepsPerRev;
  // Schrittweite des Motors ganz klein einstellen, damit präzise auf den Anfang gefahren werden kann
  timelapseStepsPerRev = 100;
  // MittelButton checken
  middleButtonState = digitalRead(middleButtonPin);
  // Solange der Schlitten nicht am Ende angelangt ist und der Mittelbutton nicht gedrückt wurde
  while(!terminated && middleButtonState == 0){
    // Überprüfen, ob der Button am Ende angelangt ist
    check_home_switch();
    // Vollgas
    timelapseDollyMotorSpeed = 60;
    // Slider bewegen
    dollyMode(HIGH);
    // MittelButton checken
    middleButtonState = digitalRead(middleButtonPin);
  }
  // Berechnete Motorschrittweite wiederherstellen
  timelapseStepsPerRev = oldValue;
  // Endschalter zurücksetzen
  terminated = false;
}


// Stop-Bedingung für Aufnahme überprüfen
void checkStopCondition(){
  // Überprüfen, ob das Ende erreicht wurde
  check_home_switch();
  // MittelButton checken
  middleButtonState = digitalRead(middleButtonPin);
  // Intervalmodus
  if(timelapseInterval > 0){
    if(middleButtonState == 1 || shotImages >= timelapseImageCount){
      if(shotImages >= timelapseImageCount){
        // Aufnahmeinformationen blinkend anzeigen
        int blinkCounter = 0;
        for(int i = 0; i < 5; i++)
        {
          printLCD(0, 1, String(float(timelapseDuration)/60), 0, false);
          printLCD(0, 2, String(timelapseImageCount), 0, false);
          delay(800);
          printLCD(0, 1, blankLine, 0, false);
          printLCD(0, 2, blankLine, 0, false);
          delay(200);
        }
      }
      // Aufnahme abbrechen
      terminated = true;
      // Menülayer und -cursor auf Ausgangsposition setzen
      menuLayer = 0;
      menuCursor = 1;
      // LCD updaten
      LCDChanged = true;
    }
  }
  else{
    if(middleButtonState == 1){
      // Aufnahme abbrechen
      terminated = true;
      // Menülayer und -cursor auf Ausgangsposition setzen
      menuLayer = 0;
      menuCursor = 1;
      // LCD updaten
      LCDChanged = true;
    }
  }
}


// Aufnahmeparamter berechnen
void calculateRecordingParameters(){
  // Intervallmodus
  if(timelapseInterval > 0){
    // Anzahl der aufzunehmenden Bilder berechnen
    timelapseImageCount = timelapseLength * int(framerateOptions[int(timelapseFrameratePointer)]);
    // Dauer der Zeitrafferaufnahme in Sekunden
    timelapseDuration = timelapseImageCount * timelapseInterval;
    // Schrittweite des Motors berechnen
    timelapseStepsPerRev = long(3200*long(314*(timelapseDistance/100)))/long(timelapseImageCount);
    // Mindestintervall bestimmen
    timelapseMinuimumInterval = long(long(timelapseStepsPerRev)/long(10000)) + 1;
    // Wenn das aktuelle Intervall unter dem Mindestintervall liegt
    if(timelapseInterval < timelapseMinuimumInterval){
      // Intervall auf den Mindestwert anheben
      timelapseInterval = timelapseMinuimumInterval;
    }
  }
  // Dollymodus
  else{
    // Schrittweite des Motors festlegen
    timelapseStepsPerRev = 3200;
    // Anzahl der Rotationen berechnen
    float numberOfRotations = (314*float(timelapseDistance/100));
    // Sekunden pro Rotation berechnen
    float timePerRotation = float(timelapseDuration*1000000/numberOfRotations);
    // Geschwindigkeit des Motors berechnen
    timelapseDollyMotorSpeed = long(long(timePerRotation)/3200);
    printLCD(1, 0, String(timePerRotation), 0, false);
    printLCD(1, 1, String(timelapseDollyMotorSpeed), 0, false);
    printLCD(1, 2, String(numberOfRotations), 2000, false);
  }
}


// Status der Aufnahme anzeigen
void displayRecordingStatus(){
  String timestring = String(float(now())/60) + "min / " + String(float(timelapseDuration)/60) + "min";
  printLCD(1, 1, timestring, 0, false);
  String shotImagesString = String(String(shotImages) + " / " + String(timelapseImageCount) + " Bilder");
  printLCD(1, 2, shotImagesString, 0, false);
}


// Helper, um Text auf LCD auszugeben
void printLCD(int column, int row, String text, int sleeptime, bool newScreen){
  if(newScreen){
    lcd.clear();
  }
  lcd.setCursor(column, row);
  lcd.print(text);
  delay(sleeptime);
}


// LCD-Anzeige, Menüstatus und Menüsteuerung verwalten
void displayControl(){
  // Button Status einlesen
  upButtonState = digitalRead(upButtonPin);
  middleButtonState = digitalRead(middleButtonPin);
  downButtonState = digitalRead(downButtonPin);

  // UpButton Status hat sich geändert
  if(upButtonState != lastUpButtonState){
    // Wenn UpButton gedrückt wurde
    if(upButtonState == 1){
      switch(menuLayer){
        // Wenn Menü "Distanz" aktiv
        case 2:
          // Solange die Distanz positiv ist
          if(timelapseDistance < 100){
            timelapseDistance += 5;
          }
        // Wenn Menü "Framerate" aktiv
        case 4:
          // Solange der Index größer als 0 ist
          if(timelapseFrameratePointer < (int(sizeof(framerateOptions) / sizeof(framerateOptions[0])) - 1)){
            timelapseFrameratePointer += 1;
          }
        // Wenn Menü "Videolänge" aktiv
        case 6:
          // Schrittweite 1 unter 30 Sekunden
          if(timelapseLength < 30){
            timelapseLength += 1;
          }
          // Schrittweite 5 über 30 Sekunden
          else if(timelapseLength < 60){
            timelapseLength += 5;
          }
          // Schrittweite 10 über 60 Sekunden
          else if(timelapseLength < 120){
            timelapseLength += 10;
          }
        // Wenn Menü "Intervall" aktiv
        case 8:
          // Wenn Dollymodus ausgewählt, umschalten auf Mindestintervall
          if(timelapseInterval < timelapseMinuimumInterval){
            timelapseInterval = timelapseMinuimumInterval;
          }
          // Solange sich das Intervall unter einer Minute ist, 1s Schritte
          else if(timelapseInterval < 60){
            timelapseInterval += 1;
          }
          // Solange des Intervall kleiner als 2 Minuten ist, 5s Schritte
          else if(timelapseInterval < 120){
            timelapseInterval += 5;
          }
        // Wenn Dolly-Menü "Distanz" aktiv
        case 11:
          // Solange die Distanz positiv ist
          if(timelapseDistance < 100){
            timelapseDistance += 5;
          }
        case 13:
          // Schrittweite 5 unter 300 Sekunden
          if(timelapseDuration <= 300){
            timelapseDuration += 5;
          }
          // Schrittweite 10 über 600 Sekunden
          else if(timelapseDuration <= 600){
            timelapseDuration += 10;
          }
          // Schrittweite 15 über 600 und unter 1200 Sekunden
          else if(timelapseDuration <= 1200){
            timelapseDuration += 15;
          }
        // Normales Menü
        default:
          // Cursorposition neu berechnen
          switch(menuCursor){
            case 1:
              menuCursor = 3;
              break;
            case 2:
              menuCursor = 1;
              break;
            case 3:
              menuCursor = 2;
          }
       }
    }
    // LCD updaten
    LCDChanged = true;
  }

  // MiddleButton Status hat sich geändert
  if(middleButtonState != lastMiddleButtonState){
    // Wenn der Button gedrückt wurde
    if(middleButtonState == 1){
      // Wenn ein Link auf dem aktuellen Cursor ist
      if(menuLinks[menuLayer][menuCursor] > -1){
        // Navigation zu dem angegebenen Menülink
        menuLayer = menuLinks[menuLayer][menuCursor];
      }
      else{
        // TODO REMOVE Fehlermeldung anzeigen 
        printLCD(0, 0, String(menuLinks[menuLayer][menuCursor]), 1000, true);
      }
      // Menücursor auf Ausgangsposition setzen
      menuCursor = 1;
    }
    // Change Display
    LCDChanged = true;
  }
  
  // DownButton Status hat sich geändert
  if(downButtonState != lastDownButtonState){
    // Wenn der Button gedrückt wurde
    if(downButtonState == 1){
      switch(menuLayer){
        // Wenn Menü "Distanz" aktiv
        case 2:
          // Solange die Distanz positiv ist
          if(timelapseDistance > 10){
            timelapseDistance -= 5;
          }
        // Wenn Menü "Framerate" aktiv
        case 4:
          // Solange die Framerate positiv ist
          if(timelapseFrameratePointer > 0){
            timelapseFrameratePointer -= 1;
          }
        // Wenn Menü "Videolänge" aktiv
        case 6:
          // Wenn die Videolänge länger als eine Sekunde ist
          if(timelapseLength > 1){
            // Schrittweite 1 unter 30 Sekunden
            if(timelapseLength <= 30){
              timelapseLength -= 1;
            }
            // Schrittweite 5 über 30 Sekunden
            else if(timelapseLength <= 60){
              timelapseLength -= 5;
            }
            // Schrittweite 10 über 60 Sekunden
            else{
              timelapseLength -= 10;
            }
          }
        // Wenn Menü "Intervall" aktiv
        case 8:
          // Solange das Intervall über dem Mindestintervall liegt (>1s)
          if(timelapseInterval > timelapseMinuimumInterval){
            // Solange das Intervall über einer Minute ist
            if(timelapseInterval <= 60){
              timelapseInterval -= 1;
            }
            // Solange das Intervall unter einer Minute und positiv ist
            else{
              timelapseInterval -= 5;
            }
          }
        // Wenn Dolly-Menü "Distanz" aktiv
        case 11:
          // Solange die Distanz positiv ist
          if(timelapseDistance > 10){
            timelapseDistance -= 5;
          }
        case 13:
          if(timelapseDuration > 45){
            // Schrittweite 5 unter 300 Sekunden
            if(timelapseDuration <= 300){
              timelapseDuration -= 5;
            }
            // Schrittweite 10 über 600 Sekunden
            else if(timelapseDuration <= 600){
              timelapseDuration -= 10;
            }
            // Schrittweite 15 über 600 und unter 1200 Sekunden
            else if(timelapseDuration <= 1200){
              timelapseDuration -= 15;
            }
          }
        // Normales Menü
         default:
          // Cursorposition neu berechnen
          switch(menuCursor){
            case 1:
              menuCursor = 2;
              break;
            case 2:
              menuCursor = 3;
              break;
            case 3:
              menuCursor = 1;
          }
      }
    }
    // Change Display
    LCDChanged = true;
  }
  
  // Aktueller Button Status speichern
  lastUpButtonState = upButtonState;
  lastMiddleButtonState = middleButtonState;
  lastDownButtonState = downButtonState;

  // Wenn der LCD geändert werden soll
  if(LCDChanged){
    LCDChanged = false;
    
    // aktuelles Menü auf LCD ausgeben
    switch(menuLayer){
      case 0:
        printLCD(0, 0, "     Hauptmen" + String(char(245)) + "      ", 0, true);
        printLCD(1, 1, "Intervallmodus", 0, false);
        printLCD(1, 2, "Dollymodus", 0, false);
        printLCD(1, 3, "Ausgangsposition", 0, false);
        //
        break;
      case 1:
        printLCD(0, 0, " Konfiguration 1/5  ", 0, true);
        printLCD(1, 1, "Distanz einstellen", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      // Distanzeinstellungen mit statischem Cursor anzeigen
      case 2:
        menuCursor = 3;
        printLCD(0, 0, " Konfiguration 1/5  ", 0, true);
        printLCD(1, 1, "Distanz:         ", 0, false);
        printLCD(10, 1, String(timelapseDistance) + "%", 0, false);
        //
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 3:
        printLCD(0, 0, " Konfiguration 2/5  ", 0, true);
        printLCD(1, 1, "Framerate einstel.", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      // Framerateeinstellungen mit statischem Cursor anzeigen
      case 4:
        menuCursor = 3;
        printLCD(0, 0, " Konfiguration 2/5  ", 0, true);
        printLCD(1, 1, "Framerate:         ", 0, false);
        printLCD(12, 1, String(framerateOptions[int(timelapseFrameratePointer)]) + "fps", 0, false);
        // TODO Vorschau: Anzahl der Bilder
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 5:
        printLCD(0, 0, " Konfiguration 3/5  ", 0, true);
        printLCD(1, 1, "Videolaenge einst.", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      // Videolängeeinstellungen mit statischem Cursor anzeigen
      case 6:
        menuCursor = 3;
        printLCD(0, 0, " Konfiguration 3/5  ", 0, true);
        printLCD(1, 1, "Videol" + String(char(225)) + "nge:             ", 0, false);
        printLCD(14, 1, String(timelapseLength) + "s", 0, false);
        printLCD(1, 2, "Anz. Bilder: " + String(timelapseLength * int(framerateOptions[int(timelapseFrameratePointer)])), 0, false);
        //
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 7:
        printLCD(0, 0, " Konfiguration 4/5  ", 0, true);
        printLCD(1, 1, "Interval einstellen", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        // Berechnung zur Bestimmung des Mindestintervalls;
        calculateRecordingParameters();
        break;
      // Intervalleinstellungen mit statischem Cursor anzeigen
      case 8:
        menuCursor = 3;
        printLCD(0, 0, " Konfiguration 4/5  ", 0, true);
        printLCD(1, 1, "Intervall:         ", 0, false);
        printLCD(12, 1, (timelapseInterval > 0 ? String(timelapseInterval) + "s" : "0(Dolly)"), 0, false);
        // TODO AUFNAHMEZEIT ANZEIGEN
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 9:
        printLCD(0, 0, " Konfiguration 5/5  ", 0, true);
        printLCD(1, 1, "Aufnahme starten", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      case 10:
        printLCD(0, 0, " Konfiguration 1/2  ", 0, true);
        printLCD(1, 1, "Distanz einstellen", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        // timelapseInterval auf 0 setzen für Dolly-Modus und Duration auf 3 Minuten einstellen
        timelapseInterval = 0;
        timelapseDuration = 180;
        break;
      // Distanzeinstellungen mit statischem Cursor anzeigen
      case 11:
        menuCursor = 3;
        printLCD(0, 0, " Konfiguration 1/2  ", 0, true);
        printLCD(1, 1, "Distanz:         ", 0, false);
        printLCD(10, 1, String(timelapseDistance) + "%", 0, false);
        //
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 12:
        printLCD(0, 0, " Konfiguration 2/2  ", 0, true);
        printLCD(1, 1, "Dauer einstellen", 0, false);
        printLCD(1, 2, "Zur" + String(char(245)) + "ck", 0, false);
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      case 13:
        menuCursor = 3;
        printLCD(0, 0, "Konfiguration 2/2", 0, true);
        printLCD(1, 1, "Dauer:              ", 0, false);
        printLCD(10, 1, String(timelapseDuration) + "s", 0, false);
        //
        printLCD(1, 3, "Weiter", 0, false);
        break;
      case 14:
        printLCD(0, 0, "Initialisiere Slider", 0, true);
        //
        //
        printLCD(1, 3, "Abbrechen", 0, false);
        break;
      default:
        printLCD(0, 0, "Initialisiere Slider", 0, true);
        delay(1000);
        // Slider auf Startposition fahren
        moveSliderToStartPosition();
        printLCD(0, 0, "Kamera starten in...", 0, true);
        delay(750);
        printLCD(8, 2, "3...", 0, false);
        delay(1000);
        printLCD(8, 2, "2...", 0, false);
        delay(1000);
        printLCD(8, 2, "1...", 0, false);
        delay(1000);
        printLCD(8, 2, "Los!", 0, false);
        delay(1000);
        printLCD(0, 0, "  Aufnahme l" + String(char(225)) + "uft   ", 0, true);
        printLCD(1, 3, "Abbrechen (Halten)", 0, false);
    }

    // Aufnahmemodus aktiv
    if(menuLayer >= menuLength - 2){
      menuCursor = 3;
    }
    // Cursor auf LCD anzeigen
    printLCD(0, 1, " ", 0, false);
    printLCD(0, 2, " ", 0, false);
    printLCD(0, 3, " ", 0, false);
    printLCD(0, menuCursor, ">", 0, false);
  }
}
