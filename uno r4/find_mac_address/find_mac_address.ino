#include <WiFiS3.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  byte mac[6];
  String mac_address = "";

  WiFi.macAddress(mac);
  
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) 
      mac_address += "0";
    mac_address += String(mac[i], HEX);
    if (i > 0) 
      mac_address += ":";
  }
  
  Serial.print("MAC Address: ");
  Serial.println(mac_address);
}

void loop() {
}
