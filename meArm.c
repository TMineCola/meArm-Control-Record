/*
    Version: v1.0 (2018/5/)
    Author: MineCola / NCNU_MOLi
    Devices:
        Arduino UNO
        meArm with four SG90 servo
        JoyStick shield
    PINS:
        Using JoyStick shield pins
            Analog (0-1023)
                A0: Left X
                A1: Left Y
                A1: Right Y
                A2: Right X
            servo: 11/10/9/5 = base/left/right/craw
        led: 3 (on JoyStick shield)
        left press: 2
        right press: 4
*/

#include <Servo.h>
// 初始化參數

// 總共有幾顆伺服馬達
const int SERVOS = 4;
// 搖桿至少扳動多少才開始動作
const int sensityOffset = 50;
// 設定讀取間隔時間(毫秒)
const int delayTime = 20;
// 搖桿未使用狀態計數器達多少次後, 將馬達斷線
const int idleTime = 100;
// 顯示狀態的Led pin腳
const int ledPin = 3;
// 設定按鈕 pin腳
const int buttonL = 2, buttonR = 4;

// 最大記憶動作數
const int maxMemory = 100;
// 記憶播放速度 1x 2x 3x
int playSpeed = 1;
int maxSpeed = 10;

int servoConfig[SERVOS][5] = {
    //JoystickPin, servoPin, minAngle, maxAngle, initialAngle
    {0, 11, 0, 180, 90}, //A0 L.X Base
    {1, 9, 60, 160, 90}, //A1 L.Y Right
    {2, 10, 40, 120, 40}, //A2 R.Y Left
    {3, 5, 0, 60, 60} //A3 R.X Claw
};

Servo myservo[SERVOS];
int armMode = 0; // 預設0為控制模式, 1為錄製模式, 2為播放模式
int actionCounter = 0; // 記憶目前動作數目
int idle[SERVOS]; // 閒置計數器
int actionMemory[maxMemory][SERVOS]; //記憶角度
int buttonPreState = 0;

void setup() {
    Serial.begin(9600);
    // 初始化針腳模式 (Button按下為0, 不按為1)
    pinMode(buttonL, INPUT_PULLUP);
    pinMode(buttonR, INPUT_PULLUP);
    // 馬達初始化
    for(int i = 0; i < SERVOS; i++) {
        myservo[i].attach(servoConfig[i][1]);
        myservo[i].write(servoConfig[i][4]);
        // 初始化閒置時間
        idle[i] = 0;
    }

    if(!digitalRead(buttonL)) {
        Serial.println("Change to Record Mode");
        armMode = 1;
        cutcut();
        cutcut();
    } else {
        cutcut();
        // 指示燈恆亮
        digitalWrite(ledPin, HIGH);
    }

    // 顯示完成初始化
    Serial.println("Initialized");
}

void loop() {
    if(armMode != 2) {
        // 播放模式無法控制外, 其餘模式都可以操作
        move_controller();
    }

    if(armMode != 0) {
        armActionMemory();
    }
}

// 控制夾子
void claw(boolean mode) {
    // 未連接則先連接
    if(!myservo[3].attached()) {
        myservo[3].attach(servoConfig[3][1]);
    }
    // 夾子開關 (0:open, 1:close)
    if(!mode) {
        myservo[3].write(servoConfig[3][3]);
    } else {
        myservo[3].write(servoConfig[3][2]);
    }
}
// 夾夾指示 (連續開合兩次)
void cutcut(){
    delay(500);
    for(int i = 0; i < 2; i++) {
        claw(true);
        delay(150);
        claw(false);
        delay(150);
    }
}

