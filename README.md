# ESP32 Mini Game Console

A small game console running on ESP32 with OLED display and SD card support.  
Includes games: Snake, Pong, Rock Dodger.  

## Hardware Required

- ESP32 board
- SSD1306 128x64 OLED display
- Micro SD card module
- 4 push buttons (Up/Down/Select/Back)
- Wires, breadboard, USB cable

## Wiring

- OLED: SDA → 21, SCL → 22, VCC → 3.3V, GND → GND  
- SD: CS → 5, MOSI → 23, MISO → 19, SCK → 18, VCC → 5V, GND → GND  
- Buttons: BTN_LEFT → 25, BTN_RIGHT → 26, BTN_SELECT → 27, BTN_BACK → 14 (INPUT_PULLUP)

## Usage

1. Open `ESP32_Console.ino` in Arduino IDE  
2. Install libraries: `Adafruit GFX`, `Adafruit SSD1306`, `ArduinoJson`, `SD`  
3. Upload sketch to ESP32  
4. Make sure your SD card is formatted in FAT32 , Create a Folder and name it "games", Drag the .json` files to the "games" folder  
5. Insert SD card, power ESP32, use buttons to select games

## Adding Games

- I already put three games: snake.json, pong.json, rockdodger.json

