#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define I2C_ADDRESS 0x3C

SSD1306AsciiWire oled;

void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
}

void loop() 
{
  oled.setFont(System5x7); // Auswahl der Schriftart
  oled.clear(); //Löschen der aktuellen Displayanzeige
  oled.println("Viel"); //Text in der ersten Zeile. "Println" sorgt dabei für einen Zeilensprung.
  oled.print("Erfolg!!!"); // Text in der zweiten Zeile. Da es keine dritte Zeile gibt, wird hier kein Zeilenumsprung benötigt.
  delay (2000);
}
