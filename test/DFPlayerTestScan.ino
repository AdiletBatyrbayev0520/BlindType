#include "USB.h"
#include "USBHIDKeyboard.h"
#include <HardwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Настройки дисплея ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// I2C пины для ESP32-S3
#define SDA_PIN 1
#define SCL_PIN 2

// --- Настройки плеера ---
HardwareSerial mySerial(1);
DFRobotDFPlayerMini myDFPlayer;

// --- Настройки клавиатуры ---
USBHIDKeyboard Keyboard;

const byte numRows = 5;  
const byte numCols = 11; 

byte rowPins[numRows] = {6, 4, 7, 5, 15}; 
byte colPins[numCols] = {38, 47, 21, 14, 13, 12, 11, 10, 9, 8, 16};

// Раскладка клавиш (E = ESC, B = BACKSPACE, R = RETURN/ENTER)
char keys[numRows][numCols] = {
  { 'E', 'i', 'o', '4', 'R', '8', 'q', 'w', 'f', '=', ' ' },
  { '`', '2', '3', 'r', '5', 'y', '9', '0', '-', 'B', ' ' },
  { 'q', 'w', 'e', 'f', 't', 'h', 'u', 'i', 'o', 'p', ']' },
  { 'a', 's', 'd', 'v', 'g', 'n', 'j', 'k', 'l', ';', '\'' },
  { 'z', 'x', 'c', ' ', 'b', ' ', 'm', ',', '.', '[', 'R' }  
};

// Карта звуков (номера файлов в папке "01" на SD-карте)
// Взята из твоего первого скетча. Если нужно, поменяй номера под новую раскладку.
int soundMap[numRows][numCols] = {
  { 5,  34, 35, 11,  0, 27, 13, 32,  1, 26,  8 },
  { 13, 32, 26,  0,  8, 27,  2, 34,  6, 11, 30 },
  { 15, 21, 31,  0, 22, 23,  5, 38, 14, 19, 28 },
  { 20, 16,  7, 10, 40, 25, 29, 37, 17,  4, 22 },
  { 39,  3, 41, 30, 43, 43, 43, 33, 17, 24, 12 } 
};

bool keyState[numRows][numCols] = {false};
String lastKey = "None";

// --- Функция обновления рабочего экрана ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,0);
  display.println("ESP32-S3 Keyboard");
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
  
  display.setCursor(0, 25);
  display.setTextSize(2);
  display.print("Key: ");
  display.println(lastKey);
  
  display.display();
}

// --- Функция вывода статуса при загрузке ---
void showBootStatus(String message, bool isError = false) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("System Booting...");
  display.setCursor(0, 30);
  display.println(message);
  
  if (isError) {
    display.setCursor(0, 50);
    display.println("Check Wiring/SD!");
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);

  // 1. Инициализация дисплея
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Ошибка OLED!");
  }
  
  showBootStatus("Init USB...");
  Keyboard.begin();
  USB.begin();

  // 2. Инициализация DFPlayer
  showBootStatus("Init DFPlayer...");
  mySerial.begin(9600, SERIAL_8N1, 17, 18); 
  delay(1000); // Даем плееру время на пробуждение
  
  if (myDFPlayer.begin(mySerial)) {
    showBootStatus("DFPlayer OK!");
    myDFPlayer.volume(25); // Громкость от 0 до 30
  } else {
    showBootStatus("DFPlayer ERROR!", true);
  }
  delay(2000); // Задержка, чтобы успеть прочитать статус на экране

  updateDisplay(); // Переключаемся на рабочий экран

  // 3. Настройка пинов матрицы
  for (int i = 0; i < numRows; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
  }

  for (int i = 0; i < numCols; i++) {
    pinMode(colPins[i], INPUT_PULLDOWN);
  }
}

void loop() {
  bool changed = false;

  for (int r = 0; r < numRows; r++) {
    digitalWrite(rowPins[r], HIGH);
    delayMicroseconds(20); 

    for (int c = 0; c < numCols; c++) {
      bool currentState = digitalRead(colPins[c]);

      if (currentState != keyState[r][c]) {
        keyState[r][c] = currentState; 
        
        uint8_t hidKey = keys[r][c];
        String dispKey = String((char)keys[r][c]);

        // Обработка спецклавиш (по заглавным буквам из массива)
        if (keys[r][c] == 'E') { 
          hidKey = KEY_ESC; 
          dispKey = "ESC"; 
        } else if (keys[r][c] == 'B') { 
          hidKey = KEY_BACKSPACE; 
          dispKey = "BACKSPACE"; 
        } else if (keys[r][c] == 'R') { 
          hidKey = KEY_RETURN; 
          dispKey = "ENTER"; 
        }

        if (currentState == HIGH) { // Кнопка нажата
          // 1. Печатаем символ
          Keyboard.press(hidKey);
          
          // 2. Воспроизводим звук
          int trackNumber = soundMap[r][c];
          if (trackNumber > 0) {
            myDFPlayer.playFolder(1, trackNumber);
          }

          // 3. Подготавливаем текст для экрана
          lastKey = dispKey;
          changed = true;
          
        } else { // Кнопка отпущена
          Keyboard.release(hidKey);
        }
      }
    }
    digitalWrite(rowPins[r], LOW);
  }
  
  // Обновляем дисплей только если было нажатие (чтобы не мерцал)
  if (changed) {
    updateDisplay();
  }

  delay(10); // Небольшой антидребезг
}
// #include <Keypad.h>

