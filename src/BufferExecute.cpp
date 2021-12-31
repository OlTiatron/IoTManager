#include "BufferExecute.h"

#include "Global.h"
#include "SoftUART.h"
#include "items/test.h"
#include "items/vCountDown.h"
#include "items/vImpulsOut.h"
#include "items/vInput.h"
#include "items/vLogging.h"
#include "items/vOutput.h"
#include "items/vPwmOut.h"
#include "items/vSensorAnalog.h"
#include "items/vSensorAny.h"
#include "items/vSensorBme280.h"
#include "items/vSensorBmp280.h"
#include "items/vSensorCcs811.h"
#include "items/vSensorDht.h"
#include "items/vSensorNode.h"
#include "items/vSensorPzem.h"
#include "items/vSensorUltrasonic.h"
#include "items/vSensorUptime.h"

#include "Class/LineParsing.h"
#include "Utils/JsonUtils.h"

#include "Class/IoTModule.h"
#include "Class/IoTSensor.h"
#include "Class/IoTVariable.h"

extern std::vector<IoTModule*> iotModules;  //v3dev: вектор ссылок базового класса IoTModule - интерфейсы для общения со всеми поддерживаемыми системой модулями
extern std::vector<IoTSensor*> iotSensors;  //v3dev: вектор ссылок базового класса IoTSensor - список всех запущенных сенсоров
extern std::vector<IoTVariable*> iotVariables;  //v3dev: вектор ссылок базового класса IoTVariable - список всех подготовленных переменных

void loopCmdAdd(const String& cmdStr) {
    if (cmdStr.endsWith(",")) {
        orderBuf += cmdStr;
        SerialPrint("I", F("Order add"), cmdStr);
#ifdef EnableUart
        if (jsonReadBool(configSetupJson, "uart")) {
            if (jsonReadBool(configSetupJson, "uartEvents")) {
                if (myUART) {
                    myUART->print(cmdStr);
                    SerialPrint("I", F("<=UART"), cmdStr);
                }
            }
        }
#endif
    }
}

void loopCmdExecute() {
    if (orderBuf.length()) {
        String tmp = selectToMarker(orderBuf, ",");  //выделяем первую команду rel 5 1,
        sCmd.readStr(tmp);                           //выполняем
        if (tmp != "") {
            sCmd.readStr(tmp);
            SerialPrint("I", F("Order done L"), tmp);
        }
        orderBuf = deleteBeforeDelimiter(orderBuf, ",");  //осекаем
    }
}

void fileCmdExecute(const String& filename) {
    String cmdStr = readFile(filename, 4096);
    csvCmdExecute(cmdStr);
}

