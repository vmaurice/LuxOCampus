#include <Arduino.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPmDNS.h>

#include <map>
#include <vector>

// Define time
#include <ctime>
#include "time.h"

// convert time
#include <sstream>
#include <iomanip>

const char *ntpServer = "0.fr.pool.ntp.org";
//const long gmtOffset_sec = 0;
//const int daylightOffset_sec = 0;
const char *tz_info = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // https://remotemonitoringsystems.ca/time-zone-abbreviations.php

// my include file
#include "google.h"
#include "fileJson.h"


// Led
#include <FastLED.h>

#define DATA_PIN 	23
#define CLOCK_PIN 	18
#define NUM_LEDS    13
#define BRIGHTNESS  64
#define LED_TYPE    SK9822
#define COLOR_ORDER BGR

CRGB leds[NUM_LEDS];

long unsigned ledElepsed;

// EEPROM
#include <EEPROM.h>
#define EEPROM_SIZE 256

WiFiManager wm; // global wm instance
const char *ssid = "ESP32";

WiFiServer server(80);
String header;

WiFiClient client;

HttpResponse request;

String url;
String access_token = "";
String refresh_token = "";
int expire = 0;

long unsigned startTimeExpire;

// Map between summary calendar and color
std::map<String, String> mapSummaryColor;

// Array list of calendar
std::vector<JsonVariant> arrayCalendarList;

// Vector of indices that reference CalendarList array
std::vector<int> indArray;

