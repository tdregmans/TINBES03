/**
 * 
 * TINBES03
 * ArduinOS
 * Student: Thijs Dregmans (1024272)
 * Version: 4.4
 * Last edited: 2023-06-08
 * 
 */

 // todo:
  // gebruik F()
  // 
#include <EEPROM.h>

//#include <SDRAM.h> // include the right libary
#include <avr/pgmspace.h>
#include "instruction_set.h"

#define MAXPROCESSES 10

#define STACKSIZE 32

#define RUNNING 'r'
#define PAUSED 'p'
#define TERMINATED 0

byte stack[STACKSIZE];
byte sp = 0;

int noOfProcesses = 0;
int noOfVars = 0;

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

typedef struct {
    char name[FILENAMESIZE];
    int processId;
    byte state;
    int pc;
    int sp;
} processType;

processType process[MAXPROCESSES];


static int FileTypeSize = sizeof(fileType);

EERef noOfFiles = EEPROM[0];


bool readToken (char Buffer[], bool spacebreak=true);

bool writeFAT (int index, fileType file) {
    int start = 1 + FileTypeSize * index;
//    Serial.print("written FAT entry at ");
//    Serial.println(start);
    EEPROM.put(start, file);
}

fileType readFATEntry (int FATEntryId) {
    fileType entry;
    int addr = FileTypeSize * FATEntryId + 1;
    EEPROM.get(addr, entry);
    return entry;
}


void help() {
    Serial.println(F("Here is a list with all commands:"));
    Serial.println(F("    help                        prints a list with all commands and their syntax."));
    Serial.println(F("    store [file] [size] [data]  creates file with specified size and puts data in file."));
    Serial.println(F("    retrieve [file]             prints the contents of file."));
    Serial.println(F("    erase [file]                deletes file."));
    Serial.println(F("    files                       prints a list with all files."));
    Serial.println(F("    freespace                   prints the free space in the filesystem."));
    Serial.println(F("    run [file]                  starts the process that is defined in file."));
    Serial.println(F("    list                        prints a list with all processes."));
    Serial.println(F("    suspend [id]                pauzes the process with processId id."));
    Serial.println(F("    resume [id]                 continues the process with processId id."));
    Serial.println(F("    kill [id]                   stops the process with processId id."));
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
}

bool spaceInUse(int addr) {
    for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        fileType file;
        file = readFATEntry(FATEntryId);

        if(file.start <= addr && (file.start + file.size) > addr) {
            return true;
        }
    }
    return false;
}

int emptySpace(int filesize) {
    int pointer = FATSIZE * FileTypeSize;
    int freespace = 0;
    do {
        if (spaceInUse(pointer)) {
            freespace = 0;
        }
        else {
            freespace++;
        }
        pointer++;
        if (freespace >= filesize) {
            return pointer - freespace;
        }
    } while (pointer < EEPROM.length());
    return -1;
}

void freespace() {
    int maxSize = 0;
    while(emptySpace(maxSize) != -1) {
        maxSize++;
    }
    Serial.print("The biggest contiguous free memmory space is ");
    Serial.print(maxSize);
    Serial.println(" bytes.");
}

void run() {
    // clear buffer
    Buffer[0] = 0;
    
    if (noOfProcesses >= MAXPROCESSES) {
        Serial.println("ERROR: Too much processes. Max 10.");
        Serial.println("Try 'list' to view all processes.");
    }
    else {
        int processId = noOfProcesses;
        process[processId].processId = processId;
        
        // get file.name
        while(!readToken(Buffer)) {
            strcpy(process[processId].name, Buffer);
        }

        bool fileExists = false;

        fileType file;
        for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
            if (!strcmp(process[processId].name, readFATEntry(FATEntryId).name)) {
                fileExists = true;
                break;
            }
        }

        if(!fileExists) {
            Serial.println("ERROR: File doesn't exists.");
            Serial.println("Try 'files' to view all files.");
            
            // clear buffer
            Buffer[0] = 0;
            
            return;
        }

        // start process
        process[processId].state = RUNNING;
        process[processId].pc = file.start;
        process[processId].sp = 0;

        noOfProcesses++;

        Serial.print("Process ");
        Serial.print(processId);
        Serial.println(" started successfully.");
    }
    
    // clear buffer
    Buffer[0] = 0;
}

void list() {
    Serial.println("This is a list with all processes:");
    Serial.println("    processId     name          state");
    for(int processId = 0; processId < noOfProcesses; processId++) {
        Serial.print("    ");
        Serial.print(process[processId].processId);
        Serial.print("             ");
        Serial.print(process[processId].name);
        Serial.print("     ");
        Serial.println((char) process[processId].state);
    }
    Serial.print(noOfProcesses);
    Serial.println(" result(s)");
}

