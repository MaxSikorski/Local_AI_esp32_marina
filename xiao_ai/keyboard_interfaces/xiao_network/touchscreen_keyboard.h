namespace lcdchat {

constexpr int SDA_PIN = D4;
constexpr int SCL_PIN = D5;
constexpr int TOUCH_INT_PIN = D7;
constexpr int BACKLIGHT_PIN = D6;
constexpr std::uint8_t TOUCH_ADDR = 0x2E;

constexpr std::uint16_t BG = TFT_BLACK;
constexpr std::uint16_t FG = TFT_WHITE;
constexpr std::uint16_t DIM = 0x8410;
constexpr std::uint16_t USER = 0xFD9F;    
constexpr std::uint16_t AI = 0x7DFF;      
constexpr std::uint16_t THINK = 0xB7FF;   
constexpr std::uint16_t ACCENT = 0xA81F;  
constexpr std::uint16_t ERR = TFT_RED;

char input[PROD_INPUT_BYTES] = {};
std::size_t input_len = 0;
char last_user[PROD_INPUT_BYTES] = {};
char last_reply[PROD_DECODE_BYTES] = {};
bool thinking = false;
bool last_ok = true;
bool touch_down = false;
std::uint32_t last_touch_ms = 0;

 
inline void i2cDelay() { delayMicroseconds(2); }
inline void sdaLow() { pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }
inline void sclLow() { pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
inline void sdaRelease() { pinMode(SDA_PIN, INPUT_PULLUP); }
inline void sclRelease() { pinMode(SCL_PIN, INPUT_PULLUP); }

void i2cStart() {
    sdaRelease(); sclRelease(); i2cDelay();
    sdaLow(); i2cDelay();
    sclLow();
}

void i2cStop() {
    sdaLow(); i2cDelay();
    sclRelease(); i2cDelay();
    sdaRelease(); i2cDelay();
}

bool i2cWriteByte(std::uint8_t v) {
    for (int i = 7; i >= 0; --i) {
        if (v & (1u << i)) sdaRelease(); else sdaLow();
        i2cDelay();
        sclRelease(); i2cDelay();
        sclLow();
    }
    sdaRelease(); i2cDelay();
    sclRelease(); i2cDelay();
    const bool ack = digitalRead(SDA_PIN) == LOW;
    sclLow();
    return ack;
}

std::uint8_t i2cReadByte(bool ack) {
    std::uint8_t v = 0;
    sdaRelease();
    for (int i = 7; i >= 0; --i) {
        sclRelease(); i2cDelay();
        if (digitalRead(SDA_PIN)) v |= (1u << i);
        sclLow(); i2cDelay();
    }
    if (ack) sdaLow(); else sdaRelease();
    sclRelease(); i2cDelay();
    sclLow(); i2cDelay();
    sdaRelease();
    return v;
}

bool readTouch(std::uint8_t* x, std::uint8_t* y) {
    if (!x || !y || digitalRead(TOUCH_INT_PIN) != LOW) return false;

    std::uint8_t d[5] = {};
    i2cStart();
    if (!i2cWriteByte((TOUCH_ADDR << 1) | 1u)) {
        i2cStop();
        return false;
    }
    for (int i = 0; i < 5; ++i) d[i] = i2cReadByte(i != 4);
    i2cStop();

    if (d[0] != 0x01) return false;
    *x = d[2];
    *y = d[4];
    return true;
}

 
void textAt(int x, int y, const char* s, std::uint16_t c = FG) {
    tft.setTextColor(c, BG);
    tft.setCursor(x, y);
    tft.print(s);
}

int wrapped(const char* prefix, const char* s, int y, int max_lines, std::uint16_t c) {
    constexpr int N = 25;
    char line[N + 1];
    int n = 0, lines = 0;

    if (prefix) {
        while (*prefix && n < N) line[n++] = *prefix++;
    }

    while (s && *s && lines < max_lines) {
        char ch = *s++;
        if (ch == '\r') continue;

        if (ch == '\n') {
            if (n) {
                line[n] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines; n = 0;
            }
            continue;
        }

        if (n == N) {
             
            int cut = -1;
            for (int i = N - 1; i > 0; --i) {
                if (line[i] == ' ') { cut = i; break; }
            }

            if (cut > 0) {
                line[cut] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines;

                int j = 0;
                for (int i = cut + 1; i < n; ++i) line[j++] = line[i];
                n = j;
            } else {
                line[n] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines; n = 0;
            }

            if (lines >= max_lines) break;
        }

         
        if (n == 0 && ch == ' ') continue;
        line[n++] = ch;
    }

    if (n && lines < max_lines) {
        line[n] = 0;
        tft.fillRect(44, y, 152, 9, BG);
        textAt(44, y, line, c);
        y += 9;
    }
    return y;
}

void drawConversation() {
     
    tft.fillRect(42, 18, 156, 78, BG);
    int y = 30;

    if (last_user[0]) {
        y = wrapped("You: ", last_user, y, 2, USER);
        y += 3;
    }

    if (thinking) wrapped("AI: ", "thinking...", y, 4, THINK);
    else if (!last_ok) wrapped("AI: ", last_reply, y, 4, ERR);
    else if (last_reply[0]) wrapped("AI: ", last_reply, y, 5, AI);
    else textAt(84, 55, "say hi :)", DIM);
}

void drawInput() {
     
    tft.fillRect(38, 96, 164, 25, 0x2104);
    tft.fillRect(44, 121, 152, 1, ACCENT);
    const char* shown = input;
    constexpr std::size_t V = 24;
    if (input_len > V) shown += input_len - V;
    textAt(47, 105, input_len ? shown : "type a message...", input_len ? FG : DIM);
}

bool numericKeyboard = false;

void drawKeyboard() {
     
     
    tft.fillRect(18, 127, 204, 99, BG);
    textAt(36, 132, numericKeyboard ? "1  2  3  4  5  6  7  8  9  0" : "q  w  e  r  t  y  u  i  o  p", FG);
    textAt(45, 154, numericKeyboard ? "+  -  *  /  =  (  )  ?  !" : "a  s  d  f  g  h  j  k  l", FG);
    textAt(55, 176, numericKeyboard ? ":  ;  ,  .  %  &  @   <" : "z  x  c  v  b  n  m   <", FG);

    tft.fillRect(34, 197, 172, 1, ACCENT);
    textAt(43, 204, numericKeyboard ? "ABC  ?     SPACE     GO" : "123  ?     SPACE     GO", THINK);
}

void drawAll() {
    tft.fillRect(0, 0, 240, 240, BG);
    tft.setTextSize(1);

     
    drawConversation();
    drawInput();
    drawKeyboard();
}

void clearInput() {
    input_len = 0;
    input[0] = 0;
    drawInput();
}

void appendChar(char c) {
    if (input_len + 1 >= sizeof(input)) return;
    input[input_len++] = c;
    input[input_len] = 0;
    drawInput();
}

void backspace() {
    if (!input_len) return;
    input[--input_len] = 0;
    drawInput();
}

void submit(const char* line) {
    if (!line || !line[0] || thinking) return;
    String question(line);  
    strlcpy(last_user, question.c_str(), sizeof(last_user));
    last_reply[0] = 0;
    thinking = true;
    last_ok = true;
    drawConversation();
    clearInput();
    String reply;
    bool ok = askAI(question, reply);
    strlcpy(last_reply, reply.c_str(), sizeof(last_reply));
    thinking = false;
    last_ok = ok;
    Serial.printf("You: %s\n%s: %s\n", question.c_str(), useOllama ? "Ollama" : "Gemini", reply.c_str());
    drawAll();
}

void handleKey(int x, int y) {
    if (thinking) return;

    if (y >= 126 && y < 149) {
        if (x >= 20 && x < 220) {
            int i = (x - 20) / 20;
            if (i < 10) appendChar((numericKeyboard ? "1234567890" : "qwertyuiop")[i]);
        }
        return;
    }

    if (y >= 149 && y < 171) {
        if (x >= 30 && x < 210) {
            int i = (x - 30) / 20;
            if (i < 9) appendChar((numericKeyboard ? "+-*/=()?!" : "asdfghjkl")[i]);
        }
        return;
    }

    if (y >= 171 && y < 194) {
        if (x >= 176 && x < 220) backspace();
        else if (x >= 42 && x < 182) {
            int i = (x - 42) / 20;
            if (i < 7) appendChar((numericKeyboard ? ":;,.%&@" : "zxcvbnm")[i]);
        }
        return;
    }

     
     
    if (y >= 194 && y < 229) {
        if (x < 30 || x >= 215) return;
        if (x < 75) { numericKeyboard = !numericKeyboard; drawKeyboard(); }
        else if (x < 112) appendChar('?');
        else if (x < 166) appendChar(' ');
        else if (x < 215 && input_len) {
            char line[PROD_INPUT_BYTES] = {};
            std::strncpy(line, input, sizeof(line) - 1);
            submit(line);
        }
    }
}

void init() {
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);
    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
    sdaRelease();
    sclRelease();

    tft.init();  
    tft.setRotation(0);
    tft.setTextWrap(false);
    drawAll();
    Serial.println("[LCD CHAT] soft round UI ready");
}

void updateTouch() {
    std::uint8_t x = 0, y = 0;
    const bool down = readTouch(&x, &y);

    if (down && !touch_down && millis() - last_touch_ms >= 120) {
        last_touch_ms = millis();
        handleKey((int)x, (int)y);
    }
    touch_down = down;
}

}  