// DateTime instant
struct tm dateTime;
struct tm dateMin;
struct tm dateMax;
char dateTimeChar[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMin[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMax[sizeof "2011-10-08T07:07:09Z"];

// the event structure
struct event
{
	String name;
	String id;
	String color;
	struct tm startDate;
	struct tm endDate;
	String subCalendarName;
};

// the sub-calendar structure
struct subCalendar
{
	String name;
	String id;
	std::vector<struct event> listEvents;
};

// List events
std::vector<struct subCalendar> listEvents;

// Define MaxDayInMonth
const int maxDayInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


// Get events
void getEvent()
{
	/*
	if (indArray.empty())
	{
		if (arrayCalendarList.size() == 0)
		{
			request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

			// Deserialize the JSON document and Test if parsing succeeds.
			checkJsonError(deserializeJson(jsonCalendarList, request.httpResponse));

			request.httpResponseCode = -1;

			//arrayCalendarList = jsonCalendarList["items"].as<JsonArray>();

			arrayCalendarList.clear();

			for (JsonVariant value : jsonCalendarList["items"].as<JsonArray>())
			{
				request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=1&access_token=" + access_token);
				if (request.httpResponseCode == 200)
				{
					//Serial.print(value["summary"].as<String>());
					arrayCalendarList.push_back(value);
				}
			}

			//Serial.print(arrayCalendarList.size());

			for (auto value : arrayCalendarList)
				mapSummaryColor[value["summary"].as<String>()] = value["colorId"].as<String>();
		}
		for (int i = 0; i < arrayCalendarList.size(); i++)
		{
			indArray.push_back(i);
		}
		Serial.println("Number of sub-calendar(s) : " + (String)indArray.size());
	}
	*/


	// print the events between 1 day before and 7 days after of calendar selected

	if (!getLocalTime(&dateTime))
	{
		Serial.println("Failed to obtain time");
		return;
	}

	dateMin = dateTime;
	dateMax = dateTime;

	dateMin.tm_mday -= 1;
	dateMax.tm_mday += 7;

	if (dateMin.tm_mday > maxDayInMonth[dateMin.tm_mon]) {
		if (dateMin.tm_mon == 1) {
			dateMin.tm_mday = 31;
			dateMin.tm_mon = 12;
			dateMin.tm_year -= 1;
		} else {
			dateMin.tm_mon -= 1;
			dateMin.tm_mday = maxDayInMonth[dateMin.tm_mon];
		}
	}

	if (dateMax.tm_mday > maxDayInMonth[dateMax.tm_mon]) {
		dateMax.tm_mday = 1;
		if (dateMax.tm_mon == 12) {
			dateMax.tm_mon = 1;
			dateMax.tm_year += 1;
		}	
		else
			dateMax.tm_mon += 1;
	}

	// Convert struct tm to String
	strftime(dateTimeChar, sizeof dateTimeChar, "%FT%TZ", &dateTime);
	strftime(dateTimeMin, sizeof dateTimeMin, "%FT%TZ", &dateMin);
	strftime(dateTimeMax, sizeof dateTimeMax, "%FT%TZ", &dateMax);

	//client.print("<p> Les événements de " + (String)dateTimeMin + " à " + (String)dateTimeMax + "</p></br>");

	Serial.print("Temps actuel : ");
	Serial.println(dateTimeChar);
	//Serial.println(dateTimeMax);

	//auto delta = difftime(mktime(&dateMin), mktime(&dateMax));
	//Serial.println(delta);

	if (indArray.empty())
		return;

	String allEvents = "{ \"Calendar\": [ ";

	for (auto i : indArray)
	{
		auto value = arrayCalendarList[i];
		request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=10&orderBy=startTime&singleEvents=True&timeMin=" + (String)dateTimeMin + "&timeMax=" + (String)dateTimeMax + "&access_token=" + access_token);
		//Serial.println("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=10&orderBy=startTime&singleEvents=True&timeMin=" + (String)dateTimeMin + "&timeMax=" + (String)dateTimeMax + "&access_token=" + access_token);
		if (request.httpResponseCode == 200)
		{
			allEvents += request.httpResponse;
			allEvents += ", ";
		}
		else 
		{
			Serial.println("Error events with : " + value["summary"].as<String>());
			Serial.print(request.httpResponseCode);
			Serial.print(" : ");
			Serial.println(request.httpResponse);
		}
	}

	allEvents = allEvents.substring(0, allEvents.length() - 2);
	allEvents += " ] } ";

	//Serial.print(allEvents);

	String colorEvent;

	// Deserialize the JSON document and Test if parsing succeeds.
	checkJsonError(deserializeJson(jsonCalendar, allEvents));

	listEvents.clear();

	JsonArray arrayCalendar = jsonCalendar["Calendar"].as<JsonArray>();
	for (JsonVariant cal : arrayCalendar)
	{
		JsonArray arr = cal["items"].as<JsonArray>();
		//client.print("<p>" + cal["summary"].as<String>() + " : </p>");
		struct subCalendar subCal;
		subCal.name = cal["summary"].as<String>();
		subCal.id = cal["id"].as<String>();
		for (JsonVariant value : arr)
		{
			if (value.containsKey("colorId"))
				colorEvent = jsonColor["event"][value["colorId"].as<String>()]["background"].as<String>();
			else
			{
				//Serial.println(cal["summary"].as<String>());
				auto cl = mapSummaryColor.find(cal["summary"].as<String>());
				if (cl == mapSummaryColor.end())
					return;
				//Serial.println(cl->first);
				colorEvent = jsonColor["event"][cl->second]["background"].as<String>();
				//Serial.println(colorEvent);
			}
			//client.print("<p style='color: " + colorEvent + "'>" + value["summary"].as<String>() + " de " + value["start"]["dateTime"].as<String>() + " à " + value["end"]["dateTime"].as<String>() + ", colorId =  " + colorEvent + ", id = " + value["id"].as<String>() + "</p>");
			String subCalendarName = "";

			struct event myEvent;
			myEvent.name = value["summary"].as<String>();
			myEvent.id = value["id"].as<String>();
			myEvent.color = colorEvent;

			struct tm eventTime;
			std::istringstream iss(value["start"]["dateTime"].as<char *>());
			iss >> std::get_time(&eventTime, "%Y-%m-%dT%H:%M:%S");
			myEvent.startDate = eventTime;

			std::istringstream iss2(value["end"]["dateTime"].as<char *>());
			iss2 >> std::get_time(&eventTime, "%Y-%m-%dT%H:%M:%S");
			myEvent.endDate = eventTime;

			myEvent.subCalendarName = cal["summary"].as<String>();

			//listEvents.push_back(myEvent);
			subCal.listEvents.push_back(myEvent);
		}
		//client.print("</br>");
		listEvents.push_back(subCal);
	}
}





// elapsed time for reload calendar
unsigned long elapsedTime;

std::vector<struct event> listColorCalendar;


struct event rainbow = {"rainbow"};
//rainbow.name = "rainbow";


void colorCalendar()
{
	if (listEvents.size() == 0)
	{
		Serial.print("Any color (any event)");
	}
	listColorCalendar.clear();
	for (auto subCal : listEvents)
	{
		for (auto value : subCal.listEvents)
		{
			if (difftime(mktime(&dateTime), mktime(&value.startDate)) > 0 && difftime(mktime(&dateTime), mktime(&value.endDate)) < 0)
			{
				listColorCalendar.push_back(value);
			}
		}
	}
	if (listColorCalendar.empty())
	{
		for (auto subCal : listEvents)
		{
			for (auto value : subCal.listEvents)
			{
				if (difftime(mktime(&dateTime), mktime(&value.endDate)) <= 0) {
					struct tm lastTime;
					if (listColorCalendar.empty())
					{
						lastTime = value.startDate;
						listColorCalendar.push_back(value);
					}
					else 
					{
						int dif = difftime(mktime(&value.startDate), mktime(&dateTime)) - difftime(mktime(&lastTime), mktime(&dateTime));
						if (dif == 0)
						{
							listColorCalendar.clear();
							listColorCalendar.push_back(value);
						}
						else if (dif < 0)
						{
							listColorCalendar.clear();
							listColorCalendar.push_back(value);
							lastTime = value.startDate;
						}
					}
				}
			}
		}
	}

	if (listColorCalendar.empty()) 
	{
		listColorCalendar.push_back(rainbow);
	}
}

void setup()
{
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

	std::vector<const char *> menu = {"notice", "wifi", "info", "restart", "exit"};
	wm.setMenu(menu); // custom menu, pass vector

	wm.setClass("invert");

	//wm.setParamsPage(true); // move params to seperate page, not wifi, do not combine with setmenu!

	// set country
	//wm.setCountry("FR"); // setting wifi country seems to improve OSX soft ap connectivity, may help others as well

	// set configrportal timeout
	wm.setConfigPortalTimeout(900);


	if (!wm.autoConnect(ssid))
	{
		delay(1000);
		Serial.println("Failed to connect");
        ESP.restart();
	}


	//if you get here you have connected to the WiFi
	Serial.println("connected...yeey :)");
	if (!MDNS.begin("esp32"))
	{
		Serial.println("Error setting up MDNS responder!");
		return;
	}

	// Init the time
	//configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	//setenv("TZ", tz_info, 1);
	configTzTime(tz_info, ntpServer);

	//start server
	server.begin();
	Serial.println("Server started");

	if (!EEPROM.begin(EEPROM_SIZE))
	{
		Serial.println("failed to initialise EEPROM");
		delay(100000);
	}

	refresh_token = EEPROM.readString(0);
	if (refresh_token != "")
	{
		Serial.println();
		Serial.print("refresh token in EEPROM : ");
		Serial.println(refresh_token);
	}

	FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  	FastLED.setBrightness(  BRIGHTNESS );

	startTimeExpire = millis();
	ledElepsed = millis();
}

String colorInst = "";

void loop()
{
	// put your main code here, to run repeatedly:

	if (millis() - ledElepsed > 1000) {
		if (!listColorCalendar.empty()) {
			//Serial.println(listColorCalendar[0].name);
			if (listColorCalendar[0].name == "rainbow") 
			{
				for (int i = 0; i<NUM_LEDS; i++)
					leds[i] = 0x000000;
			}
			else 
			{
				colorInst = listColorCalendar[0].color;
				//Serial.println(colorInst);
				colorInst = "0x" + colorInst.substring(1,colorInst.length());
				//Serial.println(colorInst);
				unsigned int colorValue;
				std::stringstream sstream;
				sstream << std::hex << colorInst.c_str();
				sstream >> colorValue;
				//Serial.println(colorValue);
				for (int i = 0; i<NUM_LEDS; i++)
					leds[i] = colorValue;
			}
			FastLED.show();
		}
		ledElepsed = millis();
	}
	


	if ((millis() - startTimeExpire > (expire - 120000) && expire != 0) || (expire == 0 && refresh_token != ""))
	{
		Serial.println();
		Serial.println("Refresh token");
		url = "client_id=" + client_id +
			  "&client_secret=" + client_secret +
			  "&refresh_token=" + refresh_token +
			  "&grant_type=refresh_token";

		Serial.println(url);

		request = httpPost("/token", url);

		if (request.httpResponseCode != 200)
		{
			for (int i = 0; i < EEPROM_SIZE; i++)
			{
				EEPROM.write(i, 0);
			}
			EEPROM.commit();
			ESP.restart();
		}

		// Deserialize the JSON document and Test if parsing succeeds.
		checkJsonError(deserializeJson(googleToken, request.httpResponse));

		access_token = googleToken["access_token"].as<String>();
		expire = googleToken["expires_in"].as<int>() * 1000;

		startTimeExpire = millis();

		Serial.print("Token : ");
		Serial.println(access_token);
		Serial.print("New token in ");
		Serial.println(expire);

		if (jsonColor.isNull())
		{
			// get the color

			request = httpGet("https://www.googleapis.com/calendar/v3/colors?access_token=" + access_token);

			// Deserialize the JSON document and Test if parsing succeeds.
			checkJsonError(deserializeJson(jsonColor, request.httpResponse));

			if (request.httpResponseCode != 200)
			{
				Serial.println("Error request colors");
				return;
			}
		}

		getEvent();
		elapsedTime = millis();
	}

	client = server.available(); // listen for incoming clients

	if (millis() - elapsedTime > 1 * 60 * 1000 && access_token != "")
	{
		getEvent();
		colorCalendar();
		elapsedTime = millis();
	}

	if (client)
	{								   // if you get a client,
		Serial.println("New Client."); // print a message out the serial port
		String currentLine = "";	   // make a String to hold incoming data from the client
		while (client.connected())
		{ // loop while the client's connected

			// refresh events calandar
			if (access_token != "")
			{
			}

			if (client.available())
			{							// if there's bytes to read from the client,
				char c = client.read(); // read a byte, then
				Serial.write(c);		// print it out the serial monitor
				header += c;
				if (c == '\n')
				{ // if the byte is a newline character

					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0)
					{
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html; charset=UTF-8");
						client.println();

						client.println("<!DOCTYPE html><html></body>");

						client.print("<h1 style=\"color:blue;text-align:center;\"><a style=\"text-decoration: none;color:inherit;\" href=\"/\">LuxOCampus</a></h1>");

						// Authentification the esp32 with the google account
						if (header.indexOf("GET /google") >= 0)
						{
							// We now create a URI for the request

							if (access_token == "")
							{

								url = "client_id=" + client_id +
									  "&scope=https://www.googleapis.com/auth/calendar.readonly";

								Serial.print("Requesting URL: ");
								Serial.println(url);

								request = httpPost("/device/code", url);

								request.httpResponseCode = -1;

								// Deserialize the JSON document and Test if parsing succeeds.
								checkJsonError(deserializeJson(googleConnect, request.httpResponse));

								url_google = googleConnect["verification_url"].as<String>();
								user_code = googleConnect["user_code"].as<String>();
								device_code = googleConnect["device_code"].as<String>();

								Serial.println(url_google);
								Serial.println(user_code);

								// the content of the HTTP response follows the header:
								client.print("<a target=\"_blank\" href=\"" + url_google + "\"> Copie le code ci dessous puis clique ici pour s'authentifier à Google. </a><br>");
								client.print("<p> code à copier : " + user_code + "</p><br></br>");

								delay(10000);

								url = "client_id=" + client_id +
									  "&client_secret=" + client_secret +
									  "&device_code=" + device_code +
									  "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code";

								Serial.println(url);

								request = httpPost("/token", url);

								while (request.httpResponseCode != 200)
								{
									request = httpPost("/token", url);
									//Serial.println(request.httpResponseCode);
									delay(3000);
								}

								// Deserialize the JSON document and Test if parsing succeeds.
								checkJsonError(deserializeJson(googleToken, request.httpResponse));

								access_token = googleToken["access_token"].as<String>();
								refresh_token = googleToken["refresh_token"].as<String>();
								expire = googleToken["expires_in"].as<int>() * 1000;

								//Serial.println(sizeof(refresh_token.c_str()));

								//EEPROM.writeInt(0, refresh_token.length());

								EEPROM.writeString(0, refresh_token);

								EEPROM.commit();

								startTimeExpire = millis();

								Serial.println(access_token);
								Serial.println(refresh_token);
								Serial.println(expire);

								// get the color

								request = httpGet("https://www.googleapis.com/calendar/v3/colors?access_token=" + access_token);

								// Deserialize the JSON document and Test if parsing succeeds.
								checkJsonError(deserializeJson(jsonColor, request.httpResponse));

								if (request.httpResponseCode != 200)
								{
									Serial.println("Error request colors");
									return;
								}

								client.print("<a href=\"/result\"> Clique ici pour voir les événements </a></br>");
							}
							else
							{
								client.print("<p> Vous êtes bien connecté </p></br>");
							}
							client.print("<a href=\"/\"> Retour page accueil </a></br>");

							// Print the events between 24h before and after of sub-calendars selected. If indArray is empty, all sub-calendars selected.
						}
						else if (header.indexOf("GET /result") >= 0)
						{
							if (access_token == "")
							{
								client.print("<p>Aucun compte google</p>");
							}
							else
							{

								if (header.indexOf("GET /result?") >= 0)
								{
									indArray.clear();
									if (header.indexOf("demo=on") > 0) 
									{
										//Serial.println(">>>> find demo");
										listEvents.clear();
									}
									else 
									{
										for (int i = 0; i < arrayCalendarList.size(); i++)
										{
											if (header.indexOf((String)i + "=on") > 0)
											{
												//client.print(arrayCalendarList[i]["summary"].as<String>() + "</br>");
												indArray.push_back(i);
											}
										}
									}
								}

								
								// print the events between 1 day before and 7 days after of calendar selected
								getEvent();

								client.print("<h2>Informations : </h2>");

								client.print("<p>La dernière mise à jour : " + (String)dateTimeChar + "</p>");
								client.print("<p>Les événements vont du " + (String)dateTimeMin + " à " + (String)dateTimeMax + "</p></br>");

								client.print("<h2>Les sous-calendriers : </h2>");

								if (listEvents.empty()) 
								{
									client.print("<p><a href=\"/choosecalendar\">Aucun sous-calendrier sélectionné, cliquez ici pour les sélectionner.</a></p>");
								}
								else 
								{
									String subCalendarName = "";

									for (auto subCal : listEvents)
									{
										client.print("<h3>" + subCal.name + "</h3>");
										for (auto value : subCal.listEvents)
										{
											char dateTimeMin[sizeof "2011-10-08T07:07:09Z"];
											strftime(dateTimeMin, sizeof dateTimeMin, "%FT%TZ", &value.startDate);
											char dateTimeMax[sizeof "2011-10-08T07:07:09Z"];
											strftime(dateTimeMax, sizeof dateTimeMax, "%FT%TZ", &value.endDate);
											client.print("<p style='color: " + value.color + "'>" + value.name + " de " + dateTimeMin + " à " + dateTimeMax + ", colorId =  " + value.color + ", id = " + value.id + "</p>");
										}
									}
								}


								client.print("</br>");

								colorCalendar();

								client.println("<h2>Prochain événement(s) affiché sur le calendrier : </h2>");
								for (auto value : listColorCalendar)
								{
									if (value.name == "rainbow")
										client.println("<p>Mode démo : " + value.name + "</p>");
									else
										client.println("<p style='color: " + value.color + "'>" + value.name + ", color : " + value.color + "</p>");
								}

								client.print("</br></br>");

								client.print("<p><a href=\"/choosecalendar\">Modifier la sélection des sous calendiers du compte.</a></p>");
							}
							client.print("<a href=\"/\"> Retour page accueil </a></br>");

							// Choose de sub-calendars
						}
						else if (header.indexOf("GET /choosecalendar") >= 0)
						{
							// On choisit les sous calendriers google
							if (access_token == "")
							{
								client.print("<p>Aucun compte google</p>");
								delay(1000);
								return;
							}
							request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

							// Deserialize the JSON document and Test if parsing succeeds.
							checkJsonError(deserializeJson(jsonCalendarList, request.httpResponse));

							client.print("<h2>Les sous-comptes de Google :</h2>");

							client.print("<p>Les calendriers sélectionnés précédemment : ");

							for (auto cal: listEvents) {
								client.print(cal.name + ", ");
							}

							client.print("</p></br></br>");

							client.print("<p>Les sous-calendriers :</p>");

							client.print("<form action=\"/result\" method=\"get\"><div>");

							// mode démo

							client.print("<input type=\"checkbox\" name=\"demo\">");
							client.print("<label for=\"demo\">  Mode démo (n'accepte pas les autres calendriers)</label></br></br>");
								

							//arrayCalendarList = jsonCalendarList["items"].as<JsonArray>();

							//JsonArray arrList = jsonCalendarList["items"].as<JsonArray>();

							arrayCalendarList.clear();

							for (JsonVariant value : jsonCalendarList["items"].as<JsonArray>())
							{
								request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=1&access_token=" + access_token);
								if (request.httpResponseCode == 200)
								{
									//Serial.print(value["summary"].as<String>());
									arrayCalendarList.push_back(value);
								}
							}

							//Serial.print(arrayCalendarList.size());

							int idName = 0;

							for (JsonVariant value : arrayCalendarList)
							{
								//client.print("<p>Summary : " + value["summary"].as<String>() + ", colorId =  " + value["colorId"].as<String>() + "</p>");
								client.print("<input type=\"checkbox\" name=\"" + (String)idName + "\">");
								client.print("<label for=\"" + (String)idName + "\">  " + value["summary"].as<String>() + "</label></br></br>");
								mapSummaryColor[value["summary"].as<String>()] = value["colorId"].as<String>();
								idName += 1;
							}

							client.print("<input type=\"submit\" value=\"Validé\">");

							client.print("</div></form>");

							client.print("</br></br>");

							client.print("<a href=\"/\"> Retour page accueil </a></br>");

							// Disconnect the esp of box and restart
						}
						else if (header.indexOf("GET /disconnect") >= 0)
						{
							//client.print("<p> L'ESP redemarre et va demarrer sur le portail pour se connecter de nouveau </p></br>");
							wm.erase();
							for (int i = 0; i < EEPROM_SIZE; i++)
							{
								EEPROM.write(i, 0);
							}
							EEPROM.commit();
							ESP.restart();

							// Index page
						}
						else
						{
							if (access_token.isEmpty())
								client.print("<p><a href=\"/google\"> Se connecter à google. </a></p></br>");
							else
							{
								client.print("<p><a href=\"/result\"> Les prochains événements. </a></p>");
								client.print("<p><a href=\"/choosecalendar\"> Modifier la sélection des sous calendiers du compte. </a></p></br></br>");
							}
							client.print("<p><a href=\"/disconnect\"> Se déconnecter et redémarrer sur le portail captif. </a></p");
						}

						client.print("</body></html>");

						// The HTTP response ends with another blank line:
						client.println();

						// break out of the while loop:
						break;
					}
					else
					{ // if you got a newline, then clear currentLine:
						currentLine = "";
					}
				}
				else if (c != '\r')
				{					  // if you got anything else but a carriage return character,
					currentLine += c; // add it to the end of the currentLine
				}
			}
		}
		// close the connection:
		header = "";
		client.stop();
		Serial.println("Client Disconnected.");
	}
}