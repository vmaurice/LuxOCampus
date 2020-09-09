
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

#include <ArduinoJson.h>

#include <google_credentials.h>

String url_google;
String user_code;
String device_code;

struct HttpResponse
{
  int httpResponseCode;
  String httpResponse;
};

HTTPClient http;

// Make a request with POST HTTP protocol
HttpResponse httpPost(String host, String url)
{
  HttpResponse request;

  //http.setTimeout(1000);

  request.httpResponseCode = 0;
  int n = 0;

  while (request.httpResponseCode <= 0 && n < 3) {
    http.begin("https://oauth2.googleapis.com" + host);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    request.httpResponseCode = http.POST(url);


    if (request.httpResponseCode < 0) {
      Serial.print("Error on sending POST: ");
      Serial.println(request.httpResponseCode);

      delay(1000);
    } else {
      request.httpResponse = http.getString(); //Get the response to the request
    }

    http.end();

    n++;

    //Serial.println(n);
  }

  return request;
}

// Make a request with GET HTTP protocol
HttpResponse httpGet(String url)
{
  HttpResponse request;

  request.httpResponseCode = 0;
  int n = 0;

  while (request.httpResponseCode <= 0 && n < 3) {

    http.begin(url);

    request.httpResponseCode = http.GET();

    if (request.httpResponseCode < 0){
      Serial.print("Error on sending GET: ");
      Serial.println(request.httpResponseCode);
      
      delay(1000);
    }
    else {
      request.httpResponse = http.getString(); //Get the response to the request
    }

    http.end();

    n++;
  }

  //Serial.println(request.httpResponseCode);   //Print return code
  //Serial.println(request.httpResponse);

  return request;
}

/*
 La méthode n'est pas utilisé
 mais je la laisse pour le moment car elle est détaille le processus de d'authentification
*/
void setup_google(WiFiServer server, WiFiClient client)
{
  HTTPClient http;

  // We now create a URI for the request
  String url = "client_id=" + client_id +
               "&scope=https://www.googleapis.com/auth/calendar.readonly";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  http.begin("https://oauth2.googleapis.com/device/code");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(url);

  String response;

  if (httpResponseCode > 0)
  {
    response = http.getString(); //Get the response to the request

    Serial.println(httpResponseCode); //Print return code
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  const size_t capacity = JSON_OBJECT_SIZE(5) + 220;
  DynamicJsonDocument doc(capacity);

  DeserializationError error = deserializeJson(doc, response);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  http.end();

  Serial.println(doc["verification_url"].as<char *>());
  Serial.println(doc["user_code"].as<char *>());
  url_google = doc["verification_url"].as<String>();
  device_code = doc["device_code"].as<String>();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.print("<a href=\"" + url_google + "\"> Se connecter a Google. </a><br>");
  client.print("<p> code a copier : " + device_code + "</p><br>");

  // The HTTP response ends with another blank line:
  client.println();

  httpResponseCode = -1;

  url = "client_id=" + client_id +
        "&client_secret=" + client_secret +
        "&device_code=" + device_code +
        "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code";

  Serial.println(url);

  while (httpResponseCode != 200)
  {
    http.begin("https://oauth2.googleapis.com/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    httpResponseCode = http.POST(url);

    //Serial.println(httpResponseCode);

    delay(5000);
  }

  if (httpResponseCode > 0)
  {
    response = http.getString(); //Get the response to the request

    Serial.println(httpResponseCode); //Print return code
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  const size_t capacity2 = JSON_OBJECT_SIZE(5) + 400;
  DynamicJsonDocument doc2(capacity2);

  error = deserializeJson(doc2, response);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  String access_token = doc2["access_token"].as<String>();

  Serial.println(access_token);

  url = "?access_token=" + access_token;

  http.begin("https://www.googleapis.com/calendar/v3/users/me/calendarList/primary" + url);

  httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    response = http.getString(); //Get the response to the request

    Serial.println(httpResponseCode); //Print return code
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}