#include <M5Cardputer.h>
#include <M5Unified.h>
#include <M5GFX.h>

enum Screen { MENU, TETRIS, SNAKE };
Screen currentScreen = MENU;

int menuIndex = 0;
const int MENU_COUNT = 2;
const char* menuNames[MENU_COUNT] = {"tetris", "snake"};

#define COL_BG      0x8410
#define COL_BTN     0x4208
#define COL_BTN_SEL 0x6B4D
#define COL_TEXT    0xFFFF
#define COL_FOOD    0xF800

int cachedBat = -1;
unsigned long lastBatRead = 0;

void drawBattery() {
  if (cachedBat < 0 || millis() - lastBatRead > 5000) {
    cachedBat = M5.Power.getBatteryLevel();
    lastBatRead = millis();
  }
  int bat = cachedBat;
  int x = 196, y = 4;
  M5Cardputer.Display.drawRect(x, y, 26, 12, COL_TEXT);
  M5Cardputer.Display.fillRect(x + 26, y + 4, 3, 4, COL_TEXT);
  if (bat >= 0 && bat <= 100) {
    int w = map(bat, 0, 100, 0, 24);
    if (w > 0) M5Cardputer.Display.fillRect(x + 1, y + 1, w, 10, COL_TEXT);
  }
  M5Cardputer.Display.setTextColor(COL_TEXT, COL_BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(x - 28, y + 2);
  if (bat >= 0 && bat <= 100) M5Cardputer.Display.printf("%d%%", bat);
  else                        M5Cardputer.Display.print("--%");
}

void drawMenuButton(int i, bool selected) {
  int y = 35 + i * 28;
  uint16_t color = selected ? COL_BTN_SEL : COL_BTN;
  M5Cardputer.Display.fillRect(10, y, 220, 22, color);
  M5Cardputer.Display.setTextColor(COL_TEXT, color);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(20, y + 2);
  M5Cardputer.Display.print(menuNames[i]);
}

void drawMenu() {
  M5Cardputer.Display.startWrite();
  M5Cardputer.Display.fillScreen(COL_BG);
  for (int i = 0; i < MENU_COUNT; i++) drawMenuButton(i, i == menuIndex);
  drawBattery();
  M5Cardputer.Display.endWrite();
}

void updateMenuSelection(int oldIdx, int newIdx) {
  M5Cardputer.Display.startWrite();
  drawMenuButton(oldIdx, false);
  drawMenuButton(newIdx, true);
  M5Cardputer.Display.endWrite();
}

bool isUp() {
  return M5Cardputer.Keyboard.isKeyPressed(KEY_FN) &&
         M5Cardputer.Keyboard.isKeyPressed(';');
}
bool isDown() {
  return M5Cardputer.Keyboard.isKeyPressed(KEY_FN) &&
         M5Cardputer.Keyboard.isKeyPressed('.');
}
bool isLeft() {
  return M5Cardputer.Keyboard.isKeyPressed(KEY_FN) &&
         M5Cardputer.Keyboard.isKeyPressed(',');
}
bool isRight() {
  return M5Cardputer.Keyboard.isKeyPressed(KEY_FN) &&
         M5Cardputer.Keyboard.isKeyPressed('/');
}
bool isEnter() {
  auto k = M5Cardputer.Keyboard.keysState();
  if (k.enter) return true;
  for (auto c : k.word) if (c == '\n' || c == '\r') return true;
  return false;
}
bool isBack() {
  auto k = M5Cardputer.Keyboard.keysState();
  if (k.del) return true;
  for (auto c : k.word) if (c == '\b') return true;
  return false;
}

#define SNAKE_COLS 20
#define SNAKE_ROWS 12
#define SNAKE_CELL 9
#define SNAKE_X0   30
#define SNAKE_Y0   15

int snakeX[SNAKE_COLS * SNAKE_ROWS];
int snakeY[SNAKE_COLS * SNAKE_ROWS];
int snakeLen = 3;
int snakeDir = 0;
int snakeFoodX = 5, snakeFoodY = 5;
unsigned long snakeLastMove = 0;
bool snakeGameOver = false;

void spawnFood() {
  if (snakeLen >= SNAKE_COLS * SNAKE_ROWS) return;
  bool ok;
  do {
    ok = true;
    snakeFoodX = random(0, SNAKE_COLS);
    snakeFoodY = random(0, SNAKE_ROWS);
    for (int i = 0; i < snakeLen; i++)
      if (snakeX[i] == snakeFoodX && snakeY[i] == snakeFoodY) { ok = false; break; }
  } while (!ok);
}

void snakeReset() {
  snakeLen = 3;
  snakeDir = 0;
  for (int i = 0; i < snakeLen; i++) {
    snakeX[i] = 3 - i;
    snakeY[i] = 5;
  }
  spawnFood();
  snakeGameOver = false;
  snakeLastMove = millis();
}

void snakeDraw() {
  M5Cardputer.Display.startWrite();
  M5Cardputer.Display.fillScreen(COL_BG);
  M5Cardputer.Display.drawRect(SNAKE_X0 - 1, SNAKE_Y0 - 1, SNAKE_COLS * SNAKE_CELL + 2, SNAKE_ROWS * SNAKE_CELL + 2, COL_TEXT);
  for (int i = 0; i < snakeLen; i++) {
    M5Cardputer.Display.fillRect(SNAKE_X0 + snakeX[i] * SNAKE_CELL, SNAKE_Y0 + snakeY[i] * SNAKE_CELL, SNAKE_CELL - 1, SNAKE_CELL - 1, COL_TEXT);
  }
  M5Cardputer.Display.fillRect(SNAKE_X0 + snakeFoodX * SNAKE_CELL, SNAKE_Y0 + snakeFoodY * SNAKE_CELL, SNAKE_CELL - 1, SNAKE_CELL - 1, COL_FOOD);
  if (snakeGameOver) {
    M5Cardputer.Display.setTextColor(COL_TEXT, COL_BG);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(60, 50);
    M5Cardputer.Display.print("GAME OVER");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(60, 75);
    M5Cardputer.Display.print("Enter - restart");
  }
  drawBattery();
  M5Cardputer.Display.endWrite();
}

void snakeStep() {
  if (snakeGameOver) return;
  int nx = snakeX[0];
  int ny = snakeY[0];
  if (snakeDir == 0) nx++;
  else if (snakeDir == 1) ny++;
  else if (snakeDir == 2) nx--;
  else if (snakeDir == 3) ny--;

  if (nx < 0 || nx >= SNAKE_COLS || ny < 0 || ny >= SNAKE_ROWS) { snakeGameOver = true; return; }
  for (int i = 0; i < snakeLen; i++) {
    if (snakeX[i] == nx && snakeY[i] == ny) { snakeGameOver = true; return; }
  }
  for (int i = snakeLen - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  snakeX[0] = nx;
  snakeY[0] = ny;

  if (nx == snakeFoodX && ny == snakeFoodY) {
    if (snakeLen < SNAKE_COLS * SNAKE_ROWS) snakeLen++;
    spawnFood();
  }
}

void snake() {
  snakeReset();
  snakeDraw();
}

#define TET_COLS 10
#define TET_ROWS 16
#define TET_CELL 6
#define TET_X0   40
#define TET_Y0   15

uint8_t tetrisField[TET_ROWS][TET_COLS];

const uint8_t tetrominoes[7][4][4][4] = {
  {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
   {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
   {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
  {{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}},
  {{{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{0,1,0,0},{1,1,0,0},{0,1,0,0}}},
  {{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{1,0,0,0},{1,1,0,0},{0,1,0,0}}},
  {{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,0,1,0},{0,1,1,0},{0,1,0,0}},
   {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{1,1,0,0},{1,0,0,0}}},
  {{{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{0,1,0,0},{0,1,0,0}},
   {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,0,0},{1,1,0,0}}},
  {{{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,0,0},{0,1,0,0},{0,1,1,0}},
   {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
   {{0,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,0,0}}}
};

int tetPiece = 0, tetRot = 0, tetX = 3, tetY = 0;
int tetScore = 0;
unsigned long tetLastFall = 0;
bool tetGameOver = false;

unsigned long tetLeftLast = 0, tetRightLast = 0, tetDownLast = 0;
bool tetLeftHeld = false, tetRightHeld = false, tetDownHeld = false;
bool tetLeftRepeating = false, tetRightRepeating = false, tetDownRepeating = false;
const unsigned long DAS_DELAY = 150;
const unsigned long DAS_REPEAT = 50;

void tetrisReset() {
  for (int y = 0; y < TET_ROWS; y++)
    for (int x = 0; x < TET_COLS; x++) tetrisField[y][x] = 0;
  tetPiece = random(0, 7);
  tetRot = 0;
  tetX = 3;
  tetY = 0;
  tetScore = 0;
  tetGameOver = false;
  tetLastFall = millis();
}

bool tetrisCanPlace(int px, int py, int rot) {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (!tetrominoes[tetPiece][rot][y][x]) continue;
      int fx = px + x;
      int fy = py + y;
      if (fx < 0 || fx >= TET_COLS || fy >= TET_ROWS) return false;
      if (fy >= 0 && tetrisField[fy][fx]) return false;
    }
  }
  return true;
}

void tetrisLock() {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (!tetrominoes[tetPiece][tetRot][y][x]) continue;
      int fx = tetX + x;
      int fy = tetY + y;
      if (fy >= 0 && fy < TET_ROWS && fx >= 0 && fx < TET_COLS) tetrisField[fy][fx] = 1;
    }
  }
  for (int y = TET_ROWS - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TET_COLS; x++) if (!tetrisField[y][x]) { full = false; break; }
    if (full) {
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < TET_COLS; x++) tetrisField[yy][x] = tetrisField[yy - 1][x];
      for (int x = 0; x < TET_COLS; x++) tetrisField[0][x] = 0;
      tetScore += 100;
      y++;
    }
  }
  tetPiece = random(0, 7);
  tetRot = 0;
  tetX = 3;
  tetY = 0;
  if (!tetrisCanPlace(tetX, tetY, tetRot)) tetGameOver = true;
}

