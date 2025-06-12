# 수동 모드 - 리모컨 조작 (remote.cpp)

## 프로젝트 개요

이 프로젝트는 RC카를 수동으로 조작할 수 있도록 만든 시스템입니다.  
아두이노가 RC 수신기로부터 PWM 신호를 받아서, 모터와 조향 서보를 제어하는 방식이며, 조종기로 RC카를 직접 조작할 수 있습니다.

---

## 전체 흐름

- RC 수신기에서 PWM 신호를 입력받음
- 받은 신호를 바탕으로 조향 및 모터를 실시간으로 제어함
- 조향값이 왼쪽이나 오른쪽으로 크게 꺾이면 해당 방향 깜빡이가 자동으로 켜짐

---

## 핀 설정

| 기능            | 핀 번호 | 설명                           |
| --------------- | ------- | ------------------------------ |
| ESC (속도 제어) | D5      | 전진/후진용 PWM 출력 핀        |
| 조향 서보       | D6      | 방향 제어용 PWM 출력 핀        |
| 조향 입력       | D3      | RC 수신기 조향 채널(PWM 입력)  |
| 속도 입력       | D2      | RC 수신기 모터 채널(PWM 입력)  |
| 왼쪽 깜빡이     | D9      | 좌측 방향지시등 제어용 출력 핀 |
| 오른쪽 깜빡이   | D8      | 우측 방향지시등 제어용 출력 핀 |

## 주요 로직 설명

### 전역 변수 선언

```cpp
Servo motor;
Servo steer;

int pwmSteer, pwmMotor;

int steerPin = 3;
int motorPin = 2;

int leftLED = 9;
int rightLED = 8;

unsigned long lastBlinkTime = 0;
int blinkCount = 0;
bool blinkingLeft = false;
bool blinkingRight = false;
```

- `Servo motor`

  - ESC(모터 컨트롤러)를 제어하기 위한 서보 객체
  - `writeMicroseconds()` 함수로 1000~2000us 신호를 보내서 전진/후진 조절

- `Servo steer`

  - 조향(steering)을 담당하는 서보모터 제어용 객체
  - 마찬가지로 PWM 신호로 방향을 조절함 (중앙: 1500, 왼쪽: 1100~1300, 오른쪽: 1700~1900 등)

- `int pwmSteer`, `int pwmMotor`

  - RC 수신기로부터 들어오는 PWM 신호값을 저장하는 변수
  - 각각 조향(PWM 채널 2번)과 속도(PWM 채널 5번) 신호를 받아 처리

- `int steerPin = 3`

  - RC 수신기의 **조향 채널(PWM 2번)**이 연결된 핀
  - `pulseIn()` 함수로 이 핀의 HIGH 상태가 유지되는 시간을 읽어서 조향값으로 사용

- `int motorPin = 2`

  - RC 수신기의 **모터 속도 채널(PWM 5번)**이 연결된 핀
  - 마찬가지로 `pulseIn()`으로 읽어서 모터에 보낼 속도를 판단함

- `int leftLED = 9`

  - **왼쪽 깜빡이용 LED가 연결된 핀**
  - 조향값이 일정 수준 이하(왼쪽으로 꺾을 때)일 때 이 핀이 깜빡이게 됨

- `int rightLED = 8`

  - **오른쪽 깜빡이용 LED 핀**
  - 오른쪽으로 조향 꺾을 때 켜지고 깜빡이게 동작함

- `unsigned long lastBlinkTime = 0`

  - 마지막으로 깜빡인 시간(ms 단위)을 저장하는 변수
  - `millis()` 함수로 시간 비교할 때 사용함 (0.1초 간격으로 깜빡임 체크)

- `int blinkCount = 0`

  - 지금까지 깜빡인 횟수 저장
  - 보통 10회(켜짐/꺼짐 포함 총 10번 반복)로 제한해서 종료

- `bool blinkingLeft = false`

  - 현재 왼쪽 깜빡이가 작동 중인지 표시하는 플래그
  - `true`이면 왼쪽 LED가 깜빡이고 있는 상태

- `bool blinkingRight = false`
  - 현재 오른쪽 깜빡이가 작동 중인지 표시하는 플래그
  - `true`이면 오른쪽 LED가 깜빡이고 있음

### setup 함수

