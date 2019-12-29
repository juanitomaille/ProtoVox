#include <string>


/* Librairie de gestion wifi et mqtt de tous les capteurs
    quelques utilities aussi comme le pourcentage batterie restant

*/
// Librairie Wifi pour l'ESP8266 et ESP8285
#include <ESP8266WiFi.h>

/* Librairie pour attaquer le serveur MQTT
disponible ici https://github.com/knolleary/pubsubclient
Site officiel https://pubsubclient.knolleary.net/

Version modifiée intégrée au projet, disponible ici https://github.com/juanitomaille/pubsubclient

*/
#include <PubSubClient.h>

/*  Librairie pour gérer les préférences Wifi
disponible ici https://github.com/tzapu/WiFiManager
Site officiel https://tzapu.com/esp8266-wifi-connection-manager-library-arduino-ide/

Version forkée sans modif ici https://github.com/juanitomaille/WiFiManager
*/
#include <WiFiManager.h>

/* Librairie de gestion de la mise à jour OTA des sensors
inspiré de l'exemple https://github.com/turgu1/mqtt_ota_example
*/
#include <lib/ota.h>


// déclaration des objets wifi, mqtt et OTA
WiFiClient client;
PubSubClient mqtt(client);
OTA upd;

/* Classe de base, il faudra surement mieux structurer ça. */
class Protovox
{
  public:
    Protovox();                                                   // Constructeur
    void                connect(const char* hardwareName, const char* topicPath);        // gère la connection au Wifi (via WifiManager) et au broker MQTT (via PubSubClient)
    void                sleep(int time);                          // met l'ESP8266 en veille profonde (conso inf 10µA)
    float               getBatteryCapacity(void);                 // mesure la tension aux bornes de la batterie
    char*               getSensorValue(char topic);               // récupère la valeur stockée dans le topic MQTT
    char                getTopic();                               // récupère le topic
    char*                getMessage();                             // récupère le string MQTT


  private:
    #define             PROTOVOX_LIB_VERSION   "v0.3"
    const char*               hardware;                                 // nom qui crée le topic MQTT pour l'update ex : topic/hardwareName/UPDATE_TOPIC donne /home/heater/ESP01S-01/update
    const char*               topicHardware;                            // chemin du topic lié au hardware par ex : /home/heater

    // connexion mqtt
    // TODO : trouver un moyen de cacher ça
    const char*         mqttServer =        "mqtt.seed.fr.nf";
    const int           mqttPort =          1883;
    const char*         mqttUser =          "jean";
    const char*         mqttPassword =      "Ugo2torm";
    char*               message;
    #define             UPDATE_TOPIC        "/update"             // là ou est stocké le nouveau firmware
    #define             MAX_MSG_LEN         (128)                  // écrase la valeur max réception de message dans PubSubClient, pas sûr que ce soit encore utile
    void                callback(char *topic, byte *payload, unsigned int length);

    // contenu du message MQTT
    char                topic;
    byte                payload;
    unsigned int        length;

    // veille autorisée ou pas ?
    int                 veille =             digitalRead(D0);       // valable sur Wemos D1 mini (ESP8266)

};

Protovox::Protovox(){}


