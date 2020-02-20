#include <Arduino.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

WiFiManager wm; // global wm instance
const char* ssid = "ESP32";

// OPTION FLAGS
bool TEST_CP  = true; // always start the configportal, even if ap found
bool TEST_NET = true; // do a network test after connect, (gets ntp time)



#include "google.h"


WiFiServer server(80);
String header;


HttpResponse request;
String url;
String access_token = "";

// In JsonFile
const size_t capacity = JSON_OBJECT_SIZE(5) + 220;
DynamicJsonDocument jsonConnect(capacity);

const size_t capacity2 = JSON_OBJECT_SIZE(5) + 400;
DynamicJsonDocument jsonToken(capacity2);


void setup() {
  // put your setup code here, to run once:

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Serial1.begin(115200);

  // Serial.setDebugOutput(true);  
  delay(1000);
  // Serial1.println("TXD1 Enabled");

  Serial.println("\n Starting");
  // WiFi.setSleepMode(WIFI_NONE_SLEEP); // disable sleep, can improve ap stability
  

  std::vector<const char *> menu = {"notice","wifi","info","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector


  wm.setClass("invert");

  //wm.setParamsPage(true); // move params to seperate page, not wifi, do not combine with setmenu!

  // set country
  //wm.setCountry("FR"); // setting wifi country seems to improve OSX soft ap connectivity, may help others as well
  

  // set configrportal timeout
  wm.setConfigPortalTimeout(300);
  


  if(!wm.autoConnect(ssid)) {
    Serial.println("failed to connect and hit timeout");
  }
  else if(TEST_CP) {
    delay(1000);
    Serial.println("TEST_CP ENABLED");
    // start configportal always
    wm.setConfigPortalTimeout(180);
    wm.startConfigPortal(ssid);
  }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
  }

  server.begin();

  Serial.println("Server started");



}

void loop() {
  // put your main code here, to run repeatedly:


  WiFiClient client = server.available();   // listen for incoming clients



  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
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
            client.println();

            if (header.indexOf("GET /google") >= 0) {
              // We now create a URI for the request
              url = "client_id=" + client_id + 
              "&scope=https://www.googleapis.com/auth/calendar.readonly";

              Serial.print("Requesting URL: ");
              Serial.println(url);

              request = httpPost("/device/code", url);

              
              
              DeserializationError error = deserializeJson(jsonConnect, request.httpResponse);
              if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
              }

              Serial.println(jsonConnect["verification_url"].as<char*>());
              Serial.println(jsonConnect["user_code"].as<char*>());
              url_google = jsonConnect["verification_url"].as<String>();
              user_code = jsonConnect["user_code"].as<String>();
              device_code = jsonConnect["device_code"].as<String>();


              // the content of the HTTP response follows the header:
              client.print("<a target=\"_blank\" href=\"" + url_google + "\"> Se connecter a Google. </a><br>");
              client.print("<p> code a copier : " + user_code + "</p><br></br>");

              client.print("<a href=\"/result\"> Resultat </a></br>");


            } else if (header.indexOf("GET /result") >= 0) {
              if (access_token == "") {
              
                request.httpResponseCode = -1;
      
                url = "client_id=" + client_id + 
                "&client_secret=" + client_secret +
                "&device_code=" + device_code +
                "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code";

                Serial.println(url);

                while (request.httpResponseCode != 200) {
                  request = httpPost("/token", url);

                  //Serial.println(request.httpResponseCode);

                  delay(5000);
                }

                DeserializationError error = deserializeJson(jsonToken, request.httpResponse);
                if (error) {
                  Serial.print(F("deserializeJson() failed: "));
                  Serial.println(error.c_str());
                  return;
                }

                access_token = jsonToken["access_token"].as<String>();

                Serial.println(access_token);
              }

              request = httpGet("https://www.googleapis.com/calendar/v3/calendars/primary/events?maxResults=10&orderBy=startTime&singleEvents=True&timeMin=2020-02-19T10:00:00Z&access_token=" + access_token);

              client.print("<p> Les evenements </p></br>");
              client.print(request.httpResponse);
            
            } else {
              client.print("<a href=\"/google\"> Se connecter à google </a></br>");
            }

            
            
            // The HTTP response ends with another blank line:
            client.println();

            // break out of the while loop:
            break;

        } else {    // if you got a newline, then clear currentLine:
          currentLine = "";
        }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    header = "";
    client.stop();
    Serial.println("Client Disconnected.");
  }

}