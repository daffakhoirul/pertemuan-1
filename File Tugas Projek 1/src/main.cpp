/**
 * @file main.cpp
 * @brief GPIO Project: Keypad PIN Lock dengan LED
 * 
 * Bahan: Keypad 4x4, 3 LED (Merah, Kuning, Hijau), Resistor
 * 
 * Cara kerja:
 * - Masukkan PIN 4 digit via keypad (default: 1234)
 * - LED Kuning kedip saat mengetik
 * - PIN benar  -> LED Hijau nyala 3 detik
 * - PIN salah  -> LED Merah kedip cepat 3 detik
 * - Tekan '#' untuk confirm, '*' untuk reset input
 * - 3x salah  -> lockout 10 detik (LED Merah nyala terus)
 */

#include <Arduino.h>

// ==================== PIN LED ====================
#define PIN_GREEN   PA0   // LED Hijau (akses diterima)
#define PIN_RED     PA1   // LED Merah (akses ditolak)
#define PIN_YELLOW  PA2   // LED Kuning (sedang input)

// ==================== KEYPAD 4x4 ====================
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// Pin ROW (output) dan COL (input pull-up)
byte rowPins[ROWS] = {PB12, PB13, PB14, PB15};
byte colPins[COLS] = {PA4, PA5, PA6, PA7};

// ==================== KONFIGURASI ====================
const char* CORRECT_PIN = "1234";
const int MAX_PIN_LENGTH = 4;
const int MAX_ATTEMPTS = 3;
const unsigned long LOCKOUT_TIME = 10000;  // 10 detik

// ==================== VARIABEL ====================
char inputPIN[5] = "";        // Buffer input (4 digit + null)
int inputIndex = 0;
int failCount = 0;
bool locked = false;
unsigned long lockStartTime = 0;
unsigned long lastKeyTime = 0;
char lastKey = 0;

// ==================== Scan Keypad (tanpa library) ====================
char scanKeypad() {
    for (byte r = 0; r < ROWS; r++) {
        // Set semua ROW HIGH
        for (byte i = 0; i < ROWS; i++)
            digitalWrite(rowPins[i], HIGH);
        
        // Set ROW aktif LOW
        digitalWrite(rowPins[r], LOW);
        delayMicroseconds(10);

        for (byte c = 0; c < COLS; c++) {
            if (digitalRead(colPins[c]) == LOW) {
                // Debounce
                delay(20);
                if (digitalRead(colPins[c]) == LOW) {
                    // Tunggu tombol dilepas
                    while (digitalRead(colPins[c]) == LOW);
                    return keys[r][c];
                }
            }
        }
    }
    return 0;  // Tidak ada tombol ditekan
}

// ==================== LED Functions ====================
void allLedOff() {
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_YELLOW, LOW);
}

void blinkLed(int pin, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(pin, HIGH);
        delay(delayMs);
        digitalWrite(pin, LOW);
        delay(delayMs);
    }
}

void resetInput() {
    memset(inputPIN, 0, sizeof(inputPIN));
    inputIndex = 0;
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  GPIO Project: Keypad PIN Lock + LED");
    Serial.println("========================================");
    Serial.println("PIN default: 1234");
    Serial.println("'#' = Confirm | '*' = Reset\n");

    // Setup LED
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    allLedOff();

    // Setup Keypad ROW sebagai OUTPUT
    for (byte r = 0; r < ROWS; r++) {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }

    // Setup Keypad COL sebagai INPUT_PULLUP
    for (byte c = 0; c < COLS; c++) {
        pinMode(colPins[c], INPUT_PULLUP);
    }

    // Startup animation
    blinkLed(PIN_YELLOW, 3, 150);
    Serial.println(">> Masukkan PIN:");
}

// ==================== LOOP ====================
void loop() {
    // === Cek lockout ===
    if (locked) {
        unsigned long elapsed = millis() - lockStartTime;
        if (elapsed < LOCKOUT_TIME) {
            // LED Merah nyala terus selama lockout
            digitalWrite(PIN_RED, HIGH);
            int sisa = (LOCKOUT_TIME - elapsed) / 1000;
            Serial.printf("\r   LOCKED! Tunggu %d detik...  ", sisa);
            delay(500);
            return;
        } else {
            // Lockout selesai
            locked = false;
            failCount = 0;
            allLedOff();
            resetInput();
            Serial.println("\n\n>> Lockout selesai. Masukkan PIN:");
        }
    }

    // === Scan keypad ===
    char key = scanKeypad();
    if (key == 0) return;  // Tidak ada tombol

    // === Proses tombol ===
    if (key == '*') {
        // Reset input
        resetInput();
        allLedOff();
        Serial.println("   [RESET] Input direset");
        Serial.println(">> Masukkan PIN:");
    }
    else if (key == '#') {
        // Confirm PIN
        Serial.printf("   PIN dimasukkan: %s\n", inputPIN);
        
        if (strcmp(inputPIN, CORRECT_PIN) == 0) {
            // === PIN BENAR ===
            Serial.println("   >> AKSES DITERIMA! <<");
            failCount = 0;
            allLedOff();
            digitalWrite(PIN_GREEN, HIGH);
            delay(3000);
            digitalWrite(PIN_GREEN, LOW);
        } else {
            // === PIN SALAH ===
            failCount++;
            Serial.printf("   >> AKSES DITOLAK! (Percobaan %d/%d)\n", failCount, MAX_ATTEMPTS);
            allLedOff();
            blinkLed(PIN_RED, 6, 150);

            if (failCount >= MAX_ATTEMPTS) {
                // Lockout
                locked = true;
                lockStartTime = millis();
                Serial.printf("\n!! %dx SALAH - LOCKOUT %d DETIK !!\n", MAX_ATTEMPTS, (int)(LOCKOUT_TIME/1000));
            }
        }
        resetInput();
        if (!locked) {
            Serial.println(">> Masukkan PIN:");
        }
    }
    else if (key >= '0' && key <= '9') {
        // Input digit
        if (inputIndex < MAX_PIN_LENGTH) {
            inputPIN[inputIndex] = key;
            inputIndex++;
            inputPIN[inputIndex] = '\0';

            // LED Kuning kedip sekali
            digitalWrite(PIN_YELLOW, HIGH);
            delay(80);
            digitalWrite(PIN_YELLOW, LOW);

            // Tampilkan asterisk
            Serial.print("   ");
            for (int i = 0; i < inputIndex; i++) Serial.print("*");
            Serial.printf(" (%d/%d)\n", inputIndex, MAX_PIN_LENGTH);
        } else {
            Serial.println("   [MAX] Tekan '#' untuk confirm");
            blinkLed(PIN_YELLOW, 2, 100);
        }
    }
    else {
        // Tombol A/B/C/D diabaikan
        Serial.printf("   [%c] Tombol tidak digunakan\n", key);
    }
}

/**
 * WIRING:
 * 
 * LED (dengan resistor 220-330 ohm):
 *   PA0 → Resistor 220Ω → LED Hijau → GND
 *   PA1 → Resistor 220Ω → LED Merah → GND
 *   PA2 → Resistor 220Ω → LED Kuning → GND
 * 
 * Keypad 4x4:
 *   ROW: PB12, PB13, PB14, PB15 (output)
 *   COL: PA4, PA5, PA6, PA7 (input pull-up)
 */
