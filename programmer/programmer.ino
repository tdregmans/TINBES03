/**

   TINBES03
   programmer
   Student: Thijs Dregmans (1024272)
   Version: 1.1
   Last edited: 2023-06-19

   This programmer converts one of the provided programs to bytecode and uploads it to the ArduinOS

   Todo:
      - take program
      - convert program to bytecode
      - upload bytecode to ArduinOS
*/

// Include libaries
#include <EEPROM.h>
// Include instruction set
#include "instruction_set.h"

// Define constants
#define BUFSIZE 12
#define FILENAMESIZE 12
#define FATSIZE 10
#define MAXBYTECODESIZE 50 // change this later

// Define struct for commands
typedef struct {
    char name[BUFSIZE];
    void *func;
} commandType;

// Define struct for files stored in File Allocation Table
typedef struct {
    char name[FILENAMESIZE];
    int start;
    int size;
} fileType;

// Define struct for programs
typedef struct {
    char name[FILENAMESIZE];
    int size; 
    byte bytecode[MAXBYTECODESIZE];
} programType;


// Array with programs
static programType programs[] = {
    {"hello", 17, {3, 72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 10, 0, 51, 135}},
    {"test_byte", 5, {2, 0, 5, 51, 135}}
};

// Initialize global variables (counters)
static int FileTypeSize = sizeof(fileType);
EERef noOfFiles = EEPROM[0];
static int noOfPrograms = sizeof(programs) / sizeof(programType);

// Initialize global variables (memory)
static char Buffer[BUFSIZE];

bool readToken (char Buffer[], bool spacebreak = true);

// Callable command: help
// Prints information about available commands on screen
void help() {
    Serial.println(F("Here is a list with all available commands:"));
    Serial.println(F("    help                        prints a list with all commands and their syntax."));
    Serial.println(F("    upload [program]            creates file with the bytecode of the program in file."));
    Serial.println(F("    read [file]                 prints the content of the file in bytecode."));
    Serial.println(F("    erase [file]                deletes file."));
    Serial.println(F("    files                       prints a list with all files."));
    Serial.println(F("    freespace                   prints the free space in the filesystem."));
    Serial.println(F("    reboot                      reboots the OS."));
}

// Callable command: upload [program]
// Stores a file with name [program] in the File Allocation Table and stores the bytecode in EEPROM
void upload() {
    // clear buffer
    Buffer[0] = 0;
  
    if (noOfFiles >= FATSIZE) {
        Serial.println(F("ERROR: Too much files! Max 10."));
        Serial.println(F("Try 'files' to view all files."));
    }
    else {
        fileType file;
    
        // get file.name
        while (!readToken(Buffer)) {
            strcpy(file.name, Buffer);
        }
    
        for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
            if (!strcmp(file.name, readFATEntry(FATEntryId).name)) {
                Serial.println(F("ERROR: A file with that name already exists!"));
                return;
            }
        }
    
        // clear buffer
        Buffer[0] = 0;

        int programId = locateProgram(file.name);

        if (programId == -1) {
            Serial.println(F("ERROR: Program not found!"));
            return;
        }
        programType program = programs[programId];
        
    
        // get file.size
        file.size = program.size;
    
        // clear buffer
        Buffer[0] = 0;
    
        // calculate empty space to store
    
        int emptySpaceStart = emptySpace(file.size);
        if (emptySpaceStart == -1) {
            Serial.println(F("ERROR: Not enough space to store file!"));
            return;
        }
    
        file.start = emptySpaceStart;
    
        writeFAT(noOfFiles, file);
    
        for (int byteId = 0; byteId < file.size; byteId++) {
            EEPROM[file.start + byteId] = program.bytecode[byteId];
        }
        noOfFiles++;
    
        // print message
        Serial.print(F("INFO: File "));
        Serial.print(file.name);
        Serial.println(F(" is stored!"));
    
        // clear buffer
        Buffer[0] = 0;
    }
}

// Callable command: read [file]
// Prints the content of file with name [file] in bytecode on screen
void read() {
    // clear buffer
    Buffer[0] = 0;
  
    char* filename;
    // get filename
    while (!readToken(Buffer)) {
        filename = Buffer;
    }
  
    int index = locateFile(filename);
  
    if (index == -1) {
        Serial.println(F("ERROR: File not found!"));
        return;
    }
    Serial.println(F("Content:"));
  
    fileType file = readFATEntry(index);
  
    for (int i = 0; i < file.size; i++) {
        Serial.println((byte) EEPROM[file.start + i]);
    }
  
    Serial.print('\n');
}

