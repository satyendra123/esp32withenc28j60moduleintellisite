#include <SPI.h>
#include <EthernetENC.h>
#include <Preferences.h>

#define relayPin 4

byte mac[] = {0xDE, 0xDA, 0xBE, 0xEF, 0xFE, 0xEC};

unsigned long previousMillis = 0;
const long interval = 3000;

Preferences preferences;

EthernetServer server(6000);
bool clientConnected = false;
EthernetClient client;

void writeIPAddressToPreferences(IPAddress ipAddr) {
  preferences.begin("my-app", false); // Open preferences with my-app namespace
  preferences.putUInt("ip1", ipAddr[0]);
  preferences.putUInt("ip2", ipAddr[1]);
  preferences.putUInt("ip3", ipAddr[2]);
  preferences.putUInt("ip4", ipAddr[3]);
  preferences.end(); // Close preferences
}

IPAddress readIPAddressFromPreferences() {
  preferences.begin("my-app", true); // Open preferences with my-app namespace, read-only
  byte ip1 = preferences.getUInt("ip1", 0);
  byte ip2 = preferences.getUInt("ip2", 0);
  byte ip3 = preferences.getUInt("ip3", 0);
  byte ip4 = preferences.getUInt("ip4", 0);
  preferences.end(); // Close preferences
  return IPAddress(ip1, ip2, ip3, ip4);
}

void setup() {
  Ethernet.init(5); 
  IPAddress ip(192, 168, 1, 152); // Default IP address
  byte mac[] = {0xDE, 0xDA, 0xBE, 0xEF, 0xFE, 0xEC};

  IPAddress storedIP = readIPAddressFromPreferences();

  if (storedIP == IPAddress(0,0,0,0))
  {
    Ethernet.begin(mac, ip);
    writeIPAddressToPreferences(ip); // Store the default IP
  }
  else
  {
    Ethernet.begin(mac, storedIP);
  }

  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  while (!Serial);

  Serial.println("Initialize Ethernet with Static IP:");

  server.begin();

  Serial.print("Static IP: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  unsigned long currentMillis = millis();

  // Handle client connection
  if (!clientConnected) {
    client = server.available();
    if (client) {
      Serial.println("New client connected");
      clientConnected = true;
    }
  }

  // Handle commands from the connected client
  if (clientConnected && client.connected()) {
    while (client.available()) {
      String command = client.readStringUntil('%');
      command.trim();

      if (command == "HI") {
        client.println("Connected");
      } else if (command == "ENTRY") 
      {
        client.println("p4 pin Entry Gate is opened");
        digitalWrite(relayPin, LOW);
        delay(500);
        digitalWrite(relayPin, HIGH);
      } else if (command == "READIP") {
        Serial.print("IP Address Requested: ");
        IPAddress currentIP = Ethernet.localIP();
        Serial.println(currentIP);
        client.println(currentIP);
      } else if (command.startsWith("WRITEIP:")) {
        String newIPString = command.substring(8);
        Serial.println("Received WRITEIP command with IP: " + newIPString);
        IPAddress newIP;
        if (newIP.fromString(newIPString)) {
          writeIPAddressToPreferences(newIP); // Update the stored IP address
          client.println("New IP address set: " + newIPString);
          client.println("PLEASE DISCONNECT AND CONNECT WITH NEW IP");
          client.println("PLEASE RESTART THE POWER");
          client.stop(); // Stop current client connection
          clientConnected = false; // Reset client connection flag
          Ethernet.begin(mac, newIP); // Use the new IP for Ethernet
        } else {
          client.println("Invalid IP address format.");
        }
      } else {
        client.println("Unknown command");
      }
    }
  } else {
    clientConnected = false;
  }
  
  // Send health packet every 3 seconds to the connected client
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (clientConnected && client.connected()) {
      client.println("|HLT%");
    }
  }

  // Reconnect if necessary
  if ((!client.connected()) && (Ethernet.linkStatus() == LinkOFF)) {
    Serial.println("Attempting to reconnect...");
    delay(1000);
    client.stop();
    server.begin();
    ESP.restart();
  }
  // Add a delay to avoid excessive looping
  delay(100);
}
//EXAMPLE-1
#include <SPI.h>
#include <EthernetENC.h>
#include <Preferences.h>