void tetrisDraw() {
  M5Cardputer.Display.startWrite();
  M5Cardputer.Display.fillScreen(COL_BG);
  M5Cardputer.Display.drawRect(TET_X0 - 1, TET_Y0 - 1, TET_COLS * TET_CELL + 2, TET_ROWS * TET_CELL + 2, COL_TEXT);
  for (int y = 0; y < TET_ROWS; y++) {
    for (int x = 0; x < TET_COLS; x++) {
      if (tetrisField[y][x]) {
        M5Cardputer.Display.fillRect(TET_X0 + x * TET_CELL, TET_Y0 + y * TET_CELL, TET_CELL - 1, TET_CELL - 1, COL_TEXT);
      }
    }
  }
  if (!tetGameOver) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (tetrominoes[tetPiece][tetRot][y][x]) {
          int fx = tetX + x;
          int fy = tetY + y;
          if (fy >= 0) M5Cardputer.Display.fillRect(TET_X0 + fx * TET_CELL, TET_Y0 + fy * TET_CELL, TET_CELL - 1, TET_CELL - 1, COL_TEXT);
        }
      }
    }
  } else {
    M5Cardputer.Display.setTextColor(COL_TEXT, COL_BG);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(50, 45);
    M5Cardputer.Display.print("GAME OVER");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(50, 70);
    M5Cardputer.Display.print("Enter - restart");
  }
  M5Cardputer.Display.setTextColor(COL_TEXT, COL_BG);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(150, 25);
  M5Cardputer.Display.printf("%d", tetScore);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setCursor(150, 45);
  M5Cardputer.Display.print("score");
  drawBattery();
  M5Cardputer.Display.endWrite();
}

