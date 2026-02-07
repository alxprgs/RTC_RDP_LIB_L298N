#include <RobotL298N.h>

// Инициализация моторов для движения по осям
L298Motor motor1({9, 8, 7});
L298Motor motor2({5, 4, 3});
L298Motor motor3({6, 10, 11});
L298Motor motor4({12, 13, 14});

// Два драйвера (для оси X и оси Y)
L298Dual driverX(motor2, motor4);
L298Dual driverY(motor1, motor3);

// Инициализация управления крестом
CrossDrive drive(driverX, driverY);

void setup() {
  Serial.begin(115200);
  drive.begin();
  drive.setSingleDriverMode(false);  // Включение режима "двух драйверов"
  drive.setStrafeEnabled(true);  // Включение бокового движения
}

void loop() {
  drive.command(100, 50, 0);  // Прямое движение с боковым движением
  drive.update(false);  // Обновление состояния
  delay(100);
}
