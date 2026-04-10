#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ============================================================
//  НАСТРОЙКИ OLED
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_SDA 1   // GPIO 1 -> SDA
#define OLED_SCL 2   // GPIO 2 -> SCL
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================
//  НАСТРОЙКИ DFPLAYER
// ============================================================
HardwareSerial mySoftwareSerial(1);
#define DFPLAYER_RX 18
#define DFPLAYER_TX 17
DFRobotDFPlayerMini myDFPlayer;

// ============================================================
//  МАТРИЦА КЛАВИАТУРЫ
// ============================================================
const byte numRows = 5;
const byte numCols = 11;

byte rowPins[numRows] = {6, 4, 7, 5, 15};
byte colPins[numCols] = {38, 47, 21, 14, 13, 12, 11, 10, 9, 8, 16};

#define ROW_0  0
#define ROW_1  1
#define ROW_2  2
#define ROW_3  3
#define ROW_4  4

#define COL_0  0
#define COL_1  1
#define COL_2  2
#define COL_3  3
#define COL_4  4
#define COL_5  5
#define COL_6  6
#define COL_7  7
#define COL_8  8
#define COL_9  9
#define COL_10 10

#define KEY_ESC_ROW     ROW_0
#define KEY_ESC_COL     COL_0
#define KEY_START_ROW   ROW_1
#define KEY_START_COL   COL_10
#define KEY_VOLDOWN_ROW ROW_0
#define KEY_VOLDOWN_COL COL_10
#define KEY_GAME1_ROW   ROW_1
#define KEY_GAME1_COL   COL_0
#define KEY_GAME2_ROW   ROW_1
#define KEY_GAME2_COL   COL_1
#define KEY_GAME3_ROW   ROW_1
#define KEY_GAME3_COL   COL_2
#define KEY_ENTER_ROW   ROW_4
#define KEY_ENTER_COL   COL_10

char keys[numRows][numCols] = {
  { 'E', 'i', 'o', '4', 'R', '8', 'q', 'w', 'f', '=', ' ' },
  { '`', '2', '3', 'r', '5', 'y', '9', '0', '-', 'B', ' ' },
  { 'q', 'w', 'e', 'f', 't', 'h', 'u', 'i', 'o', 'p', ']' },
  { 'a', 's', 'd', 'v', 'g', 'n', 'j', 'k', 'l', ';', '\'' },
  { 'z', 'x', 'c', ' ', 'b', ' ', 'm', ',', '.', '[', '\0' }
};

// ============================================================
//  ТРЕКИ DFPLAYER
// ============================================================
#define TRACK_MENU_BEEP    48
#define TRACK_GAME2_SELECT 49
#define TRACK_GAME3_SELECT 50
#define TRACK_CONFIRM      47
#define TRACK_START        52
#define TRACK_CORRECT      54
#define TRACK_WRONG        53
#define TRACK_SKIP         55
#define TRACK_VICTORY      56
#define TRACK_VOL_DOWN     44
#define TRACK_VOL_UP       45

// ============================================================
//  СОСТОЯНИЯ ИГРЫ
// ============================================================
enum GameState { IDLE, ESC_CONFIRM, MENU, IN_GAME };
GameState currentState = IDLE;

// ============================================================
//  ПЕРЕМЕННЫЕ ИГРЫ
// ============================================================
int  currentVolume   = 15;
int  selectedGame    = 0;
int  currentLetterId = -1;
char lastTypedChar   = '\0';

unsigned long delayUntil   = 0;
bool          waitingDelay = false;

enum DelayAction { DA_NONE, DA_NEXT_LETTER, DA_RESET };
DelayAction pendingAction = DA_NONE;

// ============================================================
//  АЛФАВИТ
// ============================================================
struct KazLetter {
  int         id;
  char        expectedKey;
  const char* symbol;
  int         mistakes;
  bool        passed;
};

