#include <Arduino.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

WiFiManager wm; // global wm instance
char * ssid = "ESP32";

// OPTION FLAGS
bool TEST_CP  = true; // always start the configportal, even if ap found
bool TEST_NET = true; // do a network test after connect, (gets ntp time)

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



  



}

void loop() {
  // put your main code here, to run repeatedly:
}