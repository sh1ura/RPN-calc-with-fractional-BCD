#define DEBUG 0 // send data to serial port 
#if !DEBUG
#define printStackSerial(a) 
#endif

//////
// ST7032 LCD (AQM1602Y-NLW-FBW)
//////

#include <ST7032_asukiaaa.h>

ST7032_asukiaaa lcd;
#define I2C_SDA 12
#define I2C_SCL 13

//////
// 文字列で表現した非負整数のBCD計算
//////

#define VAL(a) ((a) - '0')

// 先行する 0 を除去する
String iNorm(String a) {
  while(a.length() > 1) {
    if(a.charAt(0) != '0') {
      break;
    }
    a = a.substring(1);
  }
  return a;
}

// 値の大小比較
int iComp(String a, String b) {
  a = iNorm(a);
  b = iNorm(b);
  int sub = (int)a.length() - (int)b.length();
  if(sub != 0) {
    return sub;
  }
  return a.compareTo(b);
}

// 非負の整数の和を計算
String iAdd(String a, String b) {
  int len = (int)a.length() - (int)b.length();
  int i, sum, carry = 0;
  String c = "";
  
  if(len > 0) { // 文字列の長さを揃える
    for(i = 0; i < len; i++) {
      b = "0" + b;
    }
  }
  else if(len < 0) {
    for(i = 0; i < -len; i++) {
      a = "0" + a;
    }
  }
  len = a.length();
  for(i = len-1; i >= 0; i--) {
    int sum = VAL(a.charAt(i)) + VAL(b.charAt(i)) + carry;
    if(sum > 9) {
      sum -= 10;
      carry = 1; // 繰り上がり
    }
    else {
      carry = 0;
    }
    c = String(sum) + c;
  }
  if(carry)
    c = "1" + c;
  return iNorm(c);
}

// 差の計算 a >= b でなければならない
String iSub(String a, String b) {
  int len = (int)a.length() - (int)b.length();
  int i, sub, borrow = 0;
  String c = "";
  
  if(len > 0) { // 文字列の長さを揃える
    for(i = 0; i < len; i++) {
      b = "0" + b;
    }
  }
  else if(len < 0) {
    for(i = 0; i < -len; i++) {
      a = "0" + a;
    }
  }
  len = a.length();
  for(i = len-1; i >= 0; i--) {
    int sub = VAL(a.charAt(i)) - VAL(b.charAt(i)) - borrow;
    if(sub < 0) {
      sub += 10;
      borrow = 1; // 繰り下がり
    }
    else {
      borrow = 0;
    }
    c = String(sub) + c;
  }
  return iNorm(c);
}

// 積の計算
String iMul(String a, String b) {
  String c, amul;
  int i, j;

  c = "0";
  amul = a;
  for(j = b.length() - 1; j >= 0; j--) {
    for(i = 0; i < VAL(b.charAt(j)); i++) {
      c = iAdd(c, amul);
    }
    amul = amul + "0"; // 左シフト
  }
  return iNorm(c);
}

// 整数の除算
String rem; // 剰余を格納
String iDiv(String a, String b) {
  String mult;
  String x, ans = "0";
  int i, j;

  if(iComp(b, "0") == 0) {
    return "0";
  }
  rem = "";
  while(a.endsWith("0") && b.endsWith("0")) { // 計算時間とスタックの節約
    a = a.substring(0, a.length() - 1);
    b = b.substring(0, b.length() - 1);
    rem += "0";
  }
  x = b;
  for(i = 0; ; i++) {
    x = x + "0";
    if(iComp(a, x) < 0) {
      break;
    }
  }
  for(; i >= 0; i--) {
    if(iComp(a, "0") == 0) {
      break;
    }
    mult = "1";
    x = b;
    for(j = 0; j < i; j++) {
      mult = mult + "0";
      x = x + "0";
    }
    for(j = 0; j < 10; j++) {
      if(iComp(a, x) >= 0) {
        a = iSub(a, x);
        ans = iAdd(ans, mult);
      }
      else {
        break;
      }
    }
  }
  rem = iNorm(a + rem);
  return iNorm(ans);
}

// 剰余を読み出す関数 iDiv の実行後似呼び出し
String iRem() {
  return rem;
}

// 最大公約数の計算（互除法） a, b は 0 不可
String iGCD(String a, String b) {
  String c, r;

  if(iComp(a, b) < 0) {
    c = b;
    b = a;
    a = c;
  }
  for(;;) {
    iDiv(a, b);
    r = iRem();
    if(iComp(r, "0") == 0) {
      return b;
    }
    a = b;
    b = r;
  }
}
 
