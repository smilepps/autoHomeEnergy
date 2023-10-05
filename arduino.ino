// Подключение библиотек
#include <Wire.h>            // Для работы с I2C
#include <Adafruit_INA226.h> // Для работы с датчиком напряжения INA226

// Подключение датчика температуры к пину A0
const int temperatureSensorPin = A0;

// Подключение реле к пинам 2, 3, 4, 5
const int relayPins[] = {2, 3, 4, 5};
const int numRelays = sizeof(relayPins) / sizeof(relayPins[0]);

// Адрес датчика напряжения INA226
const uint8_t ina226Address = 0x40;

// Создание объекта для работы с датчиком напряжения
Adafruit_INA226 ina226;

// Переключатель состояния на Arduino
const int switchPin = 6;
bool switchState = false;

// Константы задержек
const unsigned long boilerRelayDelay = 1800000; // 30 минут
const unsigned long starterRelayDelay = 1500; // 1.5 секунды
const unsigned long ovenRelayDelay = 3600000; // 1 час
const unsigned long iterationDelay = 1000; // 1 секунда

void setup()
{
  // Инициализация Serial для отладочного вывода
  Serial.begin(9600);

  // Инициализация датчика напряжения
  if (!ina226.begin(ina226Address))
  {
    Serial.println("Не удалось инициализировать датчик напряжения INA226");
    while (1)
      ;
  }

  // Настройка пинов реле как выходов
  for (int i = 0; i < numRelays; i++)
  {
    pinMode(relayPins[i], OUTPUT);
  }

  // Настройка пина переключателя как входа
  pinMode(switchPin, INPUT);
}

void loop()
{
  // Считывание состояния переключателя
  switchState = digitalRead(switchPin);

  // Считывание температуры с датчика
  int temperature = analogRead(temperatureSensorPin);
  float temperatureCelsius = (temperature * 0.48875) - 50.0;

  // Считывание напряжения с датчика INA226
  float voltage = ina226.readBusVoltage();

  Serial.print("  Температура: " + String(temperatureCelsius) + "°C");
  Serial.print("  Напряжение: " + String(voltage) + " V");

  // Управление реле в зависимости от значений температуры и напряжения
  if (voltage > 18.0 && voltage < 23.0)
  {
    if (temperatureCelsius < 10.0 && switchState)
    {
      // Включение реле котла на заданное время
      digitalWrite(relayPins[3], HIGH);
      delay(boilerRelayDelay);
      digitalWrite(relayPins[3], LOW);
    }

    // Включение реле стартера на заданное время
    digitalWrite(relayPins[0], HIGH);
    delay(starterRelayDelay);
    digitalWrite(relayPins[0], LOW);

    delay(iterationDelay); // Задержка

    if (temperatureCelsius < 25.0)
    {
      // Включение реле печки на заданное время
      digitalWrite(relayPins[2], HIGH);
      delay(ovenRelayDelay);
      digitalWrite(relayPins[2], LOW);
    }

    digitalWrite(relayPins[1], LOW);
  }
  else if (temperatureCelsius < 19.0 && switchState)
  {
    if (temperatureCelsius < 10.0 && voltage > 24.0)
    {
      // Включение реле котла на заданное время
      digitalWrite(relayPins[3], HIGH);
      delay(boilerRelayDelay);
      digitalWrite(relayPins[3], LOW);
    }

    // Включение реле стартера на заданное время
    digitalWrite(relayPins[0], HIGH);
    delay(starterRelayDelay);
    digitalWrite(relayPins[0], LOW);

    delay(iterationDelay); // Задержка

    if (temperatureCelsius < 25.0)
    {
      // Включение реле печки на заданное время
      digitalWrite(relayPins[2], HIGH);
      delay(ovenRelayDelay);
      digitalWrite(relayPins[2], LOW);
    }

    digitalWrite(relayPins[1], LOW);
  }
  else
  {
    // Выключение всех реле
    for (int i = 0; i < numRelays; i++)
    {
      digitalWrite(relayPins[i], LOW);
    }
  }

  // Пауза между итерациями
  delay(iterationDelay);
}