KazLetter alphabet[42] = {
  { 1,  'f',  "A",   0, false},
  { 2,  '1',  "A'",  0, false},
  { 3,  ',',  "Б",   0, false},
  { 4,  'd',  "В",   0, false},
  { 5,  'u',  "Г",   0, false},
  { 6,  '4',  "F",   0, false},
  { 7,  'l',  "Д",   0, false},
  { 8,  't',  "E",   0, false},
  { 9,  '`',  "Ё",   0, false},
  {10,  ';',  "Ж",   0, false},
  {11,  'p',  "З",   0, false},
  {12,  'b',  "И",   0, false},
  {13,  'q',  "Й",   0, false},
  {14,  'r',  "К",   0, false},
  {15,  '7',  "К'",  0, false},
  {16,  'k',  "Л",   0, false},
  {17,  'v',  "М",   0, false},
  {18,  'y',  "Н",   0, false},
  {19,  '3',  "Н'",  0, false},
  {20,  'j',  "О",   0, false},
  {21,  '8',  "О'",  0, false},
  {22,  'g',  "П",   0, false},
  {23,  'h',  "Р",   0, false},
  {24,  'c',  "С",   0, false},
  {25,  'n',  "Т",   0, false},
  {26,  'e',  "У",   0, false},
  {27,  '6',  "У'",  0, false},
  {28,  '5',  "Ү",   0, false},
  {29,  'a',  "Ф",   0, false},
  {30,  '[',  "Х",   0, false},
  {31,  '9',  "h",   0, false},
  {32,  'w',  "Ц",   0, false},
  {33,  'x',  "Ч",   0, false},
  {34,  'i',  "Ш",   0, false},
  {35,  'o',  "Щ",   0, false},
  {36,  ']',  "Ъ",   0, false},
  {37,  's',  "Ы",   0, false},
  {38,  '2',  "І",   0, false},
  {39,  'm',  "ь",   0, false},
  {40, '\'',  "Э",   0, false},
  {41,  '.',  "Ю",   0, false},
  {42,  'z',  "Я",   0, false}
};

// ============================================================
//  ПРОТОТИПЫ
// ============================================================
void scanMatrix();
void handleKeyPress(int row, int col, char keyChar);
void startGame();
void nextLetter();
void checkAnswer();
void resetGame();
void scheduleDelay(unsigned long ms, DelayAction action);
void processDelayedAction();
void drawScreen(const char* title, const char* line1, const char* line2 = "");

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < numRows; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  for (int j = 0; j < numCols; j++) {
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], HIGH);
  }
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[OLED] Init FAILED — check wiring!"));
    for (;;);
  }
  drawScreen("System", "Init...", "");

  mySoftwareSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    drawScreen("ERROR", "DFPlayer failed", "Check wiring!");
    while (true);
  }

  myDFPlayer.volume(currentVolume);
  randomSeed(esp_random());
  delay(500);

  currentState = IDLE;
  drawScreen("READY", "Нажми START", "(row1 col10)");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  processDelayedAction();
  scanMatrix();
}

// ============================================================
//  НЕБЛОКИРУЮЩИЙ ТАЙМЕР
// ============================================================
void scheduleDelay(unsigned long ms, DelayAction action) {
  delayUntil    = millis() + ms;
  waitingDelay  = true;
  pendingAction = action;
}

void processDelayedAction() {
  if (!waitingDelay) return;
  if (millis() < delayUntil) return;

  waitingDelay = false;
  DelayAction action = pendingAction;
  pendingAction = DA_NONE;

  switch (action) {
    case DA_NEXT_LETTER: nextLetter(); break;
    case DA_RESET:       resetGame();  break;
    default: break;
  }
}

// ============================================================
//  СКАНИРОВАНИЕ МАТРИЦЫ
// ============================================================
void scanMatrix() {
  for (int c = 0; c < numCols; c++) {
    digitalWrite(colPins[c], LOW);      // тянем колонку в LOW
    
    for (int r = 0; r < numRows; r++) {
      if (digitalRead(rowPins[r]) == LOW) {  // читаем строки
        Serial.print(F("[HIT] colPin="));
        Serial.print(colPins[c]);
        Serial.print(F(" rowPin="));
        Serial.println(rowPins[r]);
        
        handleKeyPress(r, c, keys[r][c]);
        unsigned long t = millis();
        while (digitalRead(rowPins[r]) == LOW) {
          delay(10);
          if (millis() - t > 500) break;
        }
      }
    }
    digitalWrite(colPins[c], HIGH);
  }
}

