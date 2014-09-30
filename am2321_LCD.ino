#include <Wire.h> //I2Cのための初期化
#include <AM2321.h>
#include <Metro.h> //2つ以上のタイマ動作の実現
#include <LiquidCrystal.h>
#include <KanaLiquidCrystal.h> //半角カタカナの入力が出来るlcdライブラリ

//不快指数表示器
//Arduinoをマスタ、センサをスレーブとして扱う
//Use AM2321(温湿度センサ), LCD
//LCDのピンアサインに気をつけましょう http://9ensan.com/blog/diy/arduino-lcd-character-display/
//不快指数: http://ja.wikipedia.org/wiki/%E4%B8%8D%E5%BF%AB%E6%8C%87%E6%95%B0
//半角仮名表示：http://www3.big.or.jp/~schaft/hardware/KanaLiquidCrystal/page001.html
//文字列操作部分(dtostrf)：http://garretlab.web.fc2.com/arduino/reverse_lookup/index.html
//09/08/2014~09/17/2014
//Author: Hirotaka Takayama

///////////////////////////////////
//pin assign
int Blue_pin = 6;
int Yellow_pin = 5; 
int Red_pin = 4; 

KanaLiquidCrystal lcd( 8, 9, 10, 11, 12, 13 ); 
///////////////////////////////////


///////////////////////////////////
//global variable declaration
//メイン側と割り込み関数側で共用する変数にはvolatile属性をつける
volatile float humidity_main, temperature_main;
///////////////////////////////////

///////////////////////////////////
//interruput declaration
Metro i2c_Metro = Metro(2000); //interruput per 2000ms, これを変えればセンサ値取得タイミングを変えられる
///////////////////////////////////


void setup() {
//  Serial.begin(9600) ;      // パソコンとシリアル通信の準備を行う
  Wire.begin(); // ArduinoをI2C マスターとして初期化
  pinMode( Blue_pin, OUTPUT);
  pinMode( Yellow_pin, OUTPUT);
  pinMode( Red_pin, OUTPUT);
  
  lcd.kanaOn(); // この行を実行して以降、半角カナは文字化けせずに表示される
  lcd.begin( 16, 2 ); //cols, rows(LCD size指定)
}

///////////////////////////////////
///////////////////////////////////
//main loop
///////////////////////////////////
///////////////////////////////////
void loop() {
  static float temp_hum_idx = 0.0;
  temp_hum_idx = ( 0.81 * temperature_main + 0.01 * humidity_main * (0.99 * temperature_main - 14.3 ) + 46.3 ); //formula of temperature-humidity index
  led_lightning( temp_hum_idx );
  
//interrupt of AM2321
  if ( i2c_Metro.check() == 1 ) { //i2c通信割り込み
    i2c_communication();
    lcd_disp( temp_hum_idx ); //lcd情報更新
  }
  
}
///////////////////////////////////
///////////////////////////////////


///////////////////////////////////
//AM2321(温湿度センサ)とのI2C通信
///////////////////////////////////
void i2c_communication() {
  AM2321 am2321;
    am2321.read();
/*
    Serial.print("(");
    Serial.print(am2321.temperature/10.0);
    Serial.print(", ");
    Serial.print(am2321.humidity/10.0);
    Serial.println(')');
    */
    humidity_main = am2321.humidity/10.0;
    temperature_main = am2321.temperature/10.0;
}


///////////////////////////////////
//LCDへの文字表示通信
///////////////////////////////////
void lcd_disp( float temp_hum_idx ) {
  char temp_fixed[32], hum_fixed[32], temp_hum_idx_fixed[32];
  dtostrf( temperature_main, 4, 1, temp_fixed ); //double to string format. 変換する値,変換後の文字数,小数点以下の桁数,変換後
  dtostrf( humidity_main, 4, 1, hum_fixed );
  dtostrf( temp_hum_idx, 4, 1, temp_hum_idx_fixed );
  
  lcd.setCursor( 0, 0 ); //col, row
  lcd.print( temp_fixed );
  lcd.print("Td");
  lcd.setCursor( 8, 0 ); //col, row
  lcd.print( hum_fixed );
  lcd.print( "%" );
  lcd.setCursor( 0, 1 ); //col, row
  lcd.print( "ﾌｶｲｼｽｳ:" );
  lcd.print( temp_hum_idx_fixed );
  
}


///////////////////////////////////
//不快指数値に応じてLEDを制御
///////////////////////////////////
void led_lightning( float temp_hum_idx ) {
  if( temp_hum_idx >= 80.0 ) { //red(state f*ckin hot)
    digitalWrite( Red_pin, HIGH );
    digitalWrite( Blue_pin, LOW );
    digitalWrite( Yellow_pin, LOW );
  }
  else if( temp_hum_idx >= 75.0 ) { //yellow(state hot)
    digitalWrite( Yellow_pin, HIGH );
    digitalWrite( Blue_pin, LOW );
    digitalWrite( Red_pin, LOW );
  }
  else { //Blue
    digitalWrite( Blue_pin, HIGH );
    digitalWrite( Yellow_pin, LOW );
    digitalWrite( Red_pin, LOW );
  }
}

