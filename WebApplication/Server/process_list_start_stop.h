#ifndef PROCESS_LIST_START_STOP_H
#define PROCESS_LIST_START_STOP_H

#include <string>


//  */
std::string listProcesses();
// * @return std::string - Danh sách process dạng bảng (PID + Process Name)
//  * Format:
//  * ────────────────────────────────
//  * PID         Process Name
//  * ────────────────────────────────
//  * 1234        explorer.exe
//  * 5678        notepad.exe
//  * ...
//  * 
//  * Error: "Error: Cannot get process list" nếu thất bại

std::string startProcess(const std::string& process_name);
// * Start một process bằng cách nhập tên process
//   std::string - Kết quả:
//  *                       ✓ Process 'notepad.exe' started successfully
//  *                       ✗ Failed to start 'notepad.exe'


std::string stopProcess(int pid);
// * Stop/terminate a process by PID (Process ID)

#endif
