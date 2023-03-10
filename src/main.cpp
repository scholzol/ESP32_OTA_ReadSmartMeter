/**
 * @file main.cpp
 * @author Olaf Scholz (olaf.scholz@online.de)
 * @brief 
 * @version 0.1
 * @date 2023-02-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// #include section begin -------------------------------------------------------------------------------------------
#include <Arduino.h>

// OTA Header ####################################
#include "OTA.h"
// OTA Header ####################################

#include <credentials.h> // Wifi SSID and password
#include <Version.h> // contains version information

#include "updateBoardTable.h"
// #include section end ============================================================================================

// define global variables begin -----------------------------------------------------------------------------------

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
char ssidesp32[13];
uint32_t chipId = 0;
char str;

esp_chip_info_t chip_info;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


// define global variables end ======================================================================================

// helper functions begin --------------------------------------------------------------------------------------

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println();
}

// helper functions end ========================================================================================


// setup begin ------------------------------------------------------------------------------------------------------

void setup() {

// read ESP chip and Wifi data and print to serial port

  Serial.begin(115200);
  Serial.println("Booting");
	for(int i=0; i<17; i=i+8) {
	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
  snprintf(ssidesp32,13,"ESP32-%06lX",chipId); // generate chipID string as SSID

  Serial.println("\n\n================================");
  Serial.printf("Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("with %d core\n", ESP.getChipCores());
  Serial.printf("Flash Chip Size : %d \n", ESP.getFlashChipSize());
  Serial.printf("Flash Chip Speed : %d \n", ESP.getFlashChipSpeed());
  Serial.printf("ESP32-%06lX\n", chipId);
  

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("\nFeatures included:\n %s\n %s\n %s\n %s\n %s\n",
      (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded flash" : "",
      (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "2.4GHz WiFi" : "",
      (chip_info.features & CHIP_FEATURE_BLE) ? "Bluetooth LE" : "",
      (chip_info.features & CHIP_FEATURE_BT) ? "Bluetooth Classic" : "",
      (chip_info.features & CHIP_FEATURE_IEEE802154) ? "IEEE 802.15.4" : "");
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // print Wifi data
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  // print Firmware data
  Serial.print("Version: ");
  Serial.println(SemanticVersion);
  Serial.print("short SHA: ");
  Serial.println(SHA_short);

// OTA setup begin ####################################

  setupOTA("TemplateSketch", ssid, password);
  
// OTA setup end ####################################

  // start the Webserver
  server.begin();
  // insert or update the board table for the ESP32 bords in the MySQL database
  updateBoardTable(ssidesp32);
}
// setup end =============================================================================================

// loop begin --------------------------------------------------------------------------------------------

void loop() {

// OTA loop ####################################
  ArduinoOTA.handle();
// OTA loop ####################################

// initialize WebClient

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            // Web Page Heading
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            // Feel free to change style properties to fit your preferences
            client.println("<style>");
            client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("h1 {background-color: powderblue;}");
            client.println("</style>");
            client.println("</head>");

            // Web Page body
            client.println("<body>");
            client.println("<h1>Board info</h1>");
            client.println("<p>Semantic Version: " + String(SemanticVersion) + "</p>");
            client.println("<p>short commit SHA: " + String(SHA_short) + "</p>");
            client.println("<p>working directory: " + String(WorkingDirectory) + "</p>");
            client.println("<p>ChipID: " + String(ssidesp32) + "</p>");
            client.println("</body>");
            client.println("</html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }
}
// loop end =============================================================================================
