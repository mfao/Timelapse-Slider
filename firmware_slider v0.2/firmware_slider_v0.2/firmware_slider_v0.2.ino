#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display
#define SCREEN_WIDTH 128        // OLED display width, in pixels
#define SCREEN_HEIGHT 64        // OLED display height, in pixels
#define OLED_RESET    -1        // -1 for sharing Arduino reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Buttons
const int upButtonPin = 13;
const int middleButtonPin = 12;
const int downButtonPin = 11;
int upButtonState = 0;
int middleButtonState = 0;
int downButtonState = 0;

// Motorverbindungen für A4988
const int dirPin = 4;           // Richtung
const int stepPin = 3;          // Schrittweite
const byte MS1 = 8; 
const byte MS2 = 9;
const byte MS3 = 10;
bool endSwitch = false;
#define homeSwitch 2

// Motorparameter
bool doRotate = false;          // Manual Modus: Befehl zum rotieren oder nicht
bool rotateDirection = false;   // Manual Modus: Rotationsrichtung
const int sliderLength = 314;   // Länge des Sliders in mm

// Motionlapse Parameter
// TODO Save to EEPROM and read from EEPROM
int mShot = 100;  // Bilder
int mDura = 60;   // min
int mIntv = 5;    // s

// Warplapse Parameter
// TODO Save to EEPROM and read from EEPROM
int wShot = 300;  // Bilder
int wDura = 30;   // min

// Dolly Parameter
// TODO Save to EEPROM and read from EEPROM
int dDura = 60;   // s

// Menü
int menuSleep = 140;
int cMenu = 0;
int cCursor = 0;
bool cActive = false;


////////////////////////////////////////////////////// DISPLAY //////////////////////////////////////////////////////

