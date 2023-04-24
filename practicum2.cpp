/**
 * 
 * TINBES03
 * practicum2
 * Student: Thijs Dregmans (1024272)
 * Version: 2.1
 * Last edit: 2023-04-24
 * 
 */

void store() {
  Serial.println("This is the store function.");
}

void help() {
  Serial.println("This is the help function.");
}

 
#define BUFSIZE 12
static char Buffer[BUFSIZE];

typedef struct {
    char name[BUFSIZE];
    void *func;
} commandType ;

static commandType command[] = {
    {"store", &store},
    {"help", &help}
};

static int n = sizeof(command) / sizeof(commandType);

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
        for (int i = 0; i < n; i++) {
            if (!strcmp(Buffer, command[i].name)) {
                void (*func) () = command[i].func;
                func();
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
