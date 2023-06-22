/**

   TINBES03
   ArduinOS
   Student: Thijs Dregmans (1024272)
   Version: 6.2
   Last edited: 2023-06-22

   Requirements:
      Het besturingssysteem
   Completed    - kan commando’s inlezen vanaf de command line via de seriële terminal.
   ---------    - kan programma’s in de voorgeschreven bytecode (opgeslagen als bestanden in het bestandssysteem) uitvoeren op de Arduino Uno of Nano.
   ---------    - beheert een geheugen van tenminste 256 bytes.
   ---------    - kan tenminste 25 variabelen van het type CHAR (1 byte), INT (2 bytes), FLOAT (4 bytes) of STRING (zero-terminated, variabel aantal bytes) in het geheugen houden, waarvan de waarde gezet, gelezen en gemuteerd kan worden.
   Completed    - beheert een bestandssysteem ter grootte van het beschikbare EEPROM-geheugen.
   Completed    - kan hierin tenminste 10 bestanden opslaan, teruglezen en wissen met bestandsnamen van maximaal 12 tekens (inclusief terminating zero).
   Completed    - kan de nog beschikbare hoeveelheid opslagruimte weergeven.
   Completed    - kan tot 10 verschillende processen bijhouden die gestart, gepauzeerd, hervat en beëindigd kunnen worden.
   Completed    - houdt bij van alle variabelen bij welk proces ze horen, en geeft het geheugen dat de variabelen innemen vrij als het proces stopt.
   ---------    - kan per proces 1 bestand tegelijk lezen of schrijven.
   Completed    - houdt per proces een stack bij van tenminste 32 bytes.


   Todo:
      - Check names of variables and functions
      - Provide comentary in functions
      - Change clear buffer command
*/

/* Initial variables and constants
**************************************************************************************** */

// Include libaries
#include <EEPROM.h>
#include <IEEE754tools.h>
// Include instruction set
#include "instruction_set.h"

// Define constants
#define MAXPROCESSES 10
#define STACKSIZE 32
#define BUFSIZE 12
#define FILENAMESIZE 12
#define FATSIZE 10
#define MAXIMUMVAR 25
#define MEMORYSIZE 256
// Define process states
#define RUNNING 'r'
#define PAUSED 'p'
#define TERMINATED 0


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

// Define struct for variables
typedef struct {
    byte name;
    int processId;
    byte type;
    byte start;
    byte size;
} varType;

// Define struct for processes
typedef struct {
    char name[FILENAMESIZE];
    int processId;
    byte state;
    int pc;
    int sp;
    byte stack[STACKSIZE];
    byte loopAddr;
} processType;

// Initialize global variables (counters)
int noOfProcesses = 0;
int noOfVars = 0;
static int FileTypeSize = sizeof(fileType);
EERef noOfFiles = EEPROM[0];

// Initialize global variables (memory)
static char Buffer[BUFSIZE];
byte memory[MEMORYSIZE];
varType memoryTable[MAXIMUMVAR];
processType process[MAXPROCESSES];

bool readToken (char Buffer[], bool spacebreak = true);

/* Command Line Functions
**************************************************************************************** */

// Callable command: help
// Prints information about available commands on screen
void help() {
    Serial.println(F("Here is a list with all available commands:"));
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
    Serial.println(F("    reboot                      reboots the OS."));
}

