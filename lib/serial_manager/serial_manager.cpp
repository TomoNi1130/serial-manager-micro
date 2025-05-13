#include "serial_manager.hpp"

SerialMsg::SerialMsg()
{
  numbers = {0.0};
  flags = {0};
  str_msgs = {"0"};
}

void SerialManager::send_msg(const SerialMsg &send_msg)
{
  std::string send_str;
  send_str += "n";
  for (float number : send_msg.numbers)
    send_str += ":" + std::to_string(number);
  send_str += "b";
  for (bool flag : send_msg.flags)
  {
    if (flag)
      send_str += ":1";
    else
      send_str += ":0";
  }
  send_str += "s";
  for (std::string str : send_msg.str_msgs)
    send_str += ":" + str;
  send_str += "\n";
  printf("[inf]%s", send_str.c_str());
}

SerialManager::SerialManager(BufferedSerial &serial, uint8_t id) : men_serial(serial), serial_id(id) { printf("[new]%d\n", serial_id); } // マイコンのidを送る

void SerialManager::send_msg(const std::string &send_str) { printf("[inf]s:%s\n", send_str.c_str()); }

void SerialManager::send_log(const std::string &log_msg) { printf("[log]%s\n", log_msg.c_str()); }

std::optional<SerialMsg> SerialManager::receive_msg()
{
  char buf[1];
  SerialMsg zero_msg;
  while (men_serial.readable())
  {
    men_serial.read(buf, 1);
    str += buf[0];
    if (str.find("\n") != std::string::npos)
    {
      str.erase(str.length() - 2, str.length());
      size_t num_start = str.find("n");
      size_t flag_start = str.find("b");
      size_t str_start = str.find("s");
      std::string num_str = str.substr(num_start + 1, flag_start - num_start - 1);
      std::string flag_str = str.substr(flag_start + 1, str_start - flag_start - 1);
      std::string msg_str = str.substr(str_start + 1, str.size() - str_start); // nbsのぶんずらす
      str.clear();
      SerialMsg return_msg(split_strring<float>(num_str), split_strring<bool>(flag_str), split_strring<std::string>(msg_str));
      return return_msg;
    }
  }
  return std::nullopt;
}