void tetris() {
  tetrisReset();
  tetrisDraw();
}

void tetrisHandleDAS() {
  if (tetGameOver) return;
  unsigned long now = millis();
  bool fn = M5Cardputer.Keyboard.isKeyPressed(KEY_FN);

  bool leftHeld = fn && M5Cardputer.Keyboard.isKeyPressed(',');
  if (leftHeld) {
    if (!tetLeftHeld) {
      if (tetrisCanPlace(tetX - 1, tetY, tetRot)) { tetX--; tetrisDraw(); }
      tetLeftHeld = true; tetLeftRepeating = false; tetLeftLast = now;
    } else {
      if (!tetLeftRepeating) {
        if (now - tetLeftLast >= DAS_DELAY) { tetLeftRepeating = true; tetLeftLast = now; }
      } else {
        if (now - tetLeftLast >= DAS_REPEAT) {
          if (tetrisCanPlace(tetX - 1, tetY, tetRot)) { tetX--; tetrisDraw(); }
          tetLeftLast = now;
        }
      }
    }
  } else { tetLeftHeld = false; tetLeftRepeating = false; }

  bool rightHeld = fn && M5Cardputer.Keyboard.isKeyPressed('/');
  if (rightHeld) {
    if (!tetRightHeld) {
      if (tetrisCanPlace(tetX + 1, tetY, tetRot)) { tetX++; tetrisDraw(); }
      tetRightHeld = true; tetRightRepeating = false; tetRightLast = now;
    } else {
      if (!tetRightRepeating) {
        if (now - tetRightLast >= DAS_DELAY) { tetRightRepeating = true; tetRightLast = now; }
      } else {
        if (now - tetRightLast >= DAS_REPEAT) {
          if (tetrisCanPlace(tetX + 1, tetY, tetRot)) { tetX++; tetrisDraw(); }
          tetRightLast = now;
        }
      }
    }
  } else { tetRightHeld = false; tetRightRepeating = false; }

  bool downHeld = fn && M5Cardputer.Keyboard.isKeyPressed('.');
  if (downHeld) {
    if (!tetDownHeld) {
      if (tetrisCanPlace(tetX, tetY + 1, tetRot)) { tetY++; tetLastFall = now; tetrisDraw(); }
      tetDownHeld = true; tetDownRepeating = false; tetDownLast = now;
    } else {
      if (!tetDownRepeating) {
        if (now - tetDownLast >= DAS_DELAY) { tetDownRepeating = true; tetDownLast = now; }
      } else {
        if (now - tetDownLast >= DAS_REPEAT) {
          if (tetrisCanPlace(tetX, tetY + 1, tetRot)) { tetY++; tetLastFall = now; tetrisDraw(); }
          tetDownLast = now;
        }
      }
    }
  } else { tetDownHeld = false; tetDownRepeating = false; }
}