#define relayPin 4

byte mac[] =  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress defaultIp(192, 168, 1, 151);

Preferences preferences;

#define PREF_KEY_IP "StoredIP"
EthernetServer server(6000);
bool clientConnected = false;
EthernetClient client;

void writeIPAddressToPreferences(IPAddress ipAddr) {
  preferences.begin("myApp", false);
  preferences.putUInt(PREF_KEY_IP, ipAddr[0] | (ipAddr[1] << 8) | (ipAddr[2] << 16) | (ipAddr[3] << 24));
  preferences.end();
}

IPAddress readIPAddressFromPreferences() {
  preferences.begin("myApp", false);
  uint32_t storedIP = preferences.getUInt(PREF_KEY_IP, 0);
  preferences.end();
  return IPAddress(storedIP);
}

void clearStoredIPFromPreferences() {
  preferences.begin("myApp", false);
  preferences.remove(PREF_KEY_IP);
  preferences.end();
}

unsigned long previousMillis = 0;
const long interval = 3000;

void setup() {
  Ethernet.init(5); 
  Ethernet.begin(mac, defaultIp);
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  while (!Serial);

  Serial.println("Initialize Ethernet:");

  // Read the stored IP address from preferences
  IPAddress storedIP = readIPAddressFromPreferences();

  // If a valid stored IP address is available, use it; otherwise, use the default IP
  if (storedIP != IPAddress(0, 0, 0, 0)) {
    Serial.println("Using stored IP address:");
    Serial.println(storedIP);
    Ethernet.begin(mac, storedIP);
  } else {
    Serial.println("Using default IP address:");
    Serial.println(defaultIp);
    Ethernet.begin(mac, defaultIp);
    writeIPAddressToPreferences(defaultIp);
  }

  server.begin();

  Serial.print("Static IP: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  unsigned long currentMillis = millis();

  // Handle client connection
  if (!clientConnected) {
    client = server.available();
    if (client) {
      Serial.println("New client connected");
      clientConnected = true;
    }
  }

  // Handle commands from the connected client
  if (clientConnected && client.connected()) {
    while (client.available()) {
      String command = client.readStringUntil('%');
      command.trim();

      if (command == "HI") {
        client.println("Connected");
      } else if (command == "ENTRY") {
        client.println("p4 pin Entry Gate is opened");
        digitalWrite(relayPin, LOW);
        delay(500);
        digitalWrite(relayPin, HIGH);
      } else if (command == "READIP") {
        Serial.print("IP Address Requested: ");
        IPAddress currentIP = Ethernet.localIP();
        Serial.println(currentIP);
        client.println(currentIP);
      } else if (command.startsWith("WRITEIP:")) {
        String newIPString = command.substring(8);
        Serial.println("Received WRITEIP command with IP: " + newIPString);
        IPAddress newIP;
        if (newIP.fromString(newIPString)) {
          writeIPAddressToPreferences(newIP);
          client.println("New IP address set: " + newIPString);
          client.println("PLEASE DISCONNECT AND CONNECT WITH NEW IP");
          client.println("PLEASE RESTART THE POWER");
          // Ensure client is stopped
          client.stop();
          delay(1000); // Short delay to allow the client to properly close
          
          // Print the stored IP for verification
          Serial.print("Stored IP Address: ");
          Serial.println(readIPAddressFromPreferences());
          ESP.restart();
          // Reset ESP32 module
          
        } else {
          client.println("Invalid IP address format.");
        }
      } else {
        client.println("Unknown command");
      }
    }
  } else {
    clientConnected = false;
  }

  // Send health packet every 3 seconds to the connected client
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (clientConnected && client.connected()) {
      client.println("|HLT%");
    }
  }

  // Reconnect if necessary
  if ((!client.connected()) && (Ethernet.linkStatus() == LinkOFF)) {
    Serial.println("Attempting to reconnect...");
    delay(1000);
    client.stop();
    server.begin();
    ESP.restart();
  }
}
