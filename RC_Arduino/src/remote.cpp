#include <Arduino.h>
#include <Servo.h>     // 서보모터 제어용 라이브러리

// ESC랑 조향 서보용 객체 생성
Servo motor;   // 전진/후진 속도 조절하는 ESC 제어용
Servo steer;   // 방향 조정하는 조향 서보

// 수신되는 PWM 값 저장할 변수들
int pwmSteer, pwmMotor;

// 수신 핀 설정 (RC 수신기 채널 연결된 핀 번호)
int steerPin = 3;   // 조향 신호 들어오는 핀 (채널 2)
int motorPin = 2;   // 속도 신호 들어오는 핀 (채널 5)

// 깜빡이용 LED 핀
int leftLED = 9;    // 왼쪽 깜빡이 연결된 핀
int rightLED = 8;   // 오른쪽 깜빡이 연결된 핀

// 깜빡이 타이밍 관련 변수
unsigned long lastBlinkTime = 0;  // 마지막으로 깜빡인 시간 저장
int blinkCount = 0;               // 지금까지 깜빡인 횟수
bool blinkingLeft = false;        // 왼쪽 깜빡이 켜졌는지 여부
bool blinkingRight = false;       // 오른쪽 깜빡이 켜졌는지 여부

void setup() {
  Serial.begin(9600);             // 디버깅용 시리얼 출력 시작
  motor.attach(5);                // ESC 신호 출력 핀 (D5)
  steer.attach(6);                // 조향 서보 신호 출력 핀 (D6)
  
  // 입력 핀 설정 (수신기에서 PWM 받는 핀들)
  pinMode(steerPin, INPUT);
  pinMode(motorPin, INPUT);
  
  // 출력 핀 설정 (LED)
  pinMode(leftLED, OUTPUT);
  pinMode(rightLED, OUTPUT);

  // 초기 상태 설정 (정지 + 직진)
  motor.writeMicroseconds(1500);  // ESC 중립값 (정지)
  steer.writeMicroseconds(1500);  // 조향 중립값 (직진 방향)
}

void loop() {
  // PWM 신호 읽어오기 (단위: 마이크로초)
  pwmSteer = pulseIn(steerPin, HIGH, 25000);  // 조향 채널에서 PWM 읽기
  pwmMotor = pulseIn(motorPin, HIGH, 25000);  // 속도 채널에서 PWM 읽기

  // 조향 값이 정상 범위일 때만 조향 동작
  if (pwmSteer > 1000 && pwmSteer < 2000) {
    steer.writeMicroseconds(pwmSteer);  // 받은 값 그대로 조향 서보에 전달

    // 조향 값이 왼쪽/오른쪽 범위일 때 깜빡이 켜기
    if (pwmSteer < 1400) {
      startBlinking(true);   // 왼쪽으로 꺾고 있으니 왼쪽 깜빡이
    } else if (pwmSteer > 1600) {
      startBlinking(false);  // 오른쪽으로 꺾고 있으니 오른쪽 깜빡이
    }
  }

  // 속도 값도 정상 범위일 때만 처리
  if (pwmMotor > 1000 && pwmMotor < 2000) {
    // 받은 1000~2000 값을 1300~1700 사이로 줄여서 매핑
    // 너무 빠르거나 강하게 움직이지 않게 하기 위해 일부러 줄인 거임
    int adjusted = map(pwmMotor, 1000, 2000, 1300, 1700);
    motor.writeMicroseconds(adjusted);  // ESC에 전달
  }

  // 깜빡이 동작 처리 (millis 기준으로)
  updateBlinker();
}

// 깜빡이 시작할 때 호출하는 함수
// left = true → 왼쪽 깜빡이 / false → 오른쪽 깜빡이
void startBlinking(bool left) {
  if (left && !blinkingLeft) {  // 왼쪽 깜빡이 시작 조건
    blinkingLeft = true;
    blinkingRight = false;      // 반대편은 꺼야 함
    blinkCount = 0;
    lastBlinkTime = millis();   // 시작 시간 저장
  } else if (!left && !blinkingRight) {  // 오른쪽 깜빡이 시작 조건
    blinkingRight = true;
    blinkingLeft = false;
    blinkCount = 0;
    lastBlinkTime = millis();
  }
}

// 깜빡이 실제로 LED 점멸시키는 함수
// 0.1초마다 ON/OFF 반복해서 총 5번 깜빡임
void updateBlinker() {
  if (blinkingLeft || blinkingRight) {
    unsigned long now = millis();
    
    // 100ms 지났으면 한 번 깜빡일 때 됐음
    if (now - lastBlinkTime >= 100) {
      lastBlinkTime = now;
      blinkCount++;

      // 홀수 번째 → 켜고, 짝수 번째 → 끄기
      if (blinkCount % 2 == 1) {
        digitalWrite(blinkingLeft ? leftLED : rightLED, HIGH);
      } else {
        digitalWrite(blinkingLeft ? leftLED : rightLED, LOW);
      }

      // 총 10번 (켜짐/꺼짐 총합 10) 이후 종료
      if (blinkCount >= 10) {
        blinkingLeft = false;
        blinkingRight = false;
        digitalWrite(leftLED, LOW);   // 혹시 남아있을 수도 있으니 끄기
        digitalWrite(rightLED, LOW);
      }
    }
  }
}