// 手臂移動
void move_controller() {
    boolean shouldMove = false;
    int value[SERVOS];
    int currentAngle[SERVOS];

    for(int i = 0; i < SERVOS; i++) {
        // 讀取搖桿數值與當前角度
        value[i] = analogRead(servoConfig[i][0]);
        currentAngle[i] = myservo[i].read();

        // 當搖桿向右或向下移動
        if(value[i] > (512 + sensityOffset) ) {
            if(i != SERVOS - 1 && currentAngle[i] > servoConfig[i][2]) {
                currentAngle[i] -= 1 + ((value[i] > (1023 - sensityOffset/2)) ? 1 : 0);
                shouldMove = true;
                idle[i] = 0;
            } else if(i == SERVOS - 1) {
                claw(0);
                idle[i] = 0;
            }
        // 當搖桿向左或向上移動
        } else if(value[i] < (512 - sensityOffset)) {
            if(i != SERVOS - 1 && currentAngle[i] < servoConfig[i][3]) {
                currentAngle[i] += 1 + ((value[i] < sensityOffset/2) ? 1 : 0);
                shouldMove = true;
                idle[i] = 0;
            } else if(i == SERVOS - 1) {
                claw(1);
                idle[i] = 0;
            }
        } else {
            // 閒置狀態
            idle[i]++;
        }

        if(idle[i] >= idleTime) {
            myservo[i].detach();
            idle[i] = 0;
        }
    }

    if(shouldMove) {
        for(int i = 0; i < SERVOS; i++) {
            if(!myservo[i].attached()) {
                myservo[i].attach(servoConfig[i][1]);
            }
            myservo[i].write(currentAngle[i]);
        }
    }
    delay(delayTime);
}

// 錄製動作
void armActionMemory() {
    if(armMode == 1) {
        if(!digitalRead(buttonR)) {
            armMode = 2;
            Serial.println("Change to play mode by button");
            return;
        }
        int buttonPressed = !digitalRead(buttonL);
        // 重複讀取按則 return
        if(buttonPreState && buttonPressed) {
            return;
        } else if(!buttonPreState && buttonPressed) {
            buttonPreState = true;
            if(actionCounter < maxMemory) {
                for(int i = 0; i < SERVOS; i++) {
                    actionMemory[actionCounter][i] = myservo[i].read();
                }
                actionCounter++;
                Serial.println("Saved Action!");
                if(actionMemory == maxMemory) {
                    armMode = 2;
                    Serial.println("Change to play mode by full memory");
                }
            }
        } else {
            buttonPreState = buttonPressed;
        }
        // 記錄同時按下播放
        if(!digitalRead(buttonR)) {
            armMode = 2;
            Serial.println("Change to play mode by button");
        }
    } else {
        //播放模式
        autoPlay();
    }
}

// 播放模式
void autoPlay() {
    // 未連接馬達則連接, 並回到初始位置
    for(int i = 0; i < SERVOS; i++) {
        if(!myservo[i].attached()) {
            myservo[i].attach(servoConfig[i][1]);
        }
        myservo[i].write(servoConfig[i][4]);
    }

    delay(1000);
    Serial.println("Start Playing...");
    while(1) {
        for(int i = 0; i < actionCounter - 1; i++) {
            if(!digitalRead(buttonL) || !digitalRead(buttonR)) {
                armMode = 0;
                return;
            }
            armfromto(actionMemory[i], actionMemory[i+1]);
        }
        delay(500/playSpeed);
        armfromto(actionMemory[actionCounter - 1], actionMemory[0]);  //back to the first action
        delay(500/playSpeed);
    }
}

// 讓移動比較順暢
void armfromto(int *arrf, int *arrt){
    int maxAngles = 0;
    float lp[4];

    // 調整速度
    adjustSpeed();

    maxAngles = max(max(abs(arrt[0] - arrf[0]), abs(arrt[1] - arrf[1])), abs(arrt[2] - arrf[2]));
    maxAngles /= playSpeed;
    maxAngles = maxAngles < 1 ? 1 : maxAngles;

    for (int i = 0; i < SERVOS-1; i++){
        lp[i] = float(arrt[i] - arrf[i])/float(maxAngles);
    }

    for (int j = 1; j <= maxAngles; j++){
        for (int i = 0; i < SERVOS - 1; i++){
            myservo[i].write(arrf[i] + j * lp[i]);
        }
        delay(25);
    }

    // 爪子角度
    myservo[SERVOS-1].write(arrt[SERVOS-1]);
    delay(50);
}

void adjustSpeed(){
    for (int i = 0; i < SERVOS; i++){
        if (analogRead(i) > 512 + sensityOffset) playSpeed++;
        if (analogRead(i) < 512 - sensityOffset) playSpeed--;
    }
    playSpeed = playSpeed < 1 ? 1 : playSpeed;
    playSpeed = playSpeed > maxSpeed ? maxSpeed : playSpeed;
}