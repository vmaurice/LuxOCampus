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
	// print the events between half day before and 7 days after of calendar selected

	now = time(NULL);

	dateMin = now - 3600 * 12;
	dateMax = now + 3600 * 24 * 15;


	// Convert struct tm to String
	strftime(dateTimeChar, sizeof dateTimeChar, "%FT%TZ", localtime(&now));
	strftime(dateTimeMin, sizeof dateTimeMin, "%FT%TZ", localtime(&dateMin));
	strftime(dateTimeMax, sizeof dateTimeMax, "%FT%TZ", localtime(&dateMax));

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
		request = httpGet("https://www.googleapis.com/calendar/v3/calendars/" + value["id"].as<String>() + "/events?&maxResults=" + maxResult + "&orderBy=startTime&singleEvents=True&timeMin=" + (String)dateTimeMin + "&timeMax=" + (String)dateTimeMax + "&access_token=" + access_token);
		
		if (request.httpResponseCode == 200)
		{
			if (value["summary"] == value["id"])
				username = value["summary"].as<String>();
			allEvents += request.httpResponse;
			allEvents += ", ";
		}
		
		else 
		{
			Serial.println("Error events with : " + value["summary"].as<String>());
			Serial.print(request.httpResponseCode);
			Serial.print(" : ");
			Serial.println(request.httpResponse);
			if (request.httpResponseCode < 0)
				ESP.restart();
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
			myEvent.startDate = mktime(&eventTime);
			myEvent.stringStartDate = value["start"]["dateTime"].as<String>();

			std::istringstream iss2(value["end"]["dateTime"].as<char *>());
			iss2 >> std::get_time(&eventTime, "%Y-%m-%dT%H:%M:%S");
			myEvent.endDate = mktime(&eventTime);
			myEvent.stringEndDate = value["end"]["dateTime"].as<String>();

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
			//Serial.println(now);
			for (auto value : subCal.listEvents)
			{
				//Serial.print(value.name + " : ");
				//Serial.print(value.startDate);
				//Serial.print(" : ");
				//Serial.println(value.endDate);
				if (now >= value.startDate && now <= value.endDate)
				{
					listColorCalendar.push_back(value);
					//Serial.println(value.name);
				}
			}
		}

		//Serial.println("");
		//Serial.println("");

		if (listColorCalendar.empty())
		{
			time_t lastTime = now;
			for (auto subCal : listSubCalendar)
			{
				for (auto value : subCal.listEvents)
				{
					if (now < value.endDate) {
						if (listColorCalendar.empty())
						{
							lastTime = value.startDate;
							listColorCalendar.push_back(value);
						}
						else 
						{
							int dif = (value.startDate - now) - (lastTime - now);
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

	localname = "luxocampus-";
	
	String mac = WiFi.macAddress();

	localname += mac.substring(12,14);
	localname += mac.substring(15,17);

	Serial.print("MAC address : ");
	Serial.println(mac);
	Serial.print("local address : ");
	Serial.println(localname.c_str());

	if (!MDNS.begin(localname.c_str()))
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

	server.on("/", handleRoot);
	server.on("/result", handleResult);
	server.on("/choosecalendar", handleChooseCalendar);
	server.on("/disconnect", handleDisconnect);
	server.on("/disconnect_google", handleDisconnectGoogle);
	server.on("/google", handleGoogle);
	server.on("/help", handleHelp);
	server.on("/update", handleUpdate);
	server.on("/update_post", HTTP_POST, []() {
    	server.sendHeader("Connection", "close");
		server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
		}, handleUpdatePost);

	server.onNotFound(handleNotFound);

	server.begin();
}

void loop()
{
	// put your main code here, to run repeatedly: 
	
	server.handleClient();


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
					if (value["summary"] == value["id"])
						username = value["summary"].as<String>();
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
		colorCalendar();
		elapsedTime = millis();
	}

	if (millis() - elapsedTime > TIME_REFRESH && access_token != "")
	{
		//Serial.print("loop() running on core ");
  		//Serial.println(xPortGetCoreID());
		getEvent();
		colorCalendar();
		elapsedTime = millis();
	}

	/*

						

						


	*/

	

}
// "HTTP/1.1 200 OK \ Content-type:text/html; charset=UTF-8

String pageStart = "<!DOCTYPE html><html> \
					<head><meta charset=\"UTF-8\"> \
					<title>LuxOCampus</title> \
					" + (String)style_font_open_sans + " \
					</head><body " + (String)style_body + "> \
					<h1 " + (String)style_h1 + "><a " + (String)style_h1_a + " href=\"/\">LuxOCampus</a></h1>";

String pageEnd = "</body></html>";

/*
 * Index page
 */
void handleRoot() 
{
	String page = pageStart;
    if (access_token.isEmpty())
        page += "<p><a href=\"/google\">Cliquez-ici pour vous connectez avec votre compte Google.</a></p></br>";
    else
    {
        page += "<h2>Votre compte Google est " + username + "</h2></br>";
        
        // display current event
        if (listColorCalendar.size() > 1) 
            page += "<h3>Les prochains événements affichés sont : </h3>";
        else
        	page += "<h3>Le prochain événement affiché est : </h3>";



        if (listColorCalendar.size() == 0 or listColorCalendar[0].name == "rainbow") {
            page += "<p>Mode démo (Rainbow)</p>";
        }
        else {
            page += "<table " + (String)style_table + ">";
            page += "<tr " + (String)style_table_tr + "><th>Nom</th><th>Début</th><th>Fin</th><th>Couleur</th><th>Id</th></tr>";

            for (auto value : listColorCalendar)
            {
                page += "<tr style='color: " + value.color + "'><td>" + value.name + "</td><td>" + value.stringStartDate + "</td><td>" + value.stringEndDate + "</td><td>" + value.color + "</td><td>" + value.id + "</td></tr>";
            }
            page += "</table>";

        }

        page += "</br></br>";


        page += "<p><a href=\"/help\"> Aide. </a></p>";
        page += "<p><a href=\"/update\"> Update. </a></p>";
        page += "<p><a href=\"/result\"> Les prochains événements. </a></p>";
        page += "<p><a href=\"/choosecalendar\"> Modifier la sélection des sous calendiers du compte. </a></p></br></br>";
        page += "<p><a href=\"/disconnect_google\"> Se déconnecter du compte google.</a></p>";
    }
    page += "<p><a href=\"/disconnect\"> Se déconnecter de la borne wifi et redémarrer sur le portail captif. </a></p";

	page += pageEnd;

	server.send(200, "text/html", page);
}


/*
 * Result displayed
 */
void handleResult()
{
	String page = pageStart;
	
	if (access_token == "")
	{
		page +="<p>Aucun compte google.</p>";
	}
	else
	{

		if (server.args() > 0)
		{
			if (server.hasArg("demo") and server.arg("demo") == "on") 
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
					if (server.hasArg((String)i) and server.arg((String)i) == "on")
					{
						//page +=arrayCalendarList[i]["summary"].as<String>() + "</br>");
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

		page +="<h2>Informations : </h2>";

		page +="<p>Connecté avec le compte google : " + username + "</p>";

		page +="<p>La dernière mise à jour le " + (String)dateTimeChar + "</p>";
		page +="<p>Les événements vont du " + (String)dateTimeMin + " à " + (String)dateTimeMax + "</p></br>";

		page +="</br>";

		colorCalendar();

		if (listColorCalendar.size() > 1) 
			page += "<h2>Les prochains événements affichés sont : </h2>";
		else
			page += "<h2>Le prochain événement affiché est : </h2>";



		if (listColorCalendar.size() == 0 or listColorCalendar[0].name == "rainbow") {
			page += "<p>Mode démo (Rainbow)</p>";
		}
		else {
			page +="<table " + (String)style_table + ">";
			page +="<tr " + (String)style_table_tr + "><th>Nom</th><th>Début</th><th>Fin</th><th>Couleur</th><th>Id</th></tr>";

			for (auto value : listColorCalendar)
			{
				page +="<tr style='color: " + value.color + "'><td>" + value.name + "</td><td>" + value.stringStartDate + "</td><td>" + value.stringEndDate + "</td><td>" + value.color + "</td><td>" + value.id + "</td></tr>";
			}
			page +="</table>";

		}

		page +="</br>";
		


		page +="<h2>L'ensemble des résultats pour chaque calendier sélectionné (max " + (String)maxResult + ") : </h2>";

		if (listSubCalendar.empty()) 
		{
			page +="<p><a href=\"/choosecalendar\">Aucun sous-calendrier sélectionné, cliquez ici pour les sélectionner.</a></p>";
		}
		else 
		{
			String subCalendarName = "";

			for (auto subCal : listSubCalendar)
			{
				page +="<h3>" + username + "/" + subCal.name + " : </h3>";

				page +="<table " + (String)style_table + ">";
				page +="<tr " + (String)style_table_tr + "><th>Nom</th><th>Début</th><th>Fin</th><th>Couleur</th><th>Id</th></tr>";

				for (auto value : subCal.listEvents)
				{
					//char dateTimeMin[sizeof "2011-10-08T07:07:09Z"];
					//strftime(dateTimeMin, sizeof dateTimeMin, "%FT%TZ", &value.startDate);
					//char dateTimeMax[sizeof "2011-10-08T07:07:09Z"];
					//strftime(dateTimeMax, sizeof dateTimeMax, "%FT%TZ", &value.endDate);
					//page +="<p style='color: " + value.color + "'>" + value.name + " de " + dateTimeMin + " à " + dateTimeMax + ", colorId =  " + value.color + ", id = " + value.id + "</p>";
					page +="<tr style='color: " + value.color + "'><td>" + value.name + "</td><td>" + value.stringStartDate + "</td><td>" + value.stringEndDate + "</td><td>" + value.color + "</td><td>" + value.id + "</td></tr>";
				}

				page +="</table>";
			}
		}


		

		page +="</br></br>";

		page +="<p><a href=\"/choosecalendar\">Modifier la sélection des sous calendiers du compte.</a></p>";
	}
	page +="<a href=\"/\"> Retour page accueil </a></br>";

	page += pageEnd;

	server.send(200, "text/html", page);
}

/*
 * Choose sub-calendars
 */
void handleChooseCalendar() 
{
	String page = pageStart;

	// On choisit les sous calendriers google
	if (access_token == "")
	{
		page +="<p>Aucun compte google</p>";
		delay(1000);
		return;
	}

	page +="<h2>Les sous-comptes de Google :</h2>";

	page +="<p>Les calendriers sélectionnés précédemment : ";

	for (auto cal: listSubCalendar) {
		page +=cal.name + ", ";
	}

	page +="</p></br></br>";

	page +="<p>Les sous-calendriers :</p>";

	page +="<form action=\"/result\" method=\"get\"><div>";

	// mode démo

	page +="<input type=\"checkbox\" name=\"demo\">";
	page +="<label for=\"demo\">  Mode démo (n'accepte pas les autres calendriers)</label></br></br>";
	


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
			if (value["summary"] == value["id"])
				username = value["summary"].as<String>();
			//Serial.print(value["summary"].as<String>());
			arrayCalendarList.push_back(value);
		}
	}

	//Serial.print(arrayCalendarList.size());

	int idName = 0;

	for (JsonVariant value : arrayCalendarList)
	{
		//page +="<p>Summary : " + value["summary"].as<String>() + ", colorId =  " + value["colorId"].as<String>() + "</p>");
		page +="<input type=\"checkbox\" name=\"" + (String)idName + "\">";
		page +="<label for=\"" + (String)idName + "\">  " + value["summary"].as<String>() + "</label></br></br>";
		mapSummaryColor[value["summary"].as<String>()] = value["colorId"].as<String>();
		idName += 1;
	}

	page +="<input type=\"submit\" value=\"Validé\">";

	page +="</div></form>";

	page +="</br></br>";

	page +="<a href=\"/\"> Retour page accueil </a></br>";

	page += pageEnd;

	server.send(200, "text/html", page);
}


/*
 * Disconnect of google account
 */
void handleDisconnectGoogle() 
{
	String page = pageStart;
	if (SPIFFS.exists(DATA_JSON))
		SPIFFS.remove(DATA_JSON);
	access_token.clear();
	listSubCalendar.clear();
	listColorCalendar.clear();
	listColorCalendar.push_back(rainbow);
	selectCalendars.clear();
	page += "<p>Le compte Google a bien été enlevé.</p>";
	page += "<p><a href=\"/\"> Retour page accueil </a></p></br>";

	username = "";
	
	page += pageEnd;
	server.send(200, "text/html", page);
}


/*
 * Disconnect the esp of box and restart
 */
void handleDisconnect() 
{
	//Serial.println("flash esp32 and reboot");
	wm.erase();
	if (SPIFFS.exists(DATA_JSON))
		SPIFFS.remove(DATA_JSON);
	ESP.restart();
}


/* 
 * Connect to google
 */
void handleGoogle()
{

	String page = pageStart;

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
		page += "<p>Pour vous connectez, menissez-vous de votre compte Google, du code à copier (ci-dessous) afin d'autoriser l'appareillage de votre compte Google et du calendrier connecté. Suivez les étapes de Google. Google n'a pas validé notre application étant donnée que c'est un petit projet. Il n'y a aucun risque. Autorisez l'accès au projet luxOCampus et vous serez connectés. Aucune donnée n'est collectée ou utilisée à des fins commerciales. Nous respectons votre vie privée.</p>";

		page += "<a target=\"_blank\" href=\"" + url_google + "\">Copiez le code ci-dessous puis cliquez-ici pour s'authentifier à Google. </a><br>";
		page += "<p>code à copier : " + user_code + "</p><br></br>";

		server.send(100, "text/html", page);

		page = "";

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

		// get nname google account

		request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);

		while (request.httpResponseCode != 200) {
			delay(2000);
			request = httpGet("https://www.googleapis.com/calendar/v3/users/me/calendarList?access_token=" + access_token);
		}
			
		DynamicJsonDocument jsonCalendarList(jsonCapacityCalendarList);

		// Deserialize the JSON document and Test if parsing succeeds.
		checkJsonError(deserializeJson(jsonCalendarList, request.httpResponse));

		arrayCalendarList.clear();

		for (JsonVariant value : jsonCalendarList["items"].as<JsonArray>())
		{
			if (value["summary"] == value["id"]) {
				username = value["summary"].as<String>();
				break;
			}
		}



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

		page += "<p>Vous êtes connectés avec votre compte : " + username + ".<p>";
		page += "<a href=\"/result\">Cliquez-ici pour voir les prochains événements.</a></br>";
	}
	else
	{
		page += "<p> Vous êtes bien connecté </p></br>";
	}
	page += "<a href=\"/\"> Retour page accueil </a></br>";

	page += pageEnd;
	server.send(200, "text/html", page);
}


/*
 * Update
 */
void handleUpdate()
{
	Serial.println("Update page");
	String page = pageStart;
	page += "<p>Version actuelle : " + (String)version + "</p>";
	page += updateServer;
	page += "<p><a href=\"/\"> Retour page accueil </a></p></br>";
	
	page += pageEnd;
	server.send(200, "text/html", page);
}

void handleUpdatePost() {

	//Serial.println("Update Post page");

	HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } else {
		Serial.println("Error update");
	}
}

/*
 * Notice page
 */
void handleHelp()
{
	String page = pageStart;

	page += "<h3>Aides</h3>";

	if (access_token.isEmpty()) {
		page += "<p>Vous n'êtes pas connectés</p>";
	} else {
		page += "<ul>";

		page += "<li>Vous êtes actuellement connectés avec votre compte : " + username + ".</li>";
		page += "<li>Pour changer de compte ou vous déconnectez, <a href=\"/disconnect_google\">cliquez-ici</a>.</li>";
		page += "<li>Pour supprimez les sous-calendriers et passer au mode démo (rainbow), <a href=\"result?demo=on\">cliquez-ici</a>.</li>";
		page += "<li>Pour un problème mineur, débrancher et rebrancher la calendrier luxOCampus.</li>";
		page += "<li>Si le problème persiste, réinitialiser le calendrier luxOCampus. Notez-bien que le calendrier ne sera plus connecté à votre box Internet ni à votre compte Google. Il faudra depuis un téléphone ou un ordinateur vous connecter en WiFi sur le calendrier. Vous retrouverez le calendrier sous le nom de \"" + localname.substring(0,15) + "\". Une fois connecté, une page web s'ouvre pour paramétrer le calendrier mais si ce n'est pas le cas. Ouvrez un navigateur web et entrez l'adresse IP \"192.168.4.1\" dans la barre URL afin de pouvoir paramétrer le calendrier à votre box. La suite des procédures sont expliquées dans cette page, onglet Notice. Pour réinitialiser, <a href=\"/disconnect\">cliquez-ici</a>.</li>";

		page += "</ul>";
	}

	page += "<h3>Informations</h3>";

	page += "<ul>";

	page += "<li>URL : http://" + localname + ".local/</li>";
	page += "<li>Adresse IP : " + WiFi.localIP().toString() + "</li>";
	page += "<li>Adresse MAC : " + WiFi.macAddress() +  "</li>";
	page += "<li>Version système : " + (String)version + "</li>";

	page += "</ul>";

	page += "<a href=\"/\"> Retour page accueil </a></br>";

	page += pageEnd;
	server.send(200, "text/html", page);
}

/*
 * Handle Not Found, error 404
 */
void handleNotFound()
{
	String page = pageStart;

	page += "<h3>Page non trouvé, (erreur 404)</h3></br>";

	page += "<a href=\"/\"> Retour page accueil </a>";

	page += pageEnd;
	server.send(404, "text/html", page);
}