#include <RobotL298N.h>

// Инициализация пинов для моторов
L298Motor motor1({9, 8, 7});
L298Motor motor2({5, 4, 3});
L298Dual driver(motor1, motor2);

// Драйвер крестового типа
CrossDrive drive(driver, driver);

void setup() {
  Serial.begin(115200);
  drive.begin();
  drive.setSafetyTimeoutEnabled(true);  // Включение безопасного тайм-аута
  drive.setSafetyTimeoutMs(500);  // Тайм-аут 500 мс
}

void loop() {
  drive.command(180, 0, 0);  // Движение вперед
  drive.update(false);  // Применение изменений
  delay(1000);  // Задержка для симуляции задержки в отправке команды

  drive.command(0, 0, 0);  // Остановка
  drive.update(true);  // Применение тормоза
  delay(1000);
}
