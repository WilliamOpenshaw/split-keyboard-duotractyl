#pragma once

#define VENDOR_ID       0x1209
#define PRODUCT_ID      0x5070
#define DEVICE_VER      0x0002
#define MANUFACTURER    "Cosmos"
#define PRODUCT         "DuoTractyl"

#define PMW33XX_CS_PIN GP10
#define PMW33XX_CPI 3200
#define ROTATIONAL_TRANSFORM_ANGLE 90
#define POINTING_DEVICE_INVERT_X_RIGHT
//#define POINTING_DEVICE_INVERT_X
//#define POINTING_DEVICE_INVERT_Y


// Make sure this is before the QMK include
//#define MOUSE_SHARED_EP 1

//#include QMK_KEYBOARD_H

// Split keyboard configuration
//#define EE_HANDS
#define MASTER_RIGHT
#define SPLIT_USB_DETECT

// Serial communication configuration
#define SERIAL_USART_FULL_DUPLEX
#define SERIAL_USART_TX_PIN GP0
#define SERIAL_USART_RX_PIN GP1
#define SERIAL_USART_SPEED 460800   // was 115200; 460800 is what versions 2.1-2.6 used

// Split transport features
#define SPLIT_POINTING_ENABLE
// Layer/mod sync removed: the left half has no OLED/RGB that needs them,
// so they only added extra serial traffic every scan.
//#define SPLIT_LAYER_STATE_ENABLE
//#define SPLIT_MODS_ENABLE

// Configure both pointing devices
#define POINTING_DEVICE_COMBINED    // Combine reports from both sides

// SPI configuration for PMW3360 sensors
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP18
#define SPI_MISO_PIN GP20
#define SPI_MOSI_PIN GP19

// PMW3360 sensor configuration for right side

// Pointing device settings
// 1 ms = QMK default (1000 Hz). Was 10 ms, which capped the trackballs at 100 Hz.
#define POINTING_DEVICE_TASK_THROTTLE_MS 1

// RP2040 bootloader configuration
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET
#define RP2040_BOOTLOADER_DOUBLE_TAP_RESET_LED GP17