/**
 * 
 * TINBES03
 * practicum3
 * Student: Thijs Dregmans (1024272)
 * Version: 3.0
 * Last edit: 2023-04-26
 * 
 */
#include <EEPROM.h>

#define BUFSIZE 12

static char Buffer[BUFSIZE];

typedef struct {
    char name[BUFSIZE];
    void *func;
} commandType;

#define FILENAMESIZE 12
#define FATSIZE 10

typedef struct {
    char name[FILENAMESIZE];
    int start;
    int size;
} fileType;

static int FileTypeSize = sizeof(fileType);

bool writeFAT (fileType f) {
    EEPROM.put(0, f);
}

//struct FATEntry readFATEntry (int FATEntryId) {
//    struct FATEntry entry;
//    int addr = FATEntrySize * FATEntryId;
//    EEPROM.get(addr, entry);
//    return entry;
//}

void help() {
    Serial.println("Here is a list with all commands:");
    Serial.println("    help                        prints a list with all commands and their syntax.");
    Serial.println("    store [file] [size] [data]  creates file with specified size and puts data in file.");
    Serial.println("    retrieve [file]             prints the contents of file.");
    Serial.println("    erase [file]                deletes file.");
    Serial.println("    files                       prints a list with all files.");
    Serial.println("    freespace                   prints the free space in the filesystem.");
    Serial.println("    run [file]                  starts the process that is defined in file.");
    Serial.println("    list                        prints a list with all processes.");
    Serial.println("    suspend [id]                pauzes the process with processId id.");
    Serial.println("    resume [id]                 continues the process with processId id.");
    Serial.println("    kill [id]                   stops the process with processId id.");
}



void retrieve() {
    Serial.println("This is the retrieve function.");
}

void erase() {
    Serial.println("This is the erase function.");
}

void files() {
    Serial.println("This is a list with all files:");
    Serial.println("    name                        size");
//    for(int FATEntryId = 0; FATEntryId < FATSIZE; FATEntryId++) {
//        if(readFATEntry(FATEntryId).size != -1) {
//            Serial.print("    ");
//            Serial.print(readFATEntry(FATEntryId).name);
//            Serial.print("                        ");
//            Serial.println(readFATEntry(FATEntryId).size);
//        }
//    }
}

void freespace() {
    Serial.println("This is the freespace function.");
}

void run() {
    Serial.println("This is the run function.");
}

void list() {
    Serial.println("This is the list function.");
}

void suspend() {
    Serial.println("This is the suspend function.");
}

void resume() {
    Serial.println("This is the resume function.");
}

void kill() {
    Serial.println("This is the kill function.");
}

 
static commandType command[] = {
    {"help", &help},
    {"store", &store},
    {"retrieve", &retrieve},
    {"erase", &erase},
    {"files", &files},
    {"freespace", &freespace},
    {"run", &run},
    {"list", &list},
    {"suspend", &suspend},
    {"resume", &resume},
    {"kill", &kill}
};

static int commandSize = sizeof(command) / sizeof(commandType);

bool readToken (char Buffer[]) {
    int i = strlen(Buffer);
    char c;
    while (Serial.available()) {
        c = Serial.read();

        if (c == '\r' || c == '\n') {
            c = '\0';
            Buffer[i] = c;
            Serial.println(Buffer);
            
            return true;
        }

        Buffer[i] = c;
        i++;
    }
    Buffer[i+1] = '\0';
    return false;
}

EERef noOfFiles = EEPROM[0];

void store(char Buffer[]) {
    if (noOfFiles >= FATSIZE) {
        Serial.println("ERROR: Too much files. Max 10.");
    }
    else {
        // check if name already exists

        
        fileType file;
        // retrieve file.name
        while (!readToken(Buffer)) {
            file.name = Buffer;
        }
        // retrieve file.size
        while (!readToken(Buffer)) {
            file.size = int(Buffer);
        }

        // check if there is enough space
        file.addr = 1;

        writeFAT(noOfFiles, file);

        for (int byteId = 0; byteId < file.size; byteId++) {
            while (!Serial.available()) {
                EEPROM[file.addr + byteId] = Serial.read();
            }
        }
        noOfFiles++;

        // print message

        // clear buffer
    }
}


    
void setup() {
    Serial.begin(9600);
    Serial.println("ArduinoOS (Thijs Dregmans) version 2.1");
    Serial.println("Started. Waiting for commands...");
    Serial.println("Enter 'help' for help.");
    
    Serial.print("> ");

}
    
void loop() {
    if (readToken(Buffer)) {
        bool oneCalled = false;
        for (int commandId = 0; commandId < commandSize; commandId++) {
            if (!strcmp(Buffer, command[commandId].name)) {
                void (*func) (char Buffer[]) = command[commandId].func;
                func(Buffer);
                oneCalled = true;
            }
        }
        Buffer[0] = 0;
        if (!oneCalled) {
//          Serial.println("ERROR: command not known");
//          Serial.println("Enter 'help' for help.");
        }
    
        Serial.print("> ");
    }
}
