#include <Servo.h>
#include <SoftwareSerial.h>
#define SW 8
#define BZ 6
#define servo 3
#define LDR 17
#define LED 5//analog
#define LED_2 7
#define SOUND 9
#define SONIC 10
#define trigger_pin 4
#define echo_pin 2

Servo myservo;
SoftwareSerial se_read(12, 13); // write only
SoftwareSerial se_write(10, 11); // read only

struct ProjectData {
  int32_t button_stat;
  int32_t alarm_dashboard;//ส่งไปserver
  int32_t LDR_stat;//for dashboard, ดูความเข้มแสง
  int32_t count_car;//for dashboard
  int32_t LED_stat;
  //int32_t countalarm;
} project_data = {0, 0, 0, 0, 0};

struct ServerData {
  int32_t QR_check;
} server_data;

const char GET_SERVER_DATA = 1;
const char GET_SERVER_DATA_RESULT = 2;
const char UPDATE_PROJECT_DATA = 3;

void light_intense() {
  project_data.LDR_stat = analogRead(LDR);
  if (analogRead(LDR) < 200) {
    analogWrite(LED, 225);
    analogWrite(LED_2,225);
    project_data.LED_stat = 1;
  }
  else {
    analogWrite(LED, 0);
    analogWrite(LED_2,0);
    project_data.LED_stat = 0;
  }
}

  void alarm_func() {
    Serial.println("Alarm!!!");
    myservo.write(0);
    analogWrite(BZ, HIGH);
    digitalWrite(LED, HIGH);
    digitalWrite(LED_2, LOW);
    delay(400);
    digitalWrite(LED, LOW);
    digitalWrite(LED_2, HIGH);
    delay(400);
    digitalWrite(LED, HIGH);
    digitalWrite(LED_2, LOW);
    delay(400);
    digitalWrite(LED, LOW);
    digitalWrite(LED_2, HIGH);
    delay(400);
    digitalWrite(LED, HIGH);
    digitalWrite(LED_2, LOW);
    delay(400);
    digitalWrite(LED, LOW);
    digitalWrite(LED_2, HIGH);
    delay(400);
    digitalWrite(LED, HIGH);
    digitalWrite(LED_2, LOW);
    delay(400);
    digitalWrite(LED, LOW);
    digitalWrite(LED_2, HIGH);
    delay(3000);
    analogWrite(BZ, LOW);
    delay(5000);

  }

  void send_to_nodemcu(char code, void *data, char data_size) {
    char *b = (char*)data;
    char sent_size = 0;
    while (se_write.write(code) == 0) {
      delay(1);
    }
    while (sent_size < data_size) {
      sent_size += se_write.write(b, data_size);
      delay(1);
    }
  }
  void pinmode_func() {
    Serial.begin(115200);
    se_read.begin(38400);
    se_write.begin(38400);
  
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SW, INPUT);
    pinMode(BZ, OUTPUT);
    pinMode(LDR, INPUT);
    pinMode(LED, OUTPUT);
    pinMode(LED_2,OUTPUT);
    pinMode(SOUND, INPUT);
    pinMode(SONIC, INPUT);
    pinMode(trigger_pin, OUTPUT); // Sets the trigger_pin as an Output
    pinMode(echo_pin, INPUT); // Sets the echo_pin as an Input
    myservo.attach(servo);
    myservo.write(0);
    
  }

  void setup() {
    pinmode_func();
    
    while (!se_read.isListening()) {
      se_read.listen();
    }
    Serial.print("ServerData -> ");
    Serial.println((int)sizeof(ServerData));
    Serial.println("ARDUINO READY!");
  }
  int32_t countcar =0;
  int32_t count_alarm = 0;
  bool count_check = true;
  long duration, distance ;
  uint32_t last_sent_time = 0;
  bool is_data_header = false;
  char expected_data_size = 0;
  char cur_data_header = 0;
  char buffer[256];
  int8_t cur_buffer_length = -1;
  int32_t car_lot = 0;
  
  void loop() {
    uint32_t cur_time = millis();
    //send to nodemcu
    if (cur_time - last_sent_time > 500) {//always update
      send_to_nodemcu(GET_SERVER_DATA, &server_data, sizeof(ServerData));
      last_sent_time = cur_time;
    }

    //read from sensor....
    //send to nodemcu
    if (digitalRead(SW) == 0) {
      Serial.print("car sended : ");
      //Serial.print(project_data.button_stat);
      project_data.button_stat = 9;
    }

    if (digitalRead(SW) == 1) {
      project_data.button_stat = 0;
      Serial.println("Security Deactivated");
      myservo.write(0);
      delay(1000);
    }

    //read data from server pass by nodemcu
    while (se_read.available()) {
      char ch = se_read.read();
      if (cur_buffer_length == -1) {
        cur_data_header = ch;
        switch (cur_data_header) {
          case GET_SERVER_DATA_RESULT:
            expected_data_size = sizeof(ServerData);
            cur_buffer_length = 0;
            break;
        }
      } else if (cur_buffer_length < expected_data_size) {
        buffer[cur_buffer_length++] = ch;
        if (cur_buffer_length == expected_data_size) {
          switch (cur_data_header) {
            case GET_SERVER_DATA_RESULT: {
                ServerData *data = (ServerData*)buffer;
                server_data.QR_check = data->QR_check;                 
                Serial.println(server_data.QR_check);
              } break;
          }
          cur_buffer_length = -1;
        }
      }
    }

    light_intense();
    digitalWrite(trigger_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger_pin, LOW);
    duration = pulseIn(echo_pin, HIGH);
    distance = duration * 0.034 / 2;
    delay(100);
    if (distance < 14 && count_check) {
      countcar += 1;
      count_check = !count_check;
      myservo.write(90);
      delay(1000);
    }
    else if (distance >= 14) {
      count_check = true;
      delay(1000);
      myservo.write(0);
      delay(1000);
    }
    project_data.count_car = countcar;
  
  if (server_data.QR_check == 9) { //car parked ,QR pass,security on
    Serial.println("Security Activated");
    delay(1000);
    if (digitalRead(SW) == 1) { //รถออก, ส่งเตือน ,alarmดัง
      project_data.button_stat = 1;
      //Serial.println(digitalRead(SW));
      //Serial.println(project_data.alarm_dashboard);
      //ส่งไปserverแจ้งเตือน
      alarm_func();//ให้alarmดังจนกว่าservoขึ้นจนสุด
    }
  }
  Serial.print("SEND  ");
  send_to_nodemcu(UPDATE_PROJECT_DATA, &project_data, sizeof(ProjectData));
  Serial.println("DONE");
}
