
#include "setup_pins.h"
#include "secrets.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include "DHTesp.h"
#include <SensirionI2cSht3x.h>
#include <WiFi.h>
#include "scan_i2c.h"

// # Start The Code
#define TINY_GSM_DEBUG SerialMon 
#define GSM_PIN ""

// Set your APN Details / GPRS credentials
const char apn[]      = SAFAPN;
const char gprsUser[] = GPRSUSER;
const char gprsPass[] = GPRSPASS;
const char server[] = "api.thingspeak.com"; // domain name: example.com, maker.ifttt.com, etc
const int  port = 80;                             // server port number

String apiKey = THINGSPEAK_API_KEY3;

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

DHTesp dht;

SensirionI2cSht3x sensor;
static char errorMessage[64];
static int16_t error; 

#define SHT31_I2C_ADDR_44 0x44


#define SRC_TINYGSMTIME_H_
#include "TinyGsmCommon.h"
#define TINY_GSM_MODEM_HAS_TIME
// enum TinyGSMDateTimeFormat { DATE_FULL = 0, DATE_TIME = 1, DATE_DATE = 2 };
RTC_DATA_ATTR int bootCount = 0; 

void ledPinStart()
{
  //SET LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

bool setPowerBoostKeepOn(int en){
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  // I2CPower.beginTransmission(IP5306_ADDR);
  // I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}

String urlEncode(const char* msg) {
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg) {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9')) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

struct SensorData
{
  String SensorName;
  String macAddress;
  String gsm_rssi;
  String batt_level;
  // DHT 22 air temp, humidity
  String airTempInCAverage;
  String airHumidityAverage;

  // SHT31 soil temp, humidity
  String soilTempInCAverage;
  String soilHumidityAverage;
};


SensorData deviceData()
{
  SensorData data;

  // Sensor name
  data.SensorName = "Lilygo-TCall v1.4 ESP32 board";
  // RSSI signal strenth
  int csq = modem.getSignalQuality();
  data.gsm_rssi = String(csq);

  
  // macAddress
  String macAddress = String(WiFi.macAddress()); 
  data.macAddress = macAddress;

  // Voltage
  float voltage = 3.75;
  data.batt_level = String(voltage);

  // DHT22 - Air parameters
  float temperatureSum = 0.0f;
  float humiditySum = 0.0f;
  
  /* Read sensor data 3 times, averaging values */ 
  for (int i=0; i<3; ++i)
  {
    delay(1000); // wait 1 sec before next reading
    float temperature = dht.getTemperature();
    float humidity = dht.getHumidity();
    if (isnan(humidity) || isnan(temperature))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      data.airTempInCAverage = NAN;
      data.airHumidityAverage = NAN; 
      // return; 
    }
    else
    {
      temperatureSum += temperature;
      humiditySum += humidity;
    }
  }
  /*Calculate and assign averages*/
  data.airTempInCAverage = String(temperatureSum / 3.0f, 2);
  data.airHumidityAverage = String(humiditySum / 3.0f, 2);

  // SHT31 - Soil parameters
  float soilTemperatureSum = 0.0f;
  float soilHumiditySum = 0.0f;
  float sTemperature = 0.0;
  float sHumidity = 0.0;
  
  /* Read sensor data 3 times, averaging values */ 
  for (int i=0; i<3; ++i)
  {
    delay(1000); // wait 1 sec before next reading
    error = sensor.blockingReadMeasurement(sTemperature, sHumidity);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute blockingReadMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        data.soilTempInCAverage = NAN;
        data.soilHumidityAverage = NAN; 
      // return; 
    }
    else
    {
      soilTemperatureSum += sTemperature;
      soilHumiditySum += sHumidity;

    }
  }
  /*Calculate and assign averages*/
  data.soilTempInCAverage = String(soilTemperatureSum / 3.0f, 2);
  data.soilHumidityAverage = String(soilHumiditySum / 3.0f, 2);

  return data;
};

void resetModem() {
  // Power off the modem
  digitalWrite(MODEM_POWER_ON, LOW);
  delay(1000); // Wait for 1 second
  
  // Power on the modem
  digitalWrite(MODEM_POWER_ON, HIGH);
  delay(1000); // Wait for 1 second
  
  // Reset the modem
  digitalWrite(MODEM_RST, LOW);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(3000); // Wait for 3 seconds for modem to reset
}

void modemSetup()
{
  resetModem();

  const int maxAttempts = 10; // Maximum number of attempts
  const int attemptDelay = 10000; // Delay between attempts in milliseconds (10 seconds)
  int attempts = 0;

  SerialMon.print("Connecting to APN: ");
  SerialMon.println(apn);

  while (attempts < maxAttempts)
  {
    attempts++;
    SerialMon.printf("Attempt %d to connect...\n", attempts);
    modem.gprsConnect(apn, gprsUser, gprsPass);

    SerialMon.print("Waiting for network...");
  
      if (modem.waitForNetwork()) {
      SerialMon.println(" --OK-- successfully connected");
      int csq = modem.getSignalQuality();
      if (modem.isNetworkConnected()) {
        SerialMon.printf("Network connected with signal strength: %d \n", csq);
        // HTTP Request
        SensorData SensorData = deviceData();

        String macAddressEncoded = urlEncode(SensorData.macAddress.c_str());
        String sensorNameEncoded = urlEncode(SensorData.SensorName.c_str());

        String fastapiPostRequest =  apiKey + 
                          "&field1=" + sensorNameEncoded + 
                          "&field2=" + SensorData.airTempInCAverage + 
                          "&field3=" + SensorData.airHumidityAverage + 
                          "&field4=" + SensorData.soilTempInCAverage + 
                          "&field5=" + SensorData.soilHumidityAverage +
                          "&field6=" + SensorData.gsm_rssi +
                          "&field7=" + SensorData.batt_level +
                          "&field8=" + macAddressEncoded;              
        Serial.println("Print data to be sent online: *************");
        Serial.println(fastapiPostRequest);

        if (client.connect(server, port))
        {
          Serial.println("Sending Data to thingspeak...");

          client.print("GET /update HTTP/1.1\n");
          client.print("Host: api.thingspeak.com\n");
          client.print("Connection: close\n");
          client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
          client.print("Content-Type: application/x-www-form-urlencoded\n");
          client.print("Content-Length: ");
          client.print(fastapiPostRequest.length());
          client.print("\n\n");
          client.print(fastapiPostRequest);

          Serial.println(" --Sent successfully!-- ");
        }
        else {
          Serial.println("Failed to connect to ThingSpeak");
        }
        client.stop();
        Serial.println("Waiting...");
      }
      
      break;
      } 
      else 
      {
        SerialMon.println(" failed to connect");
        delay(attemptDelay);
      }
  }

  if (attempts == maxAttempts) 
  {
    SerialMon.println("Failed to connect to the network after maximum attempts.");
    return;
  }
  
}


void modemSetupOld()
{
  // resetModem();

  SerialMon.print("Connecting to APN: ");
  SerialMon.println(apn);

  modem.gprsConnect(apn, gprsUser, gprsPass);

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" failed to connect");
    delay(10000);
    return;
  }
  SerialMon.println(" --OK-- successfully connected");
  int csq = modem.getSignalQuality();
  if (modem.isNetworkConnected()) {
    SerialMon.printf("Network connected with signal strength: %d \n", csq);
  }

  // HTTP Request
  SensorData SensorData = deviceData();

  String macAddressEncoded = urlEncode(SensorData.macAddress.c_str());
  String sensorNameEncoded = urlEncode(SensorData.SensorName.c_str());

  String fastapiPostRequest =  apiKey + 
                    "&field1=" + sensorNameEncoded + 
                    "&field2=" + SensorData.airTempInCAverage + 
                    "&field3=" + SensorData.airHumidityAverage + 
                    "&field4=" + SensorData.soilTempInCAverage + 
                    "&field5=" + SensorData.soilHumidityAverage +
                    "&field6=" + SensorData.gsm_rssi +
                    "&field7=" + SensorData.batt_level +
                    "&field8=" + macAddressEncoded;              
  Serial.println("Print data: *************");
  Serial.println(fastapiPostRequest);

  if (client.connect(server, port))
  {
    Serial.println("This one works....! ");
    client.print("GET /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(fastapiPostRequest.length());
    client.print("\n\n");
    client.print(fastapiPostRequest);

    Serial.println("SUCCESS!");
    delay(10000);
  }
  client.stop();
  Serial.println("Waiting...");
}

