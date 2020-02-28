# Project-TPDH-Walrus
TP-DownHole on a one-sided board for easier production and encapsulation

## Hardware

Input Voltage: 3.3 ~ 5V 

Logic Voltage: 0 ~ Vcc (Will operate at same logic level as power supply)

### Pinout

For I2C Mode:

| Pin | Wire | Name |
|-----|------|------|
|V+|Red|Vcc|		
|V-|Black|GND|
|SIG1|Brown|SDA|
|SIG2|Orange|SCL|

For RS-485 Mode:

| Pin | Wire | Name |
|-----|------|------|
|V+|Red|Vcc|		
|V-|Black|GND|
|SIG1|Brown|A|
|SIG2|Orange|B|