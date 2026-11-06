# MCU and Bootloader
MCU = RP2040
BOOTLOADER = rp2040

# Fix for the "Invalid Keyboard" / Serial issue
SERIAL_DRIVER = vendor

# Split Keyboard Support
SPLIT_KEYBOARD = yes

# Pointing Device (Trackball) Support
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = pmw3360
MOUSEKEY_ENABLE = yes

# Features enabled based on your keymap usage
EXTRAKEY_ENABLE = yes    # Audio control and System control
CONSOLE_ENABLE = yes     # For debug prints seen in your keymap
COMMAND_ENABLE = no      # Commands are usually disabled on RP2040 to save space
NKRO_ENABLE = yes        # N-Key Rollover

# Enable VIA
VIA_ENABLE = yes