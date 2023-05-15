/**
 * 
 * TINBES03
 * practicum3
 * Student: Thijs Dregmans (1024272)
 * Version: 3.1
 * Last edit: 2023-05-10
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

EERef noOfFiles = EEPROM[0];

bool writeFAT (fileType file) {
    int start = 1 + FileTypeSize * noOfFiles;
    Serial.print("written at");
    Serial.println(start);
    EEPROM.put(0, file);
}

fileType readFATEntry (int FATEntryId) {
    fileType entry;
    int addr = FileTypeSize * FATEntryId + 1;
    EEPROM.get(addr, entry);
    return entry;
}

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
    // clear buffer
    Buffer[0] = 0;
    
    char* filename;
    // retrieve filename
    while(!readToken(Buffer)) {
        filename = Buffer;
    }
    Serial.println(filename);
    
    Serial.println("done");
    for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        if (!strcmp(Buffer, readFATEntry(FATEntryId).name)) {
            Serial.println(Buffer);
            Serial.print(readFATEntry(FATEntryId).name);
            Serial.println(" found");
            

            for (int byteId = 0; byteId < 10; byteId++) {
                Serial.print(EEPROM.read(readFATEntry(FATEntryId).start + byteId));
            }
            Serial.println("printing done");
            // clear buffer
            Buffer[0] = 0;
            return;
        }
    }
    // clear buffer
    Buffer[0] = 0;
}

void erase() {
    Serial.println("This is the erase function.");
}

void files() {
    Serial.println("This is a list with all files:");
    Serial.println("    name                        size");
    int counter = 0;
    for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        if(readFATEntry(FATEntryId).size >= 0) {
            Serial.print("    ");
            Serial.print(readFATEntry(FATEntryId).name);
            Serial.print("                        ");
            Serial.println(readFATEntry(FATEntryId).size);
            counter++;
        }
    }
    Serial.print(counter);
    Serial.println(" result(s)");

    if(counter == 0 && noOfFiles != 0) {
      noOfFiles = counter;
      Serial.println("EEPROM correction made.");
    }
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
void wipe() {
    noOfFiles = 0;
    Serial.println("Memory wiped.");
    
    Serial.println("Try 'files' to view all files.");
}
void test() {
    Serial.println("test function");
    fileType file;
    strcpy(file.name, Buffer);
    file.size = 5;
    file.start = 190;

    writeFAT(file);
}

void memory() {
    for(int i = 0; i < EEPROM.length(); i++) {
        Serial.print(i);
        Serial.print(" - ");
        Serial.println(EEPROM[i]);
    }
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
    {"kill", &kill},
    {"wipe", &wipe},
    {"test", &test},
    {"memory", &memory}
};

static int commandSize = sizeof(command) / sizeof(commandType);

bool readToken (char Buffer[]) {
    int i = strlen(Buffer);
    char c;
    while (Serial.available()) {
        c = Serial.read();

        if (c == ' ' || c == '\r' || c == '\n') {
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


void store() {
    // clear buffer
    Buffer[0] = 0;
    if (noOfFiles >= FATSIZE) {
        Serial.println("ERROR: Too much files. Max 10.");
        Serial.println("Try 'files' to view all files.");
    }
    else {
        // check if name already exists

        
        fileType file;
        // retrieve file.name
        while(!readToken(Buffer)) {
            strcpy(file.name, Buffer);
        }
        Serial.println(file.name);
        
        Serial.println("done");
        // clear buffer
        Buffer[0] = 0;
        
        // retrieve file.size
        int buf;
        while (!readToken(Buffer)) {
          buf = int(Buffer);
            strcpy(file.size, Buffer);
        }
        Serial.println(Buffer);
        
        Serial.println(buf);
        Serial.print("size=");
        Serial.println(file.size);

        int emptySpaceStart = 0;

        // check if there is enough space
        file.start = (FATSIZE * FileTypeSize) + emptySpaceStart;

        writeFAT(file);

        for (int byteId = 0; byteId < file.size; byteId++) {
            while (!Serial.available()) {
                EEPROM[file.start + byteId] = Serial.read();
            }
        }
        noOfFiles++;

        // print message

        // clear buffer
        Buffer[0] = 0;
        
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
          Serial.println("ERROR: command not known");
          Serial.println("Enter 'help' for help.");
        }
    
        Serial.print("> ");
    }
}
