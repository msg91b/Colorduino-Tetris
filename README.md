# Colorduino-Tetris

Based upon the instructable located at: http://www.instructables.com/id/Arduino-based-Bi-color-LED-Matrix-Tetris-Game

This version has been motified to work with the Colorduino RGB LED Driver Modules. All tetris pieces retain their color when piled up and the data is sent to the Colorduinos via their i2c address.

# Instructions

This code was written and tested on Arduino IDE 1.0.1 and used the Arduino Uno.

1. Upload the colorduino_i2c_firmware to each Colorduino specifying the I2C address on line 22. Each device needs it's own I2C address.
2. Upload the colorduino_tetris code to the Arduino.
3. Connect the wires accordingly: 
        Arduino ->  Colorduino \n
        A4      ->  SDA
        A5      ->  SCL
        RESET   ->  DTR
        5V      ->  VCC
        GND     ->  GND

  Momentary Switches were connected to pins D4 through D7 and ground.
  
  A Speaker is connected from D9 to a 100 Ohm resistor to the positive terminal on the speaker, and the negative terminal is connected to ground.
