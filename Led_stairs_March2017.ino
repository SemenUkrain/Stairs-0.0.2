//Actualized June 2018
#include "Tlc5940.h"

#define sensorType 2             //1=SRF05 сонар  2=PIR сенсоры

#define useResetMechanism true   //использовать ли механизм reset (имеет смысл только при использовании СОНАРОВ)

#define sonar1trig 8            //имеет смысл только при использовании СОНАРОВ
#define sonar1echo 2            //имеет смысл только при использовании СОНАРОВ
#define sonar1resetpin 4        //имеет смысл только при использовании СОНАРОВ
#define sonar2trig 6            //имеет смысл только при использовании СОНАРОВ
#define sonar2echo 7            //имеет смысл только при использовании СОНАРОВ
#define sonar2resetpin 5        //имеет смысл только при использовании СОНАРОВ

#define pir1SignalPin 8         //не менять, если не уверены на 100%, что ИМЕННО делаете!
#define pir2SignalPin 6         //не менять, если не уверены на 100%, что ИМЕННО делаете!

#define sonar1minLimit 30       //см, если обнаружена дистанция меньше, чем это число, то сонар считается сработавшим
#define sonar2minLimit 30       //см, если обнаружена дистанция меньше, чем это число, то сонар считается сработавшим

#define sensorInactiveTime 1500 //мс, время после срабатывания сенвора, в течение которого сенсор игнорит следующие срабатывания
#define stairsCount 12          //количество ступенек
#define initialPWMvalue 2       // only 0...5 (яркость первой и последней ступенек в ожидании)
#define waitForTurnOff 7000     //мс, время задержки подсветки во вкл состоянии после включения последней ступеньки

byte stairsArray[stairsCount];  //массив, где каждый элемент соответствует ступеньке. Все операции только с ним, далее sync2realLife
byte direction = 0;             //0 = снизу вверх, 1 = сверху вниз

byte ignoreSensor1Count = 0;     //счетчик-флаг, сколько раз игнорировать срабатывание сенсора 1
byte ignoreSensor2Count = 0;     //счетчик-флаг, сколько раз игнорировать срабатывание сенсора 2
boolean sensor1trigged = false;  //флаг, участвующий в реакциях на срабатывание сенсора в разных условиях
boolean sensor2trigged = false;  //флаг, участвующий в реакциях на срабатывание сенсора в разных условиях

boolean allLEDsAreOn = false;           //флаг, все светодиоды включены, требуется ожидание в таком состоянии
boolean need2LightOnBottomTop = false;  //флаг, требуется включение ступеней снизу вверх
boolean need2LightOffBottomTop = false; //флаг, требуется выключение ступеней снизу вверх
boolean need2LightOnTopBottom = false;  //флаг, требуется включение ступеней сверху вниз
boolean need2LightOffTopBottom = false; //флаг, требуется выключение ступеней сверху вниз
boolean nothingHappening = true;        //флаг, указывающий на "дежурный" режим лестницы, т.н. исходное состояние

unsigned long sensor1previousTime;       //время начала блокировки сенсора 1 на sensorInactiveTime миллисекунд
unsigned long sensor2previousTime;       //время начала блокировки сенсора 2 на sensorInactiveTime миллисекунд
unsigned long allLEDsAreOnTime;         //время начала горения ВСЕХ ступенек

void setup(){   //подготовка
  for (byte i = 1; i <= stairsCount-2; i++) stairsArray[i] = 0; //забить массив начальными значениями яркости ступенек
  stairsArray[0] = initialPWMvalue;                             //выставление дефолтной яркости первой ступеньки
  stairsArray[stairsCount-1] = initialPWMvalue;                 //выставление дефолтной яркости последней ступеньки
  Tlc.init();        //инициализация TLC-шки
  delay(500);        //нужно, чтобы предыдущая процедура (инициализация) не "подвесила" контроллер
  sync2RealLife();   //"пропихнуть" начальные значения яркости ступенек в "реальную жизнь"
  sensorPrepare(1);  //подготавливаем сенсора 1
  sensorPrepare(2);  //подготавливаем сенсора 2
}

