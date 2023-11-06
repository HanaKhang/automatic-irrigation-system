#include <WiFi.h>  //thư viện cho kết nối wifi
#include <string.h>
#include <PubSubClient.h>  //thư viện MQTT
#include <DHT.h>           //thư viện cảm biến nhiệt độ DHT11

//Cấu hình Wifi và MQTT
const char *ssid = "Hana";
const char *password = "66622888";
// const char *ssid = "Sinh vien Thuy Loi";
// const char *password = "Thuyloi2023";
  // const char *ssid = "TLU -CHUYENDOISO";
  // const char *password = "chuyendoiso@2023tlus";
  const char *mqtt_server = "phuongnamdts.com";
  const int mqtt_port = 4783;
  const char *mqtt_user = "baonammqtt";
  const char *mqtt_pass = "mqtt@d1git";

//Các cổng chủ đề trong MQTT
  const char *root_topic_subscribe = "cmnd/SOILSENSOR/command";
  const char *root_topic_publish = "HQT/SOILSENSOR/value";
//Đối tượng kết nối Wifi và MQTT
  WiFiClient espClient;
  PubSubClient client(espClient);

//Các chân cảm biến
  const int relayPin1 = 18;//19
  // const int relayPin2 = 18;
  // const int flowsensorPin = 35;
  const int soilsensorPin = 26;//27-xanh dương
  const int dht22 = 27;//26-xanh lá 

//Đối tượng cảm biến DHT
  DHT dht(dht22, DHT22);

//Biến lưu thời điểm lần cuối gửi dữ liệu lên MQTT
  unsigned long currentTimer = 0;
  unsigned long relayStartTime = 0;  //thời gian bắt đầu chạy relay máy bơm
  // unsigned long XungStartTime = 0;
  unsigned long DHTStartTime = 0;
  unsigned long SoilStartTime = 0;
  unsigned long MQTTStartTime = 0;
//Khoảng thời gian thiết lập giữa các lần gửi dữ liệu
  const int interval = 60;
  const int relayinterval = 10;
  const int Soilinterval = 10;
  const int DHTinterval = 10;
  const int MQTTinterval = 10;
  // const int Xunginterval = 10;

//biến lưu số lần ngắt từ cảm biến flow
// volatile unsigned long pulseCount = 0;

// Biến so sánh
  //relay máy bơm
    unsigned long elapsedRelayTime1 = 0;
    unsigned long elapsedRelayTime2 = 0;
  // các biến so sánh còn lại
    unsigned long elapsedDHTTime = 0;
    // unsigned long elapsedXungTime = 0;
    unsigned long elapsedSoilTime = 0;
    unsigned long elapsedMQTTTime = 0;

//Biến quy định, kiểm tra trang thái relay
  int isRelayON = LOW;
// biến chuỗi khai báo trạng thái độ ẩm DHT11
  char sensordht[15];
  char soilMoisture;
  // float flowRate;
  // char flowRateStr[15];
  char soilMoistureStr[20];
  bool isbusy = false;
//hàm tăng xung 
// void pulseCounter() {
//   pulseCount++;
// }
void setup() {
  // Khởi tạo cổng giao tiếp Serial, kết nối WiFi và cài đặt cổng cho cảm biến
    Serial.begin(9600);
    setup_wifi();
  //đèn D2 của mạch
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
  //thiết lập kết nối MQTT
    client.setKeepAlive(90);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
  //Gắn ngắt cho cảm biến flow
    // pinMode(flowsensorPin, INPUT);
    // attachInterrupt(digitalPinToInterrupt(flowsensorPin), pulseCounter, RISING);
  //cài đặt chân relay
  //relay máy bơm
    pinMode(relayPin1, OUTPUT);
    digitalWrite(relayPin1, LOW);
  //relay đo lưu lượng
    // pinMode(relayPin2, OUTPUT);
    // digitalWrite(relayPin2, LOW);
  //khởi động cảm biến DHT11
    pinMode(dht22, INPUT);
    digitalWrite(dht22, HIGH);
    dht.begin();
}

