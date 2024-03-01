#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;  

const int matrixSizeX = 12;
const int matrixSizeY = 8;
const int paddleSize = 3;

const int debugPin = D13;
// the axis of the joystick is flipped, meaning that the original x-axis is y and the original y-axis is x
// the reason: to align with the way it'll be most comfortable for the player to hold the joystick (wires going upwards, away from body)
const int joystick1XPin = 1;  // Analog pin for X-axis of the joystick #1 (left)
const int joystick1YPin = 0;  // Analog pin for Y-axis of the joystick #1 (left)
const int joystick1SwPin = D12;  // Digital pin for Switch of the joystick #1 (left)
const int joystick2XPin = 3;  // Analog pin for X-axis of the joystick #2 (right)
const int joystick2YPin = 2;  // Analog pin for Y-axis of the joystick #2 (right)
const int joystick2SwPin = D11;  // Digital pin for Switch of the joystick #2 (right)
const int joystickDeadzone = 30;  // Defines the deadzone area around the joystick's idle position to account for potentiometer inaccuracies.
                                  // Increase this if there are unintentional direction changes in one or two specific directions.
                                  // Decrease this if joystick feels unresponsive or sluggish when changing directions.

bool debugState = false;
bool isGameOver = false;
bool sw1State, sw1Prev;
bool sw2State, sw2Prev;
int x1Value, y1Value, x1Map, y1Map, x1Prev, y1Prev;
int x2Value, y2Value, x2Map, y2Map, x2Prev, y2Prev;
int paddle1Direction, paddle2Direction;
String paddle1DirectionStr, paddle2DirectionStr;


byte grid[matrixSizeY][matrixSizeX] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  Serial.println("🏓🕹️ 2-player pong game in Arduino R4 LED matrix");
  matrix.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(joystick1XPin, INPUT);
  pinMode(joystick1YPin, INPUT);
  pinMode(joystick2XPin, INPUT);
  pinMode(joystick2YPin, INPUT);
  pinMode(joystick1SwPin, INPUT_PULLUP);
  pinMode(joystick2SwPin, INPUT_PULLUP);
  pinMode(debugPin, INPUT_PULLUP);
}

void loop() {
  // Handle joysticks inputs
  handleJoystick();

  // movePaddles();

  // moveBall();

  // checkCollisions();

  // updateMatrix();

  delay(170);
}

void handleJoystick() {
  x1Value = analogRead(joystick1XPin);
  x2Value = analogRead(joystick2XPin);
  y1Value = analogRead(joystick1YPin);
  y2Value = analogRead(joystick2YPin);
  sw1State = not digitalRead(joystick1SwPin);
  sw2State = not digitalRead(joystick2SwPin);
  debugState = not digitalRead(debugPin);

  x1Map = map(x1Value, 0, 1023, 512, -512);
  x2Map = map(x2Value, 0, 1023, 512, -512);
  y1Map = map(y1Value, 0, 1023, 512, -512);
  y2Map = map(y2Value, 0, 1023, 512, -512);

  // if the game is running, only up and down are enabled, else all 4 directions are enabled
  if (!isGameOver) {
    if (y1Map <= 512 && y1Map >= joystickDeadzone) { paddle1Direction = 2, paddle1DirectionStr = "Up"; } // Up
    else if (y1Map >= -512 && y1Map <= -joystickDeadzone) { paddle1Direction = 4, paddle1DirectionStr = "Down"; } // Down

    if (y2Map <= 512 && y2Map >= joystickDeadzone) { paddle2Direction = 2, paddle2DirectionStr = "Up"; } // Up
    else if (y2Map >= -512 && y2Map <= -joystickDeadzone) { paddle2Direction = 4, paddle2DirectionStr = "Down"; } // Down
  } 
  else {
    if (x1Map <= 512 && x1Map >= joystickDeadzone && y1Map <= 511 && y1Map >= -511) { paddle1Direction = 1, paddle1DirectionStr = "Right"; } // Right 
    else if (x1Map >= -512 && x1Map <= -joystickDeadzone && y1Map <= 511 && y1Map >= -511) { paddle1Direction = 3, paddle1DirectionStr = "Left";} // Left
    else if (y1Map <= 512 && y1Map >= joystickDeadzone && x1Map <= 511 && x1Map >= -511) { paddle1Direction = 2, paddle1DirectionStr = "Up"; } // Up
    else if (y1Map >= -512 && y1Map <= -joystickDeadzone && x1Map <= 511 && x1Map >= -511) { paddle1Direction = 4, paddle1DirectionStr = "Down"; } // Down
    
    if (x2Map <= 512 && x2Map >= joystickDeadzone && y2Map <= 511 && y2Map >= -511) { paddle2Direction = 1, paddle2DirectionStr = "Right"; } // Right 
    else if (x2Map >= -512 && x2Map <= -joystickDeadzone && y2Map <= 511 && y2Map >= -511) { paddle2Direction = 3, paddle2DirectionStr = "Left";} // Left
    else if (y2Map <= 512 && y2Map >= joystickDeadzone && x2Map <= 511 && x2Map >= -511) { paddle2Direction = 2, paddle2DirectionStr = "Up"; } // Up
    else if (y2Map >= -512 && y2Map <= -joystickDeadzone && x2Map <= 511 && x2Map >= -511) { paddle2Direction = 4, paddle2DirectionStr = "Down"; } // Down
  }

  Serial.println("Joystick 1: " + paddle1DirectionStr + ", Joystick 2: " + paddle2DirectionStr);
  Serial.println();
}

void print(boolean debugState) {
  if (debugState) {
    Serial.println("Joystick 1: (" + String(x1Map) + ", " + String(y1Map) + "), Button: " + String(sw1State));
    Serial.println("Joystick 2: (" + String(x2Map) + ", " + String(y2Map) + "), Button: " + String(sw2State));
    Serial.println();
  }
}