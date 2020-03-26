#include <Arduino.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPmDNS.h>

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


//const size_t capacity3 = JSON_ARRAY_SIZE(0) + JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 3*JSON_OBJECT_SIZE(1) + 8*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(8) + 2*JSON_OBJECT_SIZE(16) + 2048;
DynamicJsonDocument jsonCalendar(20000);

DynamicJsonDocument jsonCalendarList(8192);

DynamicJsonDocument jsonColor(4096);


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
    delay(1000);
    wm.startConfigPortal(ssid);
  }
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  if (!MDNS.begin("esp32")) {
    Serial.println("Error setting up MDNS responder!");
    while(1){
      delay(1000);
    }
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
            client.println("Content-type:text/html; charset=UTF-8");
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

              client.print("<a href=\"/result\"> Clique ici après avoir autorise le compte pour voir les 10 porchains evenements de ton calendrier principal </a></br>");


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

              // Récupère la liste des agendas


              request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

              
              // Deserialize the JSON document
              DeserializationError error = deserializeJson(jsonCalendarList, request.httpResponse);

              // Test if parsing succeeds.
              if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
              }

              JsonArray arr = jsonCalendarList["items"].as<JsonArray>();
              for (JsonVariant value : arr) {
                client.print("Id : " + value["id"].as<String>() + " = " + value["summary"].as<String>() + ", colorId =  " + value["colorId"].as<String>() + "</br>");
              }


              // Récupère les couleurs


              request = httpGet("https://www.googleapis.com/calendar/v3/colors?access_token=" + access_token);

              // Deserialize the JSON document
              error = deserializeJson(jsonColor, request.httpResponse);

              // Test if parsing succeeds.
              if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
              }



              // Affiche les évènements primaires

              
              client.print("<p> Les evenements </p></br>");
              
              request = httpGet("https://www.googleapis.com/calendar/v3/calendars/primary/events?maxResults=10&orderBy=startTime&singleEvents=True&timeMin=2020-02-19T10:00:00Z&access_token=" + access_token);

              client.print(request.httpResponse);

              // Deserialize the JSON document
              error = deserializeJson(jsonCalendar, request.httpResponse);

              // Test if parsing succeeds.
              if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
                return;
              }

              client.print("</br></br>");

              String colorEvent;

              arr = jsonCalendar["items"].as<JsonArray>();
              for (JsonVariant value : arr) {
                if (value.containsKey("colorId"))
                  colorEvent = jsonColor["event"][value["colorId"].as<String>()]["background"].as<String>();
                else {
                  //colorEvent = jsonColor["calendar"][jsonCalendarList["items"][value["email"].as<String>()]["colorId"].as<String>()].as<String>()]["background"].as<String>();
                }
                client.print("<p style='color: " + colorEvent + "'>" + value["summary"].as<String>() + ", colorId =  " + colorEvent + "</p>");
              }
            
            } else if (header.indexOf("GET /disconnect") >= 0) {
                //client.print("<p> L'ESP redemarre et va demarrer sur le portail pour se connecter de nouveau </p></br>");
                wm.erase();
                ESP.restart();
            } else {
              client.print("<a href=\"/google\"> Se connecter à google </a></br></br>");
              client.print("<a href=\"/disconnect\"> Se déconnecter et redémarrer sur le portail captif </a></br>");
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