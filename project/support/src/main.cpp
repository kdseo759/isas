#include "udp_server.h"
#include <iostream>
#include <array>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>

// 로그 메시지를 작성하고 파일에 기록하는 함수
void logMessage(std::ofstream& log_file, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm now_tm = *std::localtime(&now_time_t);
    log_file << "[" << std::put_time(&now_tm, "%Y%m%d %H:%M:%S")
             << '.' << std::setw(3) << std::setfill('0') << now_ms.count() << "] "
             << message << std::endl;
}

void inputThread(std::atomic<bool>& running, std::array<uint8_t, 3>& data, std::mutex& data_mutex, std::ofstream& log_file) {
    logMessage(log_file, "Initial data: " + std::to_string(data[0]) + " " + std::to_string(data[1]) + " " + std::to_string(data[2]));

    while (running) {
        if (std::cin.peek() != EOF) {  // 입력이 있을 때만 실행
            std::cout << "Enter 3 bytes (0-255) separated by spaces: ";
            int byte1, byte2, byte3;
            std::cin >> byte1 >> byte2 >> byte3;

            if (std::cin.fail() || byte1 < 0 || byte1 > 255 || byte2 < 0 || byte2 > 255 || byte3 < 0 || byte3 > 255) {
                std::cerr << "Invalid input. Please enter 3 numbers between 0 and 255.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            std::lock_guard<std::mutex> guard(data_mutex);
            data = { static_cast<uint8_t>(byte1), static_cast<uint8_t>(byte2), static_cast<uint8_t>(byte3) };
            logMessage(log_file, "Data updated to: " + std::to_string(byte1) + " " + std::to_string(byte2) + " " + std::to_string(byte3));
            std::cout << "Data updated to: " << byte1 << " " << byte2 << " " << byte3 << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void broadcastThread(UdpServer& server, const std::array<uint8_t, 3>& data, std::mutex& data_mutex, std::atomic<bool>& running, std::ofstream& log_file) {
    while (running) {
        auto start_time = std::chrono::steady_clock::now();

        int interval;
        {
            std::lock_guard<std::mutex> guard(data_mutex);
            server.sendBroadcast(data);
            logMessage(log_file, "Broadcast sent: " + std::to_string(data[0]) + " " + std::to_string(data[1]) + " " + std::to_string(data[2]));
            interval = static_cast<int>(data[2]) * 10;  // 세 번째 바이트를 주기로 사용 10ms 단위
        }

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() * 10;
        
        if(elapsed_ms > 0 && elapsed_ms < interval) interval -= elapsed_ms;
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <byte1> <byte2> <interval_byte>\n";
        return EXIT_FAILURE;
    }

    std::array<uint8_t, 3> data;
    try {
        data[0] = static_cast<uint8_t>(std::stoi(argv[1]));
        data[1] = static_cast<uint8_t>(std::stoi(argv[2]));
        data[2] = static_cast<uint8_t>(std::stoi(argv[3])); 
    } catch (...) {
        std::cerr << "Invalid input. All bytes must be between 0 and 255.\n";
        return EXIT_FAILURE;
    }

    // 로그 파일 생성
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    std::ostringstream filename;
    filename << "log_" << std::put_time(&now_tm, "%Y%m%d_%H%M%S") << ".log";
    std::ofstream log_file(filename.str());
    if (!log_file) {
        std::cerr << "Failed to create log file.\n";
        return EXIT_FAILURE;
    }

    try {
        UdpServer server("eth0");
        std::atomic<bool> running(true);
        std::mutex data_mutex;

        std::thread input_thread(inputThread, std::ref(running), std::ref(data), std::ref(data_mutex), std::ref(log_file));
        std::thread broadcast_thread(broadcastThread, std::ref(server), std::cref(data), std::ref(data_mutex), std::ref(running), std::ref(log_file));

        input_thread.detach();  // 입력 스레드는 무기한 대기할 필요 없음
        broadcast_thread.join();
    } catch (const std::exception& e) {
        logMessage(log_file, std::string("Error: ") + e.what());
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}