/*
* Created by : Partha Singha Roy
* 
* Date : 29.01.2022
*
*/

#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>

/* ---- Globals Variabls ---- */
ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;
unsigned long messageTimestamp = 0;
int data = 0;
int duration; // for ultrasonic sensor
int distance;

/* --- Macros --- */
#define USE_SERIAL Serial
#define TRIGPIN  D4
#define ECHOPIN  D3
#define BAUD_RATE 115200
#define SSID "vivo 1816"
#define PASSWORD "12345PSR"
#define ADDRESS "test-code-0001.herokuapp.com"
#define PORT 80
#define URL "/socket.io/?EIO=4"

/* --- Function Prototype --- */
int readData();
void sendJSON();
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length);

/* --- Driver setUp --- */
void setup() {
    // USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);
    pinMode(TRIGPIN, OUTPUT); // Sets the TRIGPIN as an Output
    pinMode(ECHOPIN, INPUT);  // Sets the ECHOPIN as an Input

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

      for(uint8_t t = 4; t > 0; t--) {
          USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
          USE_SERIAL.flush();
          delay(1000);
      }

    // disable AP
    if(WiFi.getMode() & WIFI_AP) {
        WiFi.softAPdisconnect(true);
    }

    WiFiMulti.addAP(SSID, PASSWORD);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    String ip = WiFi.localIP().toString();
    USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

    // server address, port and URL
    socketIO.begin(ADDRESS, PORT, URL);

    // event handler
    socketIO.onEvent(socketIOEvent);
}

/* --- Driver Loop --- */
void loop() {
    socketIO.loop();
    sendJSON();
}

/* --- Function Definations --- */
// event handler Function --- >
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            USE_SERIAL.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
            USE_SERIAL.printf("[IOc] get event: %s\n", payload);
            break;
        case sIOtype_ACK:
            USE_SERIAL.printf("[IOc] get ack: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_ERROR:
            USE_SERIAL.printf("[IOc] get error: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_EVENT:
            USE_SERIAL.printf("[IOc] get binary: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_ACK:
            USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
            hexdump(payload, length);
            break;
    }
}

//send JSON to the server --->
void sendJSON(){
     uint64_t now = millis();
    
    if(now - messageTimestamp > 2000) {
        messageTimestamp = now;
        
        // creat JSON message for Socket.IO (event)
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();
        
        // add evnet name
        // Hint: socket.on('event_name', ....
        array.add("Chart-Data");
        data = readData();
        // add payload (parameters) for the event
        JsonObject param1 = array.createNestedObject();
        param1["distance"] = data;

        // JSON to String (serializion)
        String output;
        serializeJson(doc, output);

        // Send event
        socketIO.sendEVENT(output);

        // Print JSON for debugging
        USE_SERIAL.println(output);
    }
}

//read Ultrasonic sensor Data --- >
int readData(){
    // Clears the TRIGPIN
    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);

    // Sets the TRIGPIN on HIGH state for 10 micro seconds
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGPIN, LOW);

    // Reads the ECHOPIN, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHOPIN, HIGH);

    // Calculating the distance 
    distance = duration * 0.034 / 2;
    return distance;
}