```cpp
void setup() {
  Serial.begin(9600);
  motor.attach(5);
  steer.attach(6);

  pinMode(steerPin, INPUT);
  pinMode(motorPin, INPUT);

  pinMode(leftLED, OUTPUT);
  pinMode(rightLED, OUTPUT);

  motor.writeMicroseconds(1500);  // ESC 중립값 (정지)
  steer.writeMicroseconds(1500);  // 조향 중립값 (직진 방향)
}
```

- `Serial.begin(9600)`

  - 시리얼 통신을 9600bps 속도로 시작
  - 디버깅하거나 명령 확인을 위해 PC와 연결된 시리얼 모니터로 메시지를 출력할 수 있음

- `motor.attach(5)`

  - ESC 제어용 서보 객체를 D5 핀에 연결
  - 이 핀을 통해 PWM 신호(1000~2000us)를 ESC로 전달해서 모터 속도를 조절함
  - 일반적으로 1500us는 정지, 1600~2000us는 전진, 1400~1000us는 후진

- `steer.attach(6)`

  - 조향 서보 객체를 D6 핀에 연결
  - PWM 신호로 조향 각도를 조절함
  - 1500us가 중앙, 그보다 작으면 왼쪽, 크면 오른쪽으로 꺾이게 설정됨

- `pinMode(steerPin, INPUT)`

  - RC 수신기에서 들어오는 조향 신호를 받기 위해 D3 핀을 입력으로 설정
  - 이 핀에서 PWM 펄스를 읽어서 steer 제어에 사용함

- `pinMode(motorPin, INPUT)`

  - RC 수신기의 속도(PWM) 신호가 들어오는 D2 핀을 입력으로 설정
  - 마찬가지로 `pulseIn()` 함수로 PWM 값을 읽어서 motor 제어에 사용

- `pinMode(leftLED, OUTPUT)`

  - 왼쪽 깜빡이 LED가 연결된 D9 핀을 출력으로 설정
  - 조향값이 왼쪽으로 꺾일 때 이 핀이 깜빡이게 됨

- `pinMode(rightLED, OUTPUT)`

  - 오른쪽 깜빡이 LED가 연결된 D8 핀을 출력으로 설정
  - 조향값이 오른쪽일 때 이 핀이 깜빡이게 됨

- `motor.writeMicroseconds(1500)`

  - RC카가 시작하자마자 움직이지 않도록 모터를 정지 상태로 설정 (중립값)

- `steer.writeMicroseconds(1500)`
  - 조향도 중앙 값(1500us)으로 맞춰서 직진 방향 유지
  - 시작 시 방향이 꺾여 있는 걸 방지하기 위해 초기화함

### loop 함수

```cpp
void loop() {
  pwmSteer = pulseIn(steerPin, HIGH, 25000);
  pwmMotor = pulseIn(motorPin, HIGH, 25000);

  if (pwmSteer > 1000 && pwmSteer < 2000) {
    steer.writeMicroseconds(pwmSteer);

    if (pwmSteer < 1400) {
      startBlinking(true);
    } else if (pwmSteer > 1600) {
      startBlinking(false);
    }
  }

  if (pwmMotor > 1000 && pwmMotor < 2000) {
    int adjusted = map(pwmMotor, 1000, 2000, 1300, 1700);
    motor.writeMicroseconds(adjusted);
  }

  updateBlinker();
}
```

- `pwmSteer = pulseIn(steerPin, HIGH, 25000)`

  - D3 핀에서 들어오는 조향용 PWM 신호의 길이를 읽음
  - HIGH 상태가 얼마나 유지됐는지 마이크로초(us) 단위로 반환
  - `25000`은 타임아웃 시간 (25ms), 이 안에 펄스 안 들어오면 0 반환됨

- `pwmMotor = pulseIn(motorPin, HIGH, 25000)`
  - D2 핀에서 모터 속도용 PWM 신호를 읽음
  - RC 수신기의 속도 채널에서 들어오는 PWM을 감지함

---

- `if (pwmSteer > 1000 && pwmSteer < 2000)`
  - 조향값이 정상 범위일 때만 동작 (이상값 필터링)
  - 조향값 그대로 `steer.writeMicroseconds(pwmSteer)`로 전달해서 실시간 방향 제어

#### 깜빡이 조건

