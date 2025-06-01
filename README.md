serial_managerのマイコン側の簡単な使用例

SerialMsg構造体で送れる

send_log(std::string)でログが送れる
send_msg(SerialMsg)で情報を送れる

pc、マイコンどちらを初期化しても自動的に接続準備がされる。

受け取ったデータはreceived_msg変数に収納される
値を取りたいときはインスタンスから取るようにする。
従来のように変数に入れて使うわけでないので注意

-注意-
logとmsgはバスづまり対策のため一定間隔を開けて送信される。
同時に送ると送られない場合がある。(5ms間隔)

-導入方法-
ROS側->https://github.com/TomoNi1130/ros2_serial_manager/blob/main/README.md

serial_manager.hppをlibにファイルを入れてインクルードする。
SerialManagerクラスのインスタンスを作る (引数) -> mbed::buffer_serial,ID
-送信
send_msg -> topicで公開される
send_log -> log表示
-受信
インスタンス.received_flags or インスタンス.received_nums から送られた値を読む