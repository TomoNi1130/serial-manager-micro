# serial_manager マイコン側 仕様

複数マイコンを同時に管理するために作った。

## 基本仕様

- マイコンの識別にはID(1~254)を使っている。0は使えない
- PC・マイコンどちらを初期化しても自動的にIDの特定、通信の接続が行われる
- 必要なピンを指定するとあとからIDの確認、変更ができる
- float,boolをデータとして送信可能

## 送信
- `SerialMsg`構造体でデータ送信が可能
- `send_log(std::string)` でログを送信
- `send_msg(SerialMsg)` で情報を送信、topicで公開される

## 受信

- pcから受け取ったデータは `received_flags`(bool),`received_nums`(float) に格納される
- 値を取得したい場合はインスタンスから直接取得してください  
  （従来のように変数へコピーして使う方式はできない）


## IDの確認・変更

- インスタンス宣言時にピンを指定すると、IDの確認・変更機能が有効
- 一度ボタンを押すと「セットIDモード」になり、その後ボタンを押した回数がIDとなる
- 1.5秒間押さずにいるとセットIDモードが終了、IDの変更が適応される。


## 注意事項

- `log` と `msg` はバス詰まり対策のため、**一定間隔（5ms）を空けて送信**される
- 同時に送信すると、送られない場合がある


## 導入方法

- ROS側の手順：[ros2_serial_manager README](https://github.com/TomoNi1130/ros2_serial_manager/blob/main/README.md)
- `serial_manager` を `lib` フォルダに入れてインクルード
- `SerialManager` クラスのインスタンスを作成  
  （引数：`mbed::BufferedSerial`,`ID`）-> IDの表示変更不可  
                or  
   (引数：`mbed::BufferedSerial`, ID表示用のledピン , 変更用のユーザーボタンピン)->初期IDなし(ソフトリセットしても現在の値を保持)  
   or  
    (引数：`mbed::BufferedSerial`, ID表示用のledピン , 変更用のユーザーボタンピン)->初期IDあり(ソフトリセットすると初期に戻る)  

## 小ネタ
- 実はpc側でserial_managerが立ち上がっていなければ何も送らない。  
- なので、あろうがなかろうがほぼ変わらない状態になる。