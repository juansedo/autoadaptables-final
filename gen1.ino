#define LDR1 0  // LDR Light sensor from traffic light 1 connected in pin A0
#define LDR2 1  // LDR Light sensor from traffic light 2 connected in pin A1
#define CO2 3   // CO2 sensor connected in pin A3
#define P1 37   // Traffic light 1 button connected in pin 37
#define P2 36   // Traffic light 2 button connected in pin 36
#define CNY1 35 // Infrared sensor 1 in traffic light 1 connected in pin 35
#define CNY2 34 // Infrared sensor 2 in traffic light 1 connected in pin 34
#define CNY3 33 // Infrared sensor 3 in traffic light 1 connected in pin 33
#define CNY4 32 // Infrared sensor 4 in traffic light 2 connected in pin 32
#define CNY5 31 // Infrared sensor 5 in traffic light 2 connected in pin 31
#define CNY6 30 // Infrared sensor 6 in traffic light 2 connected in pin 30
#define LR1 22  // Red traffic light 1 connected in pin 22
#define LY1 23  // Yellow traffic light 1 connected in pin 23
#define LG1 24  // Green traffic light 1 connected in pin 24
#define LR2 25  // Red traffic light 2 connected in pin 25
#define LG2 27  // Green traffic light 2 connected in pin 27
#define LY2 26  // Yellow traffic light 2 connected in pin 26

enum States {
  A_SEMAPHORE,
  B_SEMAPHORE,
  NIGHT_MODE,
  FLASHING_RED,
};

enum SemaphoreStates {
  STOP,
  READY,
  GO,
  HOLD,
  STANDBY
};

struct Semaphore {
  int green_led;
  int yellow_led;
  int red_led;
  int button;
  bool button_was_pushed;
  int state;
  bool flashing;      // Nuevo campo para manejar el parpadeo
  bool is_flash_mode; // Nuevo campo para manejar el modo de parpadeo
};

unsigned int intervalToYellow = 3000;
unsigned int intervalToRed = 1000;
unsigned int intervalToRedAndYellow = 500;
unsigned int intervalToGreen = 1000;
unsigned int flashingInterval = 500;
unsigned int nightModeSwitchInterval = 10000; // Tiempo que alternan entre intermitente y bloqueado

struct Semaphore s1, s2;
struct Semaphore *semaphores[2];
int start_time, time;
int state;
bool force_night = false; // Indica si el modo noche está forzado por Python
unsigned long last_flash_time = 0;
unsigned long last_night_switch_time = 0;
bool flash_state = false;

void go(Semaphore *s) {
  digitalWrite(s->green_led, HIGH);
  digitalWrite(s->yellow_led, LOW);
  digitalWrite(s->red_led, LOW);
  s->state = GO;
}

void stop(Semaphore *s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, LOW);
  digitalWrite(s->red_led, HIGH);
  s->state = STOP;
}

void ready(Semaphore *s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, HIGH);
  digitalWrite(s->red_led, HIGH);
  s->state = READY;
}

void hold(Semaphore *s) {
  digitalWrite(s->green_led, LOW);
  digitalWrite(s->yellow_led, HIGH);
  digitalWrite(s->red_led, LOW);
  s->state = HOLD;
}

void standBy(Semaphore *s) {
  if (flashingInterval <= millis() - last_flash_time) {
    digitalWrite(s->red_led, !flash_state ? HIGH : LOW);
    last_flash_time = millis();
  }
}

