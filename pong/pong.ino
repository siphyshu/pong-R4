#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

#include "intro.h"
#include "continue_playing.h"
#include "yes_option.h"
#include "no_option.h"

ArduinoLEDMatrix matrix;  

const int matrixSizeX = 12;
const int matrixSizeY = 8;
const int paddleSize = 3;

const int debugPin = D13; // Connect D13 with GND to enable debugging
// the axis of the joystick is flipped, meaning that the original x-axis is y and the original y-axis is x
// the reason: to align with the way it'll be most comfortable for the player to hold the joystick (wires going upwards, away from body)
const int joystick1XPin = A1;  // Analog pin for X-axis of the joystick #1 (left)
const int joystick1YPin = A0;  // Analog pin for Y-axis of the joystick #1 (left)
const int joystick1SwPin = D12;  // Digital pin for Switch of the joystick #1 (left)
const int joystick2XPin = A3;  // Analog pin for X-axis of the joystick #2 (right)
const int joystick2YPin = A2;  // Analog pin for Y-axis of the joystick #2 (right)
const int joystick2SwPin = D11;  // Digital pin for Switch of the joystick #2 (right)
const int joystickDeadzone = 200;  // Defines the deadzone area around the joystick's idle position to account for potentiometer inaccuracies.
                                  // Increase this if there are unintentional direction changes in one or two specific directions.
                                  // Decrease this if joystick feels unresponsive or sluggish when changing directions.

bool debugState = false;
bool isGameOver = false;
bool sw1State, sw1Prev;
bool sw2State, sw2Prev;
int x1Value, y1Value, x1Map, y1Map, x1Prev, y1Prev;
int x2Value, y2Value, x2Map, y2Map, x2Prev, y2Prev;
String paddle1DirectionStr, paddle2DirectionStr;
int paddle1Direction, paddle2Direction, paddle1DirectionPrev, paddle2DirectionPrev;
int ballSpeed = 1;
int paddleSpeed = 2;
int gameTickSpeed = 100;
int ballSpeedX = ballSpeed, ballSpeedY = ballSpeed;
int p1Points = 0, p2Points = 0; 
int pointsNeededToWin = 5;
int randomVariable = 0; // just used to assign it to random() choices
int winner;


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

// forward declarations
void printText(String displayText, int scrollDirection, int speed);


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

  // We are basically checking if the joystick is moved during the intro sequence to skip it
  
  // Why not just check if the joystick is moved at all?
  // Because the joystick might not be stable at the beginning of the sequence, so we need to wait for it to stabilize
  // Or, if the joystick is not connected to power, it will read some random values ~500, so we can't use that
  // So, we are checking if the joystick is moved significantly from its initial reading

  // Get initial joystick readings as reference points
  delay(100); // Give joysticks time to stabilize
  int x1Init = analogRead(joystick1XPin);
  int y1Init = analogRead(joystick1YPin);
  int x2Init = analogRead(joystick2XPin);
  int y2Init = analogRead(joystick2YPin);

  matrix.loadSequence(intro);
  matrix.play();
  
  // Skip animation if joystick is moved
  bool skipIntro = false;
  int consecutiveMovements = 0;  // Track consecutive detected movements
  while (!matrix.sequenceDone() && !skipIntro) {
    // Check if any joystick is moved significantly
    int x1 = analogRead(joystick1XPin);
    int y1 = analogRead(joystick1YPin);
    int x2 = analogRead(joystick2XPin);
    int y2 = analogRead(joystick2YPin);
    
    // Check if there's significant movement compared to initial values
    if (abs(x1 - x1Init) > 200 || abs(y1 - y1Init) > 200 ||
        abs(x2 - x2Init) > 200 || abs(y2 - y2Init) > 200) {
      consecutiveMovements++;
      if (consecutiveMovements >= 3) {  // Require 3 consecutive readings to confirm movement
        skipIntro = true;
      }
    } else {
      consecutiveMovements = 0;  // Reset counter if no movement detected
    }
    
    delay(10);
  }
  delay(200);

  // const String pongASCII = "    PONG-R4    ";
  // printText(pongASCII, 2, 35);

  initializeGame();
}


void loop() {
  debugState = not digitalRead(debugPin);
  if (debugState) {
    gameTickSpeed = 250;
    printDebugInfo();
  } else {
    gameTickSpeed = 100;
  }

  handleJoystick();

  checkIfGameOver();

  movePaddles();

  moveBall(); 
  
  updateMatrix();

  delay(gameTickSpeed);
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

  bool collidedWithTopOrBottomWall = ball.y <= 0 || ball.y >= matrixSizeY - 1;
  bool collidedWithLeftWall = ball.x < 0;
  bool collidedWithRightWall = ball.x >= matrixSizeX;

  // This monstrous boolean statement sums up to the following english sentence:
  // Is the ball is at the edge of the screen AND is it either touching the paddle OR about to touch the edge of the paddle in the next tick?
  
  // It checks if the ball about to touch the lower or upple edge of the paddle by seeing the direction of the ball
  // Upper edge: It will touch the upper edge of the paddle in the next tick if the ball is moving downwards
  // Lower edge: It will touch the lower edge of the paddle in the next tick if the ball is moving upwards
  bool collidedWithLeftPaddle = (ball.x == 1) && ((ball.y >= paddle1[0].y) || (ball.y == paddle1[0].y-1 && ballSpeedY == ballSpeed)) && ((ball.y < (paddle1[0].y + paddleSize)) || (ball.y == (paddle1[0].y + paddleSize) && ballSpeedY == -ballSpeed));
  bool collidedWithRightPaddle = (ball.x == (matrixSizeX - 2)) && ((ball.y >= paddle2[0].y) || (ball.y == paddle2[0].y-1 && ballSpeedY == ballSpeed)) && ((ball.y < (paddle2[0].y + paddleSize)) || (ball.y == (paddle2[0].y + paddleSize) && ballSpeedY == -ballSpeed));

  // NOTE: here the corner does not refer to the absolute corner of the screen, it is with taking the paddle into account
  bool isCorner = (ball.x == 1 && ball.y == 0) || (ball.x == 1 && ball.y == 7) || (ball.x == 10 && ball.y == 0) || (ball.x == 10 && ball.y == 7);

  if (isCorner) {
    // check if the paddle is there
    // if it's there then simply bounce the ball
    // if it's not there, then do nothing
    if (collidedWithTopOrBottomWall) {
      ballSpeedY = -ballSpeedY;
    }
    if (collidedWithLeftPaddle || collidedWithRightPaddle) {
      ballSpeedX = -ballSpeedX;
    }
  }
  else {
    if (collidedWithTopOrBottomWall) {
      ballSpeedY = -ballSpeedY;
    }
    else if (collidedWithLeftPaddle || collidedWithRightPaddle) {
      ballSpeedX = -ballSpeedX;
      randomVariable = random(1,101);

      if (randomVariable >= 1 && randomVariable <= 40) {
        ballSpeedY = -ballSpeed;
      } else if (randomVariable >= 41 && randomVariable <= 80) {
        ballSpeedY = ballSpeed;
      } else if (randomVariable >= 81 && randomVariable <= 100) {
        ballSpeedY = 0;
      }
    }
    else if (collidedWithLeftWall) {
      // Player 2 will score here
      p2Points += 1;
      delay(75);
      resetBall(2);
    } 
    else if (collidedWithRightWall) {
      // Player 1 will score here
      p1Points += 1;
      delay(75);
      resetBall(1);
    }
  }

}


void resetBall(int playerTurn) {
  // playerTurn: the ball will move towards this player 
  
  // Set the ball position based on the playerTurn
  if (playerTurn == 1) { 
    ball.y = random(7);
    ball.x = 7;
    ballSpeedX = -ballSpeed;
  } else if (playerTurn == 2) {
    ball.y = random(7);
    ball.x = 4;
    ballSpeedX = ballSpeed;
  }

  // Randomly decide the ball speed direction
  if (random(2) == 0) {
    ballSpeedY = -ballSpeed;
  } else {
    ballSpeedY = ballSpeed;
  }
}


void printDebugInfo() {
  // Connect D13 with GND to enable debugging
  Serial.println("Joystick 1: (" + String(x1Map) + ", " + String(y1Map) + "), Button: " + String(sw1State));
  Serial.println("Joystick 2: (" + String(x2Map) + ", " + String(y2Map) + "), Button: " + String(sw2State));
  // Serial.println("WinnerStr = " + winnerString);
  Serial.println();
}


void printText(String displayText, int scrollDirection = 0, int speed = 35) {
  // speed: 30-50 is a readable speed

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(speed);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(displayText);


  // scrollDirection: 0 = NO SCROLL, 1 = RIGHT, 2 = LEFT
  if (scrollDirection == 0) { 
    matrix.endText();
  } else if (scrollDirection == 1) {
    matrix.endText(SCROLL_RIGHT);
  } else if (scrollDirection == 2) {
    matrix.endText(SCROLL_LEFT);
  } else {
    matrix.endText(); // fallback in case of error
  }

  matrix.endDraw();
}


void updateMatrix() {
  resetGrid();

  // drawing the paddles
  for (int i = 0; i < paddleSize; i++) {
    grid[paddle1[i].y][paddle1[i].x] = 1;
    grid[paddle2[i].y][paddle2[i].x] = 1;
  }

  // drawing the balls
  grid[ball.y][ball.x] = 1;

  // drawing the score indicator
  for (int i = 0; i < p1Points; i++) {
    grid[0][1+i] = 1;
  }

  for (int i = 0; i < p2Points; i++) {
    grid[0][10-i] = 1;
  }
  matrix.renderBitmap(grid, matrixSizeY, matrixSizeX);
}


void resetGrid() {
  // this is sort of a refresh for the screen
  for (int y = 0; y < matrixSizeY; y++) {
    for (int x = 0; x < matrixSizeX; x++) {
      grid[y][x] = 0;
    }
  }

  matrix.renderBitmap(grid, matrixSizeY, matrixSizeX);
}


void checkIfGameOver() {
  if (p1Points >= pointsNeededToWin) {
    winner = 1;
    gameOver();
  } else if (p2Points >= pointsNeededToWin) {
    winner = 2;
    gameOver();
  }
}


void gameOver() {
  isGameOver = true;
  resetGrid();

  // Display the winner screen and blink it 4 times
  for (int i=0; i<4; i++) {
    displayWinner(true, winner);
  }
  
  // Show the continue playing screen
  continuePlaying();
}


void continuePlaying() {
  matrix.loadSequence(continue_playing);
  matrix.renderFrame(0); // Default to "Yes" option
  String selectedOption = "yes";
  
  // Reset button states for clean detection
  sw1State = false;
  sw2State = false;
  
  // Variables to track hold duration
  unsigned long holdStartTime1 = 0;
  unsigned long holdStartTime2 = 0;
  bool isHolding1 = false;
  bool isHolding2 = false;
  const unsigned long HOLD_DURATION = 2000; // 2 seconds to confirm
  
  while (true) {
    handleJoystick();
    unsigned long currentTime = millis();
    
    // Use left/right to navigate options
    if (paddle1Direction == 3 || paddle2Direction == 3) {
      matrix.renderFrame(0);
      selectedOption = "yes";
      // Reset holding when changing options
      isHolding1 = false;
      isHolding2 = false;
    } else if (paddle1Direction == 1 || paddle2Direction == 1) {
      matrix.renderFrame(1);
      selectedOption = "no";
      // Reset holding when changing options
      isHolding1 = false;
      isHolding2 = false;
    }
    
    // Detect and track downward hold for player 1
    if (paddle1Direction == 4) {
      if (!isHolding1) {
        isHolding1 = true;
        holdStartTime1 = currentTime;
      } else {
        // Calculate hold duration
        unsigned long holdDuration = currentTime - holdStartTime1;
        
        // If held long enough, confirm selection
        if (holdDuration >= HOLD_DURATION) {
          break; // Selection confirmed
        }
      }
    } else {
      isHolding1 = false;
    }
    
    // Detect and track downward hold for player 2
    if (paddle2Direction == 4) {
      if (!isHolding2) {
        isHolding2 = true;
        holdStartTime2 = currentTime;
      } else {
        // Calculate hold duration
        unsigned long holdDuration = currentTime - holdStartTime2;
        
        // If held long enough, confirm selection
        if (holdDuration >= HOLD_DURATION) {
          break; // Selection confirmed
        }
      }
    } else {
      isHolding2 = false;
    }
    
    // Press any button to select (keep this for compatibility)
    if (sw1State || sw2State) {
      break;
    }
    
    delay(50); // Faster refresh for more responsive feedback
  }

  // Show brief confirmation
  resetGrid();
  delay(300);
  
  // Process selected option
  if (selectedOption == "no") {
    matrix.loadSequence(no_option);
    matrix.play();
    delay(1500);
    printText("    thx for playing!        made by siphyshu <3    ", 2, 35);
    
    // Show final score and winner indefinitely
    while (true) {
      displayWinner(false, winner);
      delay(1000);
    }
  } else if (selectedOption == "yes") {
    matrix.loadSequence(yes_option);
    matrix.play();
    delay(1500);
    
    // Reset game state
    isGameOver = false;
    p1Points = 0;
    p2Points = 0;
    
    // Start a new game
    initializeGame();
  }
}


void displayWinner(bool toBlink, int winner) {
  uint8_t scoreboard[8][12] = {
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

  if (winner == 1) {
    scoreboard[1][1] = 1; scoreboard[1][2] = 1; scoreboard[1][3] = 1; scoreboard[1][9] = 1;
    scoreboard[2][1] = 1; scoreboard[2][4] = 1; scoreboard[2][8] = 1; scoreboard[2][9] = 1;
    scoreboard[3][1] = 1; scoreboard[3][4] = 1; scoreboard[3][7] = 1; scoreboard[3][9] = 1;
    scoreboard[4][1] = 1; scoreboard[4][2] = 1; scoreboard[4][3] = 1; scoreboard[4][9] = 1;
    scoreboard[5][1] = 1; scoreboard[5][9] = 1;  
    scoreboard[6][1] = 1; scoreboard[6][7] = 1; scoreboard[6][8] = 1; scoreboard[6][9] = 1; scoreboard[6][10] = 1;

  } else if (winner == 2) {
    scoreboard[1][1] = 1; scoreboard[1][2] = 1; scoreboard[1][3] = 1; scoreboard[1][8] = 1; scoreboard[1][9] = 1;
    scoreboard[2][1] = 1; scoreboard[2][4] = 1; scoreboard[2][7] = 1; scoreboard[2][10] = 1;
    scoreboard[3][1] = 1; scoreboard[3][4] = 1; scoreboard[3][10] = 1;
    scoreboard[4][1] = 1; scoreboard[4][2] = 1; scoreboard[4][3] = 1; scoreboard[4][9] = 1;
    scoreboard[5][1] = 1; scoreboard[5][8] = 1;  
    scoreboard[6][1] = 1; scoreboard[6][7] = 1; scoreboard[6][8] = 1; scoreboard[6][9] = 1; scoreboard[6][10] = 1;
  }

  resetGrid();
  matrix.renderBitmap(scoreboard, 8, 12);
  delay(500);
  if (toBlink) {
    matrix.renderBitmap(grid, 8, 12);
    delay(500);
  };
} 


void handleJoystick() {
  x1Value = analogRead(joystick1XPin);
  x2Value = analogRead(joystick2XPin);
  y1Value = analogRead(joystick1YPin);  
  y2Value = analogRead(joystick2YPin);
  sw1State = not digitalRead(joystick1SwPin);
  sw2State = not digitalRead(joystick2SwPin);

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
}