/**
 * 
 * TINBES03
 * ArduinOS
 * Student: Thijs Dregmans (1024272)
 * Version: 4.0
 * Last edit: 2023-05-20
 * 
 */
#include <EEPROM.h>
//#include <RAM.h> // include the right libary

#define BUFSIZE 12

static char Buffer[BUFSIZE];

typedef struct {
    char name[BUFSIZE];
    void *func;
} commandType;

#define FILENAMESIZE 12
#define FATSIZE 10

#define MAXIMUMVAR 25 // check value

typedef struct {
    char name[FILENAMESIZE];
    int start;
    int size;
} fileType;

typedef struct {
    byte name;
    int processId;
    byte type;
    byte addr;
    byte size;
} varType;

static int FileTypeSize = sizeof(fileType);

EERef noOfFiles = EEPROM[0];


bool readToken (char Buffer[], bool spacebreak=true);

bool writeFAT (int index, fileType file) {
    int start = 1 + FileTypeSize * index;
//    Serial.print("written FAT entry at ");
    Serial.println(start);
    EEPROM.put(start, file);
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


void files() {
    Serial.println("This is a list with all files:");
    Serial.println("    name                        size");
    int counter = 0;
    for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
//        if(readFATEntry(FATEntryId).size >= 0) {
            Serial.print("    ");
            Serial.print(readFATEntry(FATEntryId).name);
            Serial.print("                        ");
            Serial.println(readFATEntry(FATEntryId).size);
            counter++;
//        }
    }
    Serial.print(counter);
    Serial.println(" result(s)");

    if(/*counter == 0 && */noOfFiles != 0) {
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
    for(int i = 0; i < EEPROM.length(); i++) {
        EEPROM[i] = 255;
    }
    noOfFiles = 0;
    Serial.println("Memory wiped.");
    
    Serial.println("Try 'files' to view all files.");
}

void memory() {
    // using 50 for debugging; but acctually EEPROM.length();
//    for(int i = 0; i < 50; i++) {
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
    {"memory", &memory}
};

static int commandSize = sizeof(command) / sizeof(commandType);

bool readToken (char Buffer[], bool spacebreak) {
    int i = strlen(Buffer);
    char c;
    while (Serial.available()) {
        c = Serial.read();

        if ((c == ' ' && spacebreak) || c == '\r' || c == '\n') {
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
        fileType file;
        
        // get file.name
        while(!readToken(Buffer)) {
            strcpy(file.name, Buffer);
        }
        
        // check if name exists already !!!
        
        // clear buffer
        Buffer[0] = 0;
        
        // get file.size
        while (!readToken(Buffer)) {
            file.size = atoi(Buffer);
        }
        
        // clear buffer
        Buffer[0] = 0;

        // calculate empty space to store
        
        int emptySpaceStart = 0;

        // check if there is enough space
        file.start = (FATSIZE * FileTypeSize) + emptySpaceStart;

        writeFAT(noOfFiles, file);
        // implement error message !!!

        char* content;
        while (!readToken(Buffer, false)) {
            content = Buffer;
        }

        for (int byteId = 0; byteId < file.size; byteId++) {
            EEPROM[file.start + byteId] = content[byteId];
        }
        noOfFiles++;

        // print message
        Serial.println("file stored successfully");

        // clear buffer
        Buffer[0] = 0;
        
    }
}

int locateFile(char* filename) {
    fileType file;

    for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        if (!strcmp(filename, readFATEntry(FATEntryId).name)) {
            return FATEntryId;
        }
    }
    return -1;
}

void retrieve() {
    // clear buffer
    Buffer[0] = 0;
    
    char* filename;
    // get filename
    while(!readToken(Buffer)) {
        filename = Buffer;
    }

    int index = locateFile(filename);
    Serial.print(filename);

    if (index == -1) { 
        Serial.println(" not found");
        return; 
    }
    Serial.println(" found");
    Serial.println("content:");

    fileType file = readFATEntry(index);

    for (int i = 0; i < file.size; i++) {
        Serial.print((char) EEPROM[file.start + i]);
    }

    Serial.print('\n');
}

void erase() {
    // clear buffer
    Buffer[0] = 0;
    
    char* filename;
    // get filename
    while(!readToken(Buffer)) {
        filename = Buffer;
    }

    int index = locateFile(filename);
    Serial.print(filename);

    if (index == -1) { 
        Serial.println(" not found");
        return; 
    }
    Serial.println(" found");

    fileType file = readFATEntry(index);

    noOfFiles--;

    for(int i = index; i < noOfFiles; i++) {
        writeFAT(i, readFATEntry(i + 1));
    }
    Serial.print(filename);
    Serial.println(" erased successfully");
    
    // clear buffer
    Buffer[0] = 0;
}



void setVar(byte name, int processId) {
    if (noOfVars >= MAXIMUMVAR) {
        Serial.println("Could not create another variable. Maximal reached.");
        return;
    }

    byte type = popByte(processId);
    int size = type;
    if(type == STRING) {
        size = popByte(processId);
    }

    // zoek ruimte in RAM

    int addr = findFreeMemSpace(size);

    varType var;

    // if not enough space, give error
    var[noOfVars].name = name;
    var[noOfVars].processId = processId;
    var[noOfVars].type = type;
    var[noOfVars].addr = addr;
    var[noOfVars].size = size;

    for (int i = size - 1; i >= 0; i--) {
        memory[addr + 1] = popByte(processId);
    }
    noOfVars++;
}

void testVariable() {
    pushByte(0, 'b');

    pushByte(0, CHAR);

    setVar('x', 0);
}
    
void setup() {
    Serial.begin(9600);
    Serial.println("ArduinoOS (Thijs Dregmans) version 4.0");
    Serial.println("Started. Waiting for commands...");
    Serial.println("Enter 'help' for help.");
    
    Serial.print("> ");

    testVariable();
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