void loop(){//бесконечный цикл

sensor1trigged = sensorTrigged(1);    //выставление флага сонара 1 для последующих манипуляций с ним
sensor2trigged = sensorTrigged(2);    //выставление флага сонара 2 для последующих манипуляций с ним
nothingHappening = !((need2LightOnTopBottom)||(need2LightOffTopBottom)||(need2LightOnBottomTop)||(need2LightOffBottomTop)||(allLEDsAreOn));

if (nothingHappening){               //если лестница находится в исходном (выключенном) состоянии, можно сбросить флаги-"потеряшки" на всякий случай
  ignoreSensor1Count = 0;            //сколько раз игнорировать сенсора 1
  ignoreSensor2Count = 0;            //сколько раз игнорировать сенсора 2
}

//процесс включения относительно сложен, нужно проверять кучу условий

//процесс ВКЛючения: сначала - снизу вверх (выставление флагов и счетчиков)

if ((sensor1trigged) && (nothingHappening)){ //простое включение ступенек снизу вверх из исходного состояния лестницы
  need2LightOnBottomTop = true;              //начать освение ступенек снизу вверх
  ignoreSensor2Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" сверху вниз
}
else if ((sensor1trigged) && ((need2LightOnBottomTop)||(allLEDsAreOn))){ //если ступеньки уже загоряются в нужном направлении или уже горят
  sensorDisable(1);                          //просто увеличить время ожидания полностью включенной лестницы снизу вверх
  allLEDsAreOnTime = millis();
  ignoreSensor2Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" сверху вниз
  direction = 0;                             //направление - снизу вверх
}
else if ((sensor1trigged) && (need2LightOffBottomTop)){  //а уже происходит гашение снизу вверх
  need2LightOffBottomTop = false;            //прекратить гашение ступенек снизу вверх
  need2LightOnBottomTop = true;              //начать освещение ступенек снизу вверх
  ignoreSensor2Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" сверху вниз
}
else if ((sensor1trigged) && (need2LightOnTopBottom)){   //а уже происходит освещение сверху вниз
  need2LightOnTopBottom = false;             //прекратить освещение ступенек сверху вниз
  need2LightOnBottomTop = true;              //начать освение ступенек снизу вверх
  ignoreSensor2Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" сверху вниз
}
else if ((sensor1trigged) && (need2LightOffTopBottom)){  //а уже происходит гашение сверху вниз
  need2LightOffTopBottom = false;            //прекратить гашение ступенек сверху вниз
  need2LightOnBottomTop = true;              //начать освение ступенек снизу вверх
  ignoreSensor2Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" сверху вниз
}

//процесс ВКЛючения: теперь - сверху вниз (выставление флагов и счетчиков)

if ((sensor2trigged) && (nothingHappening)){ //простое включение ступенек сверху вниз из исходного состояния лестницы
  need2LightOnTopBottom = true;              //начать освещение ступенек сверху вниз
  ignoreSensor1Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" снизу вверх
}
else if ((sensor2trigged) && ((need2LightOnTopBottom)||(allLEDsAreOn))){//если ступеньки уже загоряются в нужном направлении или уже горят
  sensorDisable(2);                          //обновить отсчет времени для освещения ступенек сверху вниз
  allLEDsAreOnTime = millis();
  ignoreSensor1Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" снизу вверх
  direction = 1;                             //направление - сверху вниз
}
else if ((sensor2trigged) && (need2LightOffTopBottom)){  //а уже происходит гашение сверху вниз
  need2LightOffTopBottom = false;            //прекратить гашение ступенек сверху вниз
  need2LightOnTopBottom = true;              //начать освещение ступенек сверху вниз
  ignoreSensor1Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" снизу вверх
}
else if ((sensor2trigged) && (need2LightOnBottomTop)){   //а уже происходит освещение снизу вверх
  need2LightOnBottomTop = false;             //прекратить освещение ступенек снизу вверх
  need2LightOnTopBottom = true;              //начать освение ступенек сверху вних
  ignoreSensor1Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" снизу вверх
}
else if ((sensor2trigged) && (need2LightOffBottomTop)){  //а уже происходит гашение снизу вверх
  need2LightOffBottomTop = false;            //прекратить гашение ступенек снизу вверх
  need2LightOnTopBottom = true;              //начать освение ступенек сверху вниз
  ignoreSensor1Count++;                      //игнорить противоположный сенсора, чтобы при его срабатывании не запустилось "загорание" снизу вверх
}

//процесс ВЫКлючения относительно прост - нужно только знать направление, и выставлять флаги

if ((allLEDsAreOn)&&((allLEDsAreOnTime + waitForTurnOff) <= millis())){ //пора гасить ступеньки в указанном направлении
  if (direction == 0) need2LightOffBottomTop = true;        //снизу вверх
  else if (direction == 1) need2LightOffTopBottom = true;   //сверху вниз
}

//непосредственная обработка флагов с "пропихиванием" массива ступенек в "реальность"

if (need2LightOnBottomTop){     //увеличим яркость за 4 итерации, уложившись в 400мс (BottomTop - снизу вверх)
  for (byte i=0; i<=3;i++){
    startBottomTop();
    sync2RealLife();
    delay(100);
  }//for
}//if

if (need2LightOffBottomTop){    //уменьшим яркость за 4 итерации, уложившись в 400мс (BottomTop - снизу вверх)
  for (byte i=0; i<=3;i++){
    stopBottomTop();
    sync2RealLife();
    delay(100);
  }//for
}//if

if (need2LightOnTopBottom){     //увеличим яркость за 4 итерации, уложившись в 400мс (TopBottom - сверху вниз)
  for (byte i=0; i<=3;i++){
    startTopBottom();
    sync2RealLife();
    delay(100);
  }//for
}//if

if (need2LightOffTopBottom){    //уменьшим яркость за 4 итерации, уложившись в 400мс (TopBottom - сверху вниз)
  for (byte i=0; i<=3;i++){
    stopTopBottom();
    sync2RealLife();
    delay(100);
  }//for
}//if
}//procedure

