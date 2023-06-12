/**

   TINBES03
   ArduinOS
   Student: Thijs Dregmans (1024272)
   Version: 5.3
   Last edited: 2023-06-09

   Requirements:
      Het besturingssysteem
   Completed    - kan commando’s inlezen vanaf de command line via de seriële terminal.
   ---------    - kan programma’s in de voorgeschreven bytecode (opgeslagen als bestanden in het bestandssysteem) uitvoeren op de Arduino Uno of Nano.
   ---------    - beheert een geheugen van tenminste 256 bytes.
   ---------    - kan tenminste 25 variabelen van het type CHAR (1 byte), INT (2 bytes), FLOAT (4 bytes) of STRING (zero-terminated, variabel aantal bytes) in het geheugen houden, waarvan de waarde gezet, gelezen en ge-muteerd kan worden.
   Completed    - beheert een bestandssysteem ter grootte van het beschikbare EEPROM-geheugen.
   Completed    - kan hierin tenminste 10 bestanden opslaan, teruglezen en wissen met bestandsnamen van maximaal 12 tekens (inclusief terminating zero).
   Completed    - kan de nog beschikbare hoeveelheid opslagruimte weergeven.
   Completed    - kan tot 10 verschillende processen bijhouden die gestart, gepauzeerd, hervat en beëindigd kunnen worden.
   ---------    - houdt bij van alle variabelen bij welk proces ze horen, en geeft het geheugen dat de variabelen innemen vrij als het proces stopt.
   ---------    - kan per proces 1 bestand tegelijk lezen of schrijven.
   Completed    - houdt per proces een stack bij van tenminste 32 bytes.


   Todo:
      - Use F macro for Serial.print()
      - Check names of variables and functions
      - Create universal error messages
      - Make Command Line output universal
      - Provide comentary in functions
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
#define MAXIMUMVAR 25 // check value
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
  int fp;
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

// Callable command: store [file] [size] [data]
// Stores a file with name [file] and size [size] in the File Allocation Table and stores [data] in EEPROM
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
    while (!readToken(Buffer)) {
      strcpy(file.name, Buffer);
    }

    for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
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
//  Serial.print(filename);

  if (index == -1) {
//    Serial.println(" not found");
    return;
  }
//  Serial.println(" found");

  fileType file = readFATEntry(index);

  noOfFiles--;

  for (int i = index; i < noOfFiles; i++) {
    writeFAT(i, readFATEntry(i + 1));
  }
  Serial.print(filename);
  Serial.println(" erased successfully");

  // clear buffer
  Buffer[0] = 0;
}

// Callable command: files
// Prints a list with stored files on screen
void files() {
  Serial.println("This is a list with all files:");
  Serial.println("    name                        size");
  int counter = 0;
  for (int FATEntryId = 0; FATEntryId < noOfFiles; FATEntryId++) {
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
}

// Callable command: freespace
// Prints the size of the largest contiguouse free memmory space on screen
void freespace() {
  int maxSize = 0;
  while (emptySpace(maxSize) != -1) {
    maxSize++;
  }
  Serial.print("The biggest contiguous free memmory space is ");
  Serial.print(maxSize);
  Serial.println(" bytes.");
}

// Callable command: run [file]
// Creates a process of the contents of file [file] and starts the process
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

// Callable command: list
// Prints a list with all processes with their processId, name and state on screen
void list() {
  Serial.println("This is a list with all processes:");
  Serial.println("    processId     name          state");
  for (int processId = 0; processId < noOfProcesses; processId++) {
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

  if (processId >= 0 && processId < noOfProcesses) {
    process[processId].state = TERMINATED;
    Serial.print("Process ");
    Serial.print(processId);
    Serial.println(" terminated successfully.");

    // !!!! delete all variables of the process !!!!
  }
  else {
    Serial.println("ERROR");
  }

  // clear buffer
  Buffer[0] = 0;
}

void wipe() {
    noOfFiles = 0;
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
  {"wipe", &wipe}
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
//      Serial.println(Buffer);

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
bool writeFAT (int index, fileType file) {
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
  return process[index].stack[process[index].sp--];
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
  }
}

// Function: readVal
// Reads a value from the EEPROM and pushes it on the stack
byte readVal(int index, int filePointer, byte Type) {
  for (byte i = 0; i < Type; i++) {
    pushByte(index, EEPROM[filePointer++]); // does this increment the local variable or the value in the struct !?!?!
  }
  pushByte(index, Type);
  return Type;
}

// Function: readStr
// Reads a string from the EEPROM and pushes it on the stack
byte readStr(int index, int fp) {
  // read # of bytes, associated with a process
  // defined by processId: index

  // push it on stack

  byte size = 0;

  do {
    pushByte(index, EEPROM[fp]);
    size++;
  } while (EEPROM[fp++]);
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
      Serial.println("ERROR");
      Serial.println("binaryOp can only be called with a known operator");
      break;
  }
  // push result on the stack

}

// Function: execute
// Executes the next step of the process
void execute(int index) {
  switch (EEPROM[process[index].pc++]) {
    case CHAR:
      process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
      break;
    case INT:
      process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
      break;
    case FLOAT:
      process[index].pc = readVal(index, process[index].pc, EEPROM[process[index].pc - 1]);
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
    default:
      Serial.print("Could not find the command ");
      Serial.println(EEPROM[process[index].pc]);
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
        Serial.println("ERROR: Too much variables. Max 25.");
        Serial.println("Try 'list' to view all processes.");
        Serial.println("Kill a process that uses a variable to get access to one.");
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
            Serial.println("no space to store variable");
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
    Serial.println("ERROR! Could not find variable.");
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
//  Serial.println("ArduinoOS (Thijs Dregmans) version 4.0");
//  Serial.println("Started. Waiting for commands...");
//  Serial.println("Enter 'help' for help.");
//
  Serial.print("> ");
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
      Serial.println("ERROR: command not known");
      Serial.println("Enter 'help' for help.");
    }

    Serial.print("> ");
  }
}
