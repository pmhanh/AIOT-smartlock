#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#define RXD2 4 // Define RX pin for ESP32-CAM
#define TXD2 0/// Define TX pin for ESP32 
#define ROWS  4
#define COLS  3
enum LockState {
  LOCK_IDLE,
  CHECKING_KEYPAD,
  CHECKING_FINGERPRINT,
  CHANGE_PASSWORD,
  GRANTED_ACCESS,
  DENIED_ACCESS,
  ADD_FINGER_PRINT,
};

LockState currentState = LOCK_IDLE; 
int changePass = 0;
int buzzer_state = 0;
String inputPassword = "";
char keyMap[ROWS][COLS] = 
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

uint8_t rowPins[ROWS] = {14, 27, 26, 25}; // rows
uint8_t colPins[COLS] = {33, 32, 18};    //columns ---18

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

String savedPassword = "*0000#";
int passwordCount = 0;    //number of saved passwords
//String inputPassword = ""; //password being entered
bool isAddingPassword = false; 
bool isCheckingPassword = false; 
LiquidCrystal_I2C lcd(0x27, 16, 2); 
const char* ssid = "rum";
const char* password = "12345679";
#define TOPIC_FINGERPRINT 0
#define TOPIC_KEYPAD 1
#define TOPIC_BUZZER 2
#define TOPIC_LED 3

const char* topics[] = {
  "/smartlock/fingerprint",
  "/smartlock/status",
  "/smartlock/led",
  "/smartlock/control",
};

const int wait_time = 50;
const int warning_time = 40;
int lastTime = 0;
int lastTimeBuzzer = 0;
int wrong_rq = 0;
const int buzzer_pin = 13;
const int accept_led_pin = 23;
const int reject_led_pin = 22;
const int lock_state_pin = 12;

const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);


void clearLCD(){
  for (int i = 0; i < 16; i++){
    for (int j = 0; j < 2; j++){
      lcd.setCursor(i, j);
      lcd.print(" ");
    }
  }
}

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  lcd.setCursor(0, 1);
  lcd.print("Connected!");
}

void mqttConnect() {
  while(!mqttClient.connected()) {
    lcd.setCursor(0, 1);
    lcd.print("MQTT Connecting...");
    delay(20);
    lcd.clear();
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str())) {
      lcd.setCursor(0, 1);
      lcd.print("MQTT Connected");
      delay(20);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Smart lock ready");
      mqttClient.subscribe("/smartLock/status");
      mqttClient.subscribe("/smartLock/control");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Retry in 5 sec...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  // lcd.setCursor(0, 0);
  // //lcd.print(topic);
  // lcd.setCursor(0, 1);
  String strMsg;
  for(int i=0; i<length; i++) {
    strMsg += (char)message[i];
  }
  //lcd.print(strMsg);

  if (strMsg == "Buzzer_On" && buzzer_state == 0) {
    buzzer_state = 1;
    // lcd.clear();
    tone(buzzer_pin, 500);
    // lcd.setCursor(0, 0);
    // lcd.print("Buzzer On");
    //delay(1000);
    resetToIdle();
    mqttClient.publish("/smartLock/status", "Buzzer_On");
  } 
  else if (strMsg == "Buzzer_Off" && buzzer_state == 1){
    buzzer_state = 0;
    noTone(buzzer_pin);
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("Buzzer Off");
    //delay(1000);
    resetToIdle();
    mqttClient.publish("/smartLock/status", "Buzzer_Off");
  }
  else if (strMsg == "Unlock") {
    digitalWrite(lock_state_pin, HIGH);
    turnOnAcceptLED();
    turnOnAcceptBuzzer();
    lcd.setCursor(0, 1);
    lcd.print("Door Locked");
    delay(1000);
    digitalWrite(accept_led_pin, LOW);
    digitalWrite(lock_state_pin, LOW);

    // String image = "https://example.com/lock-image.jpg";

    StaticJsonDocument<200> doc;
    doc["state"] = "Unlock";
    //doc["image"] = image;
    doc["result"] = "Success";

    char message[200];
    serializeJson(doc, message);
    resetToIdle();
    mqttClient.publish("/smartLock/status", message);
  } 
  else if (strMsg == "changePassword"){   
    changePass = 1;
  } 
  
}

