/**
 * 
 * TINBES03
 * practicum2
 * Student: Thijs Dregmans (1024272)
 * Version: 2.0
 * Last edit: 2023-04-24
 * 
 */

void store() {
  Serial.println("Dit is de store function.");
}

 
#define BUFSIZE 12
static char Buffer[BUFSIZE];

typedef struct {
    char name[BUFSIZE];
    void *func;
} commandType ;

static commandType command[] = {
    {"store", &store}
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
    Serial.println("Welcome to ArduinoOS...");
}
    
void loop() {
    if (readToken(Buffer)) {
        for (int i = 0; i < n; i++) {
            if (!strcmp(Buffer, command[i].name)) {
                void (*func) () = command[i].func;
                func();
            }
            else {
                Serial.println("command not found");
            }
        }
    }
}
