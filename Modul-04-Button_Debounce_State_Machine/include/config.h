/**
 * @file config.h
 * @brief Konfigurasi - Program 04 Button Debounce
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== PIN DEFINITIONS ====================
#define BUTTON_PIN      0       // GPIO0 - Boot button on DevKitC
#define LED_PIN         2       // GPIO2 - LED indicator

// ==================== DEBOUNCE CONFIG ====================
#define DEBOUNCE_MS     50      // Debounce time in milliseconds
#define LONG_PRESS_MS   1000    // Long press threshold

// ==================== SERIAL ====================
#define SERIAL_BAUD     115200

#endif // CONFIG_H