void setup() {
  Wire.begin(4, 0);
  lcd.init();               
  lcd.backlight();          

  lcd.setCursor(0, 0);
  lcd.print("Smart Lock");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  mySerial.begin(57600, SERIAL_8N1, 16, 17); 
  pinMode(buzzer_pin, OUTPUT);
  pinMode(lock_state_pin, OUTPUT);
  pinMode(accept_led_pin, OUTPUT);
  pinMode(reject_led_pin, OUTPUT);
  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(90);

  finger.begin(57600);
  if (finger.verifyPassword()) {
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint Found!");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint Error");
    while (1) { delay(1); }
  }

  // lcd.setCursor(0, 0);
  // lcd.print("Smart lock is ready");
}



void changePassword()
{
  char key = keypad.getKey();
  if (key)
  {
    lcd.setCursor(inputPassword.length(), 1);
    lcd.print("*");
    inputPassword += key;
    if (inputPassword.length() == 6)
    {
      if (inputPassword.startsWith("*") && inputPassword.endsWith("#"))
      {
        Serial.println("Password correct! Buzzer ON.");
        savedPassword = inputPassword;
        turnOnAcceptLED();
        turnOnAcceptBuzzer();
        StaticJsonDocument<200> doc;
        doc["state"] = "Change password";
        //doc["image"] = image;
        doc["result"] = "Success";

        char message[200];
        serializeJson(doc, message);
        lcd.clear();
        mqttClient.publish("/smartLock/status", message);

        grantAccess();
      }
      else 
      {
        turnOnRejectBuzzer();
        turnOnRejectLED();
        // turn on buzzer and turn on pin
        denyAccess();
        StaticJsonDocument<200> doc;
        doc["state"] = "Change password";
        //doc["image"] = image;
        doc["result"] = "Failed";
        char message[200];
        serializeJson(doc, message);
        lcd.clear();
        mqttClient.publish("/smartLock/status", message);

      }
    }
  } 
  
 
}



#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  466
#define NOTE_C5  523

// Define the duration of notes (in milliseconds)
#define WHOLE_NOTE 1000
#define HALF_NOTE 500
#define QUARTER_NOTE 250
#define EIGHTH_NOTE 125

// Success Song (Simple melody)
int successMelody[] = {NOTE_E4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_E4};  
int successDurations[] = {QUARTER_NOTE, QUARTER_NOTE, QUARTER_NOTE, QUARTER_NOTE, WHOLE_NOTE};

// Reject Song (Simple melody)
int rejectMelody[] = {NOTE_B4, NOTE_B4, NOTE_A4, NOTE_G4};
int rejectDurations[] = {QUARTER_NOTE, QUARTER_NOTE, HALF_NOTE, WHOLE_NOTE};

// Warning Song (Simple melody)
int warningMelody[] = {NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4};
int warningDurations[] = {EIGHTH_NOTE, EIGHTH_NOTE, QUARTER_NOTE, HALF_NOTE};

// Function to play a melody
void playMelody(int melody[], int durations[], int length) {
  for (int i = 0; i < length; i++) {
    tone(buzzer_pin, melody[i], durations[i]);
    delay(durations[i]); // Wait for the note to finish
    noTone(buzzer_pin); // Stop the sound
    delay(50); // Small gap between notes
  }
}

// Function to turn on the Accept Buzzer (Success sound)
void turnOnAcceptBuzzer() {
  playMelody(successMelody, successDurations, sizeof(successMelody) / sizeof(successMelody[0]));
}

// Function to turn on the Reject Buzzer (Reject sound)
void turnOnRejectBuzzer() {
  playMelody(rejectMelody, rejectDurations, sizeof(rejectMelody) / sizeof(rejectMelody[0]));
}