// Callable command: store [file] [size] [data]
// Stores a file with name [file] and size [size] in the File Allocation Table and stores [data] in EEPROM
void store() {
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
    
        // get file.size
        while (!readToken(Buffer)) {
            file.size = atoi(Buffer);
        }
    
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
    
        char* content;
        while (!readToken(Buffer, false)) {
            content = Buffer;
        }
    
        for (int byteId = 0; byteId < file.size; byteId++) {
            EEPROM[file.start + byteId] = content[byteId];
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

// Callable command: retrieve [file]
// Prints the content of file with name [file] on screen
void retrieve() {
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
        Serial.print((char) EEPROM[file.start + i]);
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
    for (int processId = 0; processId < noOfProcesses; processId++) {
        if (process[processId].name == filename && process[processId].state) {
            Serial.println(F("ERROR: File is running a process."));
            return;
        }
    }
    // doesnt work yet

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

// Callable command: run [file]
// Creates a process of the contents of file [file] and starts the process
void run() {
    // clear buffer
    Buffer[0] = 0;
  
    if (noOfProcesses >= MAXPROCESSES) {
        Serial.println(F("ERROR: Too much processes! Max 10."));
        Serial.println(F("Try 'list' to view all processes."));
    }
    else {
        int processId = findFreeProcess();
        if (processId == -1) {
            Serial.println(F("ERROR: No free process available."));
            return;
        }
        
        process[processId].processId = processId;
    
        // get file.name
        while (!readToken(Buffer)) {
            strcpy(process[processId].name, Buffer);
        }
    
        bool fileExists = false;
    
        fileType file;
        for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
            if (!strcmp(process[processId].name, readFATEntry(FATEntryId).name)) {
                fileExists = true;
                break;
            }
        }
    
        if (!fileExists) {
            Serial.println(F("ERROR: File doesn't exists."));
            Serial.println(F("Try 'files' to view all files."));
      
            // clear buffer
            Buffer[0] = 0;
      
            return;
        }
    
        // start process
        process[processId].state = RUNNING;
        process[processId].pc = file.start;
        process[processId].sp = 0;
        process[processId].loopAddr = 0;
    
        noOfProcesses++;
    
        Serial.print(F("INFO: Process "));
        Serial.print(processId);
        Serial.println(F(" started successfully."));
    }
  
    // clear buffer
    Buffer[0] = 0;
}

// Callable command: list
// Prints a list with all processes with their processId, name and state on screen
void list() {
    Serial.println(F("This is a list with all processes:"));
    Serial.println(F("    processId     name          state"));
    for (int processId = 0; processId < noOfProcesses; processId++) {
        Serial.print(F("    "));
        Serial.print(process[processId].processId);
        Serial.print(F("             "));
        Serial.print(process[processId].name);
        Serial.print(F("     "));
        Serial.println((char) process[processId].state);
    }
    Serial.print(noOfProcesses);
    Serial.println(F(" result(s)"));
}

// Callable command: suspend [id]
// Changes the state of process with processId [id] from RUNNING to PAUSED
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
  
    if (processId >= 0 && processId < noOfProcesses) {
        if (process[processId].state == RUNNING) {
            process[processId].state = PAUSED;
            Serial.print(F("INFO: Process "));
            Serial.print(processId);
            Serial.println(F(" paused successfully."));
        }
        else {
            Serial.print(F("WARNING: Process "));
            Serial.print(processId);
            Serial.println(F(" could not be paused!"));
        }
    }
    else {
      Serial.println(F("ERROR: Process could not be found!"));
    }
  
    // clear buffer
    Buffer[0] = 0;
}

// Callable command: resume [id]
// Changes the state of process with processId [id] from PAUSED to RUNNING
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
  
    if (processId >= 0 && processId < noOfProcesses) {
        if (process[processId].state == PAUSED) {
            process[processId].state = RUNNING;
            Serial.print(F("INFO: Process "));
            Serial.print(processId);
            Serial.println(F(" restarted successfully."));
        }
        else {
            Serial.print(F("WARNING: Process "));
            Serial.print(processId);
            Serial.println(F(" could not be restarted!"));
        }
    }
    else {
        Serial.println(F("ERROR: Process could not be found!"));
    }
  
    // clear buffer
    Buffer[0] = 0;
}

// Callable command: kill [id]
// Changes the state of process with processId [id] from RUNNING or PAUSED to TERMINATED and deletes all variables associated with the process
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
  
    if (processId >= 0 && processId <= MAXPROCESSES) {
        terminateProcess(processId);
    }
    else {
        Serial.println(F("ERROR: Process could not be found!"));
    }
  
    // clear buffer
    Buffer[0] = 0;
}

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0

// Callable command: reboot
// Calls function that points to begin of the program
void reboot() {
    delay(10);
    resetFunc(); //call reset
}

// Array with callable commands
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
    {"reboot", &reboot}
};

static int commandSize = sizeof(command) / sizeof(commandType);

/* EEPROM Memory management
**************************************************************************************** */

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

/* RAM Memory management
**************************************************************************************** */

// Function: memorySpaceInUse
// Checks if the memory if byte is in use a variable.
bool memorySpaceInUse(int addr) {
    for (int varId = 0; varId < noOfVars; varId++) {
        varType var;
        var = memoryTable[varId];
    
        if (var.start <= addr && (var.start + var.size) > addr) {
            return true;
        }
    }
    return false;
}

