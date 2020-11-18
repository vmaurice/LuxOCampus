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

/*     Web      */

WiFiManager wm; // global wm instance
const char *ssid = "ESP32";

WiFiServer server(80);
String header;

WiFiClient client;
HttpResponse request;



/*   DateTime   */

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


// DateTime instant
struct tm dateTime;
struct tm dateMin;
struct tm dateMax;
char dateTimeChar[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMin[sizeof "2011-10-08T07:07:09Z"];
char dateTimeMax[sizeof "2011-10-08T07:07:09Z"];




/*     LED     */

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
	struct tm startDate;
	struct tm endDate;
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

#endif