// ============================================================
//  ОБРАБОТКА НАЖАТИЯ
// ============================================================
void handleKeyPress(int row, int col, char keyChar) {

  // Лог каждого нажатия
  Serial.print(F("[KEY] row="));
  Serial.print(row);
  Serial.print(F(" col="));
  Serial.print(col);
  Serial.print(F(" char='"));
  Serial.print(keyChar);
  Serial.println(F("'"));

  // --- ESC (row0, col0) — из любого состояния кроме IDLE ---
  if (row == KEY_ESC_ROW && col == KEY_ESC_COL) {
    if (currentState != IDLE) {
      Serial.println(F("[ESC] Asking confirmation..."));
      currentState = ESC_CONFIRM;
      myDFPlayer.stop();
      drawScreen("ВЫХОД?", "ENTER = да", "др.клавиша = нет");
    }
    return;
  }

  switch (currentState) {

    // ----------------------------------------------------------
    case IDLE:
      if (row == KEY_START_ROW && col == KEY_START_COL) {
        currentState = MENU;
        myDFPlayer.playMp3Folder(TRACK_MENU_BEEP);
        drawScreen("МЕНЮ", "1=Игра1  2=Игра2", "3=Игра3");
      }
      break;

    // ----------------------------------------------------------
    case ESC_CONFIRM:
      if (row == KEY_ENTER_ROW && col == KEY_ENTER_COL) {
        // Подтверждение — выход
        Serial.println(F("[ESC] Confirmed -> resetGame"));
        resetGame();
      } else {
        // Отмена — возврат в игру
        Serial.println(F("[ESC] Cancelled -> back to IN_GAME"));
        currentState = IN_GAME;
        char buf[20];
        snprintf(buf, sizeof(buf), "Буква: %s", alphabet[currentLetterId].symbol);
        drawScreen("СЛУШАЙ", buf, "Введи букву");
        myDFPlayer.playMp3Folder(alphabet[currentLetterId].id);
      }
      break;

    // ----------------------------------------------------------
    case MENU:
      if (row == KEY_GAME1_ROW && col == KEY_GAME1_COL) {
        selectedGame = 1;
        myDFPlayer.playMp3Folder(TRACK_MENU_BEEP);
        drawScreen("ИГРА 1", "Слушай букву,", "введи клавишу");
      }
      else if (row == KEY_GAME2_ROW && col == KEY_GAME2_COL) {
        selectedGame = 2;
        myDFPlayer.playMp3Folder(TRACK_GAME2_SELECT);
        drawScreen("ИГРА 2", "Выбрана игра 2", "ENTER для старта");
      }
      else if (row == KEY_GAME3_ROW && col == KEY_GAME3_COL) {
        selectedGame = 3;
        myDFPlayer.playMp3Folder(TRACK_GAME3_SELECT);
        drawScreen("ИГРА 3", "Выбрана игра 3", "ENTER для старта");
      }
      else if (row == KEY_ENTER_ROW && col == KEY_ENTER_COL) {
        if (selectedGame > 0) {
          myDFPlayer.playMp3Folder(TRACK_CONFIRM);
          startGame();
        } else {
          drawScreen("МЕНЮ", "Сначала выбери", "игру: 1, 2, 3");
        }
      }
      break;

    // ----------------------------------------------------------
    case IN_GAME:
      if (row == KEY_VOLDOWN_ROW && col == KEY_VOLDOWN_COL) {
        currentVolume = max(0, currentVolume - 4);
        myDFPlayer.volume(currentVolume);
        myDFPlayer.playMp3Folder(TRACK_VOL_DOWN);
        Serial.print(F("[VOL] Down -> "));
        Serial.println(currentVolume);
        return;
      }
      if (row == KEY_START_ROW && col == KEY_START_COL) {
        currentVolume = min(30, currentVolume + 4);
        myDFPlayer.volume(currentVolume);
        myDFPlayer.playMp3Folder(TRACK_VOL_UP);
        Serial.print(F("[VOL] Up -> "));
        Serial.println(currentVolume);
        return;
      }
      if (row == KEY_ENTER_ROW && col == KEY_ENTER_COL) {
        checkAnswer();
        return;
      }
      if (keyChar != '\0' && keyChar != ' ') {
        lastTypedChar = keyChar;
        char buf[24];
        snprintf(buf, sizeof(buf), "Ввод: [%c]", lastTypedChar);
        drawScreen("ОТВЕТ", buf, "ENTER=проверить");
      }
      break;
  }
}

// ============================================================
//  СТАРТ ИГРЫ
// ============================================================
void startGame() {
  currentState    = IN_GAME;
  lastTypedChar   = '\0';
  currentLetterId = -1;

  Serial.print(F("[GAME] Starting game "));
  Serial.println(selectedGame);

  myDFPlayer.playMp3Folder(TRACK_START);
  drawScreen("ПОЕХАЛИ!", "Слушай букву...", "");
  scheduleDelay(1500, DA_NEXT_LETTER);
}