void startBottomTop(){  //процедура ВКЛючения снизу вверх
  for (byte i=1; i<=stairsCount; i++){  //обработка всех ступенек по очереди, добавление по "1" яркости для одной ступеньки за раз
    if (stairsArray[i-1] <=4){          //узнать, какой ступенькой сейчас заниматься
       stairsArray[i-1]++;              //увеличить на ней яркость
       return;                          //прямо сейчас "свалить" из процедуры
    }//if
    else if ((i == stairsCount)&&(stairsArray[stairsCount-1] == 5)&&(!allLEDsAreOn)){   //если полностью включена последняя требуемая ступенька
      allLEDsAreOnTime = millis();      //сохраним время начала состояния "все ступеньки включены"
      allLEDsAreOn = true;              //флаг, все ступеньки включены
      direction = 0;                    //для последующего гашения ступенек снизу вверх
      need2LightOnBottomTop = false;    //поскольку шаг - последний, сбрасываем за собой флаг необходимости
      return;                           //прямо сейчас "свалить" из процедуры
    }//if
  }//for
}//procedure

void stopBottomTop(){   //процедура ВЫКЛючения снизу вверх
  if (allLEDsAreOn) allLEDsAreOn = false;               //уже Не все светодиоды включены, очевидно

  for (byte i=0; i<=stairsCount-1; i++){                //пытаемся перебрать все ступеньки по очереди
    if ((i == 0)&&(stairsArray[i] > initialPWMvalue)){  //если ступенька первая, снижать яркость до "дежурного" уровня ШИМ, а не 0
      stairsArray[0]--;                                 //снизить яркость
      return;                                           //прямо сейчас "свалить" из процедуры
    }
    else if ((i == stairsCount-1)&&(stairsArray[i] > initialPWMvalue)){ //если последняя, то снижать яркость до дежурного уровня ШИМ, а не 0
      stairsArray[i]--;                                 //снизить яркость
      if (stairsArray[stairsCount-1] == initialPWMvalue) need2LightOffBottomTop = false; //если это последняя ступенька и на ней достигнута минимальная яркость
      return;                                           //прямо сейчас "свалить" из процедуры
    }
    else if ((i != 0) && (i != (stairsCount-1)) && (stairsArray[i] >= 1)){  //обработка всех остальных ступенек
      stairsArray[i]--;                                 //снизить яркость
      return;                                           //прямо сейчас "свалить" из процедуры
    }//if i == 0
  }//for
}//procedure

