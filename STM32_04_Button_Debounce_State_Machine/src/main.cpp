/**
 * @file main.cpp
 * @brief Button Debounce dengan State Machine - Pull-Up & Pull-Down
 * 
 * Bahan: 2 Push Button, 1 LED, Resistor 10kΩ (pull-down)
 * 
 * Cara kerja:
 * - BTN1 (PB0): Pull-Up   → idle HIGH, ditekan LOW  → toggle LED
 * - BTN2 (PB1): Pull-Down → idle LOW,  ditekan HIGH → toggle LED
 * - Masing-masing tombol punya state machine sendiri untuk debounce
 * - Debounce 50ms non-blocking (millis)
 * 
 * FSM per tombol:
 *   IDLE → DEBOUNCE_PRESS → PRESSED → DEBOUNCE_RELEASE → IDLE
 */

#include <Arduino.h>

// ==================== PIN KONFIGURASI ====================
#define BTN_PULLUP    PB0   // Tombol pull-up  (active LOW)
#define BTN_PULLDOWN  PB1   // Tombol pull-down (active HIGH)
#define PIN_LED       PA0   // LED output

// ==================== DEBOUNCE CONFIG ====================
const unsigned long DEBOUNCE_MS = 50;

// ==================== STATE MACHINE ====================
enum BtnState {
    BTN_IDLE,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE
};

const char* stateNames[] = {
    "IDLE", "DEB_PRESS", "PRESSED", "DEB_RELEASE"
};

// Struct untuk data tiap tombol
struct ButtonFSM {
    uint8_t pin;
    bool activeLow;          // true = pull-up (active LOW), false = pull-down (active HIGH)
    const char* label;
    BtnState state;
    unsigned long debStart;
};

// Dua instance FSM
ButtonFSM btnPU = { BTN_PULLUP,   true,  "PullUp  (PB0)", BTN_IDLE, 0 };
ButtonFSM btnPD = { BTN_PULLDOWN, false, "PullDown(PB1)", BTN_IDLE, 0 };

// ==================== VARIABEL ====================
bool ledState = false;
unsigned int pressCount = 0;

// ==================== FUNGSI ====================
bool isPressed(ButtonFSM &b) {
    // Pull-up: ditekan = LOW | Pull-down: ditekan = HIGH
    int val = digitalRead(b.pin);
    return b.activeLow ? (val == LOW) : (val == HIGH);
}

void toggleLed(const char* source) {
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState);
    pressCount++;
    Serial.print("  >> LED ");
    Serial.print(ledState ? "ON " : "OFF");
    Serial.print("  oleh ");
    Serial.print(source);
    Serial.print("  (total: ");
    Serial.print(pressCount);
    Serial.println(")");
}

void processButton(ButtonFSM &b) {
    bool pressed = isPressed(b);

    BtnState prev = b.state;

    switch (b.state) {

        case BTN_IDLE:
            if (pressed) {
                b.debStart = millis();
                b.state = BTN_DEBOUNCE_PRESS;
            }
            break;

        case BTN_DEBOUNCE_PRESS:
            if (!pressed) {
                b.state = BTN_IDLE;          // bounce
            } else if (millis() - b.debStart >= DEBOUNCE_MS) {
                b.state = BTN_PRESSED;
                toggleLed(b.label);          // confirmed press → toggle LED
            }
            break;

        case BTN_PRESSED:
            if (!pressed) {
                b.debStart = millis();
                b.state = BTN_DEBOUNCE_RELEASE;
            }
            break;

        case BTN_DEBOUNCE_RELEASE:
            if (pressed) {
                b.state = BTN_PRESSED;       // bounce
            } else if (millis() - b.debStart >= DEBOUNCE_MS) {
                b.state = BTN_IDLE;          // confirmed release
            }
            break;
    }

    // Print transisi state jika berubah
    if (b.state != prev) {
        Serial.print("  [");
        Serial.print(b.label);
        Serial.print("] ");
        Serial.print(stateNames[prev]);
        Serial.print(" -> ");
        Serial.println(stateNames[b.state]);
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n============================================");
    Serial.println("  Button Debounce State Machine");
    Serial.println("  Pull-Up & Pull-Down Demo");
    Serial.println("============================================");
    Serial.println("  BTN1 (PB0) : INPUT_PULLUP   → active LOW");
    Serial.println("  BTN2 (PB1) : INPUT (pull-down eksternal) → active HIGH");
    Serial.println("  LED  (PA0) : Toggle setiap press");
    Serial.print("  Debounce   : ");
    Serial.print(DEBOUNCE_MS);
    Serial.println(" ms");
    Serial.println("============================================\n");

    // Pull-up: pakai internal pull-up
    pinMode(BTN_PULLUP, INPUT_PULLUP);

    // Pull-down: pakai resistor eksternal 10kΩ ke GND
    pinMode(BTN_PULLDOWN, INPUT);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    Serial.println(">> Tekan salah satu tombol untuk toggle LED...\n");
}

// ==================== LOOP ====================
void loop() {
    processButton(btnPU);   // Proses tombol pull-up
    processButton(btnPD);   // Proses tombol pull-down
}

/**
 * WIRING:
 * 
 * BTN1 - Pull-Up (internal):
 *   PB0 ── Push Button ── GND
 *   (internal pull-up aktif, idle = HIGH, ditekan = LOW)
 *
 * BTN2 - Pull-Down (eksternal):
 *   PB1 ── Push Button ── 3.3V
 *   PB1 ── Resistor 10kΩ ── GND
 *   (resistor pull-down, idle = LOW, ditekan = HIGH)
 *
 * LED:
 *   PA0 → Resistor 220Ω → LED → GND
 * 
 * SKEMA:
 *
 *   Pull-Up (BTN1):          Pull-Down (BTN2):
 *
 *     3.3V                     3.3V
 *      │                        │
 *     [R] internal             [BTN]
 *      │                        │
 *     PB0───[BTN]──GND        PB1───[10kΩ]──GND
 *
 *   idle=HIGH, press=LOW      idle=LOW, press=HIGH
 */