// ============================================================
//  СЛЕДУЮЩАЯ БУКВА
// ============================================================
void nextLetter() {
  if (currentState != IN_GAME) return;

  int unpassedIndices[42];
  int unpassedCount = 0;

  for (int i = 0; i < 42; i++) {
    if (!alphabet[i].passed) {
      unpassedIndices[unpassedCount++] = i;
    }
  }

  Serial.print(F("[GAME] Remaining letters: "));
  Serial.println(unpassedCount);

  if (unpassedCount == 0) {
    Serial.println(F("[GAME] All letters passed -> VICTORY"));
    myDFPlayer.playMp3Folder(TRACK_VICTORY);
    drawScreen("ПОБЕДА!", "Молодец!", "ESC=в меню");
    scheduleDelay(4000, DA_RESET);
    return;
  }

  int randIndex   = unpassedIndices[random(0, unpassedCount)];
  currentLetterId = randIndex;
  lastTypedChar   = '\0';

  Serial.print(F("[GAME] Letter -> id="));
  Serial.print(alphabet[currentLetterId].id);
  Serial.print(F("  symbol="));
  Serial.print(alphabet[currentLetterId].symbol);
  Serial.print(F("  expectedKey='"));
  Serial.print(alphabet[currentLetterId].expectedKey);
  Serial.println(F("'"));

  char buf[20];
  snprintf(buf, sizeof(buf), "Буква: %s", alphabet[currentLetterId].symbol);
  drawScreen("СЛУШАЙ", buf, "Введи и жми ENT");
  myDFPlayer.playMp3Folder(alphabet[currentLetterId].id);
}

// ============================================================
//  ПРОВЕРКА ОТВЕТА
// ============================================================
void checkAnswer() {
  if (currentLetterId == -1) return;

  if (lastTypedChar == '\0') {
    drawScreen("ВНИМАНИЕ", "Нажми букву,", "затем ENTER");
    return;
  }

  Serial.print(F("[CHECK] Expected='"));
  Serial.print(alphabet[currentLetterId].expectedKey);
  Serial.print(F("'  Got='"));
  Serial.print(lastTypedChar);
  Serial.println(F("'"));

  if (lastTypedChar == alphabet[currentLetterId].expectedKey) {
    Serial.println(F("[CHECK] -> CORRECT"));
    myDFPlayer.playMp3Folder(TRACK_CORRECT);
    alphabet[currentLetterId].passed = true;
    lastTypedChar = '\0';
    drawScreen("ВЕРНО!", alphabet[currentLetterId].symbol, "Отлично!");
    scheduleDelay(1500, DA_NEXT_LETTER);

  } else {
    alphabet[currentLetterId].mistakes++;
    lastTypedChar = '\0';

    Serial.print(F("[CHECK] -> WRONG. Mistakes="));
    Serial.println(alphabet[currentLetterId].mistakes);

    if (alphabet[currentLetterId].mistakes >= 3) {
      Serial.println(F("[CHECK] -> SKIP (3 mistakes)"));
      alphabet[currentLetterId].passed = true;
      myDFPlayer.playMp3Folder(TRACK_SKIP);
      char buf[20];
      snprintf(buf, sizeof(buf), "Буква: %s", alphabet[currentLetterId].symbol);
      drawScreen("ПРОПУСК", buf, "Запомни её!");
      scheduleDelay(2500, DA_NEXT_LETTER);
    } else {
      myDFPlayer.playMp3Folder(TRACK_WRONG);
      char errBuf[16];
      snprintf(errBuf, sizeof(errBuf), "Ошибок: %d/3", alphabet[currentLetterId].mistakes);
      drawScreen("ОШИБКА!", errBuf, "Попробуй снова");
      scheduleDelay(1800, DA_NEXT_LETTER);
    }
  }
}

// ============================================================
//  СБРОС ИГРЫ
// ============================================================
void resetGame() {
  waitingDelay  = false;
  pendingAction = DA_NONE;

  currentState    = IDLE;
  selectedGame    = 0;
  currentLetterId = -1;
  lastTypedChar   = '\0';

  for (int i = 0; i < 42; i++) {
    alphabet[i].passed   = false;
    alphabet[i].mistakes = 0;
  }

  myDFPlayer.stop();
  Serial.println(F("[GAME] Reset -> IDLE"));
  drawScreen("READY", "Нажми START", "для новой игры");
}

// ============================================================
//  OLED + SERIAL ДУБЛЬ
//
//  Все три строки автоматически дублируются в Serial.
//  Формат в монитор:
//
//  [OLED] ================
//  [OLED] <title>
//  [OLED] <line1>
//  [OLED] <line2>
// ============================================================
void drawScreen(const char* title, const char* line1, const char* line2) {

  // --- Serial mirror ---
  Serial.println(F("[OLED] ================"));
  Serial.print(F("[OLED] "));
  Serial.println(title);
  if (line1 && line1[0] != '\0') {
    Serial.print(F("[OLED] "));
    Serial.println(line1);
  }
  if (line2 && line2[0] != '\0') {
    Serial.print(F("[OLED] "));
    Serial.println(line2);
  }

  // --- OLED render ---
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(title);

  display.drawLine(0, 18, 127, 18, WHITE);

  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println(line1);

  display.setCursor(0, 34);
  display.println(line2);

  display.display();
}
