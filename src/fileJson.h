#include <ArduinoJson.h>

// In JsonFile
const size_t capacity = JSON_OBJECT_SIZE(5) + 400;

// the first information to authentification the calendar with google
DynamicJsonDocument googleConnect(capacity);

// token google and authors informations (expire)
DynamicJsonDocument googleToken(capacity);


// The events of Google 
DynamicJsonDocument jsonCalendar(65536);

// The list of sub-calendars 
DynamicJsonDocument jsonCalendarList(8192);

// The color definitions for calendars and events.
DynamicJsonDocument jsonColor(4096);

// Error json
//DeserializationError errorJson;

/*
 * Test if Deserialization (parsing) succeeds.
 */
void checkJsonError(DeserializationError errorJson) {
    if (errorJson) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(errorJson.c_str());
        return;
    }
}