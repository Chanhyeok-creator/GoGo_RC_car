# PiCamera2 기반 라즈베리파이 라인 트레이싱 제어

이 프로젝트는 Raspberry Pi와 PiCamera2를 사용하여 카메라 영상에서 특정 영역(하단 100픽셀)을 이진화 처리 후 선의 중심을 계산하여, 3단계 조향 명령을 아두이노로 전송하는 라인 트레이싱(라인 따라가기) 프로그램입니다.  
아두이노는 직진, 좌/우 조향 명령을 받아 로봇 또는 차량을 제어합니다.

---

## 주요 기능

- PiCamera2로 320x240 해상도 영상 캡처 (가로 반전 포함)
- 영상 하단 100픽셀 영역에서 선 검출 및 중심 좌표 계산
- 중심 좌표 오프셋을 기준으로 3단계 좌/우 조향 명령(L, l, C, r, R) 전송
- 매 프레임마다 전진(F) 명령 전송
- 선이 없으면 정지(S) 명령 전송
- 아두이노와 시리얼 통신(9600bps)으로 명령 전송
- OpenCV로 원본 영상 및 ROI(하단 100픽셀) 시각화
- `q` 키로 프로그램 종료 가능

---

## 코드 상세 설명

### 1. 아두이노 시리얼 연결 및 예외 처리

```python
try:
    arduino = serial.Serial('/dev/ttyACM0', 9600)
    time.sleep(2)
    use_serial = True
except:
    print("⚠️ 아두이노 미연결: 시리얼 생략")
    use_serial = False
/dev/ttyACM0 포트로 9600bps 연결을 시도하고, 실패 시 시리얼 사용을 중단함

2. 명령 중복 전송 방지 함수
python
복사
편집
last_cmd = None
def send_command(cmd):
    global last_cmd
    if cmd != last_cmd and use_serial:
        arduino.write(cmd.encode())
        print(f"📤 Sent to Arduino: {cmd}")
        last_cmd = cmd
이전에 보낸 명령과 다를 때만 아두이노로 전송해 중복 통신을 줄임

3. PiCamera2 초기화 및 설정
python
복사
편집
picam2 = Picamera2()
picam2.configure(picam2.create_still_configuration(
    main={"size": (320, 240)},
    transform=Transform(hflip=1)
))
picam2.start()
320x240 해상도, 가로 좌우 반전 설정 후 카메라 시작

4. 메인 루프: 영상 캡처 및 처리
python
복사
편집
while True:
    frame = picam2.capture_array()

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    _, binary = cv2.threshold(gray, 110, 255, cv2.THRESH_BINARY_INV)
    height, width = binary.shape
    roi = binary[height - 100:height, :]
BGR 영상을 그레이스케일로 변환 후 임계값 110으로 이진화(역이진화)

화면 하단 100픽셀 영역(ROI) 추출

5. 선의 중심 계산 및 조향 명령 판단
python
복사
편집
M = cv2.moments(roi)
if M["m00"] != 0:
    cx = int(M["m10"] / M["m00"])
    center_offset = cx - (width // 2)
ROI에서 모멘트를 계산해 선의 중심 좌표(cx) 추출

화면 중앙과의 오프셋(center_offset) 산출

6. 3단계 조향 및 전진 명령 전송
python
복사
편집
if center_offset < -40:
    send_command('L')  # 강하게 왼쪽
elif center_offset < -15:
    send_command('l')  # 약하게 왼쪽
elif center_offset > 40:
    send_command('R')  # 강하게 오른쪽
elif center_offset > 15:
    send_command('r')  # 약하게 오른쪽
else:
    send_command('C')  # 중앙 정렬

send_command('F')  # 전진
오프셋 구간에 따라 좌우 강약 조향 명령 전송

항상 전진(F) 명령을 함께 전송

7. 선 미검출 시 정지 명령
python
복사
편집
else:
    send_command('S')
    print("⚠️ 선을 못 찾음 → 정지")
모멘트 값이 0이면 선이 없다고 판단해 정지(S) 명령 전송

8. 시각화 및 종료 조건
python
복사
편집
cv2.circle(roi, (cx, 50), 5, (255, 255, 255), -1)
cv2.imshow("Original", frame)
cv2.imshow("ROI", roi)

if cv2.waitKey(1) & 0xFF == ord('q'):
    break
ROI 영상에 중심 좌표 표시 후 원본 영상 및 ROI 화면 출력

q 키 입력 시 루프 종료

9. 프로그램 종료 처리
python
복사
편집
finally:
    picam2.stop()
    if use_serial:
        arduino.close()
    cv2.destroyAllWindows()
    print("✅ 시스템 종료 완료")
카메라, 시리얼, OpenCV 자원 정상 해제 및 종료 메시지 출력
