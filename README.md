# RTC_RDP_LIB_L298N - RobotL298N (API & поведение)

`RTC_RDP_LIB_L298N` - Arduino-библиотека управления **L298N**
- плавный старт/стоп (меньше рывков → меньше пиковых токов)
- ограничение мощности (чтобы L298N и питание не умирали)
- “умный” дерейтинг (сам подрежет PWM при длительной нагрузке)
- режим **Single Driver** (в один момент времени активен только один L298N-модуль)
- **Safety Timeout** (если команда не пришла вовремя → робот остановится)
- телеметрия (heatX/heatY + текущий лимит мощности)

> ⚙️ Концепция: **Arduino Mega = исполнитель** (моторы), **Raspberry Pi = мозг** (логика/камера/авто‑сценарии).  
> Pi шлёт команды по Serial, Mega их применяет через эту библиотеку.

---

## 0) Быстрый смысл пинов L298N

Каждый L298N содержит **2 H‑bridge канала**:

### Канал A (Motor A)
- `ENA` - **скорость / включение** (PWM)
- `IN1, IN2` - направление/режим

### Канал B (Motor B)
- `ENB` - **скорость / включение** (PWM)
- `IN3, IN4` - направление/режим

### Режимы управления мотором

| speed | IN1 | IN2 | EN (PWM) | Режим |
|------:|:---:|:---:|:--------:|------|
| >0    |  1  |  0  | 0..255   | вперёд |
| <0    |  0  |  1  | 0..255   | назад |
| =0    |  0  |  0  | 0        | **coast** (свободный выбег) |
| =0 + brake | 1 | 1 | 255 | **brake** (активный стоп) |

> ⚠️ Важно: для **brake** `EN` должен быть **включён**, иначе это будет coast.

---

## 1) Подключение и файлы

### Подключение в скетче

```cpp
#include <RobotL298N.h>
```

Файлы библиотеки:
- `src/RobotL298N.h`
- `src/RobotL298N.cpp`

---

## 2) Классы и API

## 2.1 `L298Motor` - один мотор (1 канал L298N)

Создание:
```cpp
L298Motor m({EN, IN1, IN2});
```

Методы:
- `begin()`  
  Настраивает пины как OUTPUT и останавливает мотор (coast).

- `setTarget(int16_t speed)`  
  Задаёт целевую скорость `-255..255`.

- `target()`  
  Возвращает целевую скорость.

- `applied()`  
  Возвращает применённую скорость (после ramp/лимитов).

- `setInverted(bool inv)`  
  Инвертирует направление.

> ⚠️ `L298Motor` сам по себе не делает “плавность”. Плавность делает `PowerManager` через `CrossDrive.update()`.

---

## 2.2 `L298Dual` - один модуль L298N (2 мотора A и B)

Создание:
```cpp
L298Dual drv(motorA, motorB);
```

Методы:
- `begin()`
- `setTargetA(speed)` / `setTargetB(speed)`
- `stop(brake=false)`  
  `brake=false` → coast, `brake=true` → brake

---

## 2.3 `PowerManager` - плавность и защита по мощности

Это “мозг” по мощности, он делает:
- ramp (плавное приближение applied к target)
- power limit (общая “верхушка” PWM)
- eco mode (быстро режет лимит примерно на -25%)
- derating (виртуальный нагрев → снижение лимита)

Методы:

### `setPowerLimitPercent(uint8_t p)`
`p = 10..100`  
Ограничивает максимальную мощность по PWM.

Пример: 70% означает, что target=255 превратится в PWM≈178.

```cpp
drive.power().setPowerLimitPercent(70);
```

### `setRampPwmPerSec(uint16_t v)`
Скорость изменения PWM в единицах PWM в секунду.

- 500 → очень мягко
- 900 → универсально
- 1500 → почти как “без плавности”

```cpp
drive.power().setRampPwmPerSec(900);
```

### `setEcoMode(bool on)`
Дополнительно снижает лимит (примерно -25%).

```cpp
drive.power().setEcoMode(true);
```

### `setDerating(bool on)`
Авто‑дерейтинг: если долго жарим на больших PWM → лимит сам опустится.

```cpp
drive.power().setDerating(true);
```

---

## 2.4 `CrossDrive` - управление 4 моторами “крестом”

Конфигурация моторов:
- M1 - верх‑лево
- M2 - верх‑право
- M3 - низ‑право
- M4 - низ‑лево

Подключение драйверов:
- `DriverX = (M2 + M4)`
- `DriverY = (M1 + M3)`

Создание:
```cpp
CrossDrive drive(driverX, driverY);
drive.begin();
```

### `command(forward, strafe, rotate)`
Основная команда движения:

- `forward`  `-255..255`  (вперёд/назад)
- `strafe`   `-255..255`  (вбок)
- `rotate`   `-255..255`  (поворот)

Пример:
```cpp
drive.command(180, 0, 0);    // вперёд
drive.command(0, 0, 160);    // поворот вправо
drive.command(-140, 0, 0);   // назад
```

### `update(brakeIfZero=false)`
Обязательный вызов в `loop()`.  
Применяет ramp/лимиты/дерейтинг к моторам.

```cpp
void loop() {
  // ...парсинг Serial команд...
  drive.update(false);
}
```

Если нужен “жёсткий стоп” при speed=0:
```cpp
drive.update(true);
```