void startTopBottom(){  //процедура ВКЛючения сверху вниз
  for (byte i=stairsCount; i>=1; i--){  //обработка всех ступенек по очереди, добавление по "1" яркости для одной ступеньки за раз
    if (stairsArray[i-1] <=4){          //узнать, какой ступенькой сейчас заниматься
       stairsArray[i-1]++;              //уменьшить на ней яркость
       return;                          //прямо сейчас "свалить" из процедуры
    }//if
    else if ((i == 1)&&(stairsArray[0] == 5)&&(!allLEDsAreOn)){ //если полностью включена последняя требуемая ступенька
      allLEDsAreOnTime = millis();      //сохраним время начала состояния "все ступеньки включены"
      allLEDsAreOn = true;              //флаг, все ступеньки включены
      direction = 1;                    //для последующего гашения ступенек сверху вниз 
      need2LightOnTopBottom = false;    //поскольку шаг - последний, сбрасываем за собой флаг необходимости
      return;                           //прямо сейчас "свалить" из процедуры
    }//if
  }//for
}//procedure

void stopTopBottom(){   //процедура ВЫКЛючения сверху вниз
  if (allLEDsAreOn) allLEDsAreOn = false;   //уже Не все светодиоды включены, очевидно

  for (byte i=stairsCount-1; i>=0; i--){    //пытаемся перебрать все ступеньки по очереди
    if ((i == stairsCount-1)&&(stairsArray[i] > initialPWMvalue)){  //если ступенька последняя, то снижать яркость до дежурного уровня ШИМ, а не 0
      stairsArray[i]--;                     //снизить яркость
      return;                               //прямо сейчас "свалить" из процедуры
    }
    else if ((i == 0)&&(stairsArray[i] > initialPWMvalue)){ //если первая, то снижать яркость до дежурного уровня ШИМ, а не 0
      stairsArray[0]--;                                     //снизить яркость
      if (stairsArray[0] == initialPWMvalue) need2LightOffTopBottom = false; //если это первая ступенька и на ней достигнута минимальная яркость
      return;                                               //прямо сейчас "свалить" из процедуры
    }
    else if ((i != 0) && (i != (stairsCount-1)) && (stairsArray[i] >= 1)){  //обработка всех остальных ступенек
      stairsArray[i]--;                     //снизить яркость
      return;                               //прямо сейчас "свалить" из процедуры
    }//if i == 0
  }//for
}//procedure

void sync2RealLife(){   //процедуры синхронизации "фантазий" массива с "реальной жизнью"
for (int i = 0; i < stairsCount; i++) Tlc.set(i, stairsArray[i]*800);   //0...5 степени яркости * 800 = вкладываемся в 0...4096
Tlc.update();
}//procedure

void sensorPrepare(byte sensorNo){    //процедура первоначальной "инициализации" сенсора
#if (sensorType == 1)
  if (sensorNo == 1){
    pinMode(sonar1trig, OUTPUT);
    pinMode(sonar1echo, INPUT);
    pinMode(sonar1resetpin, OUTPUT);
    digitalWrite(sonar1resetpin, HIGH);   //всегда должен быть HIGH, для перезагрузки сонара кратковременно сбросить в LOW
  }
  else if (sensorNo == 2){
    pinMode(sonar2trig, OUTPUT);
    pinMode(sonar2echo, INPUT);
    pinMode(sonar2resetpin, OUTPUT);
    digitalWrite(sonar2resetpin, HIGH);   //всегда должен быть HIGH, для перезагрузки сонара кратковременно сбросить в LOW
  }
#elif (sensorType == 2)
  pinMode(pir1SignalPin, INPUT);
  pinMode(pir2SignalPin, INPUT);
#endif


}//procedure