void splashScreen() {
  // Clear Screen
  display.clearDisplay();
  display.display();

  // Draw Splash Screen
  display.drawLine(10, 10, 118, 10, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(30, 20);
  display.println("Geila Slida");
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(50, 38);
  display.println("v1.0");
  display.drawLine(10, 54, 118, 54, SSD1306_WHITE);

  display.display();
  delay(1000);

  // Invert and restore display, pausing in-between
  /*
  int i = 0;
  while(i < 12){
    display.invertDisplay(true);
    delay(100);
    display.invertDisplay(false);
    delay(100);
    i++;
  }
  */
  
  // Clear the buffer
  display.clearDisplay();
  display.display();
}


void drawRectangle(int x, int y, int w, int h, bool color){
  if(color){
    display.fillRect(x, y, w, h, WHITE);
  }
  else{
    display.drawRect(x, y, w, h, WHITE);
  }
}


void drawMenu() {
  // Clear Screen
  display.clearDisplay();

  // Menu 0 - Startseite
  if(cMenu == 0){
    // Boxes
    int positions[][4] = {
      {2, 18, 28, 28}, {34, 18, 28, 28}, {66, 18, 28, 28}, {98, 18, 28, 28}
    };
    // Allocate Rectangles
    for(int i = 0; i < 4; i++){
      drawRectangle(positions[i][0], positions[i][1], positions[i][2], positions[i][3], cCursor == i);
    }
    
    // Texts
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(7, 2);
    display.println("- Timelapseslider -");
    display.setTextSize(2);
    cCursor == 0 ? display.setTextColor(BLACK) : display.setTextColor(WHITE);
    display.setCursor(11, 25);
    display.println("M");
    cCursor == 1 ? display.setTextColor(BLACK) : display.setTextColor(WHITE);
    display.setCursor(43, 25);
    display.println("W");
    cCursor == 2 ? display.setTextColor(BLACK) : display.setTextColor(WHITE);
    display.setCursor(75, 25);
    display.println("D");
    display.setTextSize(1);
    cCursor == 3 ? display.setTextColor(BLACK) : display.setTextColor(WHITE);
    display.setCursor(103, 26);
    display.println("...");
  }

  // Menu 2,3,4,10,11,17 - Rahmen für Aufnahmeparametereinstellungen
  if(cMenu >= 2 and cMenu <= 4 or cMenu == 10 or cMenu == 11 or cMenu == 17){
    // Boxes
    int positions[][4] = {
      {2, 18, 28, 28}, {98, 18, 28, 28}
    };
    // Allocate Rectangles
    for(int i = 0; i < 2; i++){
      drawRectangle(positions[i][0], positions[i][1], positions[i][2], positions[i][3], false);
    }
  }
  
  // Menu 1,5,6,7 Motionlapse Überschrift
  if(cMenu == 1 or cMenu >= 5 and cMenu <= 7){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(30, 2);
    display.println("Motionlapse");
  }
  // Menu 1 - Motionlapse
  if(cMenu == 1){
    // Texts
    display.setTextSize(1);
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Anzahl Bilder");    
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 24);
    display.println("Aufnahmedauer");
    cCursor == 2 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 36);
    display.println("Intervall");
    cCursor == 3 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Weiter");
  }
  // Menu 2,3,4 - Motionlapse - Aufnahmenparameter einstellen
  if(cMenu == 2 or cMenu == 3 or cMenu == 4){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(26, 0);
    if(cMenu == 2) display.println("Anzahl Bilder");
    if(cMenu == 3){
      display.setCursor(8, 0);
      display.println("Aufnahmedauer (min)");
    }
    if(cMenu == 4){
      display.setCursor(26, 0);
      display.println("Intervall (s)");
    }
    display.setTextSize(2);
    display.setCursor(11, 25);
    display.println("-");
    display.setCursor(45, 25);
    if(cMenu == 2) display.println(mShot);
    if(cMenu == 3) display.println(mDura);
    if(cMenu == 4) display.println(mIntv);
    display.setCursor(107, 25);
    display.println("+");
  }
  // Menu 5 - Motionlapse - Zusammenfassung
  if(cMenu == 5){
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Aufnahme starten");
    
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 24);
    display.println("Einstellungen");
    
    cCursor == 2 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Abbruch");
  }
  // Menu 7 - Motionlapse Aufnahmescreen
  if(cMenu == 7){
    display.setCursor(20, 24);
    display.println("Aufnahmescreen");
  }
  // Menu 6 - Motionlapse Aufnahme Ladescreen
  if(cMenu == 6){
    display.setCursor(8, 24);
    display.println("Aufnahme startet...");
    // Zu Aufnahmescreen wechseln
    cMenu = 7;
  }
  
  // Menu 9,12,13,14 Warplapse Überschrift
  if(cMenu == 9 or cMenu >= 12 and cMenu <= 14){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(36, 2);
    display.println("Warplapse");
  }
  // Menu 9 - Warplapse
  if(cMenu == 9){
    // Texts
    display.setTextSize(1);
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Anzahl Bilder");
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 24);
    display.println("Aufnahmedauer");
    cCursor == 2 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Weiter");
  }
  // Menu 10,11 - Warplapse - Aufnahmenparameter einstellen
  if(cMenu >= 10 and cMenu <= 11){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(26, 0);
    if(cMenu == 10) display.println("Anzahl Bilder");
    if(cMenu == 11){
      display.setCursor(8, 0);
      display.println("Aufnahmedauer (min)");
    }
    display.setTextSize(2);
    display.setCursor(11, 25);
    display.println("-");
    display.setCursor(45, 25);
    if(cMenu == 10) display.println(wShot);
    if(cMenu == 11) display.println(wDura);
    display.setCursor(107, 25);
    display.println("+");
  }
  // Menu 12 - Warplapse - Zusammenfassung
  if(cMenu == 12){
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Aufnahme starten");
    
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 24);
    display.println("Einstellungen");
    
    cCursor == 2 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Abbruch");
  }
  // Menu 14 - Warplapse Aufnahmescreen
  if(cMenu == 14){
    display.setCursor(20, 24);
    display.println("Aufnahmescreen");
  }
  // Menu 13 - Warplapse Aufnahme Ladescreen
  if(cMenu == 13){
    display.setCursor(8, 24);
    display.println("Aufnahme startet...");
    // Zu Aufnahmescreen wechseln
    cMenu = 14;
  }

  // Menu 16,18,19,20 Dolly Überschrift
  if(cMenu == 16 or cMenu >= 18 and cMenu <= 20){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(45, 2);
    display.println("Dolly");
  }
  // Menu 16 - Dolly
  if(cMenu == 16){
    // Texts
    display.setTextSize(1);
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Aufnahmedauer");
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Weiter");
  }
  // Menu 17 - Dolly - Aufnahmenparameter einstellen
  if(cMenu == 17){
    display.setTextColor(WHITE, BLACK);
    display.setTextSize(1);
    display.setCursor(14, 0);
    if(cMenu == 17) display.println("Aufnahmedauer (s)");
    display.setTextSize(2);
    display.setCursor(11, 25);
    display.println("-");
    display.setCursor(45, 25);
    if(cMenu == 17) display.println(dDura);
    display.setCursor(107, 25);
    display.println("+");
  }
  // Menu 18 - Dolly - Zusammenfassung
  if(cMenu == 18){
    cCursor == 0 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 12);
    display.println("Aufnahme starten");
    
    cCursor == 1 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 24);
    display.println("Einstellungen");
    
    cCursor == 2 ? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    display.setCursor(4, 48);
    display.println("Abbruch");
  }  
  // Menu 20 - Dolly Aufnahmescreen
  if(cMenu == 20){
    display.setCursor(20, 24);
    display.println("Aufnahmescreen");
  }
  // Menu 19 - Dolly Aufnahme Ladescreen
  if(cMenu == 19){
    display.setCursor(8, 24);
    display.println("Aufnahme startet...");
    // Zu Aufnahmescreen wechseln
    cMenu = 20;
  }
  
  // Menu 22 - Einstellungen
  if(cMenu == 22){
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(24, 2);
    display.println("Einstellungen");
  }

  // Menü ausgeben
  display.display();

  // Klick-Highlight Effekt
  if(cActive){
    cActive = false;
    delay(50);
    drawMenu();
  }
}