void setup()
{
  SerialMon.begin(115200);
  delay(100);

  // Start I2C communication
  dht.setup(DHTPIN, DHTesp::DHT22);

  // Keep power when running from battery
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);



  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(4000);

  SerialMon.println("Initializing modem... ");
  modem.restart();
  // Unlock your SIM card with a PIN if needed
  if (strlen(GSM_PIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }

  String modemInfo = modem.getModemInfo();
  String modemName = modem.getModemName();

  SerialMon.printf("Modem info: %s \n", modemInfo);
  SerialMon.printf("Modem name: %s \n", modemName);

  // /*------------------------------------*/
  Wire1.begin(I2C_SDA_2, I2C_SCL_2);
  sensor.begin(Wire1, SHT31_I2C_ADDR_44);
  sensor.stopMeasurement();
  delay(1);
  sensor.softReset();
  delay(100);
  uint16_t aStatusRegister = 0u;
  error = sensor.readStatusRegister(aStatusRegister);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute readStatusRegister(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }
  Serial.print("aStatusRegister: ");
  Serial.print(aStatusRegister);
  Serial.println();
  error = sensor.startPeriodicMeasurement(REPEATABILITY_MEDIUM,
                                          MPS_ONE_PER_SECOND);
  if (error != NO_ERROR) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
  }


  /*-- DO WHAT YOU NEED TO DO--*/
  ledPinStart();
  modemSetupOld();
  // delay(60000);
  
  /*-- Configure the wake up source as timer wake up --*/ 
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  Serial.println("Going to sleep now");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  /*-- Start the deep sleep mode--*/
  Serial.flush();
  esp_deep_sleep_start();
}


void loop() {      

}