void Protovox::connect(const char* hardwareName, const char* topicPath) {

  this->hardware = hardwareName;
  this->topicHardware = topicPath;
  Serial.println();
  Serial.print("PROTOVOX LIB VERSION :");
  Serial.println(PROTOVOX_LIB_VERSION);
  Serial.println();
  Serial.print("PROPRIETAIRE :");
  Serial.println("UP-RISE SAS F-90000 BELFORT");
  Serial.println();
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //reset settings
  //wifiManager.resetSettings();

  //wifiManager.setClass("invert"); // dark theme, ne semble pas fonctionner

  // timeout if no wifi signal, très utile pour ne pas drainer la batterie connement
  wifiManager.setConnectTimeout(20);


  //fetches ssid and pass from eeprom and tries to connect
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("ProtoVox");

  if(WiFi.status() != WL_CONNECTED){ sleep(7200); } // si pas de wifi, attendre 2h avant de recommencer, incompatible avec le portail captif


  //si vous êtes là, le wifi est connecté
  Serial.println();
  Serial.println("------|||  Wifi connected  |||------");
  Serial.println();

  mqtt.setServer(mqttServer, mqttPort);
  int _pointer = 1;

  while (!mqtt.connected()) {
    Serial.print("Connexion au broker (tentative ");
    Serial.print(_pointer);
    Serial.print(")");
    delay(1000);
    Serial.print(".");

    if (mqtt.connect(hardwareName, mqttUser, mqttPassword )) {

      Serial.println();
      Serial.println("------|||  MQTT connected  |||------");
      Serial.println();

    } else {

      Serial.println();
      Serial.print("Echec Code Erreur :  ");
      Serial.println(mqtt.state());
      // on essaye 3x avant de dormir pour 2h
      if(_pointer < 3) {
        delay(5000);
      }
      else {
        sleep(7200);
      }

    }
  _pointer++;
  }

  /* Affiche l'état de la mémoire ROM */
  Serial.println();
  Serial.println(   "Etat de la mémoire"        );
  Serial.print(     "Mémoire disponible: "      );  Serial.println(ESP.getFreeSketchSpace());
  Serial.print(     "Taille firmware: "         );  Serial.println(ESP.getSketchSize());
  Serial.print(     "Taille totale disponible: ");  Serial.println(ESP.getFlashChipRealSize());
  Serial.println();

  //mqtt.subscribe(UPDATE_TOPIC);


  /*if (veille == 0){
      if(mqtt.subscribe("test/ota")){Serial.println("Recherche un nouveau Firmware sur le serveur");}
  }*/

  /*
  Classiquement, on déclare le callback comme ci-dessous mais ça ne fonctionne pas à l'intérieur d'une librairie (uniquement dans void setup())
  mqtt.setCallback(callback);

  Voici, ci-dessous une astuce dont je ne maîtrise pas complètement la subtilité, mais ça fonctionne.
  A surveiller donc, si plus de réponse, en allant remettre la déclaration ci-dessus dans le main.ino ou main.ccp !
  */
  mqtt.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->callback(topic, payload, length); } );
}

char Protovox::getTopic(){
  return topic;
}

char* Protovox::getMessage(){
    return message;
}


void Protovox::sleep(int time){

  if (veille == 1){
    Serial.println("Entre en mode veille...");
    delay(500);
    ESP.deepSleep(time * 1000000);
  }
  else {
    Serial.println();
    Serial.println("------   |||   MODE MISE A JOUR   |||   ------");
    Serial.println();
    Serial.println("Commutateur sur position mise à jour - Mode veille désactivé ");
    Serial.println();

    // subcribe to MQTT update
    char* topic_update;
    strcat(topic_update, this->topicHardware);
    strcat(topic_update, this->hardware);
    strcat(topic_update, UPDATE_TOPIC);
    mqtt.subscribe(topic_update);
  }
}

// *** Calcul de la batterie restante

float Protovox::getBatteryCapacity(void){
  /* Mesure du niveau renvoyé par le convertisseur A/N */
  float raw_bat = 100 * analogRead(A0)/1024.00;

  return raw_bat;
  //return (raw_bat * 100/181) ; // mesure du raw_bat lorsque la batterie est chargée donne ce coeff
}

char* Protovox::getSensorValue(char topic){
    return message;
}

void Protovox::callback(char* topic, byte* payload, unsigned int length) {

    /*
    byte* _payload = payload;
    unsigned int _length = length;
    char* _topic = topic;

    payload[length] = '\0';
    String message = String((char*)payload);
    */

    std::stringstream topic_string;
    topic_string << this->topicHardware << this->hardware << UPDATE_TOPIC;
    std::string t = topic_string.str();

    if (memcmp("SIZE=", payload, 5) == 0 && topic == t) {
      long size = atol((const char *) &payload[5]);
      Serial.print("Taille déclarée: ");
      Serial.print(size);
      Serial.println(" Octets");

      if (upd.begin(size)) {
        mqtt.setStream(upd);
      }
    }
    else if (upd.isRunning()) {

      if (upd.end()) {
        if (upd.isCompleted()) {
          Serial.println("Flashage effectué. Reboot....");
          ESP.restart();
          delay(10000);
        }
        else {
          Serial.println("ERREUR: Flashage non terminé !");
        }
      }
      else {
        Serial.println("ERREUR: Taille Fichier firmware !");
      }
    }


}
