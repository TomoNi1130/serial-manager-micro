#ifndef SERIAL_MANAGER_HPP
#define SERIAL_MANAGER_HPP

#include <string>
#include <thread>
#include <vector>

#include "mbed.h"

struct SerialMsg {
 public:
  std::vector<float> numbers;
  std::vector<bool> flags;
  SerialMsg() = default;
  template <typename... Args>
  SerialMsg(const Args&... args) {
    (assign(args), ...);  // 各引数に assign を適用
  }

 private:
  template <typename T>
  void assign(const std::vector<T>& v);
  template <typename T, std::size_t S>
  void assign(const std::array<T, S>& v);
};

// vector用
template <typename T>
void SerialMsg::assign(const std::vector<T>& v) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(v.begin(), v.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(v.begin(), v.end());
  }
}

// array 用
template <typename T, std::size_t N>
void SerialMsg::assign(const std::array<T, N>& a) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(a.begin(), a.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(a.begin(), a.end());
  }
}

class SerialManager {
 public:
  SerialManager(BufferedSerial& serial, uint8_t id);
  void send_msg(const SerialMsg& send_msg);
  void send_msg(const std::vector<uint8_t>& send_msg);
  void send_log(const std::string& log_msg);

  SerialMsg received_msg;

 private:
  std::vector<uint8_t> cobs_string(const std::string& input_str);
  template <typename T>
  std::vector<uint8_t> cobs_encode(const std::vector<T>& input) {
    std::vector<uint8_t> encoded;
    if constexpr (std::is_same_v<T, float>)
      encoded.push_back(0x01);
    else if constexpr (std::is_same_v<T, uint8_t>)
      encoded.push_back(0x02);
    else
      encoded.push_back(0xff);
    encoded.push_back(0x00);
    uint8_t count = 0;  // 次にsource_data[i]に0x00が出るまでの配列番号をカウント
    int mark = 1;       // 最後に0x00が出たsource_data[i]の配列番号をキープ
    for (size_t i = 0; i < input.size(); ++i) {
      const uint8_t* raw = reinterpret_cast<const uint8_t*>(&input[i]);
      for (size_t j = 0; j < sizeof(T); ++j) {
        if (raw[j] != 0x00) {
          encoded.push_back(raw[j]);
          count++;
          if (count == 0xFF) {
            encoded[mark] = count;
            mark = encoded.size();
            encoded.push_back(0x00);
            count = 0;
          }
        } else {
          encoded[mark] = count + 1;
          mark = encoded.size();
          encoded.push_back(0x00);
          count = 0;
        }
      }
    }
    count++;
    encoded[mark] = count;
    encoded.push_back(0x00);
    return encoded;
  }

  void heartbeat();
  void receive_msg();

  BufferedSerial& men_serial;
  const uint8_t serial_id;

  Thread heartbeat_thread;
  Thread receive_thread;
};

#endif