void csvCmdExecute(String& cmdStr) {
    cmdStr.replace(";", " ");
    cmdStr += "\r\n";
    cmdStr.replace("\r\n", "\n");
    cmdStr.replace("\r", "\n");
    int count = 0;
    while (cmdStr.length()) {
        String buf = selectToMarker(cmdStr, "\n");

        buf = deleteBeforeDelimiter(buf, " ");  //отсечка чекбокса

        count++;

        if (count > 1) {
            // SerialPrint("I", "Items", buf);
            String order = selectToMarker(buf, " ");  //отсечка самой команды
            
            if (order == F("pwm-out")) {
#ifdef EnablePwmOut
                sCmd.addCommand(order.c_str(), pwmOut);
#endif
            } else if (order == F("input-value")) {
#ifdef EnableInput
                sCmd.addCommand(order.c_str(), inputValue);
#endif
            } else if (order == F("output-value")) {
#ifdef EnableOutput
                sCmd.addCommand(order.c_str(), outputValue);
#endif
            } else if (order == F("analog-adc")) {
#ifdef EnableSensorAnalog
                sCmd.addCommand(order.c_str(), analogAdc);
#endif
            } else if (order == F("ultrasonic-cm")) {
#ifdef EnableSensorUltrasonic
                sCmd.addCommand(order.c_str(), ultrasonic);
#endif
//ИНТЕГРИРУЮ: Первая интеграция в ядро. Следим за наименованием
            } else if (order == F("dallas-temp")) {
// #ifdef EnableSensorDallas
//                 sCmd.addCommand(order.c_str(), dallas);
// #endif
            } else if (order == F("dht")) {
#ifdef EnableSensorDht
                sCmd.addCommand(order.c_str(), dhtSensor);
#endif
            } else if (order == F("bme280")) {
#ifdef EnableSensorBme280
                sCmd.addCommand(order.c_str(), bme280Sensor);
#endif
            } else if (order == F("sensor")) {
#ifdef EnableSensorAny
                sCmd.addCommand(order.c_str(), AnySensor);
#endif
            } else if (order == F("bmp280")) {
#ifdef EnableSensorBmp280
                sCmd.addCommand(order.c_str(), bmp280Sensor);
#endif
            } else if (order == F("ccs811")) {
#ifdef EnableSensorCcs811
                sCmd.addCommand(order.c_str(), ccs811Sensor);
#endif
            } else if (order == F("pzem")) {
#ifdef EnableSensorPzem
                sCmd.addCommand(order.c_str(), pzemSensor);
#endif
            } else if (order == F("uptime")) {
#ifdef EnableSensorUptime
                sCmd.addCommand(order.c_str(), uptimeSensor);
#endif
            } else if (order == F("logging")) {
#ifdef EnableLogging
                sCmd.addCommand(order.c_str(), logging);
#endif
            } else if (order == F("impuls-out")) {
#ifdef EnableImpulsOut
                sCmd.addCommand(order.c_str(), impuls);
#endif
            } else if (order == F("count-down")) {
#ifdef EnableCountDown
                sCmd.addCommand(order.c_str(), countDown);
#endif
            } else if (order == F("impuls-in")) {
#ifdef EnableImpulsIn
                // sCmd.addCommand(order.c_str(), impulsInSensor);
#endif
            } else if (order == F("sensor-node")) {
#ifdef EnableSensorNode
                sCmd.addCommand(order.c_str(), nodeSensor);
#endif
            }

            sCmd.readStr(buf);
            
            //v3dev: инициируем экземпляр модулей в случае необходимости
            for (unsigned int i = 0; i < iotModules.size(); i++) {
                ModuleInfo moduleInfo = iotModules[i]->getInfo();
                //del SerialPrint("I", "moduleInfo.name", moduleInfo.name);
                //del SerialPrint("I", "order", order);
                if (moduleInfo.name == order) {  //проверка вхождения имени искомого модуля в ключе элемента настройки
                    myLineParsing.update();  //v3dev: пока используем мостик для совместимости версий, предполагается, что настройки сразу будут в JSON
                    String interval = myLineParsing.gint();
                    if (interval == "") interval = "50";
                    String pin = myLineParsing.gpin();
                    String index = myLineParsing.gindex();
                    String addr = myLineParsing.gaddr();
                    String c = myLineParsing.gc();
                    String id = myLineParsing.gkey();
                    String key = myLineParsing.gfile();
                    String db = myLineParsing.gdb();
                    myLineParsing.clear();
                    String strTmp = "{\"key\": \"" + key + "\", \"id\": \"" + id + "\", \"addr\": \"" + addr + "\", \"int\": \"" + interval + "\", \"pin\": \"" + pin + "\", \"index\": \"" + index + "\", \"c\": \"" + c + "\", \"db\": \"" + db + "\"}";
                    SerialPrint("I", "Строка параметров при инициализации модуля " + moduleInfo.name + ": ", strTmp);

                    if (moduleInfo.type == "Sensor") { 
                        iotSensors.push_back((IoTSensor*)iotModules[i]->initInstance(strTmp));
                    } else if (moduleInfo.type == "Variable") {
                        iotVariables.push_back((IoTVariable*)iotModules[i]->initInstance(strTmp));
                    }
                }
            }
        }
        cmdStr = deleteBeforeDelimiter(cmdStr, "\n");
    }
}

void spaceCmdExecute(String& cmdStr) {
    cmdStr += "\r\n";
    cmdStr.replace("\r\n", "\n");
    cmdStr.replace("\r", "\n");
    while (cmdStr.length()) {
        String buf = selectToMarker(cmdStr, "\n");
        if (buf != "") {
            sCmd.readStr(buf);
            SerialPrint("I", F("Order done W"), buf);
        }
        cmdStr = deleteBeforeDelimiter(cmdStr, "\n");
    }
}

//альтернативный вариант распределяющий нагрузку более правильно
void spaceCmdExecute2(String& cmdStr) {
    cmdStr += "\r\n";
    cmdStr.replace("\r\n", "\n");
    cmdStr.replace("\r", "\n");
    cmdStr.replace("\n", ",");
    cmdStr.replace(",,", ",");
    loopCmdAdd(cmdStr);
}

void addKey(String& key, String& keyNumberTable, int number) {
    keyNumberTable += key + " " + String(number) + ",";
}

int getKeyNum(String& key, String& keyNumberTable) {
    String keyNumberTableBuf = keyNumberTable;

    int number = -1;
    while (keyNumberTableBuf.length()) {
        String tmp = selectToMarker(keyNumberTableBuf, ",");
        String keyIncomming = selectToMarker(tmp, " ");
        if (keyIncomming == key) {
            number = selectToMarkerLast(tmp, " ").toInt();
        }
        keyNumberTableBuf = deleteBeforeDelimiter(keyNumberTableBuf, ",");
    }
    return number;
}

String getValue(String& key) {
    String live = jsonReadStr(configLiveJson, key);
    String store = jsonReadStr(configStoreJson, key);
    if (live != nullptr) {
        return live;
    } else if (store != nullptr) {
        return store;
    } else if (store == nullptr && live == nullptr) {
        return "no value";
    } else {
        return "data error";
    }
}