////////////////////////////////////////////////////// BUTTON /////////////////////////////////////////////////////

int menuButton(){
  // Pins initialisieren
  int upState = digitalRead(upButtonPin);
  int middleState = digitalRead(middleButtonPin);
  int downState = digitalRead(downButtonPin);
  int pressedBtn = 0;
  menuSleep = 120;

  // Pins lesen
  if(upState == HIGH) pressedBtn = 1;
  if(middleState == HIGH) pressedBtn = 2;
  if(downState == HIGH) pressedBtn = 3;

  // Cursor neu berechnen
  // Auswahlmenüs
  // Hauptmenü und Motionlapse Aufnahmeeinstellungen
  if(cMenu == 0 or cMenu == 1){
    if(pressedBtn == 1) cCursor < 3 ? cCursor += 1 : cCursor;
    if(pressedBtn == 3) cCursor > 0 ? cCursor -= 1 : cCursor;
  }
  // Motionlapse Zusammenfassung, Warplapse Aufnahmeeinstellungen und Zusammenfassung, Dolly Zusammenfassung
  if(cMenu == 5 or cMenu == 9 or cMenu == 12 or cMenu == 18){
    if(pressedBtn == 1) cCursor < 2 ? cCursor += 1 : cCursor;
    if(pressedBtn == 3) cCursor > 0 ? cCursor -= 1 : cCursor;
  }
  // Dolly Aufnahmeeinstellungen
  if(cMenu == 16){
    if(pressedBtn == 1) cCursor < 1 ? cCursor += 1 : cCursor;
    if(pressedBtn == 3) cCursor > 0 ? cCursor -= 1 : cCursor;
  }

  // Eingabemenüs
  // Motionlapse - Anzahl der Aufnahmen (20 - 2000)
  if(cMenu == 2){
    if(pressedBtn == 1) mShot < 2000 ? mShot += 1 : mShot;
    if(pressedBtn == 3) mShot > 20 ? mShot -= 1 : mShot;
  }
  // Motionlapse - Aufnahmedauer (10min - 10h)
  if(cMenu == 3){
    if(pressedBtn == 1) mDura < 600 ? mDura += 1 : mDura;
    if(pressedBtn == 3) mDura > 10 ? mDura -= 1 : mDura;
  }
  // Motionlapse - Intervall (3s - 5min)
  if(cMenu == 4){
    if(pressedBtn == 1) mIntv < 300 ? mIntv += 1 : mIntv;
    if(pressedBtn == 3) mIntv > 3 ? mIntv -= 1 : mIntv;
  }
  // Warplapse - Anzahl der Aufnahmen (30 - 1200)
  if(cMenu == 10){
    if(pressedBtn == 1) wShot < 1200 ? wShot += 1 : wShot;
    if(pressedBtn == 3) wShot > 30 ? wShot -= 1 : wShot;
  }
  // Warplapse - Aufnahmedauer (10min - 2h)
  if(cMenu == 11){
    if(pressedBtn == 1) wDura < 120 ? wDura += 1 : wDura;
    if(pressedBtn == 3) wDura > 10 ? wDura -= 1 : wDura;
  }
  // Dolly - Aufnahmedauer (20s - 10min)
  if(cMenu == 17){
    if(pressedBtn == 1) dDura < 600 ? dDura += 1 : dDura;
    if(pressedBtn == 3) dDura > 20 ? dDura -= 1 : dDura;
  }

  // Schnellere Scrollgeschwindigkeit beim Einstellen von Werte
  if(cMenu >= 2 and cMenu <= 4 or cMenu == 10 or cMenu == 11 or cMenu == 17) pressedBtn == 2 ? menuSleep = 140 : menuSleep = 15;

  // Menuwechsel berechnen
  if(pressedBtn == 2){
    switch(cMenu){
      // Startseite
      case 0:
        if(cCursor == 0) cMenu = 1;   // Motionlapse
        if(cCursor == 1) cMenu = 9;   // Warplapse
        if(cCursor == 2) cMenu = 16;  // Dolly
        if(cCursor == 3) cMenu = 22;  // Settings
        cCursor = 0;
        break;

      // Motionlapse - Einstellungen
      case 1:
        if(cCursor == 0) cMenu = 2;
        if(cCursor == 1) cMenu = 3;
        if(cCursor == 2) cMenu = 4;
        if(cCursor == 3) cMenu = 5;
        cCursor = 0;
        break;
      // Motionlapse - Aufnahmeparameter einstellen
      case 2:
      case 3:
      case 4:
        cCursor = cMenu - 2;
        cMenu = 1;
        break;
      // Motionlapse - Zusammenfassung
      case 5:
        if(cCursor == 0){
          cMenu = 6;                // Aufnahmescreen Startscreen
          initMotionlapse();
        }
        if(cCursor == 1) cMenu = 1; // Motionlapseeinstellungen
        if(cCursor == 2) cMenu = 0; // Startseite
        cCursor = 0;
        break;
      // Motionlapse - Ladescreen
      case 6:
        break;
      
      // Warplapse - Einstellungen
      case 9:
        if(cCursor == 0) cMenu = 10;
        if(cCursor == 1) cMenu = 11;
        if(cCursor == 2) cMenu = 12;
        cCursor = 0;
        break;
      // Warplapse - Aufnahmeparameter einstellen
      case 10:
      case 11:
        cCursor = cMenu - 10;
        cMenu = 9;
        break;
      // Warplapse - Zusammenfassung
      case 12:
        if(cCursor == 0){
          cMenu = 13;               // Aufnahmescreen Startscreen
          cCursor = 0;
          initWarplapse();
        }
        if(cCursor == 1){
          cMenu = 9;                // Warplapseeinstellungen
          cCursor = 0;
        }
        if(cCursor == 2){
          cMenu = 0;                // Startseite
          cCursor = 1;
        }
        break;
      // Warplapse - Ladescreen
      case 13:
        break;

      // Dolly - Einstellungen
      case 16:
        if(cCursor == 0) cMenu = 17;
        if(cCursor == 1) cMenu = 18;
        cCursor = 0;
        break;
      // Dolly - Aufnahmeparameter einstellen
      case 17:
        cCursor = cMenu - 17;
        cMenu = 16;
        break;
      // Dolly - Zusammenfassung
      case 18:
        if(cCursor == 0){
          cMenu = 19;                 // Aufnahmescreen Startscreen
          cCursor = 0;
          initDolly();
        }
        if(cCursor == 1){
          cMenu = 16;                 // Dollyeinstellungen
          cCursor = 0;
        }
        if(cCursor == 2){
          cMenu = 0;                  // Startseite
          cCursor = 2;
        }
        break;
      // Dolly - Ladescreen
      case 19:
        break;

      // Einstellungen
      case 22:

      default:
        // TODO Longpress to stop everything
        cMenu = 0;
        cCursor = 0;
    }
  }

  // Menu zeichnen
  if(pressedBtn > 0 or cMenu == 7 or cMenu == 14 or cMenu == 20){
    drawMenu();
    delay(menuSleep);
  }
}



