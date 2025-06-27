#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Fingerprint.h>

// WiFi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// MQTT Broker details
const char* mqtt_server = "broker.hivemq.com"; // Replace with your broker's address
#define TOPIC_FINGERPRINT 0
#define TOPIC_KEYPAD 1
#define TOPIC_BUZZER 2
#define TOPIC_LED 3
const char* topics[] = {
  "iot/smartlock/fingerprint",
  "iot/smartlock/keypad",
  "iot/smartlock/buzzer",
  "iot/smartlock/led",
};

WiFiClient espClient;
PubSubClient client(espClient);
const int wait_time = 50;
const int warning_time = 40;
int lastTime = 0;
int lastTimeBuzzer = 0;
int wrong_rq = 0;
const int buzzer_pin;
const int accept_led_pin;
const int reject_led_pin;

// Fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial0);
void setup() {
  Serial.begin(9600);

  // Setup WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(topic);
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Fingerprint sensor setup
  finger.begin(57600);
  if (finger.verifyPassword()) { // check if sensor is ok!  
    Serial.println("Found fingerprint sensor!"); 
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
}


void turnOnWarningBuzzer(){
  int lastTimeBuzzer = 0;  
  for (int x=0; x<180; x++) {
    if (millis() - lastTimeBuzzer == 40){
      // convert degrees to radians then obtain sin value
      sinVal = (sin(x*(3.1412/180)));
      // generate a frequency from the sin value
      toneVal = 2000+(int(sinVal*1000));
      tone(buzzer_pin, toneVal);   
      lastTimeBuzzer = millis();  
    }      
  }
}

void turnOffWarningBuzzer(){
  noTone(buzzer_pin);
}


void turnOnAcceptBuzzer(){
  tone(buzzer_pin, freq);
}

void turnOnRejectBuzzer(){
  int toneFrequency = 300; // Frequency in Hz
  int toneDuration = 100; // Duration in milliseconds
  int pauseDuration = 50; // Pause between beeps

  for (int i = 0; i < 3; i++) {
    ledcWriteTone(0, toneFrequency);
    delay(toneDuration);
    ledcWriteTone(0, 0); // Turn off buzzer
    delay(pauseDuration);
  }
}


void turnOnAcceptLED(){
  int lastTimeLED = 0;
  digitalWrite(accept_led_pin, 1);
}

void turnOffRejectLED(){
  int lastTimeLED = 0;
  digitalWrite(reject_led_pin, 1);
}


void turnOffAcceptLED(){
  int lastTimeLED = 0;
  digitalWrite(reject_led_pin, 1);
}

void turnOffAcceptLED(){
  int lastTimeLED = 0;
  digitalWrite(accept_led_pin, 0);
}

void loop() {
  client.loop();
  if (getFingerPrintID() == FINGERPRINT_OK){
    wrong_rq = 0;
    // turn on relay
    turnOffWarningBuzzer();
    turnOffWarningLED();
    turnOnAcceptBuzzer();
    turnOnAcceptLED();
  }
  else if (getFingerPrintID() == FINGERPRINT_NOFINGER){
    wrong_rq++;
    turnOnRejectLED();
    turnOnRejectBuzzer();
  }
  else{
    turnOffAcceptLED();
    turnOffRejectLED();
  }
  if (wrong_rq == 5){
    turnOnWarningBuzzer();
    turnOnWarningLED();
  }

}

void callback(char* topic, byte* message, unsigned int length) {
  String receivedMessage = "";
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)message[i];
  }
  Serial.print("Message received: ");
  Serial.println(receivedMessage);

  if (receivedMessage == "ENROLL") {
    Serial.println("Starting fingerprint enrollment...");
    getFingerprintEnroll();
  }
}


void beginAddFingerPrint(){
  Serial.println("Ready to enroll a fingerprint!, pressed 0 for cancel...");
  while (!getFingerprintEnroll());
}



uint8_t getFingerprintEnroll() {

  int p = -1;
  while (finger.getTemplateCount() != FINGERPRINT_OK){
    Serial.println("Communication error while get template");
  }
  uint16_t numOfFingerprints= finger.templateCount; 
  int id = numOfFingerprints + 1; 
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();  // get image from finger
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1); // from image to specified feature
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage(); // take image again
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id); // store to specified
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  //Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}
