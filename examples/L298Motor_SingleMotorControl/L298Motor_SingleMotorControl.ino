#include <RobotL298N.h>

// Инициализация пинов для управления мотором
L298Motor motor1({9, 8, 7});

void setup() {
  Serial.begin(115200);
  motor1.begin();  // Настройка пинов
  motor1.setTarget(128);  // Задание скорости
}

void loop() {
  motor1.apply(motor1.target(), false);  // Применение скорости без тормоза
  delay(100);  // Задержка для стабилизации
}