void suspend() {
    // clear buffer
    Buffer[0] = 0;
    
    // get processId
    int processId;
    while (!readToken(Buffer)) {
        processId = atoi(Buffer);
    }

    // clear buffer
    Buffer[0] = 0;
    
    if(processId >= 0 && processId < noOfProcesses) {
        if(process[processId].state == RUNNING) {
            process[processId].state = PAUSED;
            Serial.print("Process ");
            Serial.print(processId);
            Serial.println(" paused successfully.");
        }
        else {
            Serial.print("Process ");
            Serial.print(processId);
            Serial.println(" could not be paused.");
        }
    }
    else {
        Serial.println("ERROR");
    }
    
    // clear buffer
    Buffer[0] = 0;
}

void resume() {
    // clear buffer
    Buffer[0] = 0;
    
    // get processId
    int processId;
    while (!readToken(Buffer)) {
        processId = atoi(Buffer);
    }

    // clear buffer
    Buffer[0] = 0;
       
    if(processId >= 0 && processId < noOfProcesses) {
        if(process[processId].state == PAUSED) {
            process[processId].state = RUNNING;
            Serial.print("Process ");
            Serial.print(processId);
            Serial.println(" restarted successfully.");
        }
        else {
            Serial.print("Process ");
            Serial.print(processId);
            Serial.println(" could not be restarted.");
        }
    }
    else {
        Serial.println("ERROR");
    }
    
    // clear buffer
    Buffer[0] = 0;
}

void kill() {
    // clear buffer
    Buffer[0] = 0;
    
    // get processId
    int processId;
    while (!readToken(Buffer)) {
        processId = atoi(Buffer);
    }

    // clear buffer
    Buffer[0] = 0;
    
    if(processId >= 0 && processId < noOfProcesses) {
        process[processId].state = TERMINATED;
        Serial.print("Process ");
        Serial.print(processId);
        Serial.println(" terminated successfully.");
    }
    else {
        Serial.println("ERROR");
    }
    
    // clear buffer
    Buffer[0] = 0;
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
        
        for(int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
            if (!strcmp(file.name, readFATEntry(FATEntryId).name)) {
                Serial.println("file exists already");
                return;
            }
        }
        
        // clear buffer
        Buffer[0] = 0;
        
        // get file.size
        while (!readToken(Buffer)) {
            file.size = atoi(Buffer);
        }
        
        // clear buffer
        Buffer[0] = 0;

        // calculate empty space to store
        
        int emptySpaceStart = emptySpace(file.size);
//        Serial.print("file stored at ");
//        Serial.println(emptySpaceStart);
        if (emptySpaceStart == -1) {
            Serial.println("no space to store file");
            return;
        }

        file.start = emptySpaceStart;

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

void pushByte(byte b) {
    stack[sp++] = b;
}

void pushByte(int index, byte b) {
    stack[sp++] = b;
}

byte popByte() {
    return stack[--sp];
}

byte popByte(int index) {
    return stack[--sp];
}

byte readVal(int index, int filePointer, byte Type) {
    for (byte i = 0; i < Type; i++) {
        pushByte(index, EEPROM[filePointer++]);
    }
    pushByte(index, Type);
    return Type;
}

void execute(int index) {
    switch (EEPROM[process[index].pc++]) {
        case CHAR:
            process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc -1]);
            break;
        case INT:
            process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc -1]);
            break;
        case FLOAT:
            process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc -1]);
            break;
        default:
            Serial.print("Could not find the command ");
            Serial.println(EEPROM[process[index].pc]);
            break;
    }
}

void runProcess() {
    for (int i = 0; i < noOfFiles; i++) {
        execute(i);
    }
}

void setVar(byte name, int processId) {
    if (noOfVars >= MAXIMUMVAR) {
        Serial.println("Could not create another variable. Maximal reached.");
        return;
    }

    byte type = popByte(processId);
    int size = type;
    if(type == STRING) {
//        size = popByte(processId);
    }

    // zoek ruimte in RAM

    int addr = 0;//findFreeMemSpace(size);

    varType var[MAXIMUMVAR];

    // if not enough space, give error
    var[noOfVars].name = name;
    var[noOfVars].processId = processId;
    var[noOfVars].type = type;
    var[noOfVars].addr = addr;
    var[noOfVars].size = size;

    for (int i = size - 1; i >= 0; i--) {
//        memory[addr + 1] = popByte(processId);
    }
    noOfVars++;
}

//void testVariable() {
//    pushByte(0, 'b');
//
//    pushByte(0, CHAR);
//
//    setVar('x', 0);
//}
    
void setup() {
    Serial.begin(9600);
    Serial.println("ArduinoOS (Thijs Dregmans) version 4.0");
    Serial.println("Started. Waiting for commands...");
    Serial.println("Enter 'help' for help.");
    
    Serial.print("> ");

//    testVariable();
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
