// Dieses Skript sendet den State eines Buttons mit dauer wie lange dieser gedrückt wurde.
// Zusätzlich gibt es eine LED die ebenfalls über MQTT gesteuert werden kann

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>

const char* SSID = "intia";
const char* PSK = "BuntesLicht10";
const char* MQTT_BROKER = "intia.local";
const char* TOPIC = "morse"; // Toppic an das gesendet wird

const char* TOPIC_SUB = "morse/set"; // Topic das Oboniert wird
//Wird an dieses Topic "on" gesendet so geht die LED an. "off" zum aus schalten.

WiFiClient espClient;
PubSubClient client(espClient);

const int capacity = JSON_OBJECT_SIZE(50);
StaticJsonDocument<capacity> doc;


const int ledStatus =  D8;      // LED-Pin

const int ledButton =  D2;      // LED-Pin
const int button = D5;          // Button-Pinconst
const int buzzer= D6;
int buttonState = 1;            // Status des Buttons

// Zu kontrollierender Code
// 0 -> Kurz, 1 -> Lang
int code[] ={0,0,0,0,0,1,0,1,0,0,0,1,0,0,1,1,1};
int code_length=17;
int current_code=0;
int counter=0;



// Variabeln um die Zeit zu berechnen
unsigned long startTime;
unsigned long pressTime;

void setup() {
  pinMode(ledStatus, OUTPUT);
  pinMode(ledButton, OUTPUT);
   pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(callback);
}

// Diese Funktion Empfängt Topics und printet diese in an Serial. Zusätzlich schaltet diese die LED.
// ToDO: LED Status kann noch nicht gezielt abgerufen werden.
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message [");
  Serial.print(topic);
  Serial.print("] ");
  char msg[length + 1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
  }
  Serial.println();
  msg[length] = '\0';
  if(strcmp(msg, "reset") == 0){
    counter=0;
    Serial.println("RESET");
  }
    /*
  digitalWrite(buzzer, HIGH);
  delay(1000); // Verzögerung um den Button zu entprellen
  digitalWrite(buzzer, LOW);*/
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.hostname("Morse-Code");
  WiFi.begin(SSID, PSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconnecting MQTT...");
    if (!client.connect("morse_code")) {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
  client.subscribe(TOPIC_SUB);
  Serial.println("MQTT Connected...");
}

void checkButton() {
  if (buttonState != digitalRead(button)) {
    //Butten ist nicht gedrückt. Die Zeit wird berechnet
    if (digitalRead(button) != HIGH) {
      startTime = millis();
      buttonState = LOW;
      digitalWrite(ledButton, HIGH);
      delay(200); // Verzögerung um den Button zu entprellen
    }else {
      pressTime = millis() - startTime;
      current_code=code[counter];
      Serial.print("Current code: ");
      Serial.print(current_code);
      Serial.print(" Presstime: ");
      Serial.print(pressTime);
      buttonState = HIGH;
      if(current_code==1 && pressTime > 2000){
        Serial.print(" Erkannt: ");
        Serial.println("LONG");
        counter++;
      }else if(current_code==0 && pressTime <= 2000){
        Serial.print(" Erkannt: ");
        Serial.println("SHORT");
        counter++;
      }else{
        Serial.print(" Erkannt: ");
        Serial.print("Fehler");
         Serial.print(" Sollte: ");
        Serial.println(current_code);
        digitalWrite(buzzer, HIGH);
        delay(2000); // Verzögerung um den Button zu entprellen
        digitalWrite(buzzer, LOW);
      }
      
      Serial.print(counter);
      Serial.print(" von ");
      Serial.println(code_length);
      if(counter == code_length){
        Serial.println("Fertig");
        // Elemente werden an ein Json übergeben das später gesendet wird
        doc["id"] = "morse";
        doc["action"] = "solved";
        // Nachricht wird gepackt und an MQTT gesendet.
        char message[256];
        serializeJson(doc, message);
        Serial.println(message);
        
        client.publish(TOPIC, message);
        delay(20000);
        counter=0;
      }
      delay(200); // Verzögerung um den Button zu entprellen
      digitalWrite(ledButton, LOW);
    }
  }
}

// ToDO: Die Buttonsteuerung in eine eigene Funktion packen.
void loop() {
  if (!client.connected()) {
    delay(500);
    reconnect();
  }
  client.loop();
  // Überprüfung ob der Zustand des Buttons sich geändert hat
  checkButton();
}
