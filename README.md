# cardputer-adv-games

# Cardputer-Adv Games 🎮

A minimalist retro game collection for the **M5Stack Cardputer-Adv** (ESP32-S3).

## 🕹️ Included Games
* **Tetris** — classic block-stacking with DAS (Delayed Auto Shift) for smooth movement.
* **Snake** — the classic snake game with a polished UI.

## 🚀 Features
* Minimalistic grey/black UI design.
* Real-time battery level indicator (cached to avoid I2C spam).
* Optimized for the 240x135 display.
* Smooth DAS handling for Tetris.

## 🛠️ How to Build
1. Open the `.ino` file in **Arduino IDE** or **ArduinoDroid**.
2. Select **ESP32S3 Dev Module**.
3. Set **Flash Size: 8MB**, **PSRAM: Disabled**.
4. Install libraries: `M5Cardputer`, `M5Unified`, `M5GFX`.
5. Compile and upload!

## 🎮 Controls
* **Fn + ;** — Up
* **Fn + .** — Down
* **Fn + ,** — Left
* **Fn + /** — Right
* **Enter** — Select / Rotate (in Tetris)
* **Del** — Back to menu