- `pwmSteer < 1400` → 왼쪽으로 크게 꺾은 상태 → `startBlinking(true)` 실행
- `pwmSteer > 1600` → 오른쪽으로 크게 꺾은 상태 → `startBlinking(false)` 실행
- 중앙에 가까운 경우는 깜빡이 작동 안 함

---

- `if (pwmMotor > 1000 && pwmMotor < 2000)`
  - 속도값이 정상 범위일 때만 처리
  - `map(pwmMotor, 1000, 2000, 1300, 1700)`
    - RC 수신기 기준 1000~2000의 입력 값을 실제 ESC 제어에 사용할 1300~1700으로 줄여서 변환
    - 너무 빠르게 움직이지 않도록 안전하게 속도를 제한함
  - 변환된 값을 `motor.writeMicroseconds(adjusted)`로 ESC에 전달

---

- `updateBlinker()`

  - 깜빡이 상태를 확인하고 타이머에 따라 LED를 깜빡이게 처리
  - 깜빡이는 상태면 100ms마다 LED ON/OFF 반복, 총 10번 반복 후 자동 종료

### startBlinking 함수

```cpp
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
```

- `startBlinking(bool left)`
  - 인자로 `true`가 들어오면 왼쪽 깜빡이, `false`면 오른쪽 깜빡이를 켜는 함수
  - 이미 해당 방향이 깜빡이는 중이면 무시하고, 꺼져 있을 때만 새로 시작

---

- `if (left && !blinkingLeft)`
  - 왼쪽 깜빡이를 켜야 하는데, 아직 켜지지 않은 상태일 때만 실행
  - `blinkingLeft = true` → 왼쪽 깜빡이 시작
  - `blinkingRight = false` → 오른쪽은 꺼야 하니까 false로 설정
  - `blinkCount = 0` → 깜빡인 횟수 초기화
  - `lastBlinkTime = millis()` → 마지막으로 깜빡인 시간 저장 (타이머 기준용)

---

- `else if (!left && !blinkingRight)`
  - 오른쪽 깜빡이를 켜야 하고, 아직 꺼져있는 경우
  - `blinkingRight = true`, `blinkingLeft = false`로 설정해서 오른쪽만 켜지게 함
  - 나머지도 동일하게 깜빡이 타이머 초기화

### updateBlinker 함수

```cpp
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
```

- 깜빡이 동작을 시간 기준으로 처리하는 함수
- 현재 `blinkingLeft`나 `blinkingRight`가 `true`면 깜빡이는 중이라는 뜻 → 실행됨

---

- `unsigned long now = millis()`
  - 현재 시간을 밀리초(ms) 단위로 가져옴
  - `millis()`는 아두이노가 부팅된 이후 지난 시간 (오버플로우 없음)

---

- `if (now - lastBlinkTime >= 100)`
  - 마지막으로 깜빡인 이후 100ms가 지났는지 확인
  - 0.1초마다 ON/OFF 토글하는 구조

---

- `lastBlinkTime = now`

  - 깜빡인 시점을 현재 시간으로 갱신

- `blinkCount++`
  - 깜빡인 횟수 1 증가 (ON이든 OFF든 한 번으로 카운트)

---

- `if (blinkCount % 2 == 1)`

  - 깜빡이 횟수가 홀수일 때 (켜는 타이밍)
  - `digitalWrite(blinkingLeft ? leftLED : rightLED, HIGH)`
    - 왼쪽인지 오른쪽인지 판단해서 해당 LED를 켬

- `else`
  - 짝수일 때는 끄는 타이밍
  - 같은 방식으로 해당 LED를 LOW로 꺼줌

---

- `if (blinkCount >= 10)`
  - 깜빡이 횟수가 10번(ON/OFF 포함)이 되면 깜빡이 종료
  - `blinkingLeft`, `blinkingRight` 전부 false로 초기화
  - `digitalWrite()`로 양쪽 LED를 꺼서 혹시 남아있는 불빛 제거

## 정리

- RC 수신기로부터 PWM 신호를 받아 조향 및 속도 제어를 수행함
- 조향 값에 따라 방향지시등(깜빡이)이 자동으로 작동함
- 전진/후진 속도는 일정 범위로 제한하여 안정적인 주행이 가능함

---

---

# 자동 모드 - 카메라 기반 자율 주행 (auto.cpp)

## 프로젝트 개요

