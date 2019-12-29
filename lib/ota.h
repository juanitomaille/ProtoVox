#include <StreamString.h>  // ESP8266 Core lib
/*

Dans cette classe, nous utilisons la méthode du core pour l'update :

Update.begin(sizeOfFirmwareBin);
Update.write(firmwarebin);
Update.end();
Update.isFinished();

*/


class OTA : public Stream
{
private:
  size_t        length;
  bool          running;
  bool          completed;

public:
  OTA()   { running = false; }

  bool begin(size_t size) {
    Serial.println("En attente du Firmware");

    length    =       size;
    completed =       false;
    running   =       Update.begin(size);
    if (!running) showError();
    return running;
  }

  size_t write(uint8_t b) {


    if (running) {

      if (--length < 0) running = false;

      else {
        Update.write(&b, 1); // on update !
        return 1;
      }
    }
    return 0;
  }

  bool end() {
    Serial.println();
    Serial.println("Téléchargé");
    running = false;
    if (Update.end()) {
      completed = Update.isFinished();
      if (!completed) showError();
    } else showError();
    return completed;
  }

  int available() { return length;   }
  int      read() { return 0;        } // not used
  int      peek() { return 0;        } // not used

  bool isCompleted() { return completed;         }
  bool   isRunning() { return running;           }
  int     getError() { return Update.getError(); }
  void   showError() { StreamString error; Update.printError(error); Serial.print(error); }
};