////////////////////////////////////////////////////// MOTOR //////////////////////////////////////////////////////

void initMotionlapse(){
  delay(85);
}

void initWarplapse(){
  delay(85);
}

void initDolly(){
  delay(85);
}


void rotateEngine(bool direction, int steps) {
  // Motorrichtung festlegen
  digitalWrite(dirPin,direction);
  // Motor langsam eine Umdrehung fahren lassen
  for(int x = 0; x < steps ; x++) {
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(42);
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(42);
  }
}

/*
void manualMode() {
  if(digitalRead(upButtonPin) == 1){
    doRotate = true;
    rotateDirection = true;
  }
  else if(digitalRead(downButtonPin) == 1){
    doRotate = true;
    rotateDirection = false;
  }
  else {
    doRotate = false;
  }

  if(doRotate){
    rotateEngine(rotateDirection, 100);
  }
}

void timelapseMode() {
  int timelapseImageCount = 100;
  int timelapseDistance = 100;
  long steps = long(3200*long(sliderLength*(timelapseDistance/100)))/long(timelapseImageCount);
  int i = 1;
  while(i <= timelapseImageCount){
    rotateEngine(false, steps);
    splashScreen("" + i);
    i = i + 1;
  }
}

*/
////////////////////////////////////////////////////// HAUPTPROGRAMM //////////////////////////////////////////////////////

void setup() {
  // Buttons als Input initialisieren
  pinMode(upButtonPin, INPUT);
  pinMode(middleButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);

  // Motorparameter konfigurieren
  Serial.begin(9600);
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(MS1,OUTPUT);
  pinMode(MS2,OUTPUT);
  pinMode(MS3,OUTPUT);
  pinMode(homeSwitch, INPUT);
  digitalWrite(MS1,HIGH);       // Definiert die Schrittweite
  digitalWrite(MS2,HIGH);       // 
  digitalWrite(MS3,HIGH);       // 3x HIGH = 0.1125°

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  display.display();
  
  // Show Welcome Screen
  splashScreen();

  // Initiales Menu zeichen
  drawMenu();
}


void loop() {
  menuButton();
  delay(50);
}