void handleInput() {
  if (currentScreen == MENU || currentScreen == TETRIS) {
    if (!M5Cardputer.Keyboard.isChange()) return;
    if (!M5Cardputer.Keyboard.isPressed()) return;
  }

  if (currentScreen == MENU) {
    if (isUp()) {
      int old = menuIndex;
      menuIndex = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
      updateMenuSelection(old, menuIndex);
    }
    if (isDown()) {
      int old = menuIndex;
      menuIndex = (menuIndex + 1) % MENU_COUNT;
      updateMenuSelection(old, menuIndex);
    }
    if (isEnter()) {
      if (menuIndex == 0) { currentScreen = TETRIS; tetris(); }
      else                { currentScreen = SNAKE; snake(); }
    }
  } else if (currentScreen == SNAKE) {
    if (isBack()) { currentScreen = MENU; drawMenu(); return; }
    if (snakeGameOver) {
      if (isEnter()) { snakeReset(); snakeDraw(); }
      return;
    }
    if (isUp())    { if (snakeDir != 1) snakeDir = 3; }
    if (isDown())  { if (snakeDir != 3) snakeDir = 1; }
    if (isLeft())  { if (snakeDir != 0) snakeDir = 2; }
    if (isRight()) { if (snakeDir != 2) snakeDir = 0; }
  } else if (currentScreen == TETRIS) {
    if (isBack()) { currentScreen = MENU; drawMenu(); return; }
    if (tetGameOver) {
      if (isEnter()) { tetrisReset(); tetrisDraw(); }
      return;
    }
    if (isEnter()) {
      int nr = (tetRot + 1) % 4;
      if (tetrisCanPlace(tetX, tetY, nr)) tetRot = nr;
      tetrisDraw();
    }
  }
}

void loop() {
  M5Cardputer.update();
  handleInput();

  if (currentScreen == SNAKE && !snakeGameOver) {
    if (millis() - snakeLastMove > 200) {
      snakeLastMove = millis();
      snakeStep();
      snakeDraw();
    }
  } else if (currentScreen == TETRIS && !tetGameOver) {
    tetrisHandleDAS();
    if (millis() - tetLastFall > 500) {
      tetLastFall = millis();
      if (tetrisCanPlace(tetX, tetY + 1, tetRot)) tetY++;
      else tetrisLock();
      tetrisDraw();
    }
  }

  delay(10);
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5.Power.begin();
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextWrap(false);
  Serial.begin(115200);
  randomSeed(esp_random());
  drawMenu();
}
