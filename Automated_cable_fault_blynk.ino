// Fill in your Blynk Template ID, Device Name, and Auth Token from the Web Console
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "Cable Fault Monitoring"
#define BLYNK_AUTH_TOKEN    "YourAuthTokenHere"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>  // Use <WiFi.h> if you are using an ESP32 board
#include <BlynkSimpleEsp8266.h>
#include <LiquidCrystal.h>

// Your WiFi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Your_WiFi_Name";
char pass[] = "Your_WiFi_Password";

// Initialize LCD interface pins (Adjust to match your schematic)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Analog input pins linked to resistor ladder networks
const int pinRedPhase = A0; 
const int pinYellowPhase = A1; 
const int pinGreenPhase = A2; // Switched from Blue to Green to match your layout

// Alert LED Output Pins
const int ledRed = 6;
const int ledYellow = 7;
const int ledGreen = 8;

// Keep track of previous state to avoid flooding Blynk with repeated notifications
int lastDistR = 0, lastDistY = 0, lastDistG = 0;

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  
  pinMode(ledRed, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  
  // Clear LEDs initially
  digitalWrite(ledRed, LOW);
  digitalWrite(ledYellow, LOW);
  digitalWrite(ledGreen, LOW);
  
  // Connect to WiFi and Blynk Server
  Blynk.begin(auth, ssid, pass);
  lcd.clear();
}

void loop() {
  Blynk.run(); // Keep Blynk communication synchronized

  // Read raw voltage levels from network lines
  int valR = analogRead(pinRedPhase);
  int valY = analogRead(pinYellowPhase);
  int valG = analogRead(pinGreenPhase);

  // Convert raw readings to exact distance in Kilometers
  int distR = getDistance(valR);
  int distY = getDistance(valY);
  int distG = getDistance(valG);

  // Handle active fault states
  if (distR > 0) {
    displayFault('R', distR);
    sendBlynkAlert("R", distR, lastDistR, V1);
    blinkLED(ledRed);
    lastDistR = distR;
  } 
  else if (distY > 0) {
    displayFault('Y', distY);
    sendBlynkAlert("Y", distY, lastDistY, V2);
    blinkLED(ledYellow);
    lastDistY = distY;
  } 
  else if (distG > 0) {
    displayFault('G', distG);
    sendBlynkAlert("G", distG, lastDistG, V3);
    blinkLED(ledGreen);
    lastDistG = distG;
  } 
  else {
    // Healthy Normal State: Only displays R=0 Y=0 G=0
    digitalWrite(ledRed, LOW);
    digitalWrite(ledYellow, LOW);
    digitalWrite(ledGreen, LOW);
    
    lcd.setCursor(0, 0);
    lcd.print("R=0   Y=0   G=0 ");
    lcd.setCursor(0, 1);
    lcd.print("                "); // Kept completely blank as requested

    // Reset mobile metrics silently if clearing a previous fault
    if (lastDistR != 0 || lastDistY != 0 || lastDistG != 0) {
      Blynk.virtualWrite(V1, 0);
      Blynk.virtualWrite(V2, 0);
      Blynk.virtualWrite(V3, 0);
      lastDistR = 0; lastDistY = 0; lastDistG = 0;
    }
  }
  delay(100);
}

// Map the analog voltage values back to structural physical kilometers 
int getDistance(int analogValue) {
  // Calibrate these numbers (0-1023) according to your board's physical resistors
  if (analogValue > 800 && analogValue < 950) return 2; 
  if (analogValue > 500 && analogValue <= 800) return 4; 
  if (analogValue > 200 && analogValue <= 500) return 6; 
  return 0; 
}

// Updates physical 16x2 character matrix display 
void displayFault(char phase, int km) {
  lcd.setCursor(0, 0);
  lcd.print(phase);
  lcd.print("=");
  lcd.print(km);
  lcd.print("km            "); 

  lcd.setCursor(0, 1);
  lcd.print(phase);
  lcd.print(" wire fault ");
  lcd.print(km);
  lcd.print("k  ");
}

// Triggers push alert parameters to Blynk app safely without flooding data limits
void sendBlynkAlert(String phase, int currentDist, int previousDist, int targetVirtualPin) {
  if (currentDist != previousDist) {
    String alertMsg = phase + " wire fault " + String(currentDist) + "k";
    
    Blynk.virtualWrite(targetVirtualPin, currentDist); // Update numerical value widget on app
    Blynk.logEvent("fault_alert", alertMsg);           // Triggers native mobile Push Notification
  }
}

void blinkLED(int ledPin) {
  digitalWrite(ledPin, HIGH);
  delay(150);
  digitalWrite(ledPin, LOW);
  delay(150);
}