// Function: memoryEmptySpace
// Returns the first empty space where the var fits.
int memoryEmptySpace(int varsize) {
    int pointer = 0;
    int freespace = 0;
    do {
        if (memorySpaceInUse(pointer)) {
            freespace = 0;
        }
        else {
            freespace++;
        }
        pointer++;
        if (freespace >= varsize) {
            return pointer - varsize;
        }
    } while (pointer < MEMORYSIZE);
    return -1;
}

/* Read incomming char on Command Line
**************************************************************************************** */

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

/* Elementary Variable Functions
**************************************************************************************** */

// Function: saveVariable
// Saves a variable with name [name] for processId [processId] with the content and type from the stack
void saveVariable(byte name, int processId) {
    if (noOfVars >= MAXIMUMVAR) {
        Serial.println(F("ERROR: Too much variables. Max 25."));
        Serial.println(F("Try 'list' to view all processes."));
        Serial.println(F("Kill a process that uses a variable to get access to one."));
    }
    else {
        // delete var
        for (int varId = 0; varId < noOfVars; varId++) {
            if (memoryTable[varId].name == name && memoryTable[varId].processId == processId) {
                for (int i = varId; i < noOfVars; i++) {
                    memoryTable[i] = memoryTable[i + 1];
                }
                noOfVars--;
                break;
            }
        }
        // (re)create var
        varType var;
        var.name = name;
        var.processId = processId;
        var.type = popByte(processId);
        
        if (var.type == CHAR || var.type == INT || var.type == FLOAT) {
            var.size = var.type;
        }
        else {
            // pop size of STRING from stack
            var.size = popByte(processId);
        }
        
        // find empty space in memory to put variable
        int emptySpaceStart = memoryEmptySpace(var.size);
        if (emptySpaceStart == -1) {
            Serial.println(F("ERROR: No space to store variable!"));
            return;
        }
    
        var.start = emptySpaceStart;

        for (int byteId = 0; byteId < var.size; byteId++) {
            memory[var.start + byteId] = popByte(processId);
        }
        memoryTable[noOfVars++] = var;

    }
}

// Function: readVariable
// Pushes the value of variable with name [name] on the stack
int readVariable(byte name, int processId) {
    // read variable from memory and push it on the stack
    // return 0 -> pushed successfully
    // return 1 -> error
    for (int varId = 0; varId < noOfVars; varId++) {
        varType var = memoryTable[varId];
        if (var.name == name && var.processId == processId) {
            if (var.type == CHAR || var.type == INT || var.type == FLOAT) {
                for (int byteId = 0; byteId < var.size; byteId++) {
                    pushByte(processId, memory[var.start + byteId]);
                    // May need to be reversed: BIG OR LITTLE ENDIAN !?!?!
                }
            }
            else {
                // assume: var.type == STRING
                for (int byteId = 0; byteId < var.size; byteId++) {
                    pushByte(processId, memory[var.start + byteId]);
                    // May need to be reversed: BIG OR LITTLE ENDIAN !?!?!
                }
                pushByte(processId, var.size);
            }
            pushByte(processId, var.type);
            pushByte(processId, var.type); // For some stupid reason, does it only push the var.type when you push it twice
            return 0;
        }
    }
    Serial.println(F("ERROR: Could not find variable!"));
    return 1;
}


// Function: deleteVariables
// Deletes all varaiables related to the process [processId]
void deleteVariables(int processId) {
    for (int varId = 0; varId < noOfVars; varId++) {
        if (memoryTable[varId].processId == processId) {
            for (int i = varId; i < noOfVars; i++) {
                memoryTable[i] = memoryTable[i + 1];
            }
            noOfVars--;
        }
    }
}

/* Elementary Process Functions
**************************************************************************************** */

// Function: findFreProcess
// Returns the index for a new process, or -1 if there is no space
int findFreeProcess() {
    for(int i = 0; i < MAXPROCESSES; i++) {
        if (process[i].state != RUNNING && process[i].state != PAUSED) {
            return i;
        }
    }
    return -1;
}

// Function: pushByte
// Pushes one byte on the stack of a process
void pushByte(int index, byte b) {
    process[index].stack[process[index].sp++] = b;
}

// Function: popByte
// Pops one byte from the stack of a process
byte popByte(int index) {
    if (process[index].sp > 0) {
        return process[index].stack[--process[index].sp];
        // decrement operator must be pre 'sp'
    }
}

