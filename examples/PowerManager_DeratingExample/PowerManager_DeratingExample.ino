#include <RobotL298N.h>

// Инициализация пинов для моторов
L298Motor motor1({9, 8, 7});
L298Motor motor2({5, 4, 3});
L298Dual driver(motor1, motor2);

// Создание менеджера мощности
PowerManager powerManager;

void setup() {
  Serial.begin(115200);
  driver.begin();
  
  powerManager.setPowerLimitPercent(80);  // Ограничение мощности до 80%
  powerManager.setEcoMode(true);  // Включение режима энергосбережения
  powerManager.setDerating(true);  // Включение автоматического дерейтинга
}

void loop() {
  powerManager.updateModule(driver, 100, false, 0);  // Обновление состояния с дерейтом
  driver.update(false);  // Применение изменений
  delay(100);
}
