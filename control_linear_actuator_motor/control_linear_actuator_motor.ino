#include <Wire.h>
#include <VL53L0X.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 50
#define NUM_SAMPLES 10

char buffer[BUFFER_SIZE + 1];  // Buffer to store serial commands (+1 for null terminator)
VL53L0X sensor;
int out11 = 5;
int out12 = 6;
int t_pos = 100;  // Target position
int moveMode = 0; // 0=don't move, 1=move, 2=stop if go to target, 

// Array for moving average of sensor readings
int val_list[NUM_SAMPLES] = { 0 };

// Function prototypes
void addToBuffer(const char* input);
char* extractCommand(const char* input);
void splitCommand(const char* command, char** result, int* resultSize);
void moveTo(int vel);

void setup() {
  Serial.begin(9600);

  // Initialize buffer with dashes
  memset(buffer, '-', BUFFER_SIZE);
  buffer[BUFFER_SIZE] = '\0';

  // Set PWM pins as outputs
  pinMode(out11, OUTPUT);
  pinMode(out12, OUTPUT);

  Wire.begin();
  sensor.init();
  sensor.setTimeout(500);
  sensor.startContinuous();
}

void loop() {
  // Process sensor readings until a command is received via Serial
  while (true) {
    int v = sensor.readRangeContinuousMillimeters();
    if (sensor.timeoutOccurred()) {
      Serial.println("TIMEOUT");
    } else {
      // Shift values for a moving window average
      for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        val_list[i] = val_list[i + 1];
      }
      val_list[NUM_SAMPLES - 1] = v;
    }

    // Calculate the average sensor value
    long sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      sum += val_list[i];
    }
    int avg = sum / NUM_SAMPLES;

    // Calculate the squared difference scaled by 0.5 (cast to int) with a maximum of 255
    // f\left(x\right)\ =\ 255\cdot\frac{\frac{1}{1+e^{-k\left(ax-0.5\right)}}-\frac{1}{1+e^{0.5k}}}{\frac{1}{1+e^{-0.5k}}-\frac{1}{1+e^{0.5k}}}
    double diff = t_pos - avg;
    double e = 2.7182818;
    double k = 23;
    double a = 0.1667;
    double f1 = 1 / (1 + pow(e, -k * (a * abs(diff) - 0.5)));
    double f2 = 1 / (1 + pow(e, 0.5 * k));
    double f3 = 1 / (1 + pow(e, -0.5 * k));
    int val_write = 255 * ((f1 - f2) / (f3 - f2));

    // Write the PWM value to one of the outputs based on the sign of diff
    if (abs(diff) < 2) {
      analogWrite(out11, 0);
      analogWrite(out12, 0);
      Serial.print("             ");
    } else if (diff < 0) {
      analogWrite(out11, val_write);
      analogWrite(out12, 0);
      Serial.print("   Shrink หด");
    } else if (diff > 0) {
      analogWrite(out11, 0);
      analogWrite(out12, val_write);
      Serial.print("  Stretch ยืด");
    }

    Serial.print("   avg = ");
    Serial.print(avg);
    Serial.print("   t_pos = ");
    Serial.print(t_pos);
    Serial.print("   diff = ");
    Serial.print(diff);
    Serial.print("   val_write = ");
    Serial.println(val_write);
    delay(10);

    // Check for incoming Serial data
    while (Serial.available()) {
      char ch = Serial.read();
      char str[2] = { ch, '\0' };
      addToBuffer(str);
      if (ch == '>') {  // End of command marker
        goto processCommand;
      }
    }
  }

processCommand:
  {
    char* command = extractCommand(buffer);
    if (command != NULL) {
      char* splitResult[10] = { NULL };
      int splitSize = 0;
      splitCommand(command, splitResult, &splitSize);

      if (splitSize > 0) {
        if (strcmp(splitResult[0], "moveTo") == 0 && splitSize >= 2) {
          int val = atoi(splitResult[1]);
          moveTo(val);
        } else if (strcmp(splitResult[0], "up") == 0) {
          analogWrite(out11, 0);
          analogWrite(out12, 255);
          delay(1000);
        } else if (strcmp(splitResult[0], "down") == 0) {
          analogWrite(out11, 255);
          analogWrite(out12, 0);
          delay(1000);
        }
      }

      // Free allocated memory for split tokens and the command
      for (int i = 0; i < splitSize; i++) {
        free(splitResult[i]);
      }
      free(command);
    }
    // Reset the buffer after processing the command
    memset(buffer, '-', BUFFER_SIZE);
    buffer[BUFFER_SIZE] = '\0';
  }
}

void addToBuffer(const char* input) {
  // Remove newline and carriage return characters from input
  char cleanedInput[strlen(input) + 1];
  int j = 0;
  for (int i = 0; input[i] != '\0'; i++) {
    if (input[i] != '\n' && input[i] != '\r') {
      cleanedInput[j++] = input[i];
    }
  }
  cleanedInput[j] = '\0';

  int cleanedLength = strlen(cleanedInput);
  int bufferLength = strlen(buffer);

  if (cleanedLength >= BUFFER_SIZE) {
    // If input is longer than buffer, keep only the last BUFFER_SIZE characters
    strncpy(buffer, cleanedInput + (cleanedLength - BUFFER_SIZE), BUFFER_SIZE);
  } else if (bufferLength + cleanedLength > BUFFER_SIZE) {
    int shift = bufferLength + cleanedLength - BUFFER_SIZE;
    memmove(buffer, buffer + shift, bufferLength - shift);
    strncpy(buffer + (BUFFER_SIZE - cleanedLength), cleanedInput, cleanedLength);
  } else {
    strcat(buffer, cleanedInput);
  }
  buffer[BUFFER_SIZE] = '\0';
}

char* extractCommand(const char* input) {
  int length = strlen(input);
  // Ensure the command ends with '>'
  if (length == 0 || input[length - 1] != '>') {
    return NULL;
  }
  // Find the last '<' marker
  const char* start = strrchr(input, '<');
  if (start != NULL && start < input + length - 1) {
    int commandLength = (input + length - 1) - (start + 1);
    char* command = (char*)malloc(commandLength + 1);
    if (command != NULL) {
      strncpy(command, start + 1, commandLength);
      command[commandLength] = '\0';
    }
    return command;
  }
  return NULL;
}

void splitCommand(const char* command, char** result, int* resultSize) {
  // Make a temporary copy of the command string for tokenization
  char temp[strlen(command) + 1];
  strcpy(temp, command);

  *resultSize = 0;
  // Tokenize based on delimiters: '(' , ')' and ','
  char* token = strtok(temp, "(,)");
  while (token != NULL && *resultSize < 10) {
    result[*resultSize] = (char*)malloc(strlen(token) + 1);
    if (result[*resultSize] != NULL) {
      strcpy(result[*resultSize], token);
    }
    (*resultSize)++;
    token = strtok(NULL, "(,)");
  }
}

void moveTo(int vel) {
  // Update the target position
  t_pos = vel;
}
