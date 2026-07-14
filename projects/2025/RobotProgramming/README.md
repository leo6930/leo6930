# 🤖 Vision-Based Line Following (TurtleBot3 Waffle)

This project implements an autonomous line-following system using a single camera on a **TurtleBot3 Waffle** platform, accelerated by the **NVIDIA Jetson Nano**. The system employs real-time computer vision to detect the track and an advanced control algorithm to maintain stability even at high speeds or during sharp turns.

---

## 🚀 Key Technical Features

*   **Computer Vision Pipeline:** 
    *   Utilizes **OpenCV** with Gaussian blurring and HSV color space filtering for robust track detection.
    *   Implements **Region of Interest (ROI)** processing and contour area filtering to eliminate background noise.
*   **Advanced PID Control:**
    *   **Latency Compensation:** Features a predictive control logic that calculates expected lateral shifts based on previous angular velocity and system latency.
    *   **Proportional-Integral-Derivative (PID) Control:** Dynamically adjusts angular velocity to ensure smooth tracking and precise error correction.
*   **Adaptive Speed Control:** Automatically scales linear velocity based on tracking error—slowing down during sharp cornering and accelerating on straight sections.
*   **Robust Recovery Logic:** Includes a state-machine-based "Lost Line" recovery system, performing blind turns and reverse movements to re-acquire the track if temporarily lost.

---

## 🛠 Tech Stack

*   **Platform:** TurtleBot3 Waffle
*   **Computing:** NVIDIA Jetson Nano
*   **Middleware:** ROS 2 (Humble/Foxy)
*   **Language:** Python 3
*   **Vision Library:** OpenCV (with CvBridge)

---

## ⚙️ How to Use

1.  **Camera Input:** The system subscribes to `/camera/image_raw` for real-time vision processing.
2.  **Control Output:** Computed velocity commands are published to `/cmd_vel` for the TurtleBot3 base.
3.  **Debugging:** The node publishes processed images to `/camera/debug` and track centroids to `/line_centroid` for real-time monitoring.

---

*This project was developed to maximize the tracking precision and speed of the TurtleBot3 Waffle in an autonomous navigation environment.*

**Developer:** Geonho Kim (leo6930)

## Demo Video
[![Watch the video](https://youtube.com/shorts/JEkBP9lE6tw?si=z6M5vzg1-jZMte-R)
