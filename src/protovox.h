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
#include <../lib/ota.h>

/* lib de debug */
#include <../lib/debug.h>


// déclaration des objets wifi, mqtt et OTA
WiFiClient client;
PubSubClient mqtt(client);
OTA upd;




/* Classe de base, il faudra surement mieux structurer ça. */
class Protovox
{
  public:
    Protovox();                                                   // Constructeur

    void                connect(const char* command);        // gère la connection au Wifi (via WifiManager) et au broker MQTT (via PubSubClient)
    void                sleep(int time);                          // met l'ESP8266 en veille profonde (conso inf 10µA)
    float               getBatteryCapacity(void);                 // mesure la tension aux bornes de la batterie
    char*               getSensorValue(char topic);               // récupère la valeur stockée dans le topic MQTT
    char*               getTopic();                               // récupère le topic
    unsigned int        getLength();
    char*               getMessage();                             // récupère le string MQTT
    const char*         getUpdateTopic();                         // récupère le topic complet pour l'update, par ex : /home/heater/ESP01-1/update
    const char*         concatenate( const char* arg1, const char* arg2, const char* arg3, const char* arg4 , const char* arg5, const char* arg6 );  // concatene plusieurs const char*


  private:
    #define             PROTOVOX_LIB_VERSION   "0.4.1"

    // connexion mqtt
    // TODO : trouver un moyen de cacher ça
    const char*         mqttServer =        "mqtt.seed.fr.nf";
    const int           mqttPort =          1883;
    const char*         mqttUser =          "jean";
    const char*         mqttPassword =      "Ugo2torm";
    char*               message;
    const char*         UPDATE_TOPIC =      "update";              // là ou est stocké le nouveau firmware
    #define             MAX_MSG_LEN         (128)                  // écrase la valeur max réception de message dans PubSubClient, pas sûr que ce soit encore utile
    void                callback(char *topic, byte *payload, unsigned int length);
    void                updateThing(char* _topic, byte* _payload);  // réalise l'update via OTA de l'objet

    // contenu du message MQTT
    char*               topic;
    byte*               payload;
    unsigned int        length;

};

Protovox::Protovox(){}


void Protovox::connect(const char* command = NULL) {



    DPRINTLN();
    DPRINT("PROTOVOX LIB VERSION :");
    DPRINTLN(PROTOVOX_LIB_VERSION);
    DPRINTLN();
    DPRINT("PROPRIETAIRE :");
    DPRINTLN("UP-RISE SAS F-90000 BELFORT");
    DPRINTLN();


  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //reset settings
  if(command == "RESET") { wifiManager.resetSettings();}

  //wifiManager.setClass("invert"); // dark theme, ne semble pas fonctionner

  // timeout if no wifi signal, très utile pour ne pas drainer la batterie connement
  wifiManager.setConnectTimeout(30);


  //fetches ssid and pass from eeprom and tries to connect
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("ProtoVox");

  if(WiFi.status() != WL_CONNECTED){ sleep(7200); } // si pas de wifi, attendre 2h avant de recommencer, incompatible avec le portail captif


  //si vous êtes là, le wifi est connecté

    DPRINTLN();
    DPRINTLN("------|||  Wifi connected  |||------");
    DPRINTLN();



  mqtt.setServer(mqttServer, mqttPort);
  int _pointer = 1;

  while (!mqtt.connected()) {
    DPRINT("Connexion au broker (tentative ");
    DPRINT(_pointer);
    DPRINT(")");
    delay(1000);
    DPRINT(".");

    if (mqtt.connect(PROTOVOX_HARDWARE_NAME, mqttUser, mqttPassword )) {

      DPRINTLN();
      DPRINTLN("------|||  MQTT connected  |||------");
      DPRINTLN();

    } else {

      DPRINTLN();
      DPRINT("Echec Code Erreur :  ");
      DPRINTLN(mqtt.state());
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
  DPRINTLN();
  DPRINTLN(   "Etat de la mémoire"        );
  DPRINT(     "Mémoire disponible: "      );  DPRINTLN(ESP.getFreeSketchSpace());
  DPRINT(     "Taille firmware: "         );  DPRINTLN(ESP.getSketchSize());
  DPRINT(     "Taille totale disponible: ");  DPRINTLN(ESP.getFlashChipRealSize());
  DPRINTLN();

  //mqtt.subscribe(UPDATE_TOPIC);


  DPRINTLN("----> If crash After this line go to -> function connect -> setCallback");
  /*
  Classiquement, on déclare le callback comme ci-dessous mais ça ne fonctionne pas à l'intérieur d'une librairie (uniquement dans void setup())
  */
  //mqtt.setCallback(callback);
  /*
  Voici, ci-dessous une astuce dont je ne maîtrise pas complètement la subtilité, mais ça fonctionne.
  A surveiller donc, si plus de réponse, en allant remettre la déclaration ci-dessus dans le main.ino ou main.ccp !
  */
  mqtt.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->callback(topic, payload, length); } );

  // s'il n'y a pas de mode veille, il faut vérifier l'update à chaque démarrage sinon, voir méthode sleep.
  

  if (veille == 0){
      if(mqtt.subscribe(this->getUpdateTopic())){DPRINTLN("Recherche un nouveau Firmware sur le serveur");}
  }

  if (CONNECT_PIN != NULL){

    digitalWrite(CONNECT_PIN,HIGH);
    delay(200);
    digitalWrite(CONNECT_PIN,LOW);
    delay(200);
    digitalWrite(CONNECT_PIN,HIGH);
    delay(200);
    digitalWrite(CONNECT_PIN,LOW);
    delay(200);
    digitalWrite(CONNECT_PIN,HIGH);
    delay(200);
    digitalWrite(CONNECT_PIN,LOW);
    delay(200);
    digitalWrite(CONNECT_PIN,HIGH);
    delay(200);
    digitalWrite(CONNECT_PIN,LOW);
    delay(200);
    digitalWrite(CONNECT_PIN,HIGH);
  }
}

