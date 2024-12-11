// LDR and LED setup
const int North_ldr = A1;
const int East_ldr = A2;
const int South_ldr = A3;
const int West_ldr = A4;

const int CenterLED = 5;
const int NorthWhite = 6;
const int EastBlue = 7;
const int EastWhite = 8;
const int SouthWhite = 9;
const int WestBlue = 10;
const int WestWhite = 11;

const int buttonPin = 13;
const int ledMode = 12;
const int potPin = A5;

int THRESHOLD = 50;

// Ultrasonic and Humidity setup
const int trigPin = 3;
const int echoPin = 4;
const int LED = 2; // LED for humidity
const int hSensor = A0;

// Timing variables
unsigned long lastButtonTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastUltrasonicTime = 0;
unsigned long lastBlinkTime = 0;

const unsigned long buttonDebounceDelay = 200; // Debounce delay
const unsigned long logInterval = 500; // Data logging interval
const unsigned long ultrasonicInterval = 500; // Ultrasonic interval
long blinkInterval = 1000; // Blink interval for LEDs
unsigned long lastDataSendTime = 0;

// Variables
int mode = 0;
int luxReference = 600;
int lastButtonState = HIGH;
int avgLux = 0;
bool blinking = false;
int blinkCount = 0;

bool sensorActive = false; 
// Additional variables for new features
int dataFrequency = 1000;        // Default frequency of data transmission in milliseconds
String measurementUnit = "Lux";  // Default measurement unit

const int sensorStatusLED = LED_BUILTIN;  // Additional LED pin for sensor status

bool voltageU=false;
bool delayedMessage=true;

int freq=500;
//float luxValues[4];
//int ldrValues[4];
// Function declarations
void logData(int ldrValues[]);
void HumidityData();
int calculateAverage(int ldrValues[]);
void logLuxData(float luxValues[]);
void defaultMode(int ldrValues[]);
void intensityIndicator(int ldrValues[]);
void Alarm(int ldrValues[]);
void controlDirectionalLEDs(int ldrValues[]);
void turnOffAllLEDs();
void turnOnAllLEDs();

long microsecondsToInches(long microseconds);
long microsecondsToCentimeters(long microseconds);
// LDR readings
int ldrValues[4] = {
  analogRead(North_ldr),
  analogRead(East_ldr),
  analogRead(South_ldr),
  analogRead(West_ldr)
};
float analogToLux(int analogValue);

//float luxValues[4]={
//luxValues[0] = analogToLux(ldrValues[0]),
//luxValues[1] = analogToLux(ldrValues[1]),
//luxValues[2] = analogToLux(ldrValues[2]),
//luxValues[3] = analogToLux(ldrValues[3])
//};


