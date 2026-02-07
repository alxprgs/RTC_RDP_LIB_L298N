#include <RobotL298N.h>

// Инициализация пинов для управления моторами
L298Motor motor1({9, 8, 7});
L298Motor motor2({5, 4, 3});
L298Dual driver(motor1, motor2);

void setup() {
  Serial.begin(115200);
  driver.begin();  // Инициализация драйвера с двумя моторами
}

void loop() {
  driver.setTargetA(100);  // Установка скорости для первого мотора
  driver.setTargetB(150);  // Установка скорости для второго мотора
  driver.update(false);  // Применение изменения
  delay(100);
}
