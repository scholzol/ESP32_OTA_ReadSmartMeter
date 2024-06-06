/**
 * @file main.cpp
 * @author Olaf Scholz (olaf.scholz@online.de)
 * @brief  read data from S0- interface of a smart meter via IR
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

#include <MySQL_Generic.h>
#include <MySQL_credentials.h>
#include <algorithm>

// #include section end ============================================================================================

// define global variables begin -----------------------------------------------------------------------------------

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;
// Current times
unsigned long currentTime = millis();
unsigned long currentLoopTime = millis();
// Previous times
unsigned long previousTime = 0; 
unsigned long previousLoopTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
const long writeCycleTime = 600000;
char ssidesp32[13];
uint32_t chipId = 0;
char str;

esp_chip_info_t chip_info;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define BAUD_RATE 300
#define TX2 17
#define RX2 16
#define SERIAL_MODE2 SERIAL_7E1

// define MySQL variables
#define USING_HOST_NAME     false

#if USING_HOST_NAME
  // Optional using hostname, and Ethernet built-in DNS lookup
  char MySQLserver[] = "your_account.ddns.net"; // change to your server's hostname/URL
#else
  IPAddress MySQLserver(192, 168, 178, 49);
#endif

uint16_t server_port = 3306;                  //port for the access of the MySQL database (standard:3306);

MySQL_Connection conn((Client *)&client);
MySQL_Query *query_mem;

time_t timer;
typedef struct values_t {
	uint32_t electricityID1;
	uint32_t electricityID2;
	uint8_t billingPeriod;
	uint32_t activeFirmware;
	uint32_t timeSwitchPrgmNo;
	uint32_t localTime;
	uint32_t localDate;
	float sumPower11;
	float sumPower12;
	float sumPower13;
	float sumPower21;
	float sumPower22;
	float sumPower23;
	float power;
	float U_L1;
	float U_L2;
	float U_L3;
	uint32_t deviceNo;
	uint8_t freqL1;
	uint8_t freqL2;
	uint8_t freqL3;
} values_t;

char sendBuffer[100];
char retBuffer[101];
char *p_retBuffer = retBuffer;
char tmpVal[9];	// 8 Char + \0

values_t values;

time_t systime;


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

int sendbytes (char * Buffer, int Count )

  {
  int sent;

  sent = Serial2.write (Buffer, Count );
  if ( sent < 0 )
    {
    Serial.printf ( "sendbytes failed - error!" );
    return false;
    }
  if ( sent < Count )
    {
    Serial.printf ( "sendbytes failed - truncated!" );
    }
  Serial.printf("sendbytes successful\n");
  return sent;
  }

int receiveBytes ( char * retBuffer ) {

	char buf[101], c;
	int count, i = 0;
	if (Serial2.available()>0) {        
    do {

      count =	Serial2.read (&c, 1 );
      if ( c == 0x3 ) {	// ETX
          return false;
      }

      if ( count > 0 ) {
        if ( c  != '\r' && c != '\n')
          buf[i++] = c;
      }
      
    } while ( c != '\n' && i < 100 && count >= 0 );

    if ( count < 0 ) Serial.println ( "Read failed!" );
    else if ( i == 0 ) Serial.println ( "No data!" );
    else {
      buf[i] = '\0';
      snprintf ( retBuffer, i + 1, buf );
      Serial.printf (" %i Bytes: %s\n", i, buf );
    }
    return true;
  }
  return true;
}
// helper functions end ========================================================================================

void readSmartMeter() {
// smart meter reading begin --------------------------------------------------------------------------------------

  Serial.printf("start reading ....");
  sprintf ( sendBuffer, "/?!\r\n" );
  sendbytes (sendBuffer, 5 );
  usleep ( 3000000 );	// 300msec
//  usleep(100000);	// 300msec

  while ( receiveBytes ( p_retBuffer ) ) {
    if ( strncasecmp ( retBuffer, "0.0.0", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Electricity id 1: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.electricityID1 = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.0.1", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Electricity id 2: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.electricityID2 = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.1.0", 5) == 0 ) {
      snprintf ( tmpVal, 3, "%.*s", 2, retBuffer + 6 );
      printf ( "Billing period: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.billingPeriod = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.2.0", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Active firmware: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.activeFirmware = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.2.2", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Time switch prgm no.: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.timeSwitchPrgmNo = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.9.1", 5) == 0 ) {
      snprintf ( tmpVal, 8, "%.*s", 7, retBuffer + 6 );
      printf ( "Time: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.localTime = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "0.9.2", 5) == 0 ) {
      snprintf ( tmpVal, 8, "%.*s", 7, retBuffer + 6 );
      printf ( "Date: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.localDate = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "1.8.0", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 1: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower11 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "1.8.1", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 2: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower12 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "1.8.2", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 3: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower13 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "2.8.0", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 1: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower21 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "2.8.1", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 2: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower22 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "2.8.2", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Sum Power 3: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.sumPower23 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "1.25", 4) == 0 )  {
      snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 5 );
      printf ( "Power: %05.2f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.power = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "32.25", 5) == 0 ) {
      snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
      printf ( "U L1: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.U_L1 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "52.25", 5) == 0 ) {
      snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
      printf ( "U L2: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.U_L2 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "72.25", 5) == 0 ) {
      snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
      printf ( "U L3: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
      values.U_L3 = atof ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "C.1.0", 5) == 0 ) {
      snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
      printf ( "Device No.: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
      values.deviceNo = atoi ( tmpVal );
    }
    if ( strncasecmp ( retBuffer, "C.7.1", 5) == 0 ) {
      snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
      printf ( "Freq. L1: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
      values.freqL1 = strtol( tmpVal, NULL, 16 );
    }
    if ( strncasecmp ( retBuffer, "C.7.2", 5) == 0 ) {
      snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
      printf ( "Freq. L2: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
      values.freqL2 = strtol( tmpVal, NULL, 16 );
    }
    if ( strncasecmp ( retBuffer, "C.7.3", 5) == 0 ) {
      snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
      printf ( "Freq. L3: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
      values.freqL3 = strtol( tmpVal, NULL, 16 );
    }
  }
  char default_database[] = "olaf";            // database for olaf's information1
  char default_table[]    = "smartmeter";           // table for smartmeter information

  // check validity of some values

  if (values.sumPower11>300000.0)
  {
    values.sumPower11 = values.sumPower12 + values.sumPower13;
  }
  

  String INSERT_SQL = String("INSERT INTO ") + default_database + "." + default_table  + " (sumPower11,sumPower12,sumPower13,sumPower21,sumPower22,sumPower23) VALUES ('" 
    + String(values.sumPower11) + "','"
    + String(values.sumPower12) + "','" 
    + String(values.sumPower13) + "','" 
    + String(values.sumPower21) + "','" 
    + String(values.sumPower22) + "','" 
    + String(values.sumPower23) + "')";

  MySQL_Query query_mem = MySQL_Query(&conn);
  if (conn.connectNonBlocking(MySQLserver, server_port, user, passwd) != RESULT_FAIL)
  {
      delay(500);
      if (conn.connected())
      {
        MYSQL_DISPLAY(INSERT_SQL);
        if ( !query_mem.execute(INSERT_SQL.c_str()) )
        {
          MYSQL_DISPLAY("INSERT_SQL error");
          return;
        }
      }

  }
conn.close();
}
// smart meter reading end ========================================================================================

void updateBoardTable(char ssid32[13])
{
  char default_database[] = "esp32";            // database for the ESP32 information1
  char default_table[]    = "boards";           // table for ESP32 Hardware information

    // insert data into MySQL database
    
    // Sample query
    String SELECT_SQL = String("SELECT ChipID FROM ") + default_database + "." + default_table;
    String INSERT_SQL = String("INSERT INTO ") + default_database + "." + default_table  + " (ChipModel, MAC_Address, IP_Address, ChipID) VALUES ('" + String(ESP.getChipModel()) + "','" + String(WiFi.macAddress()) + "','" + WiFi.localIP().toString() + "','" + ssid32 + "')";
    String UPDATE_SQL = String("UPDATE ") + default_database + "." + default_table  + " SET ChipModel='" + String(ESP.getChipModel()) + "', MAC_Address='" + String(WiFi.macAddress()) + "', IP_Address='" + WiFi.localIP().toString() + "' WHERE ChipID='" + ssid32 + "'";
    String rowValues[20]= {" "};

    MySQL_Query query_mem = MySQL_Query(&conn);

    if (conn.connectNonBlocking(MySQLserver, server_port, user, passwd) != RESULT_FAIL)
    {
        delay(500);
        if (conn.connected())
        {
        MYSQL_DISPLAY(SELECT_SQL);
        
        // Execute the query
        // KH, check if valid before fetching
        if ( !query_mem.execute(SELECT_SQL.c_str()) )
        {
            MYSQL_DISPLAY("Select error");
            return;
        }
        else
        {
            // Fetch the columns and print them
            column_names *cols = query_mem.get_columns();

            for (int f = 0; f < cols->num_fields; f++)
            {
                MYSQL_DISPLAY0(cols->fields[f]->name);

                if (f < cols->num_fields - 1)
                {
                MYSQL_DISPLAY0(", ");
                }
            }
            
            MYSQL_DISPLAY();
            
            // Read the rows and print them
            row_values *row = NULL;
            int i = 1;
            do
            {
                row = query_mem.get_next_row();

                if (row != NULL)
                {
                for (int f = 0; f < cols->num_fields; f++)
                {
                    MYSQL_DISPLAY0(row->values[f]);
                    rowValues[i] = row->values[f];
                    if (f < cols->num_fields - 1)
                    {
                    MYSQL_DISPLAY0(", ");
                    }
                }
                
                MYSQL_DISPLAY();
                i++;
                }
            } while (row != NULL);
        }
        // now check whether the ChipID already exists
        bool isInArray = false;
        isInArray = std::find(std::begin(rowValues), std::end(rowValues), ssid32) != std::end(rowValues);

        if (isInArray)
        {
            MYSQL_DISPLAY(UPDATE_SQL);
            if ( !query_mem.execute(UPDATE_SQL.c_str()) )
            {
            MYSQL_DISPLAY("UPDATE error");
            return;
            }
        }
        else
        {
            MYSQL_DISPLAY(INSERT_SQL);
            if ( !query_mem.execute(INSERT_SQL.c_str()) )
            {
            MYSQL_DISPLAY("INSERT error");
            return;
            }

        }
    conn.close();
    }
    else
        {
        MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
        }
        conn.close();
    }
    else 
    {
        MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }
}

void WebClient() {
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
  
  Serial2.begin(BAUD_RATE, SERIAL_MODE2, RX2, TX2);

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("\nFeatures included:\n %s\n %s\n %s\n %s\n %s\n",
      (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded flash" : "",
      (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "2.4GHz WiFi" : "",
      (chip_info.features & CHIP_FEATURE_BLE) ? "Bluetooth LE" : "",
      (chip_info.features & CHIP_FEATURE_BT) ? "Bluetooth Classic" : "",
      (chip_info.features & CHIP_FEATURE_IEEE802154) ? "IEEE 802.15.4" : "");
  Serial.println();

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

  setupOTA();
  
// OTA setup end ####################################

  // start the Webserver
  server.begin();
  // insert or update the board table for the ESP32 bords in the MySQL database
  updateBoardTable(ssidesp32);

// smart meter init begin --------------------------------------------------------------------------------------
  /**
  Serial.printf("start reading ....");
  sprintf ( sendBuffer, "/?!\r\n" );
  sendbytes (sendBuffer, 5 );
  usleep ( 3000000 );	// 300msec
  printf ( "Identification: " );
  receiveBytes ( p_retBuffer );
  printf ( "%s", retBuffer );
  printf ( "\r\n" );
  usleep ( 3000000 );	// 300msec
  printf ( "Sending ACK...   ");
  sprintf ( sendBuffer, "%c040\r\n", 0x06 );
  sendbytes (sendBuffer, strlen ( sendBuffer ) );
  receiveBytes ( p_retBuffer );
  printf ( "Response: %s\r\n", retBuffer );
  **/
}
// setup end =============================================================================================

// loop begin --------------------------------------------------------------------------------------------

void loop() {

// OTA loop ####################################
  ArduinoOTA.handle();
// OTA loop ####################################

  WebClient();
  currentLoopTime = millis();
  if (currentLoopTime - previousLoopTime > writeCycleTime) {
    readSmartMeter();
    previousLoopTime = currentLoopTime;
  }

}
// loop end =============================================================================================