// const byte numRows = 5; // Пять строк
// const byte numCols = 11; // Одиннадцать столбцов

// // Ваши пины строк (Красные/Плюс)
// byte rowPins[numRows] = {4, 5, 6, 7, 15}; 

// // Ваши пины столбцов (Зеленые/Минус)
// byte colPins[numCols] = {10, 11, 12, 13, 14, 16, 8, 9, 38, 21, 47};

// // Создаем пустую карту клавиш для теста (просто занумеруем их)
// char keys[numRows][numCols];

// Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, numRows, numCols);

// void setup() {
//   Serial.begin(115200);
//   Serial.println("Тест матрицы кнопок запущен...");
//   Serial.println("Нажимайте кнопки по очереди, чтобы проверить пайку.");
  
//   // Заполняем массив для идентификации
//   for (int r = 0; r < numRows; r++) {
//     for (int c = 0; c < numCols; c++) {
//       keys[r][c] = (char)(r * numCols + c); 
//     }
//   }
// }

// void loop() {
//   char key = keypad.getKey();

//   if (key != NO_KEY) {
//     // Вычисляем, на какой строке и столбце нажата кнопка
//     int row = -1;
//     int col = -1;
    
//     for (int r = 0; r < numRows; r++) {
//       for (int c = 0; c < numCols; c++) {
//         if (keys[r][c] == key) {
//           row = r;
//           col = c;
//         }
//       }
//     }

//     Serial.print("Кнопка РАБОТАЕТ! -> ");
//     Serial.print("Строка (пин): ");
//     Serial.print(rowPins[row]);
//     Serial.print(" | Столбец (пин): ");
//     Serial.println(colPins[col]);
//   }
// }


// #include "USB.h"
// #include "USBHIDKeyboard.h"

// // Создаем объект клавиатуры
// USBHIDKeyboard Keyboard;

// const byte numRows = 5;  
// const byte numCols = 11; 

// // ЖЕЛТЫЕ ПРОВОДА (Строки / Плюс)
// byte rowPins[numRows] = {6, 4, 7, 5, 15}; 

// // ЗЕЛЕНЫЕ ПРОВОДА (Столбцы / Минус)
// byte colPins[numCols] = {38, 47, 21, 14, 13, 12, 11, 10, 9, 8, 16};

// // ВАША РАСКЛАДКА БУКВ
// char keys[numRows][numCols] = {
//   { 'E', 'i', 'o', '4', 'R', '8', 'q', 'w', 'f', '=', ' ' },
//   { '`', '2', '3', 'r', '5', 'y', '9', '0', '-', 'B', ' ' }, // <-- Исправлено: [1][0] теперь '`' (это "ё" в RU раскладке)
//   { 'q', 'w', 'e', 'f', 't', 'h', 'u', 'i', 'o', 'p', ']' },
//   { 'a', 's', 'd', 'v', 'g', 'n', 'j', 'k', 'l', ';', '\'' },
//   { 'z', 'x', 'c', ' ', 'b', ' ', 'm', ',', '.', '[', 'R' }  
// };

// bool keyState[numRows][numCols] = {false};

// void setup() {
//   for (int i = 0; i < numRows; i++) {
//     pinMode(rowPins[i], OUTPUT);
//     digitalWrite(rowPins[i], LOW);
//   }

//   for (int i = 0; i < numCols; i++) {
//     pinMode(colPins[i], INPUT_PULLDOWN);
//   }

//   Keyboard.begin();
//   USB.begin();
// }

// void loop() {
//   for (int r = 0; r < numRows; r++) {
//     digitalWrite(rowPins[r], HIGH);
//     delayMicroseconds(20); 

//     for (int c = 0; c < numCols; c++) {
//       bool currentState = digitalRead(colPins[c]);

//       if (currentState != keyState[r][c]) {
//         keyState[r][c] = currentState; 
        
//         char keyChar = keys[r][c];
//         uint8_t hidKey = keyChar;

//         // --- БЛОК СПЕЦКЛАВИШ ---
//         // 6:38 -> Строка 0 (пин 6), Столбец 0 (пин 38)
//         if (r == 0 && c == 0) hidKey = KEY_ESC; 
        
//         // Другие ваши спецклавиши (оставил как пример, можете удалить/изменить)
//         if (keyChar == 'B' && r == 1 && c == 9) hidKey = KEY_BACKSPACE; 
//         if (keyChar == 'R' && r == 4 && c == 10) hidKey = KEY_RETURN;   
//         // -----------------------

//         if (currentState == HIGH) {
//           Keyboard.press(hidKey);
//         } else {
//           Keyboard.release(hidKey);
//         }
//       }
//     }
//     digitalWrite(rowPins[r], LOW);
//   }
  
