#!/usr/bin/env python3

from picamera2 import Picamera2
from libcamera import Transform
import cv2
import serial
import time

# 아두이노 연결
try:
    arduino = serial.Serial('/dev/ttyACM0', 9600)
    time.sleep(2)
    use_serial = True
except:
    print("⚠️ 아두이노 미연결: 시리얼 생략")
    use_serial = False

# 명령 중복 전송 방지
last_cmd = None
def send_command(cmd):
    global last_cmd
    if cmd != last_cmd and use_serial:
        arduino.write(cmd.encode())
        print(f"📤 Sent to Arduino: {cmd}")
        last_cmd = cmd

# PiCamera2 설정
picam2 = Picamera2()
picam2.configure(picam2.create_still_configuration(
    main={"size": (320, 240)},
    transform=Transform(hflip=1)
))
picam2.start()
print("✅ 카메라 시작됨 (하단 100픽셀 분석, 3단계 조향)")

try:
    while True:
        frame = picam2.capture_array()

        # 이미지 처리
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        _, binary = cv2.threshold(gray, 110, 255, cv2.THRESH_BINARY_INV)
        height, width = binary.shape
        roi = binary[height - 100:height, :]  # 화면 하단 100픽셀

        M = cv2.moments(roi)
        if M["m00"] != 0:
            cx = int(M["m10"] / M["m00"])
            center_offset = cx - (width // 2)
            print(f"📍 center x: {cx} (offset: {center_offset})")

            # 조향 3단계 구간
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

            send_command('F')  # 매 프레임마다 한 번씩 톡 전진

            # 시각화
            cv2.circle(roi, (cx, 50), 5, (255, 255, 255), -1)
        else:
            send_command('S')
            print("⚠️ 선을 못 찾음 → 정지")

        # 디버깅용 화면 출력
        cv2.imshow("Original", frame)
        cv2.imshow("ROI", roi)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

        time.sleep(0.5)

except KeyboardInterrupt:
    print("🛑 Ctrl+C 종료됨")

finally:
    picam2.stop()
    if use_serial:
        arduino.close()
    cv2.destroyAllWindows()
    print("✅ 시스템 종료 완료")