void sonarReset(byte sonarNo){          //процедура ресета подвисшего сонара, на 100 мс "отбирает" у него питание
  if (sonarNo == 1){
    digitalWrite(sonar1resetpin, LOW);
    delay(100);
    digitalWrite(sonar1resetpin, HIGH);
  }
  else if (sonarNo == 2){
    digitalWrite(sonar2resetpin, LOW);
    delay(100);
    digitalWrite(sonar2resetpin, HIGH);
  }//if
}//procedure

void sensorDisable(byte sensorNo){        //процедура "запрета" сенсора
  if (sensorNo == 1) sensor1previousTime = millis();
  if (sensorNo == 2) sensor2previousTime = millis();
}

boolean sensorEnabled(byte sensorNo){     //функция, дающая знать, не "разрешен" ли уже сенсора

  if ((sensorNo == 1)&&((sensor1previousTime + sensorInactiveTime) <= millis())) return true;
  if ((sensorNo == 2)&&((sensor2previousTime + sensorInactiveTime) <= millis())) return true;
  else return false;
}

boolean sensorTrigged(byte sensorNo){     //процедура проверки, сработал ли сенсора (с отслеживанием "подвисания" сонаров)
#if (sensorType == 2)
  if (sensorNo == 1){
    if (digitalRead(pir1SignalPin) == LOW) return true;
    else return false;
  }
  if (sensorNo == 2){
    if (digitalRead(pir2SignalPin) == LOW) return true;
    else return false;
  }
#elif (sensorType == 1)
  if ((sensorNo == 1)&&(sensorEnabled(1))){
    digitalWrite(sonar1trig, LOW);
    delayMicroseconds(5);
    digitalWrite(sonar1trig, HIGH);
    delayMicroseconds(15);
    digitalWrite(sonar1trig, LOW);
    unsigned int time_us = pulseIn(sonar1echo, HIGH, 5000); //5000 - таймаут, то есть не ждать сигнала более 5мс
    unsigned int distance = time_us / 58;
    if ((distance != 0)&&(distance <= sonar1minLimit)){     //сонар считается сработавшим, принимаем "меры"

      if (ignoreSensor1Count > 0) {  //если требуется 1 раз проигнорить сонар 2
        ignoreSensor1Count--;        //проигнорили, сбрасываем за собой флаг необходимости
        sensorDisable(1);            //временно "запретить" сонар, ведь по факту он сработал (хотя и заигнорили), иначе каждые 400мс будут "ходить всё новые люди"
        return false;
      }
      sensorDisable(1);              //временно "запретить" сонар, иначе каждые 400мс будут "ходить всё новые люди"
      return true;
    }
    else if (distance == 0){        //сонар 1 "завис"
      #if (useResetMechanism)
        sonarReset(1);
      #endif
      return false;
    }//endelse
    else return false;
  }//if sensor 1

  if ((sensorNo == 2)&&(sensorEnabled(2))){
    digitalWrite(sonar2trig, LOW);
    delayMicroseconds(5);
    digitalWrite(sonar2trig, HIGH);
    delayMicroseconds(15);
    digitalWrite(sonar2trig, LOW);
    unsigned int time_us = pulseIn(sonar2echo, HIGH, 5000); //5000 - таймаут, то есть не ждать сигнала более 5мс
    unsigned int distance = time_us / 58;
    if ((distance != 0)&&(distance <= sonar2minLimit)){     //сонар считается сработавшим, принимаем "меры"

      if (ignoreSensor2Count > 0) {  //если требуется 1 раз проигнорить сонар 2
        ignoreSensor2Count--;        //проигнорили, сбрасываем за собой флаг необходимости
        sensorDisable(2);            //временно "запретить" сонар, ведь по факту он сработал (хотя и заигнорили), иначе каждые 400мс будут "ходить всё новые люди"
        return false;
      }
      sensorDisable(2);              //временно "запретить" сонар, иначе каждые 400мс будут "ходить всё новые люди"
      return true;
    }
    else if (distance == 0){        //сонар 2 "завис"
      #if (useResetMechanism)
        sonarReset(2);
      #endif
      return false;
    }//endelse
    else return false;
  }//if sensor 2
#endif
}//procedure