이 프로젝트는 RC카를 자동으로 주행시키기 위한 시스템입니다.  
라즈베리파이에서 카메라를 통해 영상을 분석하여 선을 추적하고, 그 결과에 따라 조향 및 속도를 자동 제어합니다.  
수신기 없이 자율적으로 동작하며, 아두이노는 라즈베리파이로부터 전달받은 명령을 기반으로 차량을 조종합니다.

---

## 전체 흐름

- 라즈베리파이에서 카메라로 영상 정보를 수집
- 영상 분석 결과에 따라 전진, 정지, 좌회전, 우회전 등의 명령이 생성됨
- 생성된 명령은 시리얼 통신을 통해 아두이노로 전송
- 아두이노는 해당 명령을 받아 모터와 조향 서보를 제어함

---

## 핀 설정

| 기능            | 핀 번호 | 설명                         |
| --------------- | ------- | ---------------------------- |
| ESC (속도 제어) | D5      | 전진/후진 제어용 PWM 출력 핀 |
| 조향 서보       | D6      | 방향 제어용 PWM 출력 핀      |

## 주요 로직 설명

### 전역 변수 선언

```cpp
Servo motor;
Servo steer;

char command;
int lastSteer = 1500;
```

- `Servo motor`

  - ESC를 제어하기 위한 서보 객체
  - `writeMicroseconds()` 함수를 통해 1000~2000us 범위의 PWM 신호를 ESC로 전달
  - 이 신호에 따라 전진/후진 속도를 조절할 수 있음
  - 예: 1500us → 정지, 1600us 이상 → 전진, 1400us 이하 → 후진

- `Servo steer`

  - 조향을 담당하는 서보 객체
  - PWM 신호로 조향 각도를 조절함
  - 예: 1500us → 중앙, 1300us → 왼쪽으로 살짝 꺾음, 1700us → 오른쪽으로 살짝 꺾음

- `char command`

  - 라즈베리파이로부터 시리얼로 전달받는 명령 문자를 저장하는 변수
  - 예: 'F', 'S', 'L', 'r', 'X' 등
  - 이 문자를 기반으로 RC카가 어떤 동작을 할지 결정됨

- `int lastSteer = 1500`
  - 가장 최근에 적용된 조향값을 저장하는 변수
  - 'X' 명령처럼 방향을 반대로 꺾는 동작에서 기준값으로 활용됨
  - 예: 왼쪽으로 꺾은 상태였다면, 'X' 명령 시 오른쪽으로 살짝 틀고 후진하도록 판단 가능

### shortForward / shortBackward 함수

```cpp
void shortForward() {
  motor.writeMicroseconds(1600);
  delay(150);
  motor.writeMicroseconds(1500);
  delay(100);
}

void shortBackward() {
  motor.writeMicroseconds(1400);
  delay(150);
  motor.writeMicroseconds(1500);
  delay(100);
}
```

- `shortForward()`

  - 모터에 1600us의 PWM 신호를 약 0.15초 동안 보내 전진
  - 이후 1500us로 중립값을 보내 정지
  - 마지막에 약간의 지연(`delay(100)`)을 둬서 다음 동작과의 간섭 방지
  - **짧고 순간적인 전진 동작**을 구현하는 용도로 사용됨
  - 주로 장애물 통과 직전이나 방향 수정 직후 등에 사용 가능

- `shortBackward()`
  - 구조는 `shortForward()`와 동일하지만 1400us로 후진 신호를 줌
  - 마찬가지로 0.15초 후 정지
  - **짧게 뒤로 빠지는 후진 동작**을 담당함
  - 'X' 명령처럼, 조향을 반대로 살짝 틀고 후진할 때 자주 호출됨

### setup 함수

```cpp
void setup() {
  Serial.begin(9600);
  motor.attach(5);
  steer.attach(6);
  motor.writeMicroseconds(1500);
  steer.writeMicroseconds(1500);
}
```

- `Serial.begin(9600)`

  - 시리얼 통신을 9600bps로 초기화
  - 라즈베리파이로부터 문자 명령을 받을 준비를 함
  - 이후 `Serial.read()`를 통해 문자를 받아 동작을 결정함

- `motor.attach(5)`

  - ESC 제어용 서보 객체를 D5 핀에 연결
  - D5 핀을 통해 모터에 PWM 신호를 보냄 (속도 조절용)