//////
// 分数計算部
//////

// 分数の構造体定義　分母分子の非負整数と符号で構成
struct FRAC {
  String u, b;
  int sign;
};

// 小数を表す文字列を分数に変換
struct FRAC str2FRAC(String s) {
  struct FRAC a;
  int ptPos, mul, i;

  if(s.length() == 0) {
    s = "0";
  }
  a.sign = 1;
  if(s.charAt(0) == '-') {
    a.sign = -1;
    s = s.substring(1);
  }
  ptPos = s.indexOf(".");
  if(ptPos >= 0) {
    int mul = (int)s.length() - ptPos - 1;
    a.u = s.substring(0, ptPos) + s.substring(ptPos + 1);
    if(a.u.length() == 0) {
      a.u = "0";
    }
    a.b = "1";
    for(i = 0; i < mul; i++) {
      a.b = a.b + "0";
    }
  }
  else {
    a.u = s;
    a.b = "1";
  }
  return a;
}

// 符号の反転
struct FRAC fSign(struct FRAC a) {
  a.sign = -a.sign;
  return a;
}

// 約分
struct FRAC fReduce(struct FRAC a) {
  String gcd;

  if(iComp(a.u, "0") == 0 || iComp(a.b, "0") == 0) {
    return a;
  }
  gcd = iGCD(a.u, a.b);
  if(iComp(gcd, "1") == 0) {
    return a;
  }
  a.u = iDiv(a.u, gcd);
  a.b = iDiv(a.b, gcd);
  return a;
}

// 和
struct FRAC fAdd(struct FRAC a, struct FRAC b) {
  struct FRAC c;

  c.b = iMul(a.b, b.b);
  a.u = iMul(a.u, b.b);
  b.u = iMul(b.u, a.b);
  if(a.sign == b.sign) {
    c.u = iAdd(a.u, b.u);
    c.sign = a.sign;
  }
  else {
    if(iComp(a.u, b.u) > 0) {
      c.u = iSub(a.u, b.u);
      c.sign = a.sign;
    }
    else {
      c.u = iSub(b.u, a.u);
      c.sign = b.sign;
    }
  }
  return fReduce(c);
}

// 差
struct FRAC fSub(struct FRAC a, struct FRAC b) {
  return fReduce(fAdd(a, fSign(b)));
}

// 積
struct FRAC fMul(struct FRAC a, struct FRAC b) {
  struct FRAC c;

  c.sign = a.sign * b.sign;
  c.u = iMul(a.u, b.u);
  if(iComp(c.u, "0") == 0) {
    c.b = "1";
    c.sign = 1;
    return c;
  }
  c.b = iMul(a.b, b.b);
  return fReduce(c);
}

// 商
struct FRAC fDiv(struct FRAC a, struct FRAC b) {
  struct FRAC c;

  c.sign = a.sign * b.sign;
  c.u = iMul(a.u, b.b);
  c.b = iMul(a.b, b.u);
  if(iComp(c.b, "0") == 0) {
    c.u = "0"; // NaN
  }
  return fReduce(c);
}

// 分数から float の値を求める（現在は未使用）
float fNum(struct FRAC a) {
  float u = 0, b = 0;
  int i;

  for(i = 0; i < a.u.length(); i++) {
    u = u * 10.0 + VAL(a.u.charAt(i));
  }
  for(i = 0; i < a.b.length(); i++) {
    b = b * 10.0 + VAL(a.b.charAt(i));
  }
  u /= b;
  u *= a.sign;
  return u;
}

//////
// スタック周り
//////

#define STACK_DEPTH 16
struct STACK {
  struct FRAC data[STACK_DEPTH];
} stack;

void push(struct FRAC val) {
  for(int i = STACK_DEPTH-1; i > 0; i--) {
    stack.data[i] = stack.data[i-1];
  }
  stack.data[0] = val;
  printStackSerial("push");
}

struct FRAC pop(void) {
  int i;
  struct FRAC val;

  val = stack.data[0];
  for(int i = 0; i < STACK_DEPTH-1; i++) {
    stack.data[i] = stack.data[i+1];
  }
  // スタックの底から 0 が出てくる方式
  stack.data[STACK_DEPTH-1] = str2FRAC("0");
  return val;
}

void initStack(void) {
  for(int i = 0; i < STACK_DEPTH; i++) {
    stack.data[i] = str2FRAC("0");
  }
}

//////
// Undo まわり
//////