void setup() {
  pinMode(CenterLED, OUTPUT);
  pinMode(NorthWhite, OUTPUT);
  pinMode(EastBlue, OUTPUT);
  pinMode(EastWhite, OUTPUT);
  pinMode(SouthWhite, OUTPUT);
  pinMode(WestBlue, OUTPUT);
  pinMode(WestWhite, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(ledMode, OUTPUT);
  pinMode(sensorStatusLED, OUTPUT);
  digitalWrite(sensorStatusLED, LOW); 

  Serial.begin(115200);
}

void loop() {
  unsigned long currentTime = millis();
  // Button handling with debounce
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH && (currentTime - lastButtonTime >= buttonDebounceDelay)) {
    mode = (mode + 1) % 3; // Cycle through modes
    lastButtonTime = currentTime;
  }
  lastButtonState = buttonState;

  // LDR readings
  int ldrValues[4] = {
    analogRead(North_ldr),
    analogRead(East_ldr),
    analogRead(South_ldr),
    analogRead(West_ldr)
  };

  luxReference = map(analogRead(potPin), 0, 1023, 300, 800);
  avgLux = calculateAverage(ldrValues);

  // Mode-specific tasks
  switch (mode) {
    case 0:
      defaultMode(ldrValues);
      break;
    case 1:
      intensityIndicator(ldrValues);
      break;
    case 2:
      Alarm(ldrValues);
      break;
  }

  // Ultrasonic readings periodically
  if (currentTime - lastUltrasonicTime >= ultrasonicInterval) {
    long duration, inches, cm;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);   

    lastUltrasonicTime = currentTime;
  }

    // Command Handling via Serial
      if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Correct: modifies `input` directly, does not return a value.
        int separatorIndex = input.indexOf(' ');

        if (separatorIndex != -1) {
            String cmd = input.substring(0, separatorIndex); // Extract command
            String param = input.substring(separatorIndex + 1); // Extract parameter

            cmd.trim();  // Trim the command
            param.trim(); // Trim the parameter

            // Process commands
            if (cmd.equalsIgnoreCase("MODE")) {
                int newMode = param.toInt();
                if (newMode >= 0 && newMode <= 2) { // Valid mode range
                    mode = newMode;
                    Serial.print("Mode set to: ");
                    Serial.println(mode);
                } else {
                    Serial.println("Invalid MODE parameter. Use 0, 1, or 2.");
                }

                /////////////////////////////////////////////////////
            } else if (cmd.equalsIgnoreCase("UNIT")) {
                if (param.equalsIgnoreCase("Lux") ) {
                    voltageU=false;
                    measurementUnit = param;
                    Serial.print("Measurement unit set to: ");
                    Serial.println(measurementUnit);
                } else if(param.equalsIgnoreCase("Voltage")){
                    voltageU=true;
                    measurementUnit = param;
                    Serial.print("Measurement unit set to: ");
                    Serial.println(measurementUnit);
                    
                }else{
                    Serial.println("Invalid UNIT parameter. Use 'Lux' or 'Voltage'.");
                }
            } else if (cmd.equalsIgnoreCase("FREQ")) {
                delayedMessage=true;
                int freq = param.toInt();
                if (freq > 0) {
                    dataFrequency = freq;
                    Serial.print("Data frequency set to: ");
                    Serial.print(dataFrequency);
                    Serial.println(" ms");
                    delay(freq);
                    
                } else {
                    delayedMessage=true;
                    Serial.println("Invalid FREQ parameter. Provide a positive number.");
                }
            } else if (cmd.equalsIgnoreCase("SENSOR")) {
                if (param.equalsIgnoreCase("ENABLE")) {
                    sensorActive = true;
                    digitalWrite(sensorStatusLED, HIGH);
                    Serial.println("Sensor output ENABLED.");
                } else if (param.equalsIgnoreCase("DISABLE")) {
                    sensorActive = false;
                    digitalWrite(sensorStatusLED, LOW);
                    Serial.println("Sensor output DISABLED.");
                } else {
                    Serial.println("Invalid SENSOR parameter. Use 'ENABLE' or 'DISABLE'.");
                }
            } else if (cmd.equalsIgnoreCase("THRESHOLD")) {
                int newThreshold = param.toInt();
                if (newThreshold > 0) {
                    THRESHOLD = newThreshold;
                    Serial.print("Threshold set to: ");
                    Serial.println(THRESHOLD);
                } else {
                    Serial.println("Invalid THRESHOLD parameter. Provide a positive number.");
                }
            } else if (cmd.equalsIgnoreCase("LUXREF")) {
                int newLuxRef = param.toInt();
                if (newLuxRef > 0) {
                    luxReference = newLuxRef;
                    Serial.print("Lux reference set to: ");
                    Serial.println(luxReference);
                } else {
                    Serial.println("Invalid LUXREF parameter. Provide a positive number.");
                }
            } else if (cmd.equalsIgnoreCase("BLINK")) {
                int newBlinkInterval = param.toInt();
                if (newBlinkInterval > 0) {
                    blinkInterval = newBlinkInterval;
                    Serial.print("Blink interval set to: ");
                    Serial.print(blinkInterval);
                    Serial.println(" ms");
                } else {
                    Serial.println("Invalid BLINK parameter. Provide a positive number.");
                }
            } else {
                Serial.println("Unknown command.");
            }
        } else {
            Serial.println("Invalid input. Use format: cmd=PARAM.");
        }
    }

    // Regularly send sensor data if enabled
    if (sensorActive && millis() - lastDataSendTime >= dataFrequency) {
        sensorActive=true;
        lastDataSendTime = millis();
    }

  // Check if SENSOR is active
  if (sensorActive) {
    //logLuxData(luxValues);
    logData(ldrValues);
    HumidityData(); // Call humidity function

    Serial.print("Distance: ");
    long duration, inches, cm;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);

    Serial.print(inches);
    Serial.print(" in | ");
    Serial.print(cm);
    Serial.println(" cm");
  }else{
    //logLuxData(luxValues);
    logData(ldrValues);
  }
  // Log lux values periodically
  if (currentTime - lastLogTime >= logInterval) {
    float luxValues[4];
    for (int i = 0; i < 4; i++) {
      luxValues[i] = analogToLux(ldrValues[i]);
    }
    //logLuxData(luxValues);
    lastLogTime = currentTime;
    //logData(ldrValues);
  }
  
}

void defaultMode(int ldrValues[]) {
  // Example of non-blocking LED blinking
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  // Check if all LDR values are similar
  bool allSimilar = true;
  for (int i = 1; i < 4; i++) {
    if (abs(ldrValues[i] - ldrValues[0]) > THRESHOLD) {
      allSimilar = false;
      break;
    }
  }

  if (allSimilar) {
    turnOffAllLEDs();
    static unsigned long lastBlinkCenterTime = millis();
    const unsigned long blinkIntervalCenter = 500; // Blink interval for CenterLED (500 ms)
    unsigned long currentBlinkTime = millis();
    if (currentBlinkTime - lastBlinkCenterTime >= blinkIntervalCenter) {
      digitalWrite(CenterLED, HIGH);
      lastBlinkCenterTime = currentBlinkTime;
    }
  } else {
    controlDirectionalLEDs(ldrValues);
  }
}

