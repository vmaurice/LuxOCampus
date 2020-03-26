# LuxOCampus
  LuxOCampus est un projet pour l'université Paul Sabatier sur la mise en place d'un calendrier en temps réel se basant sur les événements de son calendrier Google qui sera réalisé avec un esp32 et des leds adressables.



## Pour récupérer notre travail:
```
git clone https://github.com/vmaurice/LuxOCampus.git
```


## PlateformIO (https://platformio.org):
  Sera notre écosystème afin de programmer notre calendrier dans de bonnes conditions afin que chacun puisse travailler sur les mêmes librairies.



## Applications utilisées


### WifiManager (https://github.com/tzapu/WiFiManager/tree/development):
  Permets de créer un access point de notre esp32 afin de pouvoir lui rentrer les paramètres pour se connecter sur une borne wifi

### Google OAUTH (https://developers.google.com/identity/protocols/OAuth2ForDevices):
  API de google permettant de se connecter à ses services

### ArduinoJson (https://arduinojson.org):
  Pour sauvegarder des informations dans l'esp32



## Supports en plus:

### Exemple pour récupérer un token google api:
https://create.arduino.cc/projecthub/phpoc_man/arduino-login-into-google-and-facebook-9ae91d

### MDNS (pour récupérer l'ip quand on a le nom de l'esp):
https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/protocols/mdns.html

