/**

   TINBES03
   ArduinOS
   Student: Thijs Dregmans (1024272)
   Version: 5.7
   Last edited: 2023-06-19

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
      - FLOAT IEEE754 implementation
*/

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

// Function: pushByte
// Pushes one byte on the stack of a process
void pushByte(int index, byte b) {
    process[index].stack[process[index].sp++] = b;
}

// Function: popByte
// Pops one byte from the stack of a process
byte popByte(int index) {
    return process[index].stack[--process[index].sp];
    // check if decrement operator must be pre or post 'sp'
}

// Function: popVal
// Pops the CHAR, INT or FLOAT from the stack of a process
float popVal(int index) {
    // assume that a variable of type INT, FLOAT or CHAR is on stack
  
    byte type = popByte(index);
    switch (type) {
        case INT:
            // read 2 bytes -> big-endian
            byte byte1 = popByte(index);
            byte byte2 = popByte(index);
      
            return byte1 + (byte2 * 256);
        case FLOAT:
            // read 4 bytes
            byte bytes[FLOAT];
            for (int i = 0; i < FLOAT; i++) {
              bytes[i] = popByte(index);
            }
      
            return doublePacked2Float(bytes);
        case CHAR:
            // read 1 byte
            return popByte(index);
        // no default case 
    }
}

void pushInt(int index, int i) {
    byte byte1 = lowByte(i);
    byte byte2 = highByte(i);

    pushByte(index, byte1);
    pushByte(index, byte2);
    pushByte(index, INT);
}

void pushFloat(int index, float f) {
    byte byte1 = 0x00;
    byte byte2 = 0x00;
    byte byte3 = 0x00;
    byte byte4 = 0x00;

    // apply IEEE-754 protocol
    pushByte(index, byte1);
    pushByte(index, byte2);
    pushByte(index, byte3);
    pushByte(index, byte4);
    
    pushByte(index, FLOAT);
}

void pushVal(int index, float value, int type) {
    switch (type) {
        case INT:
            pushInt(index, (int) value);
            break;
        case FLOAT:
            pushFloat(index, value);
            break;
        default:
            Serial.println(F("FATAL ERROR: Could not push value on the stack!"));
            break;
    }
}

// function may not be needed
void pushString(int index, char* s) {
    int size = sizeof(s);
    for(int i = size; i == 0; i--) {
        pushByte(index, s[i]);
    }
    
    pushByte(index, size);
    pushByte(index, STRING);
}

char *popString(int index) {
    // assume that a variable of type STRING is on stack
    int size = popByte(index);
    process[index].sp -= size;
    return (char *)(process[index].stack + process[index].sp);
}

// Function: readVal
// Reads a value from the EEPROM and pushes it on the stack
byte readVal(int index, int addr, byte Type) {
    for (byte i = 0; i < Type; i++) {
        Serial.println(EEPROM[addr + i]);
        pushByte(index, EEPROM[addr + i]);
    }
    pushByte(index, Type);
     debugStack(index);
    return Type;
}

void debugStack(int index) {
    for(int i = 0; i < process[index].sp -1; i++) {
      Serial.print(" -> ");
      Serial.println(process[index].stack[i]);
    }
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

// Function: binaryOp
// Perform a binary operation and push results on the stack
void binaryOp(int index, int op) {
    float y = popVal(index);
    float x = popVal(index);
    float result;
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
            break;
    }
    // push result on the stack
}

int findFreeProcess() {
    for(int i = 0; i < MAXPROCESSES; i++) {
        if (process[i].state != RUNNING && process[i].state != PAUSED) {
            return i;
        }
    }
    return -1;
}

void fork(int index) {
    int indexProcess = findFreeProcess();
    if (indexProcess == -1) {
        Serial.println(F("FATAL ERROR: Max no of processes reached! Could not fork."));
        return;
    }

    if (popByte(index) != STRING) {
        Serial.println(F("FATAL ERROR: No program name found on the stack to fork."));
        return;
    }

    char* filename = popString(index);
    int indexFile = locateFile(filename);

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
    if ((int) millis() < popVal(index)) {
        process[index].pc--;
    }
    else {
        popVal(index);
    }
}

void printStack(int index) {
    switch (process[index].stack[--process[index].sp]) {
        case CHAR:
            Serial.print((char) popVal(index));
            break;
        case INT:
            Serial.print("printing int");
            Serial.print((int) popVal(index));
            
            break;
        case FLOAT:
            Serial.print((float) popVal(index));
            break;
        case STRING:
            Serial.print(popString(index));
            break;  
        default:
            Serial.print(F("FATAL ERROR: Could not print the value on the stack!"));
            break;
    }
}

void printlnStack(int index) {
    printStack(index);
    Serial.println();
}

void terminateProcess(int processId) {
    process[processId].state = TERMINATED;
    Serial.print("INFO: Process ");
    Serial.print(processId);
    Serial.println(" terminated successfully!");

    // Delete all variables of the process
    deleteVariables(processId);
}

void get(int index) {
    char name = EEPROM[process[index].pc++];
    readVariable(name, index);
}

void set(int index) {
    char name = EEPROM[process[index].pc++];
    saveVariable(name, index);
}

void increment(int index) {
    pushVal(index, popVal(index) + 1, process[index].stack[process[index].sp + 1]); // check for errors pls
}

void decrement(int index) {
    pushVal(index, popVal(index) - 1, process[index].stack[process[index].sp + 1]); // check for errors pls
}

void delayTime(int index) {
    int miliseconds = popVal(index);
    delay(miliseconds);
}

// Function: execute
// Executes the next step of the process
void execute(int index) {
//    Serial.println(EEPROM[process[index].pc]) ;
    switch (EEPROM[process[index].pc]) {
      case CHAR:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc]);
          break;
      case INT:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc]);
          break;
      case FLOAT:
          process[index].pc += readVal(index, process[index].pc, EEPROM[process[index].pc]);
          break;
      case STRING:
          process[index].pc += readStr(index, process[index].pc);
          break;
      case PLUS:
          binaryOp(index, EEPROM[process[index].pc]);
          break;
      case MINUS:
          binaryOp(index, EEPROM[process[index].pc]);
          break;
      case TIMES:
          binaryOp(index, EEPROM[process[index].pc]);
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
      case PRINT:
          Serial.println("printing");
          printStack(index);
          break;
      case PRINTLN:
          printlnStack(index);
          break;
      case STOP:
          terminateProcess(index);
          break;
      case DELAY:
          delayTime(index);
          break;
      default:
          Serial.print(F("FATAL ERROR: Could not find the command "));
          Serial.println(EEPROM[process[index].pc]);
          break;
    }
    process[index].pc++;
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
        if (spaceInUse(pointer)) {
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
            return 0;
        }
    }
    Serial.println(F("ERROR: Could not find variable!"));
    return 1;
}

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
    Serial.println(F("ArduinoOS (Thijs Dregmans) version 5.7 booted"));
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
    runProcess();
}
