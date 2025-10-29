#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <ArduinoJson.h>

// ==== Hardware Config ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SD_CS 5

#define BTN_LEFT 25   // snake left / menu up
#define BTN_RIGHT 26  // snake right / menu down
#define BTN_SELECT 27 // select game
#define BTN_BACK 14   // exit game

// ==== Game Menu ====
#define MAX_GAMES 10
String gameList[MAX_GAMES];
int gameCount = 0;
int selected = 0;

// ==== Helper ====
void showMessage(String msg){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(msg);
  display.display();
}

// ==== Setup ====
void setup(){
  Serial.begin(115200);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    while(true) Serial.println("OLED init failed");
  }

  showMessage("Initializing SD...");
  if(!SD.begin(SD_CS)){
    showMessage("SD init failed!");
    while(true);
  }

  loadGames();
  if(gameCount==0){
    showMessage("No games found!");
    while(true);
  }
}

// ==== Main Loop ====
void loop(){
  showMenu();

  if(digitalRead(BTN_SELECT)==LOW){ 
    playGame(gameList[selected]); 
    delay(400);
  }
}

// ==== Load games from SD ====
void loadGames(){
  File dir = SD.open("/games");
  while(true){
    File entry = dir.openNextFile();
    if(!entry) break;
    String name = entry.name();
    if(name.endsWith(".json") && gameCount<MAX_GAMES){
      gameList[gameCount++] = String(name);
    }
    entry.close();
  }
  dir.close();
}

// ==== Show Menu ====
void showMenu(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("== GAME CONSOLE ==");
  for(int i=0;i<gameCount;i++){
    if(i==selected) display.print("> ");
    else display.print("  ");
    display.println(gameList[i]);
  }
  display.display();

  // Navigate menu
  if(digitalRead(BTN_LEFT)==LOW){ 
    selected = (selected-1+gameCount)%gameCount; 
    delay(200);
  }
  if(digitalRead(BTN_RIGHT)==LOW){ 
    selected = (selected+1)%gameCount; 
    delay(200);
  }
}

// ==== Play Game ====
void playGame(String filename){
  showMessage("Loading "+filename);
  File f = SD.open("/games/"+filename);
  if(!f){ showMessage("Failed to open file"); delay(2000); return; }

  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,f)){
    showMessage("JSON parse error");
    f.close();
    delay(2000); return;
  }
  f.close();

  if(filename.indexOf("snake")>=0) runSnake(doc);
  else if(filename.indexOf("rockdodger") >= 0) runRockDodger(doc);
  else(filename.indexOf("pong")>=0); runPong(doc);
}

// ==== Snake Engine (2-button + score + disappearing apples) ====
void runSnake(DynamicJsonDocument &doc){
  int snakeX = doc["snake"]["x"];
  int snakeY = doc["snake"]["y"];
  int snakeLength = doc["snake"]["length"];
  String dir = doc["snake"]["dir"];
  int speed = doc["speed"];

  const int MAX_APPLES = 5; // number of apples on screen
  int applesX[MAX_APPLES], applesY[MAX_APPLES];
  bool appleAlive[MAX_APPLES];

  // Initialize apples at random positions
  for(int i=0;i<MAX_APPLES;i++){
    applesX[i] = random(2, SCREEN_WIDTH-2);
    applesY[i] = random(2, SCREEN_HEIGHT-2);
    appleAlive[i] = true;
  }

  int snakeBodyX[64], snakeBodyY[64];
  for(int i=0;i<snakeLength;i++){
    snakeBodyX[i] = snakeX-i*2;
    snakeBodyY[i] = snakeY;
  }

  unsigned long lastMove = millis();
  int score = 0;

  while(true){
    if(digitalRead(BTN_BACK)==LOW) break;

    // Turn snake
    if(digitalRead(BTN_LEFT)==LOW){
      if(dir=="up") dir="left";
      else if(dir=="left") dir="down";
      else if(dir=="down") dir="right";
      else if(dir=="right") dir="up";
      delay(150);
    }
    if(digitalRead(BTN_RIGHT)==LOW){
      if(dir=="up") dir="right";
      else if(dir=="right") dir="down";
      else if(dir=="down") dir="left";
      else if(dir=="left") dir="up";
      delay(150);
    }

    // Move snake based on speed
    if(millis()-lastMove>speed){
      lastMove = millis();

      // Shift body
      for(int i=snakeLength-1;i>0;i--){
        snakeBodyX[i]=snakeBodyX[i-1];
        snakeBodyY[i]=snakeBodyY[i-1];
      }

      // Update head
      if(dir=="up") snakeY-=2;
      else if(dir=="down") snakeY+=2;
      else if(dir=="left") snakeX-=2;
      else if(dir=="right") snakeX+=2;

      snakeBodyX[0]=snakeX;
      snakeBodyY[0]=snakeY;

      // Boundary check
      if(snakeX<0) snakeX=0;
      if(snakeX>127) snakeX=127;
      if(snakeY<0) snakeY=0;
      if(snakeY>63) snakeY=63;

      // Check apple collisions
      for(int i=0;i<MAX_APPLES;i++){
        if(appleAlive[i] && abs(snakeX-applesX[i])<2 && abs(snakeY-applesY[i])<2){
          snakeLength++;
          if(snakeLength>64) snakeLength=64;
          score++;

          // Respawn apple at new random location
          applesX[i] = random(2, SCREEN_WIDTH-2);
          applesY[i] = random(2, SCREEN_HEIGHT-2);
        }
      }
    }

    // Draw everything
    display.clearDisplay();

    // Draw snake
    for(int i=0;i<snakeLength;i++){
      display.fillRect(snakeBodyX[i],snakeBodyY[i],2,2,SSD1306_WHITE);
    }

    // Draw apples
    for(int i=0;i<MAX_APPLES;i++){
      display.fillRect(applesX[i],applesY[i],2,2,SSD1306_WHITE);
    }

    // Draw score
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print("Score: "); display.print(score);

    display.display();
  }
}


