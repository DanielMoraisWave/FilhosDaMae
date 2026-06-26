# Filhos da Mãe — Matraquilhos Scoreboard

Smart foosball table scoreboard built on an **ESP32-C3 Mini**.  
Tracks goals via LDR sensors, displays the score on a 16×2 LCD, plays audio through a DFRobot DF1201S MP3 module, and serves a live WebSocket scoreboard to any phone connected to its WiFi.

---

## Features

- Live scoreboard accessible from any phone — no internet required
- 16×2 LCD display on the table
- Audio feedback via DFRobot DF1201S (goal sounds, win jingles, negra, idle ambient)
- Negra mode at 3–3 (golden goal)
- Auto-reset after a team wins (4 goals)
- Reset button on the web scoreboard

---

## Hardware

| Component | Details |
|---|---|
| MCU | ESP32-C3 Mini |
| Goal sensors | 2× LDR on GPIO0 and GPIO1 |
| Display | 16×2 I2C LCD (address `0x20`) |
| Audio | DFRobot DF1201S — RX: GPIO20, TX: GPIO21 |

---

## Software Dependencies

Install these via **Arduino IDE → Library Manager**:

| Library | Author |
|---|---|
| AsyncTCP | mathieucarbou |
| ESPAsyncWebServer | mathieucarbou |
| LiquidCrystal I2C | Frank de Brabander |
| DFRobot_DF1201S | DFRobot |

---

## Audio File Mapping (DF1201S)

| File # | Event |
|---|---|
| 1 | Sporting goal |
| 2 | Benfica goal |
| 3 | Startup jingle |
| 4 | Shared goal (alt) |
| 5 | Benfica wins |
| 6 | Sporting wins |
| 7 | Idle ambient |
| 8 | Shared goal (rare) |
| 9 | Negra (3–3) |

---

## Setup

1. Set `AP_PASS` in `MCodeFDMLite.ino` to your desired WiFi password
2. In Arduino IDE set the board to **ESP32C3 Dev Module** with partition scheme **No OTA (2MB APP/2MB SPIFFS)**
3. Upload the sketch
4. Upload the web files in `data/` using the **ESP32 LittleFS Data Upload** tool
5. Connect a phone to the **Punhada** WiFi and open `http://192.168.4.1`
