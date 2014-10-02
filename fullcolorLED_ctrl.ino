//4小節(4拍*4 = 16拍)で変化させる。拍数変えれば良い感じ
//BPM 170なら、(60/170sec)で1拍分の長さとなり
//(60/170)* 16 = 5.6470

#include <Metro.h> //2つ以上のタイマ動作の実現
#define BEAT 16 //16拍

///////////////////////////////////
//pin assign (naming rule: first letter will set capital)
int Cathode_pin[3] = {
  13, 12, 11}; //左が上位、右が下位の温度7セグのカソード
int Segment_7LED[6] = {
  10, 8, 7, 4, 3, 2 }; //左からA,…,DPのダイオードに相当するピンを設定(f, DPは抜かしてある)
//pin assign of full color LED
int fulclr_R = 5;
int fulclr_G = 6;
int fulclr_B = 9;


///////////////////////////////////
//global variable declaration
volatile unsigned long delay_set_time; //volatileは最適化から外すとかそういうやつ
volatile int bpm;

//7セグの各表示{8つのダイオードを使っており、上位ビットからDP,G,F,E, D,C,B,Aのダイオードに相当する}
/* //original
unsigned char seg[10] = { 
  0x3f, 0x06, 0x5b, 0x4f, 0x66,  //0,1,2,3,4 
  0x6d, 0x7d, 0x27, 0x7f, 0x67 //5,6,7,8,9
};
*/

unsigned char seg[10] = { 
  0x1f, 0x10, 0x2b, 0x2f, 0x26,  //0,1,2,3,4 
  0x2d, 0x5d, 0x07, 0x7f, 0x47 //5,6,7,8,9
};//0x4d
//DP f g e?

///////////////////////////////////

///////////////////////////////////
//interruput timing declaration
Metro LEDchange_Metro = Metro( 22 ); //22/2
Metro dinamic_Metro = Metro(5); //interrput per 5ms(要調整)
Metro chattering_Metro = Metro(200); //chattering delete timing(200msは割といい感じ)
///////////////////////////////////


///////////////////////////////////
//setup
///////////////////////////////////
void setup() {
  Serial.begin(9600);	// 9600bpsでシリアルポートを開く(for debug
  
  pinMode( fulclr_R, OUTPUT );
  pinMode( fulclr_G, OUTPUT );
  pinMode( fulclr_B, OUTPUT );
  for( int i = 0; i < 6; i++ ) {
    pinMode( Segment_7LED[i], OUTPUT );
    if( i < 3 ) {
      pinMode( Cathode_pin[i], OUTPUT );
    }
  }
  
  bpm = 170; //initialized
}


///////////////////////////////////
//main loop
///////////////////////////////////
void loop() {
  //static int ReceiveData[3];
  float change_time;
  /* //don't use
  if ( Serial.available() >= 3 ) { // シリアルバッファに3byte受信された
    for( int i = 0; i < 3; i++ ) {
      ReceiveData[i] = Serial.read(); // 受信データを読み込む
    }
  
    bpm = ((int)ReceiveData[0]) * 100 + ((int)ReceiveData[1]) * 10 + (int)ReceiveData[2];
    
    //基本的に170に合わせるので変なデータは全て弾いて170に調整
    if( bpm < 150 || bpm > 200 ) bpm = 170;
    
    //BPMによりLED変化速度を変化させる
    change_time = ( 60 / bpm ) * BEAT; //5.64
    delay_set_time = 1000 * change_time / 256; //1000*5.64/256 = 22.031ms
  }
  */
  
  
  //LOWになったらボタンが押されているということ
  if( chattering_Metro.check() == 1 ) {
    if( analogRead(0) < 10 ) {
      bpm += 5; //
      if( bpm > 180 ) bpm = 170; //170 175 180と進んだら、次は170に戻る
    }
  }
  
  
  //BPMによりLED変化速度を変化させる
  change_time = (float)( ( BEAT * 60.0 ) / bpm); //bpm170:5.64, bpm175:5.48, bpm180:5.33
  delay_set_time = (int)(1000.0 * change_time / 256.0); //bpm170:1000*5.64/256 = 22.031ms, bpm175:21.406ms, bpm180:20.82ms

  //bpm175, bpm状態に応じてintervalを変化
  if( delay_set_time == 21 ) {
    LEDchange_Metro.interval(11); //21/2
  }
  else if( delay_set_time == 20 ) { //bpm180
    LEDchange_Metro.interval(10); //20/2
  }
  else { //bpm170
    LEDchange_Metro.interval(12); //22/2 +1
  }
  
  //interrupt timing settings
  if ( LEDchange_Metro.check() == 1 ) { //interrupt of fulclrLED
    LED_bright_set();
  }
  if( dinamic_Metro.check() == 1 ) { //ダイナミック点灯割り込み
    segment_lightning(); //7セグ点灯
  }
  
}