#define UNDO_DEPTH 32
int undoTop = 0;
struct STACK undoBuf[UNDO_DEPTH];

void backup(void) {
  if(undoTop >= UNDO_DEPTH) {
    for(int i = 0; i < UNDO_DEPTH - 1; i++) {
      undoBuf[i] = undoBuf[i + 1];
    }
    undoTop --;
  }
  undoBuf[undoTop] = stack;
  undoTop++;
}

void restore(void) {
  if(undoTop == 0) // no undo data
    return;
  undoTop--;
  stack = undoBuf[undoTop];
}

//////
// キースキャン（スライドスイッチ含む）
//////

#define K_OUT_N 2 // キースキャンの出力ポート数
#define K_IN_N 11 // キースキャンの入力ポート数

int KeyPinOut[K_OUT_N] = {28, 27}; // 出力ポートのリスト
int KeyPinIn[K_IN_N] = {1, 2, 3, 4, 5, 6, 7, 8, 14, 15, 26}; // 入力ポートのリスト

// キーマップ表
// 小文字 : スライドスイッチ
// その他 : 押しボタン
char keyMap[K_OUT_N][K_IN_N] = {
  {'8', '9', '7', '0', '5', '3', '6', '4', ' ', '2', '1' },
  {'>', '.', '/', ' ', '-', '=', '*', '+', 'a', ' ', 'C' }
};

#define SW_N 1 // スライドスイッチの数
int swStat[SW_N]; // スライドスイッチの状態　'a' --> swStat[0], 'b' --> swStat[1], ... 

char keyCheck(void) {
  char c = '\0';
   for(int i = 0; i < SW_N; i++) { // スイッチ状態を一旦リセット
    swStat[i] = 0;
  }
  for(int o = 0; o < K_OUT_N; o++) {
    for(int i = 0; i < K_OUT_N; i++) {
      digitalWrite(KeyPinOut[i], HIGH);
    }
    digitalWrite(KeyPinOut[o], LOW);
    for(int i = 0; i < K_IN_N; i++) {
      if(digitalRead(KeyPinIn[i]) == LOW) {
        char c2 = keyMap[o][i];
        if('a' <= c2 && c2 <= 'z') { // スライドスイッチの場合
          swStat[c2 - 'a'] = 1;
        }
        else {
          c = c2;
        }
      }
    }
  }
  return c;
}

void waitRelease(int timeout) { // キーを離すまで待つ
  unsigned long time = millis();
  delay(30);
  while(keyCheck()) {
    if(millis() > time + timeout) {
      break; // タイムアウト
    }
    delay(30);
  }
}

// キー入力をつなげて文字列にする
boolean entering = false; // 文字入力中フラグ
boolean point = false; // 小数点を打ったかどうか
String ent; // 入力中の文字
void addNum(char key) {
  if(!entering) {
    ent = "";
    entering = true;
  }
  if(key == '.') {
    if(point) return; // 小数点は１個まで
    point = true;
  }
  ent = ent + String(key);
}

// 入力文字を確定してスタックに入れる
void enter(void) {
  if(entering) {
    entering = false;
    point = false;
    push(str2FRAC(ent));
    ent = "";
  }
}

// 整数を桁数制限 limit 内で E 表示する
String dispE(String s, int limit) {
  if(s.length() <= limit) {
    return s;
  }
  limit -= 2; // "En" のスペース
  if((int)s.length() - 9 > limit) // ...E10 みたいに２桁になる
    limit--;
  s = s.substring(0, limit) + "e" + String((int)s.length() - limit);
#if DEBUG
  Serial.println("dispE : limit = " + String(limit) + " : " + s);
#endif
  return s;
}

#define DIGIT_LIM 16 // 表示桁数制限（表示デバイスとフォントサイズ依存）

// 分数の表示用文字列生成
String FRAC2fstr(struct FRAC a) {
  int lim = DIGIT_LIM;
  int reduce = false;
  String s = "";

  if(iComp(a.b, "0") == 0) { // 分母が 0 は NaN と表示
    return "NaN";
  }
  if(a.sign == -1) {
    lim--; // 負号 '-' のスペースを確保しておく
  }
  int blimit = lim - 1 - a.u.length();
  if(blimit < lim / 2 - 1) // 分母に約半分の文字数を確保
    blimit = lim / 2 - 1;
  s = "/" + dispE(a.b, blimit);
  lim -= (int)s.length(); // 分子に使用できる桁数
  s = dispE(a.u, lim) + s;

  if(a.sign < 0) { // 負号をつける
    s = "-" + s;
  }
  return s;
}

