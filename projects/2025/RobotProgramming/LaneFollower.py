#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from geometry_msgs.msg import Twist, Point 
from cv_bridge import CvBridge
import cv2
import numpy as np
import time

class State:
    FOLLOWING = 0
    BLIND_TURN = 1
    RECOVERY = 2

class LineFollower(Node):
    def __init__(self):
        super().__init__('line_follower')
        
        self.publisher_ = self.create_publisher(Twist, '/cmd_vel', 10)
        self.debug_pub = self.create_publisher(Image, '/camera/debug', 10)
        self.centroid_pub = self.create_publisher(Point, '/line_centroid', 10) 
        self.subscription = self.create_subscription(
            Image, '/camera/image_raw', self.image_callback, 10)
        self.br = CvBridge()
        
        # ==========================================
        # [★ 핵심 튜닝] PID 제어 및 지연 보상 변수
        # ==========================================
        
        # 1. 배경 오인식 해결 (면적 필터)
        self.MIN_CONTOUR_AREA = 200   
        self.MAX_CONTOUR_AREA = 8000  
        
        # 2. PID 제어 강화
        self.KP = 0.0035 
        self.KI = 0.00001
        self.KD = 0.035   
        self.ALPHA = 0.5  
        
        # 3. 속도 설정
        self.MAX_SPEED = 0.20
        self.MIN_SPEED = 0.08
        
        # 4. 라인 놓침 설정
        self.LOST_ANGULAR_SPEED = 2.0
        self.LOST_LINEAR_SPEED = 0.08
        self.LOST_SEARCH_TIME = 1.5
        
        # 5. HSV 설정
        self.lower_black = np.array([0, 0, 0])
        self.upper_black = np.array([179, 255, 117])
        
        # 6. 제어 지연 보상 설정 (NEW)
        self.LATENCY_TIME = 0.15     # 예상되는 제어 지연 시간 (150ms)
        self.PREDICTION_FACTOR = 100 # 각속도 단위를 픽셀 오차로 변환하는 계수
        self.last_angular_z = 0.0    # 직전 각속도 저장

        # 상태 및 오차 변수 초기화
        self.current_state = State.FOLLOWING
        self.line_lost_time = 0.0
        self.last_turn_dir = 0
        self.prev_filtered_err = 0.0
        self.last_error_d = 0.0
        self.integral_err = 0.0 
        self.log_counter = 0

        self.get_logger().info(">>> Line Follower: PID 제어 + 지연 보상 모드 <<<")

    def process_roi(self, frame, hsv, y_start, y_end, debug_view, color=(0, 255, 0), margin_ratio=0.2):
        h, w = frame.shape[:2]
        roi_hsv = hsv[y_start:y_end, 0:w]
        
        mask = cv2.inRange(roi_hsv, self.lower_black, self.upper_black)
        
        roi_h, roi_w = mask.shape
        side_margin = int(roi_w * margin_ratio)
        mask[:, 0:side_margin] = 0
        mask[:, roi_w-side_margin:roi_w] = 0
        
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        best_cnt = None
        max_area = 0
        cx, cy = 0, 0
        found = False
        
        for cnt in contours:
            area = cv2.contourArea(cnt)
            
            # 면적 필터 적용 (배경 무시)
            if self.MIN_CONTOUR_AREA < area < self.MAX_CONTOUR_AREA:
                if area > max_area:
                    max_area = area
                    best_cnt = cnt
        
        if best_cnt is not None:
            found = True
            M = cv2.moments(best_cnt)
            if M['m00'] > 0:
                cx = int(M['m10'] / M['m00'])
                local_cy = int(M['m01'] / M['m00'])
                cy = local_cy + y_start
                
                cv2.drawContours(debug_view, [best_cnt + [0, y_start]], -1, color, 2)
                cv2.circle(debug_view, (cx, cy), 6, (0, 0, 255), -1)
        
        cv2.rectangle(debug_view, (0, y_start), (w, y_end), color, 1)
        
        return found, cx, cy

    def image_callback(self, msg):
        try:
            cv_image = self.br.imgmsg_to_cv2(msg, "bgr8")
            if cv_image is None: return
            frame = cv_image
            h, w, _ = frame.shape
            
            frame_blur = cv2.GaussianBlur(frame, (5, 5), 0)
            hsv = cv2.cvtColor(frame_blur, cv2.COLOR_BGR2HSV)
            debug_view = frame.copy()

            # ROI 설정: 바닥 근처 15% 영역만 관찰
            y_start = int(h * 0.80)
            y_end = int(h * 0.95)
            
            found, cx, cy = self.process_roi(
                frame, hsv, y_start, y_end, debug_view, 
                color=(0, 255, 0), margin_ratio=0.2
            )

            twist = Twist()
            current_time = time.time()
            debug_str = ""
            
            # --- 상황판단 A: 라인 발견 (FOLLOWING) ---
            if found:
                self.current_state = State.FOLLOWING
                self.line_lost_time = 0.0
                
                centroid_msg = Point()
                centroid_msg.x = float(cx)
                centroid_msg.y = float(cy)
                self.centroid_pub.publish(centroid_msg)

                # 오차 계산
                raw_err = cx - (w / 2)
                
                if raw_err != 0:
                    self.last_turn_dir = 1 if raw_err > 0 else -1

                # 필터링
                filtered_err = (raw_err * self.ALPHA) + (self.prev_filtered_err * (1.0 - self.ALPHA))
                self.prev_filtered_err = filtered_err 
                
                # --- [NEW] 제어 지연 보상 로직 ---
                # 직전 각속도와 지연 시간을 이용해 예상되는 횡방향 오차를 계산
                predicted_shift = self.last_angular_z * self.PREDICTION_FACTOR * self.LATENCY_TIME
                compensated_err = filtered_err + predicted_shift # 보상된 오차
                
                # I 제어 (오차 누적)
                self.integral_err += filtered_err # I 제어는 순수 오차로 계산
                self.integral_err = np.clip(self.integral_err, -3000, 3000)

                # PD Control
                # D-term은 보상된 오차의 변화율을 기반으로 계산 (미래 오차 예측)
                error_derivative = compensated_err - self.last_error_d
                self.last_error_d = compensated_err 

                # ★★★ PID 제어 적용 (보상된 오차 사용) ★★★
                angular_z = - (self.KP * compensated_err + self.KI * self.integral_err + self.KD * error_derivative)
                twist.angular.z = np.clip(angular_z, -2.0, 2.0)
                
                # 다음 프레임을 위한 각속도 저장
                self.last_angular_z = angular_z 

                # 속도 제어
                corner_threshold = w * 0.25 
                if abs(filtered_err) > corner_threshold:
                    twist.linear.x = self.MIN_SPEED
                    debug_str = f"⚠️ 코너링 | Err:{filtered_err:.0f}"
                else:
                    error_ratio = abs(filtered_err) / (w/2)
                    target_speed = self.MAX_SPEED * (1.0 - error_ratio**2)
                    twist.linear.x = max(target_speed, self.MIN_SPEED)
                    debug_str = f"🚀 직선 주행 | Err:{filtered_err:.0f}"

            # --- 상황판단 B: 라인 놓침 (Lost) ---
            else:
                cv2.putText(debug_view, "LINE LOST!", (int(w/2)-80, int(h/2)), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)
                
                self.integral_err = 0 # 라인 놓치면 I 제어 초기화
                self.last_angular_z = 0.0 # 각속도 초기화
                
                if self.current_state == State.FOLLOWING:
                    self.current_state = State.BLIND_TURN
                    self.line_lost_time = current_time
                
                if self.current_state == State.BLIND_TURN:
                    # ... (기존 탐색 로직 유지) ...
                    debug_str = "🔄 탐색 중"
                    if current_time - self.line_lost_time < self.LOST_SEARCH_TIME:
                        twist.linear.x = self.LOST_LINEAR_SPEED
                        turn_dir = self.last_turn_dir if self.last_turn_dir != 0 else 1
                        twist.angular.z = self.LOST_ANGULAR_SPEED * (-turn_dir)
                    else:
                        self.current_state = State.RECOVERY
                
                elif self.current_state == State.RECOVERY:
                    # ... (기존 후진 로직 유지) ...
                    debug_str = "🔙 후진"
                    if current_time - self.line_lost_time < (self.LOST_SEARCH_TIME + 2.5):
                        twist.linear.x = -0.12
                        twist.angular.z = 0.0
                    else:
                        self.current_state = State.BLIND_TURN
                        self.line_lost_time = current_time 

            self.publisher_.publish(twist)
            try:
                self.debug_pub.publish(self.br.cv2_to_imgmsg(debug_view, "bgr8"))
            except: pass
            
            self.log_counter += 1
            if self.log_counter % 15 == 0:
                self.get_logger().info(debug_str)

        except Exception as e:
            self.get_logger().warn(f'Error: {e}')

def main(args=None):
    rclpy.init(args=args)
    node = LineFollower()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        stop = Twist()
        node.publisher_.publish(stop)
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