// Function: pushChar
// Pushes a Char [c] to the strack
void pushChar(int index, char c) {
    pushByte(index, (byte) c);
    pushByte(index, CHAR);
}

// Function: popChar
// Pops the Char from the stack of a process
char popChar(int index) {
    popByte(index);
    return (char) popByte(index);
}

// Function: pushInt
// Pushes a Int [c] to the strack
void pushInt(int index, int i) {
    pushByte(index, (byte) highByte(i));
    pushByte(index, (byte) lowByte(i));
    pushByte(index, INT);
}

// Function: popInt
// Pops the Int from the stack of a process
int popInt(int index) {
    popByte(index);
    return popByte(index) * 256 + popByte(index);
}

// Function: pushFloat
// Pushes a Float [c] to the strack
void pushFloat(int index, float f) {
    // Source: https://stackoverflow.com/questions/63049683/arduino-convert-float-to-hex-ieee754-single-precision-32-bit
    byte bytes[FLOAT];
    
    byte* f_byte = reinterpret_cast<byte*>(&f);
    memcpy(bytes, f_byte, 4);
    
    for (int i = FLOAT - 1; i > -1; i--) {
        pushByte(index, bytes[i]);
    }
    
    pushByte(index, FLOAT);
}

// Function: popFloat
// Pops the Float from the stack of a process
float popFloat(int index) {
    // Source: https://forum.arduino.cc/t/8-bytes-to-double-conversion/952352/2
    popByte(index);

    float f;
    byte bytes[FLOAT];
    for (int i = FLOAT - 1; i > -1; i--) {
        bytes[i] = popByte(index);
    }
    memcpy (&f, bytes, 4);
    
    return f;
}

// Function: pushString
// Pushes a string [s] to the strack
void pushString(int index, char* s) {
    int size = sizeof(s);
    for(int i = size; i == 0; i--) {
        pushByte(index, s[i]);
    }
    
    pushByte(index, size);
    pushByte(index, STRING);
}

// Function: popString
// Pops the String from the stack of a process
String popString(int index) {
    // assume that a variable of type STRING is on stack
    String s;
    popByte(index);
    int size = popByte(index);

    for (int i = 0; i < size; i++) {
        s += (char) popByte(index);
    }
    
    return s;
}

// Function: readVal
// Reads a value from the EEPROM and pushes it on the stack
byte readVal(int index, int addr, byte Type) {
    for (byte i = 0; i < Type; i++) {
        pushByte(index, EEPROM[addr + i]);
    }
    pushByte(index, Type);
    return Type;

}

// Function: readStr
// Reads a string from the EEPROM and pushes it on the stack
byte readStr(int index, int pc) {
    // read # of bytes, associated with a process
    // defined by processId: index
  
    // push it on stack
  
    byte size = 0;
    
    do {
        pushByte(index, EEPROM[pc]);
        size++;
    } while (EEPROM[pc++]);
    
    pushByte(index, size);
    pushByte(index, STRING);

    return size;
}

/* Program Commands functions
**************************************************************************************** */

// Function: binaryOp
// Perform a binary operation and push results on the stack
void binaryOp(int index, int op) {
    // define type somehow
    byte type = popByte(index); // check this
    switch (type) {
        case INT:
            int x = popInt(index);
            int y = popInt(index);
            int result;

            // perform binary operation
            switch (op) {
                case PLUS:
                    result = x + y;
                    break;
                case MINUS:
                    result = x - y;
                    break;
                case TIMES:
                    result = x * y;
                    break;
                default:
                    Serial.println(F("FATAL ERROR: Binary operations can only be called with a known operator on the stack!"));
                    return;
            }

            pushInt(index, result);
            return;
        case FLOAT:
            float xf = popFloat(index);
            float yf = popFloat(index);
            float resultf;

            // perform binary operation
            switch (op) {
                case PLUS:
                    resultf = xf + yf;
                    break;
                case MINUS:
                    resultf = xf - yf;
                    break;
                case TIMES:
                    resultf = xf * yf;
                    break;
                default:
                    Serial.println(F("FATAL ERROR: Binary operations can only be called with a known operator on the stack!"));
                    return;
            }

            
            pushFloat(index, resultf);
            return;
        default:
            Serial.println("ERROR! No type found on the stack to perform binary operation!");
            return;
    }
}

