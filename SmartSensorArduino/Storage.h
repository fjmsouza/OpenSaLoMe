#ifndef Storage_h
#define Storage_h

#include "FS.h"
#include "LittleFS.h" // Biblioteca para acessar o LittleFS

class Storage
{
public:
    const char *UPPER_THRESHOLD_PATH = "/UPPER_THRESHOLD.txt";
    const char *LOWER_THRESHOLD_PATH = "/LOWER_THRESHOLD.txt";

    void setup();
    void writeString(const char *caminho, String dado);
    String readString(const char *caminho);
    bool fileExists(const char *caminho);
    void createFile(const char *caminho);
};
#endif