## GSM module
SIM800L VDD ↔ Arduino 5v
SIM800L GND (UART TTL) ↔ Arduino GND
SIM800L SIM_TXD ↔ Arduino D2 (read through for the reason)
SIM800L SIM_RXD ↔ Arduino D3 (read through for the reason)
SIM800L 5VIN (POWER) ↔ External Positive Power Supply pin
SIM800L GND (POWER) ↔ External Negative Power Supply pin

The SIM800L LED indicator will blink once every 2 or 3 seconds when it has completely registered your Sim to a network, when the LED indicator is blinking every second once, this means that the SIM800L is still searching for a network to register onto. When the LED indicator does not blink, your power supply do provide low current even if you calibrated well the voltage


## SHTx 
Red - 3-5v
Yellow - Clock
Blue - Data
Black/Green - Gnd