### `setMotorInvert(motorId, inv)`
Инвертирует конкретный мотор `1..4`.

```cpp
drive.setMotorInvert(3, true); // M3 крутится “не туда”
```

### `setSingleDriverMode(bool on)`
Режим “в один момент времени активен только один драйвер”.

- при `true`: если есть strafe → работает DriverX, иначе DriverY

```cpp
drive.setSingleDriverMode(true);
```

### `setStrafeEnabled(bool on)`
Разрешает/запрещает боковое движение.

```cpp
drive.setStrafeEnabled(false);
```

### Настройки мощности
```cpp
drive.power().setPowerLimitPercent(80);
drive.power().setRampPwmPerSec(900);
drive.power().setEcoMode(false);
drive.power().setDerating(true);
```

---

## 3) Safety Timeout (Fail‑safe)

Чтобы робот не уехал навсегда при обрыве связи:

- если команда не пришла **дольше 300 мс** → `CrossDrive` делает **coast stop** (моторы выключены)

По умолчанию включено:
- `enabled = true`
- `timeout = 300ms`

Настройка:

```cpp
drive.setSafetyTimeoutEnabled(true);
drive.setSafetyTimeoutMs(300);
```

Важно:
- если вы отправляешь команды, например, раз в 50–100 мс - всё в порядке
- если команды редкие (раз в секунду) - робот будет останавливаться между ними

---

## 4) Телеметрия (для Raspberry Pi)

Доступно:

```cpp
uint16_t hx = drive.getHeatX();
uint16_t hy = drive.getHeatY();

uint8_t  limX = drive.getCurrentLimitX();
uint8_t  limY = drive.getCurrentLimitY();
uint8_t  lim  = drive.getCurrentLimit();
```

Смысл:
- `heatX/heatY` - “виртуальный нагрев” модулей (0..60000)
- `getCurrentLimit*()` - текущий “потолок” PWM после powerLimit/ECO/derating (0..255)

Можно отдавать на Pi как `STAT HX HY LIMX LIMY`.

---

## 5) Правильное использование (RAMP)

**Золотое правило:**  
Мы НЕ делаем `analogWrite()` сами.  
Мы задаём `command(...)`, а в `loop()` вызываем `update()`.

### Пример “правильно”
```cpp
drive.command(200, 0, 0);
drive.update();
```

Что будет происходить:
- target станет 200
- applied будет плавно увеличиваться: 0 → 30 → 60 → 90 → ... → 200

---

## 6) Рекомендации по Power Limit / режимам

- **80%** - почти без потери динамики, но уже легче драйверу
- **60–70%** - если греется / батарея проседает
- **40–50%** - режим выживания

ECO Mode включай когда:
- батарея садится
- робот тяжёлый
- драйверы горячие

Derating полезно почти всегда:
```cpp
drive.power().setDerating(true);
```

---

## 7) Ограничения и грабли

### 7.1 ENA/ENB должны быть PWM
Иначе `analogWrite()` превратится в ON/OFF.

### 7.2 Джамперы ENA/ENB на L298N
Чтобы PWM работал - надо снять джамперы ENA/ENB на каждом модуле.

### 7.3 Servo.h и PWM на Mega
Если используется `Servo.h` на Mega, **не используйте PWM пины 44/45/46** под ENA/ENB (Timer5).
Лучше EN‑пины в диапазоне **2–13**.

---

## 8) Мини‑пример (интеграция)

```cpp
#include <RobotL298N.h>

// DriverX = M2 + M4
L298Motor m2({2,  26, 27});
L298Motor m4({3,  28, 29});
L298Dual driverX(m2, m4);

// DriverY = M1 + M3
L298Motor m1({10, 22, 23});
L298Motor m3({11, 24, 25});
L298Dual driverY(m1, m3);

CrossDrive drive(driverX, driverY);

void setup() {
  Serial.begin(115200);
  drive.begin();

  drive.setSingleDriverMode(true);
  drive.setStrafeEnabled(false);

  drive.setSafetyTimeoutEnabled(true);
  drive.setSafetyTimeoutMs(300);

  drive.power().setPowerLimitPercent(80);
  drive.power().setRampPwmPerSec(900);
  drive.power().setEcoMode(false);
  drive.power().setDerating(true);

  Serial.println("READY");
}

void loop() {
  // Тут должен быть Serial парсер от Raspberry Pi.
  // Пример: допустим получили: forward=180 rotate=0
  drive.command(180, 0, 0);

  // применяем плавность + защиту
  drive.update(false);

  // телеметрия (пример)
  // Serial.print("STAT ");
  // Serial.print(drive.getHeatX()); Serial.print(' ');
  // Serial.print(drive.getHeatY()); Serial.print(' ');
  // Serial.print(drive.getCurrentLimitX()); Serial.print(' ');
  // Serial.println(drive.getCurrentLimitY());
}
```

---

## Что библиотека гарантирует

✅ Плавное изменение PWM (уменьшение пикового тока)  
✅ Лимит общей мощности (защищает L298N и питание)  
✅ Возможность включать/выключать режимы “на лету”  
✅ Режим “не более 1 драйвера одновременно” (Single Driver)  
✅ Инверсия моторов без перепайки  
✅ Safety Timeout (если связь пропала → остановка)  
✅ Телеметрия (heat + текущий лимит мощности)