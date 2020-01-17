
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DBEGIN(...)    Serial.begin(__VA_ARGS__)     //DBEGIN is a macro, debug in serial
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DBEGIN(...)     //now defines a blank line
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif
