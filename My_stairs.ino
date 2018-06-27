//Actualized June 2018 @Borisov Sergei 
#include "Tlc5940.h"

#define sonar1trig 8            //имеет смысл только при использовании СОНАРОВ
#define sonar1echo 2            //имеет смысл только при использовании СОНАРОВ
#define sonar1resetpin 4        //имеет смысл только при использовании СОНАРОВ

#define sonar2trig 6            //имеет смысл только при использовании СОНАРОВ
#define sonar2echo 7            //имеет смысл только при использовании СОНАРОВ
#define sonar2resetpin 5        //имеет смысл только при использовании СОНАРОВ

#define sonar1limit 30       //см, если обнаружена дистанция меньше, чем это число, то сонар считается сработавшим
#define sonar2limit 30       //см, если обнаружена дистанция меньше, чем это число, то сонар считается сработавшим

#define sensorInactiveTime 1500 //мс, время после срабатывания сонара, в течение которого сенсор игнорит следующие срабатывания
#define stairsCount 16          //количество ступенек
#define initialPWMvalue 800       // only 0...4095 (яркость первой и последней ступенек в ожидании)
#define waitForTurnOff 7000     //мс, время задержки подсветки во вкл состоянии после включения последней ступеньки
#define Speed 1.2         //скорость включения 

int StairsArrayPWM [stairsCount];
bool direction = false;                                                             // направление : false = снизу вверх, true = сверху вниз                                             
long AllLight_OffDelay = 0;                                                         //Время до выключения лестницы
unsigned long PreTime = 0;                                                          // отметка системного времени в предыдущем цикле                                                             
unsigned long ActTime =0;                                                           // отметка системного времени в текущем цикле
unsigned long Cycle = 0 ;                                                                    // мс, длительность цикла

bool sensor1trig = false;
bool sensor2trig = false;
long time1,time2;

bool AllStairs_ON = false;                                                       // освещение включено полностью, требуется ожидание в таком состоянии
bool ON_BottomTop = false;                                                       // требуется включение ступеней снизу вверх
bool OFF_BottomTop = false;                                                      // требуется выключение ступеней снизу вверх
bool ON_TopBottom = false;                                                       // требуется включение ступеней сверху вниз
bool OFF_TopBottom = false;                                                      // требуется выключение ступеней сверху вниз
bool InitialState = true;                                                        // "дежурный" режим лестницы, т.н. исходное состояние

void setup()
{

  init_sensonrs();
	for (byte i = 1; i < stairsCount-2; i++)  StairsArrayPWM[i] = 0;              //задаём 0 уровень яркости ступеням от 2 до stairsCount-1
	StairsArrayPWM[0] = StairsArrayPWM[stairsCount-1] = initialPWMvalue;          //Дежурный уровень яркости 1 и последеней ступени
    Tlc.init();                                                                 //иннициализация tlc5940
    delay(500); 
    Ainimation_First();
    PreTime = millis();                                                               // сохранить системное время
   }


void loop()
{
  InitialState = !( (AllStairs_ON) || (ON_BottomTop) || (OFF_BottomTop) || (ON_TopBottom) || (OFF_TopBottom) ); // Если ничего не происходит лестница находится в базовом сосотоянии

  if (AllStairs_ON) {                                                                 // если лестница находится во включенном состоянии, можно сбросить флаги на всякий случай
    ON_BottomTop = false;                                                           // включение освещения снизу вверх
    ON_TopBottom = false;                                                           // включение освещения сверху вниз
  }


	check_sensors();                                             //опрашиваем сенсоры 
                                                   // - действия при срабатывания "НИЖНЕГО" сенсора 
  
  if (sensor1trig && InitialState) ON_BottomTop = true;                              // при срабатывании сенсора из исходного состояния лестницы начать включение освещения ступеней снизу вверх
  else if (sensor1trig && AllStairs_ON) AllLight_OffDelay = waitForTurnOff ;           // если освещение уже полностью включено, обновить отсчет задержки выключения освещения     
  else if (sensor1trig && OFF_BottomTop) {                                           // если происходит выключение освещения ступеней снизу вверх,
    OFF_BottomTop = false;                                                          // прекратить выключение освещения
    ON_BottomTop = true;                                                            // вновь начать включение освещения ступеней снизу вверх
  }
  else if (sensor1trig && ON_TopBottom) {                                            // если происходит включение освещения ступеней сверху вниз
    ON_TopBottom = false;                                                           // прекратить включение освещения сверху вниз
    ON_BottomTop = true;                                                            // начать включение освещения ступеней снизу вверх
  }
  else if (sensor1trig && OFF_TopBottom) {                                           // если происходит выключение освещения ступеней сверху вниз
    OFF_TopBottom = false;                                                          // прекратить выключение освещения сверху вниз
    ON_BottomTop = true;                                                            // начать включение освещения ступеней снизу вверх
  }

                                                  // - действия при срабатывания "ВЕРХНЕГО" сенсора 

  if (sensor2trig && InitialState) ON_TopBottom = true;                              // при срабатывании сенсора из исходного состояния лестницы начать включение освещения ступеней сверху вниз
  else if (sensor2trig && AllStairs_ON) AllLight_OffDelay = waitForTurnOff ;         // если освещение уже полностью включено, обновить отсчет задержки выключения освещения     
  else if (sensor2trig && OFF_TopBottom) {                                           // если происходит выключение освещения ступеней сверху вниз
    OFF_TopBottom = false;                                                           // прекратить выключение освещения
    ON_TopBottom = true;                                                             // вновь начать включение освещения ступеней сверху вниз
  }

  else if (sensor2trig && ON_BottomTop) {                                            // если происходит включение освещения ступеней снизу вверх
    ON_BottomTop = false;                                                           // прекратить включение освещения снизу вверх
    ON_TopBottom = true;                                                            // начать включение освещения ступеней сверху вниз
  }
  
  else if (sensor2trig && OFF_BottomTop) {                                           // если происходит выключение освещения ступеней снизу вверх
    OFF_BottomTop = false;                                                          // прекратить выключение освещения снизу вверх
    ON_TopBottom = true;                                                            // начать включение освещения ступеней сверху вниз
  }

      // - определение необходимости и направления выключения освещения

  if (AllStairs_ON && !(AllLight_OffDelay > 0)) {                                     // пора гасить освещение
    OFF_BottomTop = !direction;                                                        // снизу вверх
    OFF_TopBottom = direction;                                                         // сверху вниз
  }

 
     // - управление включением/выключением освещения ступеней

  if (ON_BottomTop) Light_ON_bottom_top();                                          // вызов процедуры включения ступеней снизу вверх
  if (OFF_BottomTop) Light_OFF_bottom_top();                                        // вызов процедуры выключения ступеней снизу вверх
  if (ON_TopBottom) Light_ON_top_bottom();                                          // вызов процедуры включения ступеней сверху вниз
  if (OFF_TopBottom) Light_OFF_top_bottom();                                        // выключение ступеней сверху вниз
  PWM_Output();                                                                     // передать в LED-драйвер актуальные уровни яркости

                        // - завершение цикла
  Delay_Read_Sensors();  //Задержка для работы сенсоров считывания
  Count_Cycle_Time();    //Считаем время постоянной подсветки лестницы

}

void Light_OFF_bottom_top() {  
  AllStairs_ON = false;                                                                    // сбросить признак того, что освещение лестницы полностью включено
  for (byte i = 0; i < stairsCount ; i++) {                                              // перебор ступеней снизу вверх
    if (((i == 0) || (i == stairsCount-1)) && (StairsArrayPWM[i] > initialPWMvalue)) {   // если это первая или последняя ступень и уровень яркости выше дежурного уровня,
      StairsArrayPWM[i] = StairsArrayPWM[i] / Speed ;                                    // уменьшить яркость
    if (StairsArrayPWM[i] < initialPWMvalue ) StairsArrayPWM[i] = initialPWMvalue ;      // ограничить минимальную яркость
      return;                                                                            // и выйти из процедуры   
    }  
    else if ((i != 0) && (i != stairsCount-1) && (StairsArrayPWM[i] > 0)) {              // для остальных ступеней, если уровень яркости выше "0",
      StairsArrayPWM[i] = StairsArrayPWM[i] / Speed ;                                    // уменьшить яркость
    if (StairsArrayPWM[i] < 5 ) StairsArrayPWM[i] = 0 ;                                  // ограничить минимальную яркость
      return;                                                                            // и выйти из процедуры
    }
  }
  OFF_BottomTop = false;                                                                 // закончить выключение ступеней снизу вверх
}

void Light_ON_bottom_top() { 
  direction = false;                                                                   // запомнить направление = снизу вверх
  for (byte i = 0; i < stairsCount; i++) {                                                    // перебор ступеней снизу вверх
    if (StairsArrayPWM[i] < 1) {													                          // если очередная ступень выключена,
	  StairsArrayPWM[i] = 5; 	  									                                    // задать для неё начальный уровень яркости
	  return;																		                                      // и выйти из процедуры
	  }  
	  else if (StairsArrayPWM[i] < 4095 ){                                     		    // иначе, если уровень яркости ступени меньше максимального,
      StairsArrayPWM[i] = StairsArrayPWM[i]  * Speed ; 	  								          // увеличить уровень яркости  
	  if (StairsArrayPWM[i]  > 4095 ) StairsArrayPWM[i]  = 4095 ;               		    // ограничить максимальную яркость
      return;                                                                       // и выйти из процедуры                            
    }
  }  
  AllStairs_ON  = true;                                                            	   // освещение лестницы полностью включено
  AllLight_OffDelay = waitForTurnOff;
  ON_BottomTop = false;                                                         	   // закончить включение ступеней снизу вверх
}


void Light_OFF_top_bottom() {  
  AllStairs_ON = false;                                                               // сбросить признак того, что освещение лестницы полностью включено
  for (byte i = stairsCount; i > 0; i--) {                                                    // перебор ступеней вниз, начиная с верхней
    if (((i == 1) || (i == stairsCount)) && ( StairsArrayPWM[i-1] > initialPWMvalue)) {       // если это первая или последняя ступень и уровень яркости выше дежурного уровня,
     StairsArrayPWM[i-1] = StairsArrayPWM[i-1] / Speed ;                           // уменьшить яркость
	  if (StairsArrayPWM[i-1] < initialPWMvalue ) StairsArrayPWM[i-1] = initialPWMvalue ; // ограничить минимальную яркость
      return;                                                                       // и выйти из процедуры	  
    }  
    else if ((i != 1) && (i != stairsCount) && (StairsArrayPWM[i-1] > 0)) {                   // для остальных ступеней, если уровень яркости выше "0",
      StairsArrayPWM[i-1] = StairsArrayPWM[i-1] / Speed ;                           // уменьшить яркость
	  if (StairsArrayPWM[i-1] < 5 ) StairsArrayPWM[i-1] = 0 ; 							          // ограничить минимальную яркость
      return;                                                                       // и выйти из процедуры
    }
  }
  OFF_TopBottom = false;                                                            // закончить выключение ступеней сверху вниз
}

void Light_ON_top_bottom(){
  direction = true;                                                                    // запомнить направление = сверху вниз
  for (byte i = stairsCount; i > 0; i--) {                                                    // перебор ступеней сверху вниз
    if (StairsArrayPWM[i-1] < 1) {                                                  // если очередная ступень выключена,
    StairsArrayPWM[i-1] = 5;                                                        // задать для неё начальный уровень яркости
    return;                                                                         // и выйти из процедуры
    }  
    else if (StairsArrayPWM[i-1] < 4095 ){                                          // иначе, если уровень яркости ступени меньше максимального,
      StairsArrayPWM[i-1] = StairsArrayPWM[i-1] * Speed ;                           // увеличить уровень яркости  
      if (StairsArrayPWM[i-1] > 4095 ) StairsArrayPWM[i-1] = 4095 ;                 // ограничить максимальную яркость
      return;                                                                       // и выйти из процедуры                            
    } 
  } 
  AllStairs_ON = true;                                                                // освещение лестницы полностью включено
  AllLight_OffDelay = waitForTurnOff;
  ON_TopBottom = false;                                                             // закончить включение ступеней сверху вниз
}

void PWM_Output() {  
  for (int i = 0; i < 16; i++) Tlc.set(i, StairsArrayPWM[i]);                      	// яркость ступеней в диапазоне 0...4096 
  Tlc.update();
}

void init_sensonrs(){                                                                          
    pinMode (sonar1trig, OUTPUT);                                                         // - инициализация сенсора 1  
    pinMode (sonar1echo, INPUT);
    pinMode (sonar1resetpin, OUTPUT);
    digitalWrite (sonar1trig, LOW);
    sensor1trig = false;
        
    pinMode (sonar2trig, OUTPUT);                                                         //- инициализация сенсора 2
    pinMode (sonar2echo, INPUT);
    pinMode (sonar2resetpin, OUTPUT);
    digitalWrite (sonar2trig, LOW);
    sensor2trig = false;
    
    PreTime = millis();                                                               // сохранить системное время
}

void check_sensors(){

    digitalWrite(sonar1trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(sonar1trig, LOW);
    byte distance1 = pulseIn(sonar1echo, HIGH, 5000) / 58 ;                          // расчет дистанции до препятствия (5000 - таймаут, то есть не ждать сигнала более 5мс) 
    sensor1trig = (distance1 > 0) && (distance1 <= sonar1limit);                            // SRF05 : сонар 1 зафиксировал присутствие
    if(sensor1trig) time1 = millis();
    if(sensor1trig && (time1+sensorInactiveTime)<=millis()) sensor1trig = true;
    
    if (!(distance1 > 0)) {                                                                 // перезапуск зависшего сонара - отключить питание до конца цикла
    digitalWrite(sonar1resetpin, LOW);
    delay(100); 
    }                              
                                 

    digitalWrite(sonar2trig, HIGH);                                                         // - проверить состояние "ВЕРХНЕГО" сенсора       
    delayMicroseconds(10);
    digitalWrite(sonar2trig, LOW);
    byte distance2 = pulseIn(sonar2echo, HIGH, 5000) / 58 ;                          // расчет дистанции до препятствия (5000 - таймаут, то есть не ждать сигнала более 5мс) 
    sensor2trig = (distance2 > 0) && (distance2 <= sonar2limit);                            // SRF05 : сонар 2 зафиксировал присутствие
    if(sensor1trig) time2 = millis();
    if(sensor1trig && (time2+sensorInactiveTime)<=millis()) sensor2trig = true;

    if (!(distance2 > 0)) {
    digitalWrite(sonar2resetpin, LOW);                                            // перезапуск зависшего сонара - отключить питание до конца цикла
    delay(100);
    }

  }

  void Ainimation_First(){
    ON_BottomTop = true;                                                        //ожидание конца инициализации
    while(ON_BottomTop){                                                        //Запускаем вкл снизу в вверх
       Light_ON_bottom_top();
       PWM_Output();                                                            //Синхронизируем яркость 
    }

    OFF_TopBottom = true;

    while(OFF_TopBottom){
    Light_OFF_top_bottom();
    PWM_Output();
    }
   
    }

    void Delay_Read_Sensors(){
      delay(50);                                                                      // задержка для обеспечения паузы перед следующим опросом сонаров
                                                                                    // восстановление питания, если был перезапуск сонаров
    digitalWrite (sonar1resetpin, HIGH);                                               // всегда должен быть HIGH, для перезапуска сонара 1 установить в LOW
    digitalWrite (sonar2resetpin, HIGH);                                               // всегда должен быть HIGH, для перезапуска сонара 2 установить в LOW
    }

    void Count_Cycle_Time(){
    ActTime = millis();                                                               // отметка времени для расчета длительности цикла
    Cycle = ActTime - PreTime;                                                        // прошло времени с предыдущего цикла программы 

    if (AllLight_OffDelay > 0) AllLight_OffDelay = AllLight_OffDelay - Cycle;         // если время освещения лестницы не истекло, уменьшить на длительность цикла
    else AllLight_OffDelay = 0;                                                       // иначе, время до начала отключения освещения = 0
 
    PreTime = millis();                                                               // отметка времени для расчета длительности цикла
    }
