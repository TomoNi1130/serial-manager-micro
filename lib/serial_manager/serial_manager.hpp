#ifndef SERIAL_MANAGER_HPP
#define SERIAL_MANAGER_HPP

#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "mbed.h"

struct SerialMsg {
 public:
  std::vector<float> numbers;
  std::vector<bool> flags;
  std::vector<std::string> str_msgs;
  SerialMsg();
  SerialMsg(std::string str) { str_msgs = {str}; };
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

template <typename T>
void SerialMsg::assign(const std::vector<T>& v) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(v.begin(), v.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(v.begin(), v.end());
  } else if constexpr (std::is_same_v<T, std::string>) {
    str_msgs.assign(v.begin(), v.end());
  }
}

// array 用
template <typename T, std::size_t N>
void SerialMsg::assign(const std::array<T, N>& a) {
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    numbers.assign(a.begin(), a.end());
  } else if constexpr (std::is_same_v<T, bool>) {
    flags.assign(a.begin(), a.end());
  } else if constexpr (std::is_same_v<T, std::string>) {
    str_msgs.assign(a.begin(), a.end());
  }
}

class SerialManager {
 public:
  SerialManager(BufferedSerial& serial, uint8_t id);
  void send_msg(const SerialMsg& send_msg);
  void send_msg(const std::string& send_str);
  void send_log(const std::string& send_str);
  std::optional<SerialMsg> receive_msg();

 private:
  template <typename T>
  std::vector<T> split_strring(const std::string& target_string) {  //: で分ける
    std::vector<T> result;
    std::stringstream ss(target_string);
    std::string token;
    while (std::getline(ss, token, ':'))
      if (!token.empty()) {
        if constexpr (std::is_same_v<T, float>)
          result.push_back(std::stof(token));
        else if constexpr (std::is_same_v<T, double>)
          result.push_back(std::stod(token));
        else if constexpr (std::is_same_v<T, std::string>)
          result.push_back(token);
        else if constexpr (std::is_same_v<T, bool>) {
          if (token == "0")
            result.push_back(false);
          else if (token == "1")
            result.push_back(true);
        }
      }

    return result;
  }

  BufferedSerial& men_serial;
  const uint8_t serial_id;

  std::string str;
};

#endif