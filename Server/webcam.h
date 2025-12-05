#ifndef WEBCAM_H
#define WEBCAM_H

#include <string>

// ===== LỆnh CŨ (GIỮ NGUYÊN) =====
// Mở ứng dụng webcam
std::string webcamStart();

// Đóng ứng dụng webcam
std::string webcamStop();

// Chụp ảnh từ webcam (1 frame)
std::string webcamCapture();

// Quay video 10-20 giây (tham số: duration in seconds, default 15s)
std::string webcamRecordVideo(int durationSeconds = 15);

// Bắt đầu stream video realtime từ webcam (gửi MJPEG frame liên tục)
std::string webcamStreamStart();

// Dừng stream video
std::string webcamStreamStop();

#endif
