#include "main.h"






#define TIME_REFRESH 10 * 1000

long unsigned startTimeExpire;

// Map between summary calendar and color
std::map<String, String> mapSummaryColor;

// Array list of calendar
std::vector<JsonVariant> arrayCalendarList;

String googleColor;




// Get events
void getEvent()
{
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



	if (selectCalendars.isEmpty())
		return;


	// The list of sub-calendars 
	DynamicJsonDocument jsonCalendarList(jsonCapacityCalendarList);

	checkJsonError(deserializeJson(jsonCalendarList, selectCalendars));

	//Serial.println(jsonCalendarList.as<String>());

	String allEvents = "{ \"Calendar\": [ ";

	for (JsonVariant value : jsonCalendarList["Calendar"].as<JsonArray>())
	{
		//Serial.println(value.as<String>());
		request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=10&orderBy=startTime&singleEvents=True&timeMin=" + (String)dateTimeMin + "&timeMax=" + (String)dateTimeMax + "&access_token=" + access_token);
		
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

	DynamicJsonDocument jsonCalendar(jsonCapacityCalendar);

	// Deserialize the JSON document and Test if parsing succeeds.
	checkJsonError(deserializeJson(jsonCalendar, allEvents));

	listSubCalendar.clear();

	//Serial.println(jsonCalendar.as<String>());

	DynamicJsonDocument jsonColor(jsonCapacityJsonColor);

	checkJsonError(deserializeJson(jsonColor, googleColor));

	JsonArray arrayCalendar = jsonCalendar["Calendar"].as<JsonArray>();
	for (JsonVariant cal : arrayCalendar)
	{
		JsonArray arr = cal["items"].as<JsonArray>();
		struct subCalendar subCal;
		subCal.name = cal["summary"].as<String>();
		subCal.etag = cal["etag"].as<String>();
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
		listSubCalendar.push_back(subCal);
	}

	
}





// elapsed time for reload calendar
unsigned long elapsedTime;

std::vector<struct event> listColorCalendar;


struct event rainbow = {"rainbow"};
//rainbow.name = "rainbow";


void colorCalendar()
{
	listColorCalendar.clear();
	if (listSubCalendar.size() == 0)
	{
		//Serial.println("Any color (mode démo)");
	}
	else 
	{
		for (auto subCal : listSubCalendar)
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
			for (auto subCal : listSubCalendar)
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
								//listColorCalendar.clear();
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
	}
	if (listColorCalendar.empty()) 
	{
		listColorCalendar.push_back(rainbow);
	}
}


// thread for led
TaskHandle_t taskLed;
String colorInst = "";

void ledThread(void * pvParameters) {
	for (;;)
	{
		//Serial.print("ledThread() running on core ");
		//Serial.println(xPortGetCoreID());
		if (!listColorCalendar.empty() && listColorCalendar[0].name != "rainbow") 
		{
			int i = 0;
			while (i < listColorCalendar.size()) {
				//Serial.println(value.name);
				colorInst = listColorCalendar[i].color;
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
				FastLED.show();
				delay(delay_shift_color);
				i++;
			}
		} 
		else
		{
			for (int j = 0; j < 255; j++) {
				for (int i = 0; i < NUM_LEDS; i++) {
				leds[i] = CHSV(i - (j * 2), BRIGHTNESS, SATURATION); /* The higher the value 4 the less fade there is and vice versa */ 
				}
				FastLED.show();
				delay(20); /* Change this to your hearts desire, the lower the value the faster your colors move (and vice versa) */
			}
		}
		yield();
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

	if(!SPIFFS.begin(true)){
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}

	if (SPIFFS.exists(DATA_JSON)) {
		File file = SPIFFS.open(DATA_JSON);
		if(file){
			size_t size = file.size();
			if ( size == 0 ) {
				Serial.println("data file empty");
			} else {
				char buf[size];
				file.readBytes(buf, size);

				StaticJsonDocument<512> jsonDataSave;

				// Deserialize the JSON document and Test if parsing succeeds.
				checkJsonError(deserializeJson(jsonDataSave, buf));

				refresh_token = jsonDataSave["token"].as<String>();
				Serial.print("refresh token in SPIFFS : ");
				Serial.println(refresh_token);

				if (!jsonDataSave["calendars"].isNull())
					saveCalendars = jsonDataSave["calendars"].as<String>();

			}
		} else {
			Serial.print("Can not open");
			Serial.println(DATA_JSON);
			SPIFFS.remove(DATA_JSON);
		}

		file.close();
	
	} else {
		Serial.print("Any");
		Serial.println(DATA_JSON);
	}
	

	FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  	FastLED.setBrightness(  BRIGHTNESS );
	FastLED.setCorrection(0xFFF0F0);

	startTimeExpire = millis();
	
	
	xTaskCreatePinnedToCore(
      ledThread, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &taskLed,  /* Task handle. */
      0); /* Core where the task should run */


	//Serial.print("setup() running on core ");
  	//Serial.println(xPortGetCoreID());
}

void loop()
{
	// put your main code here, to run repeatedly: 
	


	if ((millis() - startTimeExpire > (expire - 120000) && expire != 0) || (expire == 0 && refresh_token != ""))
	{
		bool calSPIFFS= false;


		if (expire == 0 and !saveCalendars.isEmpty())
			calSPIFFS = true;
			
		

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
			if (SPIFFS.exists("/data.json"))
				SPIFFS.remove(DATA_JSON);
			ESP.restart();
		}

		DynamicJsonDocument googleToken(jsonCapacitygoogleConnect);

		// Deserialize the JSON document and Test if parsing succeeds.
		checkJsonError(deserializeJson(googleToken, request.httpResponse));

		access_token = googleToken["access_token"].as<String>();
		expire = googleToken["expires_in"].as<int>() * 1000;

		startTimeExpire = millis();

		Serial.print("Token : ");
		Serial.println(access_token);
		Serial.print("New token in ");
		Serial.println(expire);

		

		if (googleColor.isEmpty())
		{
			// get the color

			//DynamicJsonDocument jsonColor(jsonCapacityJsonColor);

			request = httpGet("https://www.googleapis.com/calendar/v3/colors?access_token=" + access_token);

			
			// Deserialize the JSON document and Test if parsing succeeds.
			//checkJsonError(deserializeJson(jsonColor, request.httpResponse));

			if (request.httpResponseCode != 200)
			{
				Serial.println("Error request colors");
				return;
			}
			
			googleColor = request.httpResponse;
		}

		// associate with calendars data in DATA_JSON
		if (calSPIFFS) {

			Serial.println(">>>>> calendar SPIFFS");

			// list all calendars and check if permission for each calendar

			request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

			DynamicJsonDocument jsonCalendarList(jsonCapacityCalendarList);

			// Deserialize the JSON document and Test if parsing succeeds.
			checkJsonError(deserializeJson(jsonCalendarList, request.httpResponse));

			arrayCalendarList.clear();

			Serial.println(saveCalendars);

			DynamicJsonDocument jsonDataSave(jsonCapacityDataSave);

			checkJsonError(deserializeJson(jsonDataSave, saveCalendars));

			
			selectCalendars = "{ \"Calendar\": [ ";


			for (JsonVariant value : jsonCalendarList["items"].as<JsonArray>())
			{
				request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?maxResults=1&access_token=" + access_token);
				if (request.httpResponseCode == 200)
				{
					//Serial.print(value["summary"].as<String>());
					arrayCalendarList.push_back(value);
					if (jsonDataSave.containsKey(value["summary"].as<String>())) {
						selectCalendars += value.as<String>() + ",";
						mapSummaryColor[value["summary"].as<String>()] = value["colorId"].as<String>();
					}
						
				}
			}

			selectCalendars = selectCalendars.substring(0, selectCalendars.length()-1);
			selectCalendars += " ] }";
			//Serial.println(selectCalendars);
		}

		getEvent();
		elapsedTime = millis();
	}

	client = server.available(); // listen for incoming clients

	if (millis() - elapsedTime > TIME_REFRESH && access_token != "")
	{
		//Serial.print("loop() running on core ");
  		//Serial.println(xPortGetCoreID());
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

						client.println("<!DOCTYPE html><html>");

						client.println("<head><meta charset=\"UTF-8\">");
						client.println("<title>LuxOCampus</title>");

						client.println("</head><body>");

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

								DynamicJsonDocument googleConnect(jsonCapacitygoogleConnect);

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

								DynamicJsonDocument googleToken(jsonCapacityGoogleToken);

								// Deserialize the JSON document and Test if parsing succeeds.
								checkJsonError(deserializeJson(googleToken, request.httpResponse));

								access_token = googleToken["access_token"].as<String>();
								refresh_token = googleToken["refresh_token"].as<String>();
								expire = googleToken["expires_in"].as<int>() * 1000;

								//Serial.println(sizeof(refresh_token.c_str()));


								if (SPIFFS.exists(DATA_JSON))
									SPIFFS.remove(DATA_JSON);

								File file = SPIFFS.open(DATA_JSON, "w+");

								if (!file) {
									Serial.print("Error with");
									Serial.println(DATA_JSON);
									ESP.restart();
								}

								String txt = "{ \"token\": \"" + refresh_token + "\"}";

								DynamicJsonDocument jsonDataSave(jsonCapacityDataSave);

								checkJsonError(deserializeJson(jsonDataSave, txt));

								Serial.println(">>>> token : %s" + jsonDataSave["token"].as<String>());

								if (serializeJson(jsonDataSave, file) == 0) {
									Serial.println(F("Failed to write to file"));
								}

								startTimeExpire = millis();

								Serial.println(access_token);
								Serial.println(refresh_token);
								Serial.println(expire);

								// get the color

								request = httpGet("https://www.googleapis.com/calendar/v3/colors?access_token=" + access_token);

								//DynamicJsonDocument jsonColor(jsonCapacityJsonColor);

								// Deserialize the JSON document and Test if parsing succeeds.
								//checkJsonError(deserializeJson(jsonColor, request.httpResponse));

								if (request.httpResponseCode != 200)
								{
									Serial.println("Error request colors");
									return;
								}

								googleColor = request.httpResponse;

								client.print("<a href=\"/result\"> Clique ici pour voir les événements </a></br>");
							}
							else
							{
								client.print("<p> Vous êtes bien connecté </p></br>");
							}
							client.print("<a href=\"/\"> Retour page accueil </a></br>");

							// Print the events between 24h before and after of sub-calendars selected. If selectCalendar is empty, all sub-calendars selected.
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
									if (header.indexOf("demo=on") > 0) 
									{
										//Serial.println(">>>> find demo");
										listSubCalendar.clear();
										listColorCalendar.clear();
										listColorCalendar.push_back(rainbow);
										selectCalendars.clear();
									}
									else 
									{
										String txtJsonData = "{ \"token\": \"" + refresh_token + "\", \"calendars\" : {";

										selectCalendars = "{ \"Calendar\": [ ";
										for (int i = 0; i < arrayCalendarList.size(); i++)
										{
											if (header.indexOf((String)i + "=on") > 0)
											{
												//client.print(arrayCalendarList[i]["summary"].as<String>() + "</br>");
												selectCalendars += arrayCalendarList[i].as<String>() + ",";
												txtJsonData += "\"" + arrayCalendarList[i]["summary"].as<String>() + "\" : " + arrayCalendarList[i]["etag"].as<String>() + ",";
											}
										}
										selectCalendars = selectCalendars.substring(0, selectCalendars.length()-1);
										selectCalendars += " ] }";
										//Serial.println(selectCalendars);
										
										txtJsonData = txtJsonData.substring(0, txtJsonData.length() - 1);
										txtJsonData += "} }";

										Serial.println(txtJsonData);
										//Serial.println(txtJsonData.length());

										StaticJsonDocument<512> jsonDataSave;

										checkJsonError(deserializeJson(jsonDataSave, txtJsonData));

										if (SPIFFS.exists(DATA_JSON))
											SPIFFS.remove(DATA_JSON);

										File file = SPIFFS.open(DATA_JSON, "w+");

										if (serializeJson(jsonDataSave, file) == 0) {
											Serial.println(F("Failed to write to file"));
										}

										file.close();
									}
								}

								
								// print the events between 1 day before and 7 days after of calendar selected
								getEvent();

								client.print("<h2>Informations : </h2>");

								client.print("<p>La dernière mise à jour : " + (String)dateTimeChar + "</p>");
								client.print("<p>Les événements vont du " + (String)dateTimeMin + " à " + (String)dateTimeMax + "</p></br>");

								client.print("<h2>Les sous-calendriers : </h2>");

								if (listSubCalendar.empty()) 
								{
									client.print("<p><a href=\"/choosecalendar\">Aucun sous-calendrier sélectionné, cliquez ici pour les sélectionner.</a></p>");
								}
								else 
								{
									String subCalendarName = "";

									for (auto subCal : listSubCalendar)
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

						}
						
						
						// Choose sub calendars
						else if (header.indexOf("GET /choosecalendar") >= 0)
						{
							// On choisit les sous calendriers google
							if (access_token == "")
							{
								client.print("<p>Aucun compte google</p>");
								delay(1000);
								return;
							}

							client.print("<h2>Les sous-comptes de Google :</h2>");

							client.print("<p>Les calendriers sélectionnés précédemment : ");

							for (auto cal: listSubCalendar) {
								client.print(cal.name + ", ");
							}

							client.print("</p></br></br>");

							client.print("<p>Les sous-calendriers :</p>");

							client.print("<form action=\"/result\" method=\"get\"><div>");

							// mode démo

							client.print("<input type=\"checkbox\" name=\"demo\">");
							client.print("<label for=\"demo\">  Mode démo (n'accepte pas les autres calendriers)</label></br></br>");
							


							// list all calendars and check if permission for each calendar

							request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

							DynamicJsonDocument jsonCalendarList(jsonCapacityCalendarList);

							// Deserialize the JSON document and Test if parsing succeeds.
							checkJsonError(deserializeJson(jsonCalendarList, request.httpResponse));

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
							if (SPIFFS.exists(DATA_JSON))
								SPIFFS.remove(DATA_JSON);
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