//   delay(10); 
// }
/*
 * ФИНАЛЬНАЯ USB-КЛАВИАТУРА + DFPLAYER (Исправленные пины)
 * Направление: Желтый (+) -> Зеленый (-)
 */

/*
 * ФИНАЛЬНАЯ USB-КЛАВИАТУРА + DFPLAYER (Исправленные пины)
 * Направление: Желтый (+) -> Зеленый (-)
 */

// #include "USB.h"
// #include "USBHIDKeyboard.h"
// #include <HardwareSerial.h>
// #include "DFRobotDFPlayerMini.h"

// USBHIDKeyboard Keyboard;

// // DFPlayer подключен к пинам: RX=17, TX=18
// HardwareSerial mySerial(1);
// DFRobotDFPlayerMini myDFPlayer;

// const byte numRows = 5;  
// const byte numCols = 11; 

// // ЖЕЛТЫЕ ПРОВОДА (Строки / Плюс)
// byte rowPins[numRows] = {4, 5, 6, 7, 15}; 

// // ЗЕЛЕНЫЕ ПРОВОДА (Столбцы / Минус)
// // 43 и 44 УБРАНЫ. Вместо них поставлены безопасные 8 и 9.
// byte colPins[numCols] = {10, 11, 12, 13, 14, 16, 8, 9, 38, 21, 47};

// // ВАША РАСКЛАДКА БУКВ
// char keys[numRows][numCols] = {
//   { 'u', 'i', 'o', 'p', 'R', '6', 'q', 'w', 'f', 'e', 't' },
//   { 'q', 'w', 'e', '0', 't', '6', '1', 'i', '4', 'p', '[' },
//   { '7', '8', '9', 'B', 'g', 'h', 'u', '2', 'r', '3', '5' },
//   { 'j', 'k', 'l', ';', '\'', 'n', 'a', 's', 'v', 'd', 'g' }, 
//   { 'm', ',', '.', '[', ' ', ' ', ' ', 'x', 'V', 'c', 'b' }  
// };

// // КАРТА ЗВУКОВ (Номера файлов в папке "01" на SD-карте)
// int soundMap[numRows][numCols] = {
//   { 5,  34, 35, 11,  0, 27, 13, 32,  1, 26,  8 },
//   { 13, 32, 26,  0,  8, 27,  2, 34,  6, 11, 30 },
//   { 15, 21, 31,  0, 22, 23,  5, 38, 14, 19, 28 },
//   { 20, 16,  7, 10, 40, 25, 29, 37, 17,  4, 22 },
//   { 39,  3, 41, 30, 43, 43, 43, 33, 17, 24, 12 } 
// };

// void setup() {
//   // Скорость для связи с компьютером
//   Serial.begin(115200);
  
//   // Запускаем USB-эмуляцию
//   Keyboard.begin();
//   USB.begin();

//   // Запускаем DFPlayer (RX = 17, TX = 18). Строго 9600 бод!
//   mySerial.begin(9600, SERIAL_8N1, 17, 18); 
//   Serial.println("Инициализация DFPlayer...");
  
//   delay(1000); // Время на пробуждение плеера
  
//   if (myDFPlayer.begin(mySerial)) {
//     Serial.println("DFPlayer готов!");
//     myDFPlayer.volume(25); // Громкость (0-30)
//   } else {
//     Serial.println("Ошибка DFPlayer! Проверьте SD-карту и провода 17/18.");
//   }

//   // Настраиваем пины матрицы
//   for (int r = 0; r < numRows; r++) {
//     pinMode(rowPins[r], INPUT_PULLUP);
//   }
//   for (int c = 0; c < numCols; c++) {
//     pinMode(colPins[c], OUTPUT);
//     digitalWrite(colPins[c], HIGH);
//   }
// }

// void loop() {
//   for (int c = 0; c < numCols; c++) {
    
//     // Открываем путь току (даем Минус)
//     digitalWrite(colPins[c], LOW);
//     delayMicroseconds(50); 

//     for (int r = 0; r < numRows; r++) {
      
//       // Если кнопка нажата
//       if (digitalRead(rowPins[r]) == LOW) {
        
//         char pressedKey = keys[r][c];
//         int trackNumber = soundMap[r][c];
        
//         // 1. ИГРАЕМ ЗВУК
//         if (trackNumber > 0) {
//           myDFPlayer.playFolder(1, trackNumber); 
//         }

//         // 2. ПЕЧАТАЕМ БУКВУ В КОМПЬЮТЕР
//         if (pressedKey == 'R') {
//           Keyboard.press(KEY_RETURN); 
//         } 
//         else if (pressedKey == 'B') {
//           Keyboard.press(KEY_BACKSPACE); 
//         } 
//         else if (pressedKey != '0') { 
//           Keyboard.press(pressedKey); 
//         }
        
//         // 3. ЖДЕМ ОТПУСКАНИЯ КНОПКИ
//         while(digitalRead(rowPins[r]) == LOW) {
//            delay(10); 
//         }
        
//         // 4. ОТПУСКАЕМ КНОПКУ
//         Keyboard.releaseAll(); 
//       }
//     }
    
//     // Закрываем столбец
//     digitalWrite(colPins[c], HIGH);
//   }
// }