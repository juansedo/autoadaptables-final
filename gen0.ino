#define LDR1  0   // LDR Light sensor from traffic light 1 connected in pin A0
#define LDR2  1   // LDR Light sensor from traffic light 2 connected in pin A1
#define CO2   3   // CO2 sensor connected in pin A3
#define P1    37  // Traffic light 1 button connected in pin 37
#define P2    36  // Traffic light 2 button connected in pin 36
#define CNY1  35  // Infrared sensor 1 in traffic light 1 connected in pin 35
#define CNY2  34  // Infrared sensor 2 in traffic light 1 connected in pin 34
#define CNY3  33  // Infrared sensor 3 in traffic light 1 connected in pin 33
#define CNY4  32  // Infrared sensor 4 in traffic light 2 connected in pin 32
#define CNY5  31  // Infrared sensor 5 in traffic light 2 connected in pin 31
#define CNY6  30  // Infrared sensor 6 in traffic light 2 connected in pin 30
#define LR1   22  // Red traffic light 1 connected in pin 22
#define LY1   23  // Yellow traffic light 1 connected in pin 23
#define LG1   24  // Green traffic light 1 connected in pin 24
#define LR2   25  // Red traffic light 2 connected in pin 25
#define LG2   27  // Green traffic light 2 connected in pin 27
#define LY2   26  // Yellow traffic light 2 connected in pin 26

enum States {
  A_SEMAPHORE,
  B_SEMAPHORE,
};

enum SemaphoreStates {
  STOP,
  READY,
  GO,
  HOLD
};

struct Semaphore {
  int green_led;
  int yellow_led;
  int red_led;
  int button;
  bool button_was_pushed;
  int state;
};

unsigned int intervalToYellow = 3000;
unsigned int intervalToRed = 1000;
unsigned int intervalToRedAndYellow = 500;
unsigned int intervalToGreen = 1000;

struct Semaphore s1, s2;
struct Semaphore* semaphores[2]; 
int start_time, time;
int state;

void go(Semaphore* s) {
  digitalWrite(s->green_led, HIGH);
  digitalWrite(s->yellow_led, LOW);
  digitalWrite(s->red_led, LOW);

  s->state = GO;
}

void stop(Semaphore* s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, LOW);
  digitalWrite(s->red_led, HIGH);

  s->state = STOP;
}

void ready(Semaphore* s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, HIGH);
  digitalWrite(s->red_led, HIGH);

  s->state = READY;
}

void hold(Semaphore* s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, HIGH);
  digitalWrite(s->red_led, LOW);

  s->state = HOLD;
}

void setup() {
  Serial.begin(9600);
  start_time = millis();

  s1.red_led = LR1;
  s1.yellow_led = LY1;
  s1.green_led = LG1;
  s1.button = P1;
  s1.button_was_pushed = false;
  go(&s1);

  s2.red_led = LR2;
  s2.yellow_led = LY2;
  s2.green_led = LG2;
  s2.button = P2;
  s2.button_was_pushed = false;
  s2.state = STOP;
  stop(&s2);

  semaphores[0] = &s2;
  semaphores[1] = &s1;

  state = A_SEMAPHORE;

  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  
  pinMode(P1, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);
  
  pinMode(LR1, OUTPUT);
  pinMode(LY1, OUTPUT);
  pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT);
  pinMode(LY2, OUTPUT);
  pinMode(LG2, OUTPUT);
  
  pinMode(CNY1, INPUT);
  pinMode(CNY2, INPUT);
  pinMode(CNY3, INPUT);
  pinMode(CNY4, INPUT);
  pinMode(CNY5, INPUT);
  pinMode(CNY6, INPUT);
}


void checkButton(Semaphore* s) {
  if (digitalRead(s->button) != LOW) {
    s->button_was_pushed = true;
  }
}

int waitTime() {
  int button_reducer = 0;
  if (s1.button_was_pushed && s1.state == GO) {
    button_reducer = 2000;
  }
  if (s2.button_was_pushed && s2.state == GO) {
    button_reducer = 2000;
  }
  return intervalToYellow - button_reducer;
}

void resetSemaphore(Semaphore* s) {
  s->button_was_pushed = false;
}

void transition() {
  stop(semaphores[0]);
  hold(semaphores[1]);
  delay(intervalToRed);
  stop(semaphores[0]);
  stop(semaphores[1]);
  delay(intervalToRedAndYellow);
  ready(semaphores[0]);
  stop(semaphores[1]);
  delay(intervalToGreen);
  go(semaphores[0]);
  stop(semaphores[1]);

  Semaphore* aux = semaphores[0];
  semaphores[0] = semaphores[1];
  semaphores[1] = aux;
  
  resetSemaphore(semaphores[0]);
  resetSemaphore(semaphores[1]);  
}

void loop() {
  time = millis();

  checkButton(&s1);
  checkButton(&s2);

  if (waitTime() < time - start_time) {
    transition();
    start_time = millis();
  }
}