void controlDirectionalLEDs(int ldrValues[]) {
  const int ledPins[4][2] = {{NorthWhite, -1}, {EastBlue, EastWhite}, {SouthWhite, -1}, {WestBlue, WestWhite}};
  const int thresholds[4][2] = {{900, -1}, {650, 900}, {900, -1}, {650, 750}};

  int activeLEDs = 0;
  int activeDirections = 0;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      if (ledPins[i][j] != -1 && ldrValues[i] >= thresholds[i][j]) {
        digitalWrite(ledPins[i][j], HIGH);
        activeLEDs++;
        // Count active non-opposite directions
        if (i == 0 || i == 2) { // North or South
          activeDirections |= 1 << i; // Set bit for North or South
        } else { // East or West
          activeDirections |= 1 << (i - 1); // Set bit for East or West
        }
      } else if (ledPins[i][j] != -1) {
        digitalWrite(ledPins[i][j], LOW);
      }
    }
  }

  // Ensure at most two non-opposite LEDs are active
  int oppositeDirections = (activeDirections & 0x3) ^ 0x3; // Mask for opposite directions (North <-> South, East <-> West)
  if (oppositeDirections != 0 && activeLEDs > 2) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 2; j++) {
        if (ledPins[i][j] != -1 && (oppositeDirections & (1 << i)) != 0) {
          digitalWrite(ledPins[i][j], LOW);
          activeLEDs--;
          if (activeLEDs <= 2) break;
        }
      }
      if (activeLEDs <= 2) break;
    }
  }
}


void intensityIndicator(int ldrValues[]) {
  const long blinkInterval = 500; 
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  turnOffAllLEDs();

  const int ledPins[5] = {EastWhite, EastBlue, CenterLED, WestBlue, WestWhite};
  int level = (avgLux > 550 && avgLux <= 650) ? 1 :
              (avgLux > 650 && avgLux <= 750) ? 2 :
              (avgLux > 750 && avgLux <= 850) ? 3 :
              (avgLux > 850 && avgLux <= 950) ? 4 :
              (avgLux > 950) ? 5 : 0;

  for (int i = 0; i < level; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}

void Alarm(int ldrValues[]) {
  //blinkLED(ledMode, 3, 100);
  const long blinkInterval = 250; 
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  if (avgLux > luxReference) {
    Serial.println("ALARM ACTIVATED");
    for (int i = 0; i < 3; i++) {
      turnOnAllLEDs();
      delay(200);
      turnOffAllLEDs();
      delay(200);
    }
  }
}



void logLuxData(float luxValues[]) {
  if(!voltageU){
    Serial.print(" | North: "); Serial.print(luxValues[0], 2);
    Serial.print(" | East: "); Serial.print(luxValues[1], 2);
    Serial.print(" | South: "); Serial.print(luxValues[2], 2);
    Serial.print(" | West: "); Serial.print(luxValues[3], 2);
    Serial.print(" | LUX REF: "); Serial.println(luxReference);
    delay(freq);
  }else{
    float voltage[4];  // Array to store voltage values
    for (int i = 0; i < 4; i++) {
        delay(freq);
        int analogValue = analogRead(i);  // Read the analog value for each LDR
        voltage[i] = map(analogValue, 0, 1023, 0, 5000) / 1000.0; // Convert to voltage (0-5V)
        Serial.print("volts "); Serial.print(i); Serial.print(": "); Serial.print(voltage[i], 3); Serial.println(" V");
        delay(1000);
        
    }///////////////////////////////////////////////////////////////////
  }
}

void turnOffAllLEDs() {
  digitalWrite(CenterLED, LOW);
  digitalWrite(NorthWhite, LOW);
  digitalWrite(EastBlue, LOW);
  digitalWrite(EastWhite, LOW);
  digitalWrite(SouthWhite, LOW);
  digitalWrite(WestBlue, LOW);
  digitalWrite(WestWhite, LOW);
}
void turnOnAllLEDs() {
  digitalWrite(CenterLED, HIGH);
  digitalWrite(NorthWhite, HIGH);
  digitalWrite(EastBlue, HIGH);
  digitalWrite(EastWhite, HIGH);
  digitalWrite(SouthWhite, HIGH);
  digitalWrite(WestBlue, HIGH);
  digitalWrite(WestWhite, HIGH);
}

int calculateAverage(int ldrValues[]) {
  int sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += ldrValues[i];
  }
  return sum / 4;
}

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
   return microseconds / 29 / 2;
}
void logData(int ldrValues[]) {
  float luxValues[4] = {
  analogToLux(ldrValues[0]),
  analogToLux(ldrValues[1]),
  analogToLux(ldrValues[2]),
  analogToLux(ldrValues[3])
  };
  //logLuxData(luxValues);
  Serial.print("MODE: ");
  Serial.print(mode == 0 ? "Default | " : mode == 1 ? "Intensity Indicator | " : "Alarm | ");
  Serial.print("AVG LUX: "); Serial.print(avgLux);
  logLuxData(luxValues);
  delay(freq);

  
  // Ultrasonic and Humidity readings  
}
void HumidityData(){
  int humidityValue = map(analogRead(hSensor), 0, 1023, 100, 0);
  Serial.print("Humidity: ");
  Serial.print(humidityValue);
  Serial.println("%");
  delay(freq);

  if (humidityValue > 3) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
float analogToLux(int analogValue) {
  return (analogValue / 1024.0) * 1000; 
}
