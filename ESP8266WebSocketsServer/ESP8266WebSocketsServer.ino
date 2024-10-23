#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>

const char* ssid = "A";
const char* password = "''''''''";

const int ledPin1 = D0;
const int ledPin2 = D1;

bool led1Status = false;
bool led2Status = false;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}

void handleRoot() {
  String html = "<!DOCTYPE HTML><html><head>";
  html += "<title>LED Control</title>";
  html += "<script>";
  html += "var connection = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "connection.onmessage = function(event) {";
  html += "  var statuses = event.data.split(',');";
  html += "  document.getElementById('status1').innerHTML = statuses[0];";
  html += "  document.getElementById('status2').innerHTML = statuses[1];";
  html += "};";
  html += "function sendRequest(led, state) {";
  html += "  connection.send(led + '=' + state);";
  html += "}";
  html += "</script></head><body>";
  html += "<h1>LED Control</h1>";

  html += "<button type='button' onclick=\"sendRequest('LED1', 'ON')\">LED 1 ON</button>";
  html += "<button type='button' onclick=\"sendRequest('LED1', 'OFF')\">LED 1 OFF</button>";
  html += "<p>LED 1 Status: <span id='status1'>" + String(led1Status ? "ON" : "OFF") + "</span></p>";

  html += "<button type='button' onclick=\"sendRequest('LED2', 'ON')\">LED 2 ON</button>";
  html += "<button type='button' onclick=\"sendRequest('LED2', 'OFF')\">LED 2 OFF</button>";
  html += "<p>LED 2 Status: <span id='status2'>" + String(led2Status ? "ON" : "OFF") + "</span></p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String request = String((char*)payload);
    Serial.println("WebSocket message: " + request);

    // Control LED
    if (request.indexOf("LED1=ON") != -1) {
      digitalWrite(ledPin1, HIGH);
      led1Status = true;
    }
    if (request.indexOf("LED1=OFF") != -1) {
      digitalWrite(ledPin1, LOW);
      led1Status = false;
    }
    if (request.indexOf("LED2=ON") != -1) {
      digitalWrite(ledPin2, HIGH);
      led2Status = true;
    }
    if (request.indexOf("LED2=OFF") != -1) {
      digitalWrite(ledPin2, LOW);
      led2Status = false;
    }

    String statusMessage = String(led1Status ? "ON" : "OFF") + "," + String(led2Status ? "ON" : "OFF");
    webSocket.broadcastTXT(statusMessage);
  }
}
