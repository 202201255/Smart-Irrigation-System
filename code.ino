#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>


// Initialize the LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Blynk Credentials
char auth[] = "gnezYkp8ZdawHbXVopYIZnNfvpJiQpUL";  // Blynk Auth token
// char ssid[] = "Redmi9";                            // WIFI name
// char pass[] = "57575757";                          // WIFI password
char ssid[] = "MADHAV";                            // WIFI name
char pass[] = "12345678";                          // WIFI password

BlynkTimer timer;


#define DHTPIN D2  // Use GPIO4 (D2 on NodeMCU)
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


bool manualRelayState = 0;
bool autoControlEnabled = true;



// Define component pins
#define sensor A0
#define waterPumpRelay D4
#define MoistureRelay D5

// Auto Control Variables
unsigned long previousMillis = 0;
unsigned long ON_DURATION = 15000;  // 15 seconds on
unsigned long OFF_DURATION = 5000;  // 5 seconds off
bool isOn = true;
bool flag= true;

// Function Prototypes
void soilMoistureSensor();
void autoRelayControl();

// Setup Function
void setup() {
  Serial.begin(9600);

  // Initialize Relay Pins
  pinMode(waterPumpRelay, OUTPUT);
  pinMode(MoistureRelay, OUTPUT);
  pinMode(sensor, INPUT);
  digitalWrite(DHTPIN, INPUT);
  digitalWrite(waterPumpRelay, LOW);
  digitalWrite(MoistureRelay, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Initialize Blynk
  Serial.println("Connecting to Blynk...");
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  // Display Loading on LCD
  lcd.setCursor(1, 0);
  lcd.print("System Loading");
  for (int a = 0; a <= 2; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(500);
  }

  dht.begin();
  // Set up Timers
  timer.setInterval(2000L, soilMoistureSensor);  // 1-second interval for soil moisture
  timer.setInterval(1000L, autoRelayControl);    // 1-second interval for auto control
}

// Blynk Write Function for Manual Relay Control

BLYNK_WRITE(V1) {
  if (autoControlEnabled) {
    Serial.println("Manual control ignored: Auto mode is active");
    return;  // Ignore manual control if auto mode is enabled
  }

  manualRelayState = param.asInt();

  Serial.print("Button pressed in Blynk app. Relay state: ");
  Serial.println(manualRelayState);

  digitalWrite(MoistureRelay, HIGH);

  if (manualRelayState == 1) {
    // Manual Control: Turn OFF Motor
    digitalWrite(waterPumpRelay, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Motor is OFF  ");  // Add spaces to clear leftover text
    Serial.println("Motor is OFF");
  } else {
    // Manual Control: Turn ON Motor
    digitalWrite(waterPumpRelay, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Motor is ON   ");  // Add spaces to clear leftover text
    Serial.println("Motor is ON");
  }
}

// Optional: Blynk Write Function to Enable/Disable Auto Control
BLYNK_WRITE(V2) {
  autoControlEnabled = param.asInt();
  Serial.print("Auto Control Enabled: ");
  Serial.println(autoControlEnabled);

  if (autoControlEnabled) {
    Serial.println("Automatic Relay Control Enabled");
    lcd.setCursor(0, 1);
    lcd.print("Auto Control ON ");
  } else {
    Serial.println("Automatic Relay Control Disabled");
    lcd.setCursor(0, 1);
    lcd.print("Auto Control OFF");
    digitalWrite(waterPumpRelay, LOW);  // Ensure relay is OFF when auto control is disabled
    digitalWrite(MoistureRelay, LOW);   // Ensure moisture relay is OFF
  }
}

// Soil Moisture Sensor Function
void soilMoistureSensor() {
  int value = analogRead(sensor);
  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  Blynk.virtualWrite(V0, value);

  if (isnan(h) || isnan(t)) {
    // Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V3, t);  // Temperature
  Blynk.virtualWrite(V4, h);

  Serial.print("Soil Moisture: ");
  Serial.print(value);
  Serial.print(" % | Temperature: ");
  Serial.print(t);
  Serial.print(" °C | Humidity: ");
  Serial.print(h);
  Serial.println(" %");


  lcd.setCursor(0, 0);
  lcd.print("Moisture : ");
  lcd.print(value);
  lcd.print("%  ");  // Add spaces to clear leftover characters
}

// Automated Relay Control Function
void autoRelayControl() {
  if (!autoControlEnabled) {
    // digitalWrite(waterPumpRelay, LOW);
    // digitalWrite(MoistureRelay, LOW);
    return;
  }





  unsigned long currentMillis = millis();

  if (isOn) {
    if (currentMillis - previousMillis < ON_DURATION) {

        digitalWrite(MoistureRelay, HIGH);
        delay(2000);
      if (flag) {
        Serial.println("flag");
        float t = dht.readTemperature();  // Read the current temperature
        if (!isnan(t)) {       
          
          if (t < 21) {
            OFF_DURATION = 15000;
            ON_DURATION = 5000;
          } else if (t >= 21 && t <= 25) {
            OFF_DURATION = 10000;
            ON_DURATION = 10000;
          } else if (t >= 26 && t <= 30) {
            OFF_DURATION = 10000;
            ON_DURATION = 15000;
          } else if (t >= 31 && t <= 35) {
            OFF_DURATION = 5000;
            ON_DURATION = 15000;
          } else {
            OFF_DURATION = 5000;
            ON_DURATION = 20000;
          }
        } else {
          Serial.println("Failed to read temperature for OFF_DURATION adjustment");
        }
        
        Serial.print("ON_DURATION  ");
        Serial.println(ON_DURATION);

        Serial.print("OFF_DURATION  ");
        Serial.println(OFF_DURATION);
        flag = false;
      }
      // Turn on Moisture Relay
      // digitalWrite(MoistureRelay, HIGH);

      // Read sensor value
      int value = analogRead(sensor);
      value = map(value, 0, 1024, 0, 100);
      value = (value - 100) * -1;

      // value = map(value, 300, 800, 0, 100);
      // value = (value - 100) * -1;

      // Check moisture level and control water pump
      if (value < 40) {
        digitalWrite(waterPumpRelay, HIGH);
        lcd.setCursor(0, 1);
        lcd.print("Pump ON       ");
      } else {
        digitalWrite(waterPumpRelay, LOW);
        lcd.setCursor(0, 1);
        lcd.print("Pump OFF      ");
      }

      Serial.println("Auto Control: Sensor ON");
    } else {
      // Switch to OFF state
      digitalWrite(MoistureRelay, LOW);
      digitalWrite(waterPumpRelay, LOW);
      previousMillis = currentMillis;
      isOn = false;

      lcd.setCursor(0, 1);
      lcd.print("Sensor OFF    ");
      Serial.println("Auto Control: Sensor OFF");
    }
  } else {
    // Sensor OFF state
    if (currentMillis - previousMillis >= OFF_DURATION) {
      // Switch back to ON state
      previousMillis = currentMillis;
      isOn = true;
      flag=true;
    }
  }
}

// Loop Function
void loop() {
  Blynk.run();  // Run Blynk
  timer.run();  // Run Timers
}