// 小数点以下の '0' や末尾の '.' を取り除く
String trimZero(String s) {
  if(s.indexOf(".") < 0) { // 小数点がない場合は処理しない
    return s;
  }
  while(s.length() > 1) {
    if(s.endsWith("0")) { // 小数点以下の '0' を削除
      s = s.substring(0, (int)s.length() - 1);
    }
    else if(s.endsWith(".")) { // 末尾の '' を削除
      s = s.substring(0, (int)s.length() - 1);
      break; // 小数点より前は処理しない
    }
    else {
      break;
    }
  }
  return s;
}

// 小数の表示用文字列生成関数　DIGIT_LIM桁に収めて表示
String FRAC2dstr(struct FRAC a) {
  int limit = DIGIT_LIM;
  String u = a.u, b = a.b, s;
  int slide = 0, lendiff, i, ptPos;

  if(iComp(b, "0") == 0) { // 分母が 0 は NaN と表示
    return "NaN";
  }
  if(iComp(u, "0") == 0) {
    return "0";
  }
  if(a.sign < 0) {
    limit--; // 負号 '-' のスペースを確保しておく
  }
  // n.xxxx の形式を前提とするためにまず桁数を揃える
  lendiff = (int)u.length() - (int)b.length();
  if(lendiff > 0) { // 分子が長い
     for(i = 0; i < lendiff; i++) {
        b = b + "0";
        slide++;
    }
  }
  else if(lendiff < 0) { //分母が長い
    for(i = 0; i < -lendiff; i++) {
      u = u + "0";
      slide--;
    }
  }
  if(iComp(u, b) < 0) { // 分子の値が小さいと 0.xxx になってしまうため
    u = u + "0"; // １桁ずらして n.xxxxの形式にする
    slide--;
  }

  // 各桁の数値を求める
  for(i = 0; i <  DIGIT_LIM - 1; i++) { // 分子を大きくする（整数除算のため）
    u = u + "0";
    slide--;
  }
  s = iDiv(u, b); // 分母分子を除算して数字の並びを求める
  ptPos = slide + (int)s.length();// 小数点の位置（文字列の先頭からの位置）
      
  // フォーマッティング
  if(0 < ptPos && ptPos < limit) { // 小数点が表示桁内に置ける場合
    u = s.substring(0, ptPos); // 小数点より前の文字列
    b = s.substring(ptPos); // 小数点より後ろの文字列
    s = u + "." + b;
    s = trimZero(s); // 小数点より後ろの 0 を取り除く
    s = s.substring(0, limit);
  }
  else if(ptPos > limit || ptPos < -limit/3) { // 絶対値が非常に大きい・小さい場合
    // x.xxxEyy の形式で表示する
    s = s.substring(0, 1) + "." + s.substring(1);
    s = trimZero(s);
    if(ptPos > 0) {
        s = s.substring(0, limit - 3); // "Eyy" の文字数を確保する
    }
    else {
        s = s.substring(0, limit - 4); // "E-yy" の文字数を確保する
    }
    s += "e" + String(ptPos - 1);
  }
  else if(ptPos == limit) { // ちょうど小数点がない場合
    s = s.substring(0, limit);
  }
  else { // 0.0000xxx の形式の場合
    for(i = 0; i < -ptPos; i++) {
        s = "0" + s;
    }
    s = "0." + s;
    s = trimZero(s);
    s = s.substring(0, limit);
  }
  if(a.sign < 0) s = "-" + s; // 負号をつける
  return s;
}

boolean fracmode = false; // 分数表示モードフラグ
String FRAC2str(struct FRAC a) { // 切り替え表示
  if(fracmode) {
    return FRAC2fstr(a);
  }
  else {
    return FRAC2dstr(a);
  }
}

#if DEBUG
void printStackSerial(String s) {
  Serial.println(s);
  Serial.println((stack.data[1].sign < 0 ? "-" : " ") + stack.data[1].u + "/" + stack.data[1].b + " = " + FRAC2dstr(stack.data[1]));
  Serial.println((stack.data[0].sign < 0 ? "-" : " ") + stack.data[0].u + "/" + stack.data[0].b + " = " + FRAC2dstr(stack.data[0]));
  Serial.println("----");
}
#endif

// FRAC2dstr() が遅いので，演算後に表示文字列を作っておく
String dispStr[2];
void refreshStr(void) {
  dispStr[0] = FRAC2str(stack.data[0]);
  dispStr[1] = FRAC2str(stack.data[1]);
}