// Callable command: erase [file]
// Deletes file with name [file] from the File Allocation Table
void erase() {
    // clear buffer
    Buffer[0] = 0;
  
    char* filename;
    // get filename
    while (!readToken(Buffer)) {
        filename = Buffer;
    }
  
    int index = locateFile(filename);
  
    if (index == -1) {
        Serial.println(F("ERROR: File not found!"));
        return;
    }
  
    fileType file = readFATEntry(index);

    // check if file is running a process
    // not needed in this programmer

    // erase file
    noOfFiles--;
  
    for (int i = index; i < noOfFiles; i++) {
        writeFAT(i, readFATEntry(i + 1));
    }
  
    // print message
    Serial.print(F("INFO: File "));
    Serial.print(file.name);
    Serial.println(F(" is deleted!"));
  
    // clear buffer
    Buffer[0] = 0;
}

// Callable command: files
// Prints a list with stored files on screen
void files() {
    Serial.println(F("This is a list with all files:"));
    Serial.println(F("    name                        size"));
    for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        Serial.print(F("    "));
        Serial.print(readFATEntry(FATEntryId).name);
        Serial.print(F("                        "));
        Serial.println(readFATEntry(FATEntryId).size);
    }
    Serial.print(noOfFiles);
    Serial.println(F(" result(s)"));
}

// Callable command: freespace
// Prints the size of the largest contiguouse free memory space on screen
void freespace() {
    int maxSize = 0;
    while (emptySpace(maxSize) != -1) {
        maxSize++;
    }
    Serial.print(F("INFO: Largest contiguous free memory space is "));
    Serial.print(maxSize);
    Serial.println(F(" bytes."));
}

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0

void reboot() {
    delay(10);
    resetFunc(); //call reset
}

// Array with callable commands
static commandType command[] = {
    {"help", &help},
    {"upload", &upload},
    {"read", &read},
    {"erase", &erase},
    {"files", &files},
    {"freespace", &freespace},
    {"reboot", &reboot}
};

static int commandSize = sizeof(command) / sizeof(commandType);

// Function: spaceInUse
// Checks if a EEPROM byte is in use by any file.
bool spaceInUse(int addr) {
    for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        fileType file;
        file = readFATEntry(FATEntryId);
    
        if (file.start <= addr && (file.start + file.size) > addr) {
            return true;
        }
    }
    return false;
}

// Function: spaceInUse
// Returns the first empty space where the file fits.
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

// Function: readToken
// Used to read a token from the buffer, enabling the program to have non-blocking input
bool readToken (char Buffer[], bool spacebreak) {
    int i = strlen(Buffer);
    char c;
    while (Serial.available()) {
        c = Serial.read();
    
        if ((c == ' ' && spacebreak) || c == '\r' || c == '\n') {
            c = '\0';
            Buffer[i] = c;
            Serial.println(Buffer);
            // Disable the line above to disable printing of user input
      
            return true;
        }
    
        Buffer[i] = c;
        i++;
    }
    Buffer[i + 1] = '\0';
    return false;
}

// Function: writeFAT
// Puts the file in the File Allocation Table
void writeFAT (int index, fileType file) {
    int start = 1 + FileTypeSize * index;
    EEPROM.put(start, file);
}

// Function: readFATEntry
// Returns the file from the File Allocation Table
fileType readFATEntry (int FATEntryId) {
    fileType entry;
    int addr = FileTypeSize * FATEntryId + 1;
    EEPROM.get(addr, entry);
    return entry;
}

// Function: locateFile
// Searches the file in the Fat Allocation Table and returns its index
int locateFile(char* filename) {
    fileType file;
  
    for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
        if (!strcmp(filename, readFATEntry(FATEntryId).name)) {
            return FATEntryId;
        }
    }
    return -1;
}

// Function: locateProgram
// Searches the program in the program table and returns its index
int locateProgram(char* filename) {
    programType program;
  
    for (int programId = 0; programId < noOfPrograms; programId++) {
        if (!strcmp(programs[programId].name, filename)) {
            Serial.println(F("INFO: Program exists!"));
            
            return programId;
        }
    }
    return -1;
}



void clearSerialBuffer() {
    delayMicroseconds(1024);
    while (Serial.available()) {
        Serial.read();
        delayMicroseconds(1024);
    }
}

// Function: setup
// Called by Arduino to start program
void setup() {
  
    Serial.begin(9600);
    Serial.println(F("ArduinoOS programmer (Thijs Dregmans) version 1.1 booted"));
    Serial.println(F("Started. Waiting for commands..."));
    Serial.println(F("Enter 'help' for help."));
  
    Serial.print(F("> "));
}

// Function: loop
// Called by Arduino repeatedly to run the program  
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
            Serial.println(F("ERROR: command not known"));
            Serial.println(F("Enter 'help' for help."));
        }
    
        Serial.print(F("> "));
    }
    
}
