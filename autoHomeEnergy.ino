#include <Wire.h>                 // Для работы с I2C
#include <GyverINA.h>             // Для работы с датчиком напряжения INA226
#include <Adafruit_Fingerprint.h> // Для работы со ёмкостным сканером отпечатков пальцев R503

// Подключение датчика температуры к пину A0
const int temperatureSensorPin = A0;

// Подключение реле к пинам 2, 3, 4, 5
const int relayPins[] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numRelays = sizeof(relayPins) / sizeof(relayPins[0]);

// Создание объекта для работы с датчиком напряжения
INA226 ina226;

// Переключатель состояния на Arduino
const int switchPin = 6;
bool switchState = false;

// Константы задержек
const unsigned long boilerRelayDelay = 1800000; // 30 минут
const unsigned long starterRelayDelay = 1500;   // 1.5 секунды
const unsigned long ovenRelayDelay = 3600000;   // 1 час
const unsigned long iterationDelay = 1000;      // 1 секунда

// Пины для подключения ёмкостного сканера отпечатков пальцев R503
const int fingerprintSensorRX = 10;
const int fingerprintSensorTX = 11;
const int fingerprintSensorWAKEUP = 12;

// Флаг, указывающий, что прерывание от сканера отпечатков пальцев произошло
volatile bool fingerprintInterruptFlag = false;

// Создание объекта для работы со ёмкостным сканером отпечатков пальцев R503
Adafruit_Fingerprint fingerprintSensor = Adafruit_Fingerprint(&Serial);

void setup()
{
  // Инициализация Serial для отладочного вывода
  Serial.begin(9600);

  // Инициализация датчика напряжения
  ina226.begin();

  // Настройка пинов реле как выходов
  for (int i = 0; i < numRelays; i++)
  {
    pinMode(relayPins[i], OUTPUT);
  }

  // Настройка пина переключателя поддержания температуры как входа
  pinMode(switchPin, INPUT);

  // Настройка пинов для соединения с ёмкостным сканером отпечатков пальцев R503
  SoftwareSerial mySerial(fingerprintSensorRX, fingerprintSensorTX);
  mySerial.begin(57600);
  fingerprintSensor.begin(mySerial);

  if (fingerprintSensor.verifyPassword())
  {
    Serial.println("Сканер отпечатков пальцев найден!");
  }
  else
  {
    Serial.println("Сканер отпечатков пальцев не найден. Проверьте подключение.");
    while (1)
      ;
  }
}

void loop()
{
  // Считывание состояния переключателя
  switchState = digitalRead(switchPin);

  // Считывание температуры с датчика
  int temperature = analogRead(temperatureSensorPin);
  float temperatureCelsius = (temperature * 0.48875) - 50.0;

  // Считывание напряжения с датчика INA226
  float voltage = ina226.getVoltage();

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

  // Проверка флага прерывания от сканера отпечатков пальцев
  if (fingerprintInterruptFlag)
  {
    fingerprintInterruptFlag = false;
    // Ваш код для обработки прерывания от сканера отпечатков пальцев R503
    // Например, проверка идентификации отпечатка пальца и управление реле
  }

  // Пауза между итерациями
  delay(iterationDelay);
}
// Обработчик прерывания для сканера отпечатков пальцев
void fingerprintInterrupt()
{
  fingerprintInterruptFlag = true;

  // Проверка количества отпечатков пальцев в базе данных
  if (fingerprintSensor.getTemplateCount() < 5)
  {
    addFingerprints();
  }
  else
  {
    // Ожидание, пока палец не будет обнаружен
    while (!fingerprintSensor.getImage())
    {
      delay(50);
    }

    // Создание изображения отпечатка пальца
    int result = fingerprintSensor.image2Tz();
    if (result != FINGERPRINT_OK)
    {
      Serial.println("Ошибка создания изображения отпечатка пальца");
      return;
    }

    // Поиск отпечатка пальца в базе данных
    result = fingerprintSensor.fingerFastSearch();
    if (result == FINGERPRINT_OK)
    {
      uint8_t fingerID = fingerprintSensor.fingerID;
      Serial.print("Идентификатор пальца: ");
      Serial.println(fingerID);
      switch (fingerID)
      {
      case 1:
        addFingerprints();
        break;
      case 2:
      case 3:
        // Включение реле открытия главной двери
        digitalWrite(relayPins[6], HIGH);
        delay(100);
        digitalWrite(relayPins[6], LOW);
        break;
      case 4:
      case 5:
        // Выключение реле открытия главной двери
        digitalWrite(relayPins[7], HIGH);
        delay(100);
        digitalWrite(relayPins[7], LOW);
        break;
      case 6:
      case 7:
        // Включение реле открытия задней двери
        digitalWrite(relayPins[8], HIGH);
        delay(100);
        digitalWrite(relayPins[8], LOW);
        break;
      case 8:
      case 9:
        // Выключение реле открытия задней двери
        digitalWrite(relayPins[9], HIGH);
        delay(100);
        digitalWrite(relayPins[9], LOW);
        break;
      default:
        break;
      }
    }
  }
}
void addFingerprints()
{
  // Очистка базы данных и переход в режим добавления отпечатков
  fingerprintSensor.emptyDatabase();

  for (int i = 1; i < 10; i++)
  {
    addFingerprint(1);
  }
}
void addFingerprint(uint8_t fingerID)
{
  // Ожидание, пока палец не будет обнаружен
  while (!fingerprintSensor.getImage())
  {
    delay(50);
  }

  // Создание изображения отпечатка пальца
  int result = fingerprintSensor.image2Tz();
  if (result != FINGERPRINT_OK)
  {
    Serial.println("Ошибка создания изображения отпечатка пальца");
    return;
  }

  // Сохранение отпечатка пальца в базе данных
  result = fingerprintSensor.storeModel(fingerID);
  if (result == FINGERPRINT_OK)
  {
    Serial.println("Отпечаток пальца успешно добавлен в базу данных");
  }
  else if (result == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Ошибка связи с датчиком отпечатков пальцев");
  }
  else if (result == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Ошибка сохранения отпечатка пальца в базе данных");
  }
  else if (result == FINGERPRINT_FLASHERR)
  {
    Serial.println("Ошибка записи во флэш-память");
  }
  else
  {
    Serial.println("Неизвестная ошибка при добавлении отпечатка пальца");
  }
}
