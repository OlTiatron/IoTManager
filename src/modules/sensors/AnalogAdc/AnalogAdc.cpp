#include "Global.h"
#include "classes/IoTItem.h"

extern IoTGpio IoTgpio;

// Это файл сенсора, в нем осуществляется чтение сенсора.
// для добавления сенсора вам нужно скопировать этот файл и заменить в нем текст AnalogAdc на название вашего сенсора
// Название должно быть уникальным, коротким и отражать суть сенсора.

//ребенок       -       родитель
class AnalogAdc : public IoTItem {
   private:
    //=======================================================================================================
    // Секция переменных.
    // Это секция где Вы можете объявлять переменные и объекты arduino библиотек, что бы
    // впоследствии использовать их в loop и setup
    unsigned int _pin;
    unsigned int _avgSteps, _avgCount;
    unsigned long _avgSumm;

   public:
    //=======================================================================================================
    // setup()
    // это аналог setup из arduino. Здесь вы можете выполнять методы инициализации сенсора.
    // Такие как ...begin и подставлять в них параметры полученные из web интерфейса.
    // Все параметры хранятся в перемененной parameters, вы можете прочитать любой параметр используя jsonRead функции:
    // jsonReadStr, jsonReadBool, jsonReadInt
    AnalogAdc(String parameters) : IoTItem(parameters) {
        _pin = jsonReadInt(parameters, "pin");
        _avgSteps = jsonReadInt(parameters, "avgSteps");
        if (!_avgSteps) {
            jsonRead(parameters, F("int"), _interval, false);
        }
        _avgSumm = 0;
        _avgCount = 0;
    }

    //=======================================================================================================
    // doByInterval()
    // это аналог loop из arduino, но вызываемый каждые int секунд, заданные в настройках. Здесь вы должны выполнить чтение вашего сенсора
    // а затем выполнить regEvent - это регистрация произошедшего события чтения
    // здесь так же доступны все переменные из секции переменных, и полученные в setup
    // если у сенсора несколько величин то делайте несколько regEvent
    // не используйте delay - помните, что данный loop общий для всех модулей. Если у вас планируется длительная операция, постарайтесь разбить ее на порции
    // и выполнить за несколько тактов
    void doByInterval() {
        if (_avgSteps <= 1) value.valD = IoTgpio.analogRead(_pin);
        regEvent(value.valD, "AnalogAdc");  //обязательный вызов хотяб один
    }

    //=======================================================================================================
    // loop()
    // полный аналог loop() из arduino. Нужно помнить, что все модули имеют равный поочередный доступ к центральному loop(), поэтому, необходимо следить
    // за задержками в алгоритме и не создавать пауз. Кроме того, данная версия перегружает родительскую, поэтому doByInterval() отключается, если
    // не повторить механизм расчета интервалов.
    void loop() {
        if (_avgSteps > 1) {
            if (_avgCount > _avgSteps) {
                value.valD = _avgSumm / _avgSteps;
                _avgSumm = 0;
                _avgCount = 0;
            }

            _avgSumm = _avgSumm + IoTgpio.analogRead(_pin);
            _avgCount++;
        }

        currentMillis = millis();
        difference = currentMillis - prevMillis;
        if (difference >= _interval) {
            prevMillis = millis();
            this->doByInterval();
        }
    }

    ~AnalogAdc(){};
};

// после замены названия сенсора, на функцию можно не обращать внимания
// если сенсор предполагает использование общего объекта библиотеки для нескольких экземпляров сенсора, то в данной функции необходимо предусмотреть
// создание и контроль соответствующих глобальных переменных
void* getAPI_AnalogAdc(String subtype, String param) {
    if (subtype == F("AnalogAdc")) {
        return new AnalogAdc(param);
    } else {
        return nullptr;
    }
}