void stackValsEquals(int index) {
    switch (popByte(index)) {
        case CHAR: // stackValsEquals only tested for case INT
            pushByte(index, CHAR);
            char char1 = popChar(index);
            popByte(index);
            char char2 = popChar(index);

            if (char1 == char2) {
                pushByte(index, 0x01);
            }
            else {
                pushByte(index, 0x00);
            }
            break;
        case INT:
            pushByte(index, INT);
            int int1 = popInt(index);
            popByte(index);
            int int2 = popInt(index);
            // for program 'test_fork': 
            // NOTE: something goes wrong with reading of the INT.
            // the program calls readVal to put the bytes in the EEPROM on the stack. Sometimes I use little endian, but sometimes big endian !!!

            if (int1 == int2) {
                pushByte(index, 0x01);
            }
            else {
                pushByte(index, 0x00);
            }
            break;
        case FLOAT: // stackValsEquals only tested for case INT
            pushByte(index, FLOAT);
            float float1 = popFloat(index);
            popByte(index);
            float float2 = popFloat(index);

            if (float1 == float2) {
                pushByte(index, 0x01);
            }
            else {
                pushByte(index, 0x00);
            }
            break;
         default:
            Serial.println("ERROR! No value to compare for EQUAL!");
            break;
    }
}

void get(int index) {
    // read a variable name from the file, and push the variable value unto the stack
    char name = EEPROM[process[index].pc++];
    readVariable(name, index);
}

void set(int index) {
    char name = EEPROM[process[index].pc++];
    saveVariable(name, index);
}

void increment(int index) {
    byte type = popByte(index);
//    pushByte(index, type); // push type back on the stack

    switch (type) {
        case INT:
            pushInt(index, popInt(index) + 1);
            break;
        case FLOAT:
            pushFloat(index, popFloat(index) + 1.0);
            break;
        case CHAR:
            pushChar(index, (int) popChar(index) + 1);
            break;
        default:
            Serial.println("ERROR! No type found on the stack!");
            break;
    }
}

void decrement(int index) {
     byte type = popByte(index);
//    pushByte(index, type); // push type back on the stack

    switch (type) {
        case INT:
            pushInt(index, popInt(index) - 1);
            break;
        case FLOAT:
            break;
        case CHAR:
            pushChar(index, (int) popChar(index) - 1);
            break;
        default:
            Serial.println("ERROR! No type found on the stack!");
            break;
    }
}

void fork(int index) {
    Serial.println("FORKING");
    int indexProcess = findFreeProcess();
    if (indexProcess == -1) {
        Serial.println(F("FATAL ERROR: Max no of processes reached! Could not fork."));
        return;
    }

    if (popByte(index) != STRING) {
        Serial.println(F("FATAL ERROR: No program name found on the stack to fork."));
        return;
    }

    
    char* filename = popString(index).c_str();
    int indexFile = locateFile(filename);

    Serial.println(filename);
    if(indexFile == -1) {
        Serial.println(F("FATAL ERROR: Could not find file to fork."));
        return;
    }

    fileType file = readFATEntry(indexFile);

    strcpy(process[indexProcess].name, file.name);
    process[indexProcess].processId = indexProcess;
    process[indexProcess].pc = file.start;
    process[indexProcess].sp = 0;
    process[indexProcess].state = RUNNING;

    pushInt(index, process[indexProcess].processId);

}

void delayUntil(int index) {
    if ((int) millis() < popInt(index)) {
        process[index].pc--;
    }
    else {
        popInt(index);
    }
}

void pushMillis(int index) {
    Serial.println("Pushing millis on the stack");
    pushInt(index, millis());
    debugStack(index);
}

void printStack(int index) {
    switch (popByte(index)) {
        case CHAR:
            // pushByte not needed for CHAR type
            Serial.print((char) popChar(index));
            break;
        case INT:
            // pushByte not needed for INT type
            Serial.print((int) popInt(index));
            break;
        case FLOAT:
            // pushByte not needed for FLOAT type
            Serial.print((float) popFloat(index));
            break;
        case STRING:
            // pushByte not needed for STRING type
            Serial.print(popString(index));
            break;  
        default:
            terminateProcess(index);
            Serial.print(F("FATAL ERROR: Could not print the value on the stack!"));
            break;
    }
}

void printlnStack(int index) {
    printStack(index);
    Serial.println();
}

