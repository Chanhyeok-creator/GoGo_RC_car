#include <Arduino.h>
#include <Servo.h>     // 서보모터/ESC 제어용 라이브러리

// 모터와 조향 서보 객체 생성
Servo motor;  // DC 모터 → ESC로 제어
Servo steer;  // 조향 서보 → 방향 제어

char command;            // 수신한 명령 저장용
int lastSteer = 1500;    // 마지막 조향 값 기억용 (X 명령에 필요)

// 짧게 앞으로 전진하는 함수 (0.15초 정도)
void shortForward() {
  motor.writeMicroseconds(1600);  // 살짝 전진
  delay(150);                     // 0.15초 유지
  motor.writeMicroseconds(1500);  // 정지
  delay(100);                     // 살짝 쉬어가기
}

// 짧게 뒤로 후진하는 함수
void shortBackward() {
  motor.writeMicroseconds(1400);  // 살짝 후진
  delay(150);
  motor.writeMicroseconds(1500);  // 정지
  delay(100);
}

void setup() {
  Serial.begin(9600);      // 시리얼 통신 시작 (속도: 9600bps)
  motor.attach(5);         // ESC 핀 연결 (D5)
  steer.attach(6);         // 조향 서보 핀 연결 (D6)

  motor.writeMicroseconds(1500);  // 모터 정지 상태
  steer.writeMicroseconds(1500);  // 조향 중앙으로 초기화
}

void loop() {
  // 시리얼로 명령 들어오면 처리
  if (Serial.available()) {
    command = Serial.read();         // 명령 1바이트 읽기
    Serial.print("📥 받은 명령: ");
    Serial.println(command);         // 어떤 명령 받았는지 확인용 출력

    switch (command) {
      case 'F':  // 전진
        shortForward();
        break;

      case 'S':  // 정지
        motor.writeMicroseconds(1500);
        break;

      case 'C':  // 조향 중앙으로 리셋
        lastSteer = 1500;
        steer.writeMicroseconds(1500);
        break;

      case 'L':  // 강하게 왼쪽 (많이 꺾기)
        lastSteer = 1100;
        steer.writeMicroseconds(1100);
        break;

      case 'l':  // 약하게 왼쪽 (살짝 꺾기)
        lastSteer = 1300;
        steer.writeMicroseconds(1300);
        break;

      case 'R':  // 강하게 오른쪽
        lastSteer = 1900;
        steer.writeMicroseconds(1900);
        break;

      case 'r':  // 약하게 오른쪽
        lastSteer = 1700;
        steer.writeMicroseconds(1700);
        break;

      case 'X':  // 방금 조향값의 반대방향으로 살짝 꺾은 후, 톡톡 후진
        // 최근 조향값 기준으로 반대 방향 약하게 꺾기
        if (lastSteer < 1450) {
          steer.writeMicroseconds(1700);  // 반대인 약 오른쪽
        } else if (lastSteer > 1550) {
          steer.writeMicroseconds(1300);  // 반대인 약 왼쪽
        } else {
          steer.writeMicroseconds(1500);  // 중앙으로
        }

        shortBackward();  // 살짝 후진해서 틀어진 방향 정렬
        break;
    }
  }
}
