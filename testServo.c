#include <Servo.h>

// 宣告馬達
Servo myservo;

void setup() {
  Serial.begin(9600);
  // 讀取Pin腳, 5爪/9右馬達/10左馬達/11底座
  myservo.attach(5);
}

void loop() {
  // 從0~180度寫入測試最小角度與最大角度
  for(int j = 0; j <= 180; j++) {
    myservo.write(j);
    // 印出目前數值
    Serial.println(myservo.read());
    delay(150);
  }
}