void print(String s) {
  for(int i = 0; i < s.length(); i++) { // 0 の斜線は不要なので、O に置き換える
    if(s[i] == '0')
      s[i] = 'O';
  }
  lcd.print(s);
}

// 数値表示
void disp(void) {
  String s;
  String clear = "                ";
    lcd.setCursor(0, 0);
    lcd.print(clear);
    lcd.setCursor(0, 1);
    lcd.print(clear);

  if(entering) { // 数値の入力中
    s = ent;
    if(s.length() > DIGIT_LIM) { // 長過ぎるとき，文字の上位桁を省略表示
      s = ".." + s.substring((int)s.length() - DIGIT_LIM + 2);
    }
    lcd.setCursor(16 - dispStr[0].length(), 0);
    print(dispStr[0]);
    lcd.setCursor(0, 1);
    print(s);
  }
  else {  
    printStackSerial("disp");
    lcd.setCursor(16 - dispStr[1].length(), 0);
    print(dispStr[1]);
    lcd.setCursor(16 - dispStr[0].length(), 1);
    print(dispStr[0]);
  }
}

void setup() {
  // LCD 初期化
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  lcd.begin(16, 2); // 列数、行数
  lcd.setContrast(16);

  // キースキャン初期化
  for(int i = 0; i < K_OUT_N; i++) {
    pinMode(KeyPinOut[i], OUTPUT);
    digitalWrite(KeyPinOut[i], HIGH);
  }
  for(int i = 0; i < K_IN_N; i++) {
    pinMode(KeyPinIn[i], INPUT_PULLUP);
  }

  initStack();
  refreshStr();
  disp();
#if DEBUG
  Serial.begin( 9600 );
  Serial.println( "Start" );
#endif
}

// 長押しの時間 ミリ秒
#define LONGPRESS 500

// メインループ
void loop(void) {
  struct FRAC a;
  int longpress = false;
  static int sw = 0;
  
  char key = keyCheck();
  if(!key && sw == swStat[0])
    return;
  sw = swStat[0];
  if(!entering) { // 長押し検出するのは入力モード以外
    if(key == '=' || key == '-' || key == '*' || key == '/') {
      // 長押し処理をするボタン
      unsigned long time = millis();
      waitRelease(LONGPRESS);
      if(millis() > time + LONGPRESS) {
        longpress = true;
      }
    }
  }
#if DEBUG
  char keystr[] = "key[ ]";
  keystr[4] = key;
  Serial.println(keystr);
#endif
  switch(key) {
  case '0':  
  case '1':  
  case '2':  
  case '3':  
  case '4':  
  case '5':  
  case '6':  
  case '7':  
  case '8':  
  case '9':  
  case '.':
    addNum(key); // 新しい数値入力
    break;
  case '=': // ENTER キー
    backup();
    if(entering) {
      enter();
    }
    else { // 確定済みのときは DUP（複製）
      a = pop();
      push(a);
      push(a);  
    }
    break;
  case 'C': // 確定してDROP（削除）
    if(entering) { //　入力中は１文字削除
      if(ent.length() > 1) {
        ent = ent.substring(0, ent.length() - 1);
      }
      else {
        ent = "";
        entering = false;
      }
    }
    else {
      backup();
      enter();
      pop();
    }
    break;
  case '+':
    backup();
    enter();
    a = pop();
    stack.data[0] = fAdd(stack.data[0], a);
    break;
  case '-': // 引き算または負号反転
    backup();
    enter();
    if(!longpress) {
      a = pop();
      stack.data[0] = fSub(stack.data[0], a);
      break;
    } // 長押しのときは符号反転
    enter();
    stack.data[0] = fSign(stack.data[0]);
    break;
  case '*': // 掛け算
    backup();
    enter();
    a = pop();
    stack.data[0] = fMul(stack.data[0], a);
    break;
  case '/': // 割り算または SWAP
    backup();
    enter();
    if(!longpress) {
      a = pop();
      stack.data[0] = fDiv(stack.data[0], a);
      break;
    } // 長押しのときは SWAP するので breakしない
  case '%': // SWAP (% キーがある場合)
    backup();
    enter();
    a = stack.data[0];
    stack.data[0] = stack.data[1];
    stack.data[1] = a;
    break;
  case '>': // undo
    if(entering) {
      backup();
      enter();
    }
    restore();
    break;
  }
  fracmode = !sw; // 小数表示モード
  // 小数値を再計算
  if(!entering) {
    refreshStr();
  }
  disp();
  waitRelease(10000); // 事実上離すまで待ち続ける
  delay(100);
}
