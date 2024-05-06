#include <SPI.h>
#include <EthernetENC.h>
#include <EEPROM.h>

#define relayPin 4

byte mac[] = {0xDE, 0xDA, 0xBE, 0xEF, 0xFE, 0xEC};

unsigned long previousMillis = 0;
const long interval = 3000;

#define EEPROM_SIZE 4

EthernetServer server(6000);
bool clientConnected = false;
EthernetClient client;

void writeIPAddressToEEPROM(int startAddress, IPAddress ipAddr) {
  for (int i = 0; i < 4; ++i) {
    EEPROM.write(startAddress + i, ipAddr[i]);
  }
  EEPROM.commit();
  delay(10);
}

IPAddress readIPAddressFromEEPROM(int startAddress) {
  byte ipBytes[4];
  for (int i = 0; i < 4; ++i) {
    ipBytes[i] = EEPROM.read(startAddress + i);
  }
  return IPAddress(ipBytes);
}

void clearStoredIPFromEEPROM() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(10);
}

void setup() {
  Ethernet.init(5); 
  IPAddress ip(192, 168, 1, 152); // Default IP address
  byte mac[] = {0xDE, 0xDA, 0xBE, 0xEF, 0xFE, 0xEC};

  EEPROM.begin(EEPROM_SIZE);
  IPAddress storedIP = readIPAddressFromEEPROM(0);

  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  while (!Serial);

  Serial.println("Initialize Ethernet with Static IP:");

  Serial.print("Stored IP: ");
  Serial.println(storedIP);

  if (storedIP == IPAddress(0,0,0,0))
  {
    Ethernet.begin(mac, ip);
    writeIPAddressToEEPROM(0, ip); // Store the default IP
  }
  else
  {
    Ethernet.begin(mac, storedIP);
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
          writeIPAddressToEEPROM(0, newIP); // Update the stored IP address
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