///////////////////////////////////
//tape LED control
///////////////////////////////////
//256*20ms程度 = 5秒ぐらいで切り替わる
//別の関数に飛ばそうとしたら最適化の都合か上手く動作しなかったので汚いfuncになっている
void LED_bright_set() {
  static int called_counter = 0; //staticにして、再呼び出しでも変えないように
  
  static int up_counter = 0, down_counter = 255;
  if( called_counter < 256 ) {
    //case:R brightness
    analogWrite( fulclr_R, up_counter );
    analogWrite( fulclr_G, 0 );
    analogWrite( fulclr_B, down_counter );
  }
  else if( called_counter < 512 ) {
    //G brightness
     analogWrite( fulclr_R, down_counter );
     analogWrite( fulclr_G, up_counter );
     analogWrite( fulclr_B, 0 );
  }
  else if( called_counter < 768 ) {
    //B brightness
    analogWrite( fulclr_R, 0 );
    analogWrite( fulclr_G, down_counter );
    analogWrite( fulclr_B, up_counter );
  }
  
  //処理をpattern1に戻す
  ++called_counter;
  if( called_counter >= 768 ) {
    called_counter = 0;
  }
  
  //counter inc or dec and initialize
  if( up_counter < 255 && down_counter > 0 ) {
     up_counter++; 
     down_counter--;
  }
  else {
    up_counter = 0;
    down_counter = 255;
  }
}



///////////////////////////////////
//4桁7セグメントLEDのダイナミック点灯
///////////////////////////////////
void segment_lightning() {
  static unsigned char sel = 0; //sel 桁を選択するため 関数が終了しても状態を消さないため static
  unsigned char dig1, dig10, dig100; //温度7セグそれぞれの桁への対応

  dig1 = seg[ bpm % 10 ]; //4ケタの一番下の桁
  dig10 = seg[ ( bpm / 10) % 10 ]; //4ケタの右から2番目の桁
  dig100 = seg[ ( bpm / 100) % 10 ]; //4ケタの右から3番目 %10はなくてもいい
  
  switch( sel ){
  case 0: 
    digitalWrite( Cathode_pin[2], HIGH ); //LSBの7セグLEDを点灯
    digitalWrite( Cathode_pin[0], LOW );
    digitalWrite( Cathode_pin[1], LOW );
    for( int i = 0; i < 6; i++ ) { //DPはLOW
      if( bitRead( dig1, i ) ) { //i桁目がHIGHかLOWかを判定
        digitalWrite( Segment_7LED[i], HIGH ); //ONならHIGH
      }
      else {
        digitalWrite( Segment_7LED[i], LOW );
      }
    }
    break;

  case 1: 
    digitalWrite( Cathode_pin[1], HIGH );
    digitalWrite( Cathode_pin[0], LOW );
    digitalWrite( Cathode_pin[2], LOW );
    for( int i = 0; i < 6; i++ ) {
      if( bitRead( dig10, i ) ) { //i桁目がHIGHかLOWかを判定
        digitalWrite( Segment_7LED[i], HIGH ); //ONならHIGH
      }
      else { 
        digitalWrite( Segment_7LED[i], LOW );
      }
    }
    break;

  case 2: 
    digitalWrite( Cathode_pin[0], HIGH );
    digitalWrite( Cathode_pin[1], LOW );
    digitalWrite( Cathode_pin[2], LOW );
    for( int i = 0; i < 6; i++ ) {
      if( bitRead( dig100, i ) ) { //i桁目がHIGHかLOWかを判定
        digitalWrite( Segment_7LED[i], HIGH ); //ONならHIGH
      }
      else { 
        digitalWrite( Segment_7LED[i], LOW );
      }
    }
    break;

  } //end switch
  sel++;
  if( sel > 2 ) sel = 0; //
}
