#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;  

const int matrixSizeX = 12;
const int matrixSizeY = 8;
const int paddleSize = 3;

const int debugPin = D13;
// the axis of the joystick is flipped, meaning that the original x-axis is y and the original y-axis is x
// the reason: to align with the way it'll be most comfortable for the player to hold the joystick (wires going upwards, away from body)
const int joystick1XPin = A1;  // Analog pin for X-axis of the joystick #1 (left)
const int joystick1YPin = A0;  // Analog pin for Y-axis of the joystick #1 (left)
const int joystick1SwPin = D12;  // Digital pin for Switch of the joystick #1 (left)
const int joystick2XPin = A3;  // Analog pin for X-axis of the joystick #2 (right)
const int joystick2YPin = A2;  // Analog pin for Y-axis of the joystick #2 (right)
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
int paddle1Direction, paddle2Direction, paddle1DirectionPrev, paddle2DirectionPrev;
int paddleSpeed = 2;
int ballSpeedX = 1, ballSpeedY = 1;
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

struct Point {
  int x;
  int y;
};

Point paddle1[paddleSize];
Point paddle2[paddleSize];
Point ball;

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

  initializeGame();
}

void loop() {
  handleJoystick();
  
  if (isGameOver) {
    resetGrid();
  }
  else {
    movePaddles();

    moveBall();

    // checkCollisions();

    updateMatrix();
  }

  delay(100);
}


void initializeGame() {
  resetGrid();

  Point paddle1Start = {0, 0};
  Point paddle2Start = {11, 0};

  for (int i = 0; i < paddleSize; i++) {
    paddle1[i].x = paddle1Start.x;
    paddle1[i].y = paddle1Start.y + i;
    paddle2[i].x = paddle2Start.x;
    paddle2[i].y = paddle2Start.y + i;
  }

  ball.x = 2;
  ball.y = 3;

  for (int i = 0; i < paddleSize; i++) {
    grid[paddle1[i].y][paddle1[i].x] = 1;
    grid[paddle2[i].y][paddle2[i].x] = 1;
  }

  matrix.renderBitmap(grid, matrixSizeX, matrixSizeY);
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
    else { paddle1Direction = 0, paddle1DirectionStr = "Idle"; }

    if (y2Map <= 512 && y2Map >= joystickDeadzone) { paddle2Direction = 2, paddle2DirectionStr = "Up"; } // Up
    else if (y2Map >= -512 && y2Map <= -joystickDeadzone) { paddle2Direction = 4, paddle2DirectionStr = "Down"; } // Down
    else { paddle2Direction = 0, paddle2DirectionStr = "Idle"; }
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


void movePaddles() {
  // Move Paddle 1
  if (paddle1Direction == 2) {
    movePaddle(paddle1, 1);  // Move up
  } else if (paddle1Direction == 4) {
    movePaddle(paddle1, -1);  // Move down
  } else if (paddle1Direction == 0) {
    movePaddle(paddle1, 0); // Don't move
  }

  // Move Paddle 2
  if (paddle2Direction == 2) {
    movePaddle(paddle2, 1);  // Move up
  } else if (paddle2Direction == 4) {
    movePaddle(paddle2, -1);  // Move down
  } else if (paddle2Direction == 0) {
    movePaddle(paddle2, 0); // Don't move
  }
}


void movePaddle(Point paddle[], int direction) {
  // Move paddle within bounds
  paddle[0].y = constrain(paddle[0].y - direction * paddleSpeed, 0, matrixSizeY - paddleSize);
  for (int i=1; i<paddleSize; i++) {
    paddle[i].y = paddle[i-1].y+1;
  }
}


void moveBall() {
  // Move the ball
  ball.x += ballSpeedX;
  ball.y += ballSpeedY;

  // Check for collisions with the top or bottom walls; in such case, the ball will bounce
  if (ball.y <= 0 || ball.y >= matrixSizeY - 1) {
    ballSpeedY = -ballSpeedY;
  }

  // Check for collisions with paddles; in such case, the ball will bounce
  if (ball.x == 1 && ball.y >= paddle1[0].y && ball.y <= paddle1[0].y + paddleSize) {
    ballSpeedX = -ballSpeedX;
  }
  if (ball.x == matrixSizeX - 2 && ball.y >= paddle2[0].y && ball.y <= paddle2[0].y + paddleSize) {
    ballSpeedX = -ballSpeedX;
  }

  // Check for scoring
  if (ball.x < 0 || ball.x >= matrixSizeX) {
    isGameOver = true;
  }
}


void print(boolean debugState) {
  if (debugState) {
    Serial.println("Joystick 1: (" + String(x1Map) + ", " + String(y1Map) + "), Button: " + String(sw1State));
    Serial.println("Joystick 2: (" + String(x2Map) + ", " + String(y2Map) + "), Button: " + String(sw2State));
    Serial.println();
  }
}


void resetGrid() {
  for (int y = 0; y < matrixSizeY; y++) {
    for (int x = 0; x < matrixSizeX; x++) {
      grid[y][x] = 0;
    }
  }
}


void updateMatrix() {
  resetGrid();

  for (int i = 0; i < paddleSize; i++) {
    grid[paddle1[i].y][paddle1[i].x] = 1;
    grid[paddle2[i].y][paddle2[i].x] = 1;
  }

  grid[ball.y][ball.x] = 1;
  matrix.renderBitmap(grid, matrixSizeY, matrixSizeX);
}