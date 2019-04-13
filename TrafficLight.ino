#include "TaskScheduler.h" // 包含此头文件，才能使用调度器

#define COOLING_TIME 3  // 人行道冷却时间
#define INIT_TIME 14    // 初始化绿灯时的剩余时间
#define TARGET_PEOPLE 5 // 转换为绿灯所需要的人数
#define BILNK_TIME 7    // 开始闪烁的时间

#define SIDEWALK_YELLOW 3 // 人行道黄灯
#define SIDEWALK_GREEN 4  // 人行道绿灯
#define LANE_RED 5        // 车道红灯
#define LANE_YELLOW 6     // 车道黄灯

const short unsigned LED_PIN[] = {SIDEWALK_YELLOW, SIDEWALK_GREEN, LANE_RED, LANE_YELLOW};
const short unsigned SENSOR_PIN[] = {10, 9, 12, 11, 8}; // 定义传感器引脚，每两个为一组

short int numberOfPeople = 0;       // 人数
short int timeLeft = -COOLING_TIME; // 人行道剩余可通过时间

void setup()
{
    Serial.begin(115200);

    for (unsigned pin : LED_PIN)
    {
        pinMode(pin, OUTPUT);    // 设置 LED 引脚为输出模式
        digitalWrite(pin, HIGH); // 开机自检
    }
    delay(3000);
    for (unsigned pin : LED_PIN)
        digitalWrite(pin, LOW);
    for (unsigned pin : SENSOR_PIN)
    {
        pinMode(pin, INPUT); // 设置传感器引脚为输入模式
        // 开机自检
        if (digitalRead(pin))
        {
            Serial.println(pin);
            while (true)
            {
                for (unsigned pin : LED_PIN)
                    digitalWrite(pin, HIGH);
                delay(250);
                for (unsigned pin : LED_PIN)
                    digitalWrite(pin, LOW);
                delay(250);
            }
        }
    }

    Sch.init(); // 初始化调度器

    /* 向调度器中添加任务
    第一个参数为要添加任务的函数名
    第二个参数为任务第一次执行的时间，合理设置有利于防止任务重叠，有利以提高任务执行的精度
    第三个参数是任务执行的周期
    第二、三个参数的单位均为毫秒，也可配置定时器修改其单位
    第四个参数代表任务是合作式还是抢占式，一般取1就可以 */
    Sch.addTask(light, 0, 1000, true);

    Sch.start(); // 启动调度器
}

void loop()
{
    Sch.dispatchTasks(); // 执行被调度的任务，用调度器时放上这一句即可
    detect(SENSOR_PIN[0], SENSOR_PIN[1], false);
    detect(SENSOR_PIN[2], SENSOR_PIN[3], false);
    keepGreen();
}

// 交通灯处理函数
void light()
{
    if (timeLeft > -COOLING_TIME)
    {
        --timeLeft;
        Serial.println(timeLeft);
    }
    else if (numberOfPeople >= TARGET_PEOPLE)
    {
        timeLeft = INIT_TIME;
        numberOfPeople = 0;
    }
    if (timeLeft > 0)
    {
        if (timeLeft <= BILNK_TIME)
        {
            digitalWrite(LANE_RED, LOW);
            digitalWrite(SIDEWALK_GREEN, LOW);
            delay(500); // 形成 0.5s 的脉冲闪烁，警示时间即将结束
        }
        digitalWrite(LANE_RED, HIGH);
        digitalWrite(LANE_YELLOW, LOW);
        digitalWrite(SIDEWALK_YELLOW, LOW);
        digitalWrite(SIDEWALK_GREEN, HIGH);
    }
    else
    {
        digitalWrite(LANE_RED, LOW);
        digitalWrite(SIDEWALK_YELLOW, HIGH);
        digitalWrite(SIDEWALK_GREEN, LOW);
        digitalWrite(LANE_YELLOW, HIGH);
        delay(100); // 形成 0.1s 的黄灯脉冲闪烁
        digitalWrite(LANE_YELLOW, LOW);
        digitalWrite(SIDEWALK_YELLOW, LOW);
    }
}

// 绿灯延时函数
void keepGreen()
{
    if (digitalRead(SENSOR_PIN[4]) && timeLeft > BILNK_TIME)
        timeLeft = INIT_TIME;
}

// 人数检测计算函数
void detect(unsigned pin1, unsigned pin2, bool isOut)
{
    if (timeLeft == -COOLING_TIME && digitalRead(pin1) && !digitalRead(pin2))
    {
        while (digitalRead(pin1)) // 等待，直到引脚1变为低电平
        {
            Sch.dispatchTasks();
            keepGreen();
        }
        short unsigned startTime = micros();
        while (micros() - startTime < 300 || !digitalRead(pin2)) // 等待，直到超过 300ms 或引脚2变为高电平
        {
            Sch.dispatchTasks();
            keepGreen();
        }
        if (digitalRead(pin2))
        {
            while (digitalRead(pin2)) // 等待，直到引脚2变为低电平
            {
                Sch.dispatchTasks();
                keepGreen();
            }
            if (isOut)
            {
                if (numberOfPeople > 0)
                    --numberOfPeople;
            }
            else
            {
                ++numberOfPeople;
            }
            Serial.println(numberOfPeople); // 输出值。用于调试
        }
    }
    else if (!isOut)
    {
        detect(pin2, pin1, true);
    }

    // if (timeLeft == -COOLING_TIME && pulseIn(pin1, HIGH, 1000) > 0 && pulseIn(pin2, HIGH, 500000) > 0)
    //     isOut ? --numberOfPeople : ++numberOfPeople;
    // else
    //     detect(pin2, pin1, true);
}
