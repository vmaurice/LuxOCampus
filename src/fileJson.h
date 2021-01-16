#include <ArduinoJson.h>

// In JsonFile
const size_t capacity = JSON_OBJECT_SIZE(5) + 400;

// the first information to authentification the calendar with google
int jsonCapacitygoogleConnect = capacity;

// token google and authors informations (expire)
int jsonCapacityGoogleToken = capacity;


// The events of Google 
int jsonCapacityCalendar = 32768;

// The list of sub-calendars 
int jsonCapacityCalendarList = 8192;

// The color definitions for calendars and events.
int jsonCapacityJsonColor = 4096;

// The data definitions to save data of calendars and events.
int jsonCapacityDataSave = 512;


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