- `steer.attach(6)`

  - 조향 서보 객체를 D6 핀에 연결
  - D6 핀을 통해 서보에 PWM 신호를 보냄 (방향 조절용)

- `motor.writeMicroseconds(1500)`

  - RC카가 켜졌을 때 바로 움직이지 않도록 정지 상태(중립값)로 초기화

- `steer.writeMicroseconds(1500)`
  - 조향을 중앙값으로 초기화해서 차가 직진 상태로 시작하도록 설정

### loop 함수

```cpp
void loop() {
  if (Serial.available()) {
    command = Serial.read();
    Serial.print("받은 명령: ");
    Serial.println(command);

    switch (command) {
      case 'F':
        shortForward();
        break;

      case 'S':
        motor.writeMicroseconds(1500);
        break;

      case 'C':
        lastSteer = 1500;
        steer.writeMicroseconds(1500);
        break;

      case 'L':
        lastSteer = 1100;
        steer.writeMicroseconds(1100);
        break;

      case 'l':
        lastSteer = 1300;
        steer.writeMicroseconds(1300);
        break;

      case 'R':
        lastSteer = 1900;
        steer.writeMicroseconds(1900);
        break;

      case 'r':
        lastSteer = 1700;
        steer.writeMicroseconds(1700);
        break;

      case 'X':
        if (lastSteer < 1450) {
          steer.writeMicroseconds(1700);
        } else if (lastSteer > 1550) {
          steer.writeMicroseconds(1300);
        } else {
          steer.writeMicroseconds(1500);
        }

        shortBackward();
        break;
    }
  }
}
```

- `Serial.available()`

  - 시리얼 버퍼에 데이터가 들어왔는지 확인
  - 라즈베리파이가 문자 명령을 보내면 아래 로직이 실행됨

- `command = Serial.read()`

  - 들어온 문자 하나를 읽어와서 `command` 변수에 저장
  - 이후 `switch`문에서 어떤 동작을 할지 분기 처리함

- `Serial.print(...)`, `Serial.println(...)`
  - 디버깅 용도로 받은 명령을 시리얼 모니터에 출력함
  - 실제 동작과는 무관하지만 상태 확인에 유용함

---

#### 명령: `'F'`

- `shortForward()` 호출
- 약 0.15초간 전진 후 정지

#### 명령: `'S'`

- `motor.writeMicroseconds(1500)`
- ESC 중립값으로 모터를 정지 상태로 만듦

#### 명령: `'C'`

- 조향을 중앙값(1500us)으로 설정
- `lastSteer`도 중앙값으로 갱신해 이후 조향 상태 기준점이 됨

#### 명령: `'L'`

- 조향을 왼쪽으로 강하게 꺾기 (1100us)
- `lastSteer`에 현재 조향값 저장

#### 명령: `'l'`

- 조향을 왼쪽으로 약하게 꺾기 (1300us)
- `lastSteer`에 현재 조향값 저장

#### 명령: `'R'`

- 조향을 오른쪽으로 강하게 꺾기 (1900us)
- `lastSteer`에 현재 조향값 저장

#### 명령: `'r'`

- 조향을 오른쪽으로 약하게 꺾기 (1700us)
- `lastSteer`에 현재 조향값 저장

#### 명령: `'X'`

- 이전 조향값(`lastSteer`)을 기준으로 반대 방향으로 약하게 꺾은 뒤 후진
- 왼쪽으로 꺾은 상태였으면 오른쪽으로 살짝 틀고 후진
- 오른쪽으로 꺾은 상태였다면 왼쪽으로 틀고 후진
- 중립(1500) 근처였으면 조향 변경 없이 그대로 후진
- 마지막에 `shortBackward()` 호출해서 짧게 후진

---

- 이 구조를 통해 라즈베리파이에서 보낸 문자 하나로 RC카의 동작을 완전하게 제어할 수 있음
- 각 명령은 ESC 또는 조향 서보에 직접 PWM 신호를 보내는 방식으로 구현됨

## 정리리

- 카메라와 영상 분석을 기반으로 RC카의 조향과 속도를 자동으로 제어함
- 아두이노는 라즈베리파이로부터 수신한 문자 명령을 해석하여 동작을 수행함
- 외부 조작 없이 자율적으로 경로를 따라 주행할 수 있음