// Function to turn on the Warning Buzzer (Warning sound)
void turnOnWarningBuzzer() {
  playMelody(warningMelody, warningDurations, sizeof(warningMelody) / sizeof(warningMelody[0]));
}

void turnOnAcceptLED(){
  digitalWrite(accept_led_pin, HIGH);
}

void turnOnRejectLED(){
  digitalWrite(reject_led_pin, HIGH);
}











// Updated to use unique name
unsigned long lastActionTime = 0;
bool isPasswordCorrect = false;

// Timeout duration for feedback display
const unsigned long feedbackTimeout = 5000;
int lastBuzzerTime = 0;
void resetToIdle() {
  changePass = 0;
  inputPassword = "";
  isPasswordCorrect = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Lock Ready");
  lcd.setCursor(0, 1);
  lcd.print("                  ");
  currentState = LOCK_IDLE; // Updated to match unique name
}

void checkKeypadInput() {
  char key = keypad.getKey();
  if (key) {
    lcd.setCursor(inputPassword.length(), 1);
    lcd.print("*");
    inputPassword += key;

    if (inputPassword.length() == 6) {
      if (inputPassword == savedPassword) {
        isPasswordCorrect = true;
        currentState = GRANTED_ACCESS;
      } else {
        wrong_rq++;
        currentState = DENIED_ACCESS;
      }
      lastActionTime = millis();
    }
  }
}



void grantAccess() {
  wrong_rq = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Granted!");
  
  turnOnAcceptBuzzer();
  turnOnAcceptLED();
  digitalWrite(lock_state_pin, HIGH);
  
  delay(2000);
  digitalWrite(accept_led_pin, 0);
  digitalWrite(reject_led_pin, 0);
  digitalWrite(lock_state_pin, LOW);
  StaticJsonDocument<200> doc;
  doc["state"] = "Unlock";
  //doc["image"] = image;
  doc["result"] = "Success";

  char message[200];
  serializeJson(doc, message);
  lcd.clear();
  mqttClient.publish("/smartLock/status", message);
  resetToIdle();
}

void denyAccess() {
  if (wrong_rq >= 5){
    char buffer[10] = "Alert";
    mqttClient.publish("/smartLock/status", buffer);
    turnOnWarningBuzzer();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Denied!");
  turnOnRejectBuzzer();
  turnOnRejectLED();
  delay(2000);
  digitalWrite(reject_led_pin, 0);
  digitalWrite(accept_led_pin, 0);
  StaticJsonDocument<200> doc;
  doc["state"] = "Unlock";
  //doc["image"] = image;
  doc["result"] = "Failed";

  char message[200];
  serializeJson(doc, message);
  lcd.clear();
  mqttClient.publish("/smartLock/status", message);
  resetToIdle();
}



void loop() {
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  // Move variable declaration outside the switch
  char key;

  switch (currentState) {
    case LOCK_IDLE:
      // Check keypad input
      key = keypad.getKey();
      if (changePass == 1 && key){
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New password: ");
        currentState = CHANGE_PASSWORD;
      }
      else if (key) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
        currentState = CHECKING_KEYPAD;
      }
      
      // Check fingerprint
      if (finger.getImage() == FINGERPRINT_OK) {
        currentState = CHECKING_FINGERPRINT;
      }
      break;

    case CHECKING_KEYPAD:
      checkKeypadInput();
      break;
    
    case CHANGE_PASSWORD:
      changePassword();
      break;

    case CHECKING_FINGERPRINT:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Fingerprint..");
      lcd.setCursor(0, 1);
      lcd.print("Checking....");
      handleFingerprintInput();
      break;

    case GRANTED_ACCESS:
      grantAccess();
      break;

    case DENIED_ACCESS:
      denyAccess();
      break;
  }

  // Reset to LOCK_IDLE after feedback timeout
  if ((currentState == GRANTED_ACCESS || currentState == DENIED_ACCESS) &&
      (millis() - lastActionTime > feedbackTimeout)) {
    resetToIdle();
  }
}
