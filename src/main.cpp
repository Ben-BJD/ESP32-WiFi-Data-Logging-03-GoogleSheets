#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

#include <ESP_Google_Sheet_Client.h>

#define WIFI_SSID "<WIFI_SSID_NAME>"
#define WIFI_PASSWORD "<WIFI_PASSWORD>"

#define PROJECT_ID "<PROJECT_ID>" //Google Cloud Project ID

//Service Account's client email
#define CLIENT_EMAIL "<CLIENT_EMAIL>"

#define SPREADSHEET_ID "<SPREADSHEET_ID>" //Google Sheets ID

//Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----.....-----END PRIVATE KEY-----\n";

bool taskComplete = false;

//helper function to format date and time
void formatDateTime(const char* dateTimeFormat,char* result,size_t resultSize)
{
  struct tm timeinfo;
  if( getLocalTime(&timeinfo) )
  {
    strftime(result,resultSize, dateTimeFormat, &timeinfo);
  }
}

void setup()
{
    Serial.begin(9600);

    // Set auto reconnect WiFi or network connection
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    //Let's set the time via NTP so we can have a valid time stamp to log in Google Sheets
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 0;
    const int   daylightOffset_sec = 3600;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    //Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    // In case SD/SD_MMC storage file access, mount the SD/SD_MMC card.
    // SD_Card_Mounting(); // See src/GS_SDHelper.h

    // Or begin with the Service Account JSON file that uploaded to the Filesystem image or stored in SD memory card.
    // GSheet.begin("path/to/serviceaccount/json/file", esp_google_sheet_file_storage_type_flash /* or esp_google_sheet_file_storage_type_sd */);
}


void loop()
{
    //Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();

    if (ready && !taskComplete)
    {
        //For basic FirebaseJson usage example, see examples/FirebaseJson/Create_Edit_Parse/Create_Edit_Parse.ino

        FirebaseJson response;
        // Instead of using FirebaseJson for response, you can use String for response to the functions 
        // especially in low memory devices that deserializing large JSON responses may fail due to lack of memory.

        Serial.println("Update spreadsheet...");
        Serial.println("---------------------------------------------------------------");
        
        //here we format the current date and time and record it as a variable 'theTime'
        const char* dateFormat = "%Y-%m-%d %H:%M:%S";
        size_t bufferSize = 20;
        char theTime[bufferSize];
        formatDateTime(dateFormat,theTime,bufferSize);

        // See https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
        // for more information about the parameters of the append function.
        FirebaseJson valueRange;

        //here we set the data for the columns A and B of the next available row
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", theTime);//current timestamp
        valueRange.set("values/[1]/[0]", random(0, 1000) );//we will set a random number for column B

        bool success = GSheet.values.append(&response, SPREADSHEET_ID,"Sheet1!A1:C1", &valueRange);
        
        response.toString(Serial, true);
        Serial.println();

        taskComplete = true;
    }
}
