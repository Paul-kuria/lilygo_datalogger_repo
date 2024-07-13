# Lilygo IoT repository
Repository to collect weather data using ESP32, Air Temp/Humidity sensor, Soil Temp/Humidity sensor etc.. 
Send collected parameters to database via RestAPI.

Frameworks Core Tech: Python, C++, C

Backend Service: FastAPI

Database: MongoDB

Documentation: Swagger

## Hardware Required
- ESP32
- DHT22 air temperature / humidity sensor
- SHTx soil sensor
- RTC
- SIM800L with antenna
- Battery
- On/Off switch + Relay
- Solar panel


## How the project works
The DHT22 and SHTx sensors are interfaced with the ESP32 board, which reads the temp/humidity data from the air & soil. The ESP32 board records these parameters, and sends this data to the database chosen. The data collection interval will be between 5 - 20 minutes, depending on power management and variations in weather parameters. 