void ifTrueStack(int index) {
    popByte(index);
    if(popByte(index)) {
        Serial.println("True");
        process[index].pc++;
        
    }
    else {
        Serial.println("False");
        // read int from EEPROM
        pushByte(index, EEPROM[process[index].pc++]);
        int lengthOfTrueCode = (int) popByte(index);
        process[index].pc += lengthOfTrueCode;
    }
} // not tested completely

void endIf(int index) {
    // manual says to remove the value from the stack! Which one?
      debugStack(index);
}

void terminateProcess(int processId) {
    process[processId].state = TERMINATED;
    // Serial.print("INFO: Process ");
    // Serial.print(processId);
    // Serial.println(" terminated successfully!");

    // Delete all variables of the process
    deleteVariables(processId);
}

void delayTime(int index) {
    int miliseconds = popByte(index);
    delay(miliseconds);
}

void startLoop(int index) {
    process[index].loopAddr = process[index].pc;
    Serial.println("Started LOOP");
}

void endLoop(int index) {
    process[index].pc = process[index].loopAddr;
}

/* Execute function
**************************************************************************************** */

// Function: execute
// Executes the next step of the process
void execute(int index) {
    switch (EEPROM[process[index].pc++]) {
      case CHAR:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
          break;
      case INT:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
          break;
      case FLOAT:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
          break;
      case STRING:
          process[index].pc += readStr(index, process[index].pc);
          break;
      case PLUS:
          binaryOp(index, EEPROM[process[index].pc - 1]);
          break;
      case MINUS:
          binaryOp(index, EEPROM[process[index].pc - 1]);
          break;
      case TIMES:
          binaryOp(index, EEPROM[process[index].pc - 1]);
          break;
      case EQUALS:
          stackValsEquals(index);
          break;
      case GET:
          get(index);
          break;
      case SET:
          set(index);
          break;
      case INCREMENT:
          increment(index);
          break;
      case DECREMENT:
          decrement(index);
          break;
      case FORK:
          fork(index);
          break;
      case DELAYUNTIL:
          delayUntil(index);
          break;
      case MILLIS:
          pushMillis(index);
          break;
      case PRINT:
          printStack(index);
          break;
      case PRINTLN:
          printlnStack(index);
          break;
      case IF:
          ifTrueStack(index);
          break;
      case ENDIF:
          endIf(index);
          break;
      case STOP:
          terminateProcess(index);
          break;
      case DELAY:
          delayTime(index);
          break;
      case LOOP:
          startLoop(index);
          break;
      case ENDLOOP:
          endLoop(index);
          break;
      default:
          Serial.print(F("FATAL ERROR: Could not find the command "));
          Serial.println(EEPROM[process[index].pc - 1]);
          break;
    }
}

// Function: runProcess
// Calls the execute function for all RUNNING processes
void runProcess() {
    for (int processId = 0; processId < noOfProcesses; processId++) {
        if (process[processId].state == RUNNING) {
            execute(processId);
        }
    }
}


/* Debug help functions
**************************************************************************************** */

void printVars() {
    Serial.println("    name  type  processId   size  content");
    for (int varId = 0; varId < noOfVars; varId++) {
        Serial.print("    ");
        Serial.print(memoryTable[varId].name);
        Serial.print("    ");
        Serial.print(memoryTable[varId].type);
        Serial.print("    ");
        Serial.print(memoryTable[varId].processId);
        Serial.print("    ");
        for (int byteId = 0; byteId < memoryTable[varId].size; byteId++) {
            Serial.print(memory[memoryTable[varId].start + byteId]);
            Serial.print(",");
        }
        Serial.println();
    }
}

void debugStack(int index) {
    Serial.println("Stack debugger: ----");
    Serial.println(process[index].sp);
    for(int i = 0; i < process[index].sp; i++) {
      Serial.print(" -> ");
      Serial.println(process[index].stack[i]);
    }
    Serial.println("----");
}

/* Setup of OS
**************************************************************************************** */

// Function: setup
// Called by Arduino to start program
void setup() {
  
    Serial.begin(9600);
    Serial.println(F("ArduinoOS (Thijs Dregmans) version 6.2 booted"));
    Serial.println(F("Started. Waiting for commands..."));
    Serial.println(F("Enter 'help' for help."));
  
    Serial.print(F("> "));
}


/* MAIN FUNCTION: loop()
**************************************************************************************** */

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
    runProcess();
}