void loop() {
  //khai báo các biến lưu thời gian cảm biến
  currentTimer++;
  elapsedRelayTime1 = (currentTimer - relayStartTime);
  elapsedDHTTime = (currentTimer - DHTStartTime);
  // elapsedXungTime = (currentTimer - XungStartTime);
  elapsedSoilTime = (currentTimer - SoilStartTime);
  elapsedMQTTTime = (currentTimer - MQTTStartTime);


  // kiểm tra thời gian để cập nhật trang thái relay máy bơm và gửi dữ liệu lên MQTT
  if (elapsedRelayTime1 >= interval && isRelayON == LOW) {
    relayStartTime = currentTimer;
    digitalWrite(relayPin1, HIGH);
    // digitalWrite(relayPin2, HIGH);
    isRelayON = HIGH;
    isbusy = true;
    Serial.println("Bật relay bơm");
  }
  elapsedRelayTime2 = (currentTimer - relayStartTime);
  // Kiểm tra nếu đã đủ thời gian bơm (10 giây)
  if (elapsedRelayTime2 >= relayinterval && isRelayON == HIGH) {
    digitalWrite(relayPin1, LOW);  // Tắt relay sau khi đã bơm đủ thời gian
    // digitalWrite(relayPin2, LOW);
    isRelayON = LOW;
    isbusy = false;
    Serial.println("tắt relay bơm");
    relayStartTime = currentTimer;
  }

  // Đọc dữ liệu từ cảm biến DHT và gửi lên MQTT
  if (elapsedDHTTime >= DHTinterval) {  // Cập nhật sau thời gian đã setup
    //Đọc nhiệt độ từ cảm biến DHT11
    float temp = dht.readTemperature();
    snprintf(sensordht, sizeof(sensordht), "%.2f", temp);
    DHTStartTime = currentTimer;
  }


  //Đọc số xung từ cảm biến flow và tính lưu lượng
  // if (elapsedXungTime >= Xunginterval) {
  //   noInterrupts();
  //   //tính lưu lượng
  //   flowRate = pulseCount / 7.5;
  //   pulseCount = 0;
  //   Serial.println("Xung");
  //   interrupts();
  //   XungStartTime = currentTimer;
  //   snprintf(flowRateStr, sizeof(flowRateStr), "%.2f", flowRate);
  // }
  
  // Đọc độ ẩm đất từ cảm biến độ ẩm đất
  if (elapsedSoilTime >= Soilinterval) {
    soilMoisture = digitalRead(soilsensorPin);
    Serial.print("Soil Moisture");
    Serial.println(String(soilMoisture, HEX));
    sprintf(soilMoistureStr, "%d", soilMoisture);
    SoilStartTime = currentTimer;
  }
  // Kiểm tra độ ẩm đất và điều khiển relay
  if (!isbusy) {
    if (soilMoisture == 1 && isRelayON == HIGH) {
      digitalWrite(relayPin1, HIGH);
      // digitalWrite(relayPin2, HIGH);
    } else {
      digitalWrite(relayPin1, LOW);
      // digitalWrite(relayPin2, LOW);
    }
  }

  // Kiểm tra kết nối MQTT và gửi dữ liệu
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected() && elapsedMQTTTime >= MQTTinterval) {
    char msg[36];
    // String str = String(flowRateStr) + ";" + String(soilMoistureStr) + ";" + String(sensordht);
    String str = String(soilMoistureStr) + ";" + String(sensordht);
    str.toCharArray(msg, 36);
    //Gửi dữ liệu lên MQTT
    client.publish(root_topic_publish, msg);
    MQTTStartTime = currentTimer;
  }
  delay(1000);
  //lặp lại việc nhận và gửi dữ liệu từ MQTT
  client.loop();
}

// Thiết lập kết nối WiFi
void setup_wifi() {
  // delay(10);
  Serial.println();
  Serial.print("Dang ket noi wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Da ket noi!");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}
// Kết nối lại khi mất kết nối MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Ket noi den server MQTT...");
    String clientId = "ESP32_WROOM_KHANG";

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Da ket noi MQTT!");

      if (client.subscribe(root_topic_subscribe)) {
        Serial.println("Dang ky OK");
      } else {
        Serial.println("Loi dang ky");
      }
    } else {
      Serial.print("loi ket noi -> ");
      Serial.print(client.state());
      Serial.println(" thu lai sau 5 giay ");
      delay(5000);
    }
  }
}
//Hàm xử lý khi nhận được tin nhắn từ MQTT gửi
void callback(char *topic, byte *payload, unsigned int length) {
  String incoming = "";
  Serial.print("Du lieu duoc ghi vao -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Dang doc du lieu -> " + incoming);

  if (incoming == "ON") {
    digitalWrite(relayPin1, HIGH);
    // digitalWrite(relayPin2, HIGH);
    delay(10000);
    digitalWrite(relayPin1, LOW);
    // digitalWrite(relayPin2, LOW);
  } else if (incoming == "OFF") {
    digitalWrite(relayPin1, LOW);
    // digitalWrite(relayPin2, LOW);
  } else {
    digitalWrite(relayPin1, LOW);
    // digitalWrite(relayPin2, LOW);
  }
}
