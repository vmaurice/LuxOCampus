#ifndef main_h
#define main_h

// my include file
#include "fileJson.h"
#include "google.h"

#include <Arduino.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPmDNS.h>

#include <SPIFFS.h>
#define DATA_JSON "/data.json"

String selectCalendars;
String saveCalendars;


#include <map>
#include <vector>

String localname = "";
String username = "";

int maxResult = 8;

/*	   OTA		*/

#include "ota.h"

/* 	   CSS		*/

#include "style.h"

/*     Web      */

#include <WebServer.h>

WiFiManager wm; // global wm instance
const char *ssid = "ESP32";

WebServer server(80);

//WiFiClient client;
HttpResponse request;

void handleRoot();
void handleResult();
void handleChooseCalendar();
void handleDisconnectGoogle();
void handleDisconnect();
void handleGoogle();
void handleUpdate();
void handleUpdatePost();
void handleHelp();
void handleNotFound();



/*   DateTime   */

// Define time
#include <ctime>
#include "time.h"

// convert time
#include <sstream>
#include <iomanip>

const char *ntpServer = "pool.ntp.org";
//const long gmtOffset_sec = 0;
//const int daylightOffset_sec = 0;
const char *tz_info = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // https://remotemonitoringsystems.ca/time-zone-abbreviations.php


// DateTime instant
time_t now;
time_t dateMin;
time_t dateMax;
char dateTimeChar[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMin[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMax[sizeof "2011-10-08T07:07:09Z"];




/*     LED     */

int delay_shift_color = 10000;

// Led
#include <FastLED.h>

#define DATA_PIN 	23
#define CLOCK_PIN 	18
#define NUM_LEDS    13
#define BRIGHTNESS  255
#define SATURATION 	255
#define LED_TYPE    SK9822
#define COLOR_ORDER BGR

CRGB leds[NUM_LEDS];

long unsigned ledElepsed;


// the event structure
struct event
{
	String name;
	String id;
	String color;
	time_t startDate;
	time_t endDate;
	String stringStartDate;
	String stringEndDate;
	String subCalendarName;
};

// the sub-calendar structure
struct subCalendar
{
	String name;
	String etag;
	std::vector<struct event> listEvents;
};

// List events
std::vector<struct subCalendar> listSubCalendar;

// Define MaxDayInMonth
const int maxDayInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/* Style CSS */

const char* style_font_open_sans = "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\"><link href=\"https://fonts.googleapis.com/css2?family=Open+Sans&display=swap\" rel=\"stylesheet\">";

const char * style_body = "style = ' font-family: open sans; '";

const char * style_h1 = "style= ' color:blue; text-align:center; '";

const char * style_h1_a = "style= ' text-decoration: none; color:inherit; '";

const char * style_table = "style= 'width: 90%; '";

const char * style_table_tr = "style= ' text-align: left; '";


#endif