// ==== Pong Engine (score + ball reset) ====
void runPong(DynamicJsonDocument &doc){
  int ballX=doc["ball"]["x"];
  int ballY=doc["ball"]["y"];
  int vx=doc["ball"]["vx"];
  int vy=doc["ball"]["vy"];

  int paddle1X=doc["paddle1"]["x"];
  int paddle1Y=doc["paddle1"]["y"];
  int paddle2X=doc["paddle2"]["x"];
  int paddle2Y=doc["paddle2"]["y"];

  int score1=0, score2=0;
  unsigned long lastMove=millis();

  while(true){
    if(digitalRead(BTN_BACK)==LOW) break;

    // Move paddle1 with left/right buttons
    if(digitalRead(BTN_LEFT)==LOW) paddle1Y-=2;
    if(digitalRead(BTN_RIGHT)==LOW) paddle1Y+=2;
    if(paddle1Y<0)paddle1Y=0;
    if(paddle1Y>44)paddle1Y=44;

    // Paddle2 AI (simple follow)
    if(ballY>paddle2Y+10) paddle2Y+=1;
    if(ballY<paddle2Y+10) paddle2Y-=1;
    if(paddle2Y<0)paddle2Y=0;
    if(paddle2Y>44)paddle2Y=44;

    // Move ball
    if(millis()-lastMove>50){
      lastMove=millis();
      ballX+=vx; ballY+=vy;

      // Bounce top/bottom
      if(ballY<=0 || ballY>=63) vy=-vy;

      // Paddle collisions
      if(ballX<=paddle1X+5 && ballY>=paddle1Y && ballY<=paddle1Y+20) vx=-vx;
      if(ballX>=paddle2X-5 && ballY>=paddle2Y && ballY<=paddle2Y+20) vx=-vx;

      // Score
      if(ballX<0){ score2++; ballX=64; ballY=32; vx=1; vy=1; }
      if(ballX>127){ score1++; ballX=64; ballY=32; vx=-1; vy=1; }
    }

    // Draw
    display.clearDisplay();
    display.fillRect(paddle1X,paddle1Y,5,20,SSD1306_WHITE);
    display.fillRect(paddle2X,paddle2Y,5,20,SSD1306_WHITE);
    display.fillCircle(ballX,ballY,3,SSD1306_WHITE);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print(score1);
    display.setCursor(120,0);
    display.print(score2);

    display.display();
  }
}
void runRockDodger(DynamicJsonDocument &doc){
  int playerX = doc["player"]["x"];
  int playerY = doc["player"]["y"];
  int playerW = doc["player"]["width"];
  int playerH = doc["player"]["height"];

  const int ROCKS = doc["rocks"]["count"];
  int rockW = doc["rocks"]["width"];
  int rockH = doc["rocks"]["height"];
  int rockX[ROCKS], rockY[ROCKS];

  randomSeed(analogRead(0)); // ESP32 random seed

  // Initialize rocks above the screen
  for(int i=0;i<ROCKS;i++){
    rockX[i] = random(0, SCREEN_WIDTH-rockW);
    rockY[i] = random(-SCREEN_HEIGHT,0);
  }

  int speed = doc["speed"];
  int lives = doc["lives"];
  int score = 0;
  unsigned long lastMove = millis();

  while(true){
    // Exit game
    if(digitalRead(BTN_BACK)==LOW) break;

    // Player movement (two buttons)
    if(digitalRead(BTN_LEFT)==LOW) playerX -= 2;
    if(digitalRead(BTN_RIGHT)==LOW) playerX += 2;
    if(playerX<0) playerX=0;
    if(playerX>SCREEN_WIDTH-playerW) playerX=SCREEN_WIDTH-playerW;

    // Update rocks
    if(millis()-lastMove > speed){
      lastMove = millis();

      for(int i=0;i<ROCKS;i++){
        rockY[i] += 2;

        // Collision check
        if(playerX < rockX[i]+rockW && playerX+playerW > rockX[i] &&
           playerY < rockY[i]+rockH && playerY+playerH > rockY[i]){
          lives--;
          rockY[i] = random(-SCREEN_HEIGHT,0);
          rockX[i] = random(0, SCREEN_WIDTH-rockW);
          if(lives <= 0) return; // back to menu
        }

        // Rock passed bottom
        if(rockY[i] > SCREEN_HEIGHT){
          rockY[i] = random(-SCREEN_HEIGHT,0);
          rockX[i] = random(0, SCREEN_WIDTH-rockW);
          score++;
        }
      }
    }

    // Draw
    display.clearDisplay();
    // Player
    display.fillRect(playerX,playerY,playerW,playerH,SSD1306_WHITE);
    // Rocks
    for(int i=0;i<ROCKS;i++){
      display.fillRect(rockX[i],rockY[i],rockW,rockH,SSD1306_WHITE);
    }
    // Score & lives
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print("Score: "); display.print(score);
    display.setCursor(90,0);
    display.print("Lives: "); display.print(lives);

    display.display();
  }
}