void sendLog() {
  int ldr1_value = analogRead(LDR1);
  int ldr2_value = analogRead(LDR2);
  int co2_value = analogRead(CO2);
  int cny1_value = digitalRead(CNY1);
  int cny2_value = digitalRead(CNY2);
  int cny3_value = digitalRead(CNY3);
  int cny4_value = digitalRead(CNY4);
  int cny5_value = digitalRead(CNY5);
  int cny6_value = digitalRead(CNY6);

  Serial.print("LDR1: ");
  Serial.print(ldr1_value);
  Serial.print(", LDR2: ");
  Serial.print(ldr2_value);
  Serial.print(", CO2: ");
  Serial.print(co2_value);
  Serial.print(", CNY1: ");
  Serial.print(cny1_value);
  Serial.print(", CNY2: ");
  Serial.print(cny2_value);
  Serial.print(", CNY3: ");
  Serial.print(cny3_value);
  Serial.print(", CNY4: ");
  Serial.print(cny4_value);
  Serial.print(", CNY5: ");
  Serial.print(cny5_value);
  Serial.print(", CNY6: ");
  Serial.println(cny6_value);
}

void setup() {
  Serial.begin(9600);
  start_time = millis();

  s1.red_led = LR1;
  s1.yellow_led = LY1;
  s1.green_led = LG1;
  s1.button = P1;
  s1.button_was_pushed = false;
  s1.flashing = false;
  s1.is_flash_mode = true;
  go(&s1);

  s2.red_led = LR2;
  s2.yellow_led = LY2;
  s2.green_led = LG2;
  s2.button = P2;
  s2.button_was_pushed = false;
  s2.flashing = false;
  s2.is_flash_mode = false;
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

void checkButton(Semaphore *s) {
  if (digitalRead(s->button) != LOW) {
    s->button_was_pushed = true;
    // Cambiar estado de parpadeo
    s1.is_flash_mode = !s1.is_flash_mode;
    s2.is_flash_mode = !s2.is_flash_mode;
    // Restablecer el temporizador de alternancia de noche
    last_night_switch_time = millis();
  }
  else if (digitalRead(s->button) != HIGH) {
    s->button_was_pushed = false;
  }
}

bool isRoadEmpty() {
  int sensors1 = digitalRead(CNY1) + digitalRead(CNY2) + digitalRead(CNY3);
  int sensors2 = digitalRead(CNY4) + digitalRead(CNY5) + digitalRead(CNY6);
  return (sensors1 > 2) && (sensors2 > 2);
}

bool getNightModeFromSerial() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("CMD:")) {
      String time_of_day = input.substring(4);
      Serial.println(time_of_day);
      if (time_of_day == "NIGHT") {
        return true;
      }
      else if (time_of_day == "DAY") {
        return false;
      }
    }
  }
}

bool getNightModeFromLightSensors(int sensor) {
  return (analogRead(LDR1) < 100) && (analogRead(LDR2) < 100);
}

void checkNightMode() {
  force_night = getNightModeFromSerial();
  bool is_daylight = getNightModeFromLightSensors();
  bool is_road_empty = isRoadEmpty();

  return (force_night || is_daylight) && is_road_empty;
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

void resetSemaphore(Semaphore *s) {
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

  Semaphore *aux = semaphores[0];
  semaphores[0] = semaphores[1];
  semaphores[1] = aux;

  resetSemaphore(semaphores[0]);
  resetSemaphore(semaphores[1]);
}

void nightTransition() {
  if (s1.is_flash_mode) {
    standBy(&s1);
  } else {
    stop(&s1);
  }

  if (s2.is_flash_mode) {
    standBy(&s2);
  } else {
    stop(&s2);
  }
}

void switchNightMode() {
  unsigned long current_time = millis();
  if (nightModeSwitchInterval <= current_time - last_night_switch_time) {
    s1.is_flash_mode = !s1.is_flash_mode;
    s2.is_flash_mode = !s2.is_flash_mode;
    last_night_switch_time = current_time;
  }
}

void loop() {
  time = millis();

  checkButton(&s1);
  checkButton(&s2);  

  sendLog(); // Enviar los datos de los sensores por serial

  if (checkNightMode()) {
    digitalWrite(LG1, LOW);
    digitalWrite(LG2, LOW);
    switchNightMode();
    nightTransition();
  } else {
    if (waitTime() < time - start_time) {
      transition();
      start_time = millis();
    }
  }
}