const char* Protovox::concatenate( const char* arg1, const char* arg2, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL){
  std::string _result;
  _result += arg1;
  _result += arg2;
  _result += arg3;
  _result += arg4;
  _result += arg5;
  _result += arg6;
  DPRINT("DEBUG CONCATENATE : ");DPRINTLN((const char*)_result.c_str());
  return (const char*)_result.c_str();
}

char* Protovox::getTopic(){
  return topic;
}

unsigned int Protovox::getLength(){
  return length;
}

char* Protovox::getMessage(){
  byte* _payload = this->payload;
  return (char*)_payload;
}

const char* Protovox::getUpdateTopic(){

  DPRINTLN("---->function getUpdateTopic");
  const char* _SLASH = "/";
  DPRINTLN(PROTOVOX_TOPIC_PATH);
  DPRINTLN(PROTOVOX_HARDWARE_NAME);
  DPRINTLN(UPDATE_TOPIC);
  DPRINT("-- TOPIC : ");DPRINTLN(this->concatenate(PROTOVOX_TOPIC_PATH, PROTOVOX_HARDWARE_NAME, _SLASH, UPDATE_TOPIC));


  return this->concatenate(PROTOVOX_TOPIC_PATH, PROTOVOX_HARDWARE_NAME, _SLASH, UPDATE_TOPIC);
}


void Protovox::sleep(int time){


    if (veille == 1){
      DPRINTLN("Entre en mode veille...");
      delay(500);
      ESP.deepSleep(time * 1000000);
    }

  else {
    DPRINTLN();
    DPRINTLN("------   |||   MODE MISE A JOUR   |||   ------");
    DPRINTLN();
    DPRINTLN("Commutateur sur position mise à jour - Mode veille désactivé ");
    DPRINTLN();

    // subcribe to MQTT update

    mqtt.subscribe(this->getUpdateTopic());
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

  /* update OTA */
void Protovox::updateThing(char* _topic, byte* _payload){

  /* voir ota.h pour le fonctionnement.
      etape 1 : envoi de la taille du firmware en octets sur le topic dédié (par ex /home/heater/ESP01-1/update)
      etape 2 : envoi du .bin sur le même topic.*/
  if (memcmp("SIZE=", _payload, 5) == 0 && _topic == this->getUpdateTopic()) {

    // on affiche la taille donnée dans MQTT
    long _size = atol((const char *) &_payload[5]);
    DPRINT("Taille déclarée: ");
    DPRINT(_size);
    DPRINTLN(" Octets");

    if (upd.begin(_size)) {
      mqtt.setStream(upd); // on se met en mode de réception de binaire
    }
  }
  else if (upd.isRunning()) {

    if (upd.end()) {
      if (upd.isCompleted()) {
        DPRINTLN("Flashage effectué. Reboot....");
        ESP.restart();
        delay(10000);
      }
      else {
        DPRINTLN("ERREUR: Flashage non terminé !");
      }
    }
    else {
      DPRINTLN("ERREUR: Taille Fichier firmware !");
    }
  }
}


/*  Fonction prototype imposée par la lib PubSubClient  */
void Protovox::callback(char* topic, byte* payload, unsigned int length) {

    DPRINTLN("---->function callback");

    this->updateThing(topic,payload); // on vérifie si on a un nouveau firmware disponible et on fait l'update puis reboot.

    // si pas d'update on actualise les variables de classe
    this->topic = topic;
    this->payload = payload;
    this->length = length;
}
