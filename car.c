#pragma region
#define true 1
#define false 0
typedef int bool;
typedef unsigned int uint;

// LED & Button
#define Gpin 21
#define Rpin 22
#define BtnPin 24

// Left Wheel
#define PWMA 1
#define AIN2 2
#define AIN1 3

// Right Wheel
#define PWMB 4
#define BIN2 5
#define BIN1 6

// Path Tracing Sensor
#define LEFTPTS 23
#define MIDDLEPTS 24
#define RIGHTPTS 25

// Infrared Detectors
#define LEFTID 26
#define RIGHTID 27

// Ultrasonic
#define USTRIG 28
#define USECHO 29

// #define SHOW_RUN_LOG
// #define ENABLE_INFRARED_DETECTORS
// #define ENABLE_ULTRASONIC
#pragma endregion

#include "header.h"
#include "raspberrypi.h"

// LED
void ledInit();
void ledOn();
void ledOff();
void led(char *);

// Test Move
void t_up(uint, uint);
void t_down(uint, uint);
void t_left(uint, uint);
void t_right(uint, uint);
void t_stop(uint);

// Move
void motorInit();
void run_forward(uint, uint);
void run_back(uint, uint);
void run_left(uint, uint);
void run_right(uint, uint);
void run_brake(uint);

// Ultrasonic
void ultrasonic_init();
float distanceMeasure();

uint gspeed = 375, rspeed = 175;
uint gtime = 0, rtime = 0;

int SL, SM, SR;
int IDL, IDR;

int toStop = 0;

int main()
{
#pragma region Initialize Wiring PI
    printf("WIRINGPI_SETTINGUP\n");
    if (wiringPiSetup() == -1)
    {
        printf("WIRINGPI_SETUP_FAILED\n");
        return 1;
    }
    printf("WIRINGPI_SETTINGUP_FINISHED\n");
#pragma endregion

#pragma region Initialize Pin
    ledInit();

    motorInit();

#ifdef ENABLE_ULTRASONIC
    ultrasonic_init();
#endif
#pragma endregion

#pragma region Main Loop
    while (true)
    {
        SL = digitalRead(LEFTPTS);
        SM = digitalRead(MIDDLEPTS);
        SR = digitalRead(RIGHTPTS);

        IDL = digitalRead(LEFTID);
        IDR = digitalRead(RIGHTID);

#if defined(ENABLE_ULTRASONIC) && defined(SHOW_RUN_LOG)
        printf("ULTRASONIC: %.2f\n", distanceMeasure());
#endif

        if (SL == HIGH && SR == HIGH)
        {
#if defined(ENABLE_ULTRASONIC) && defined(SHOW_RUN_LOG)
            if (toStop > 512)
#elif !defined(ENABLE_ULTRASONIC) && defined(SHOW_RUN_LOG)
            if (toStop > 65536)
#else
            if (toStop > 2097152)
#endif
            {
#ifdef SHOW_RUN_LOG
                printf("Go Stop\n");
#endif
                run_brake(100);
            }
            else if (toStop < 2147483000)
            {
                toStop++;
#ifdef SHOW_RUN_LOG
                printf("Try Go Stop: %d\n", toStop);
#endif
            }
        }
#if !defined(ENABLE_ULTRASONIC) && defined(ENABLE_INFRARED_DETECTORS)
        else if (IDL == LOW && IDR == LOW)
#elif defined(ENABLE_ULTRASONIC) && !defined(ENABLE_INFRARED_DETECTORS)
        else if (distanceMeasure() < 30)
#elif defined(ENABLE_ULTRASONIC) && defined(ENABLE_INFRARED_DETECTORS)
        else if ((IDL == LOW && IDR == LOW) || distanceMeasure() > 30)
#endif
        {
#if defined(ENABLE_INFRARED_DETECTORS) || defined(ENABLE_ULTRASONIC)
#ifdef SHOW_RUN_LOG
            printf("Go Back\n");
#endif
            run_back(gspeed, 3000);
            delay(300);
#ifdef SHOW_RUN_LOG
            printf("Go Left\n");
#endif
            run_left(rspeed, rtime);
#endif
        }
#ifdef ENABLE_INFRARED_DETECTORS
        else if (IDL == HIGH && IDR == LOW)
        {
#ifdef SHOW_RUN_LOG
            printf("Go Left\n");
#endif
            run_left(rspeed, rtime);
        }
        else if (IDL == LOW && IDR == HIGH)
        {
#ifdef SHOW_RUN_LOG
            printf("Go Right\n");
#endif
            run_right(rspeed, rtime);
        }
        else
#endif
        {
            if (SL == LOW && SR == LOW && SM == HIGH)
            {
#ifdef SHOW_RUN_LOG
                printf("Go Forward\n");
#endif
                run_forward(gspeed, gtime);
                toStop = 0;
            }
            else if (SL == HIGH && SR == LOW)
            {
#ifdef SHOW_RUN_LOG
                printf("Go Left\n");
#endif
                run_left(rspeed, rtime);
                toStop = 0;
            }
            else if (SL == LOW && SR == HIGH)
            {
#ifdef SHOW_RUN_LOG
                printf("Go Right\n");
#endif
                run_right(rspeed, rtime);
                toStop = 0;
            }
        }
    }
#pragma endregion

    return 0;
}

void motorInit()
{
    pinMode(BtnPin, INPUT);

    pinMode(PWMA, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(PWMB, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);

    softPwmCreate(PWMA, 0, 500);
    softPwmCreate(PWMB, 0, 500);
    printf("MOTOR_INITIATED\n");
}

void ledInit()
{
    pinMode(Gpin, OUTPUT);
    pinMode(Rpin, OUTPUT);
    printf("LED_INITIATED\n");
}

void ledOn()
{
    digitalWrite(Gpin, HIGH);
    digitalWrite(Rpin, HIGH);
#ifdef SHOW_RUN_LOG
    printf("LED_ON\n");
#endif
}

void ledOff()
{
    digitalWrite(Gpin, LOW);
    digitalWrite(Rpin, LOW);
#ifdef SHOW_RUN_LOG
    printf("LED_OFF\n");
#endif
}

void led(char *color)
{
    ledInit();
    if (color == "RED")
    {
        digitalWrite(Rpin, HIGH);
        digitalWrite(Gpin, LOW);
    }
    else if (color == "GREEN")
    {
        digitalWrite(Gpin, HIGH);
        digitalWrite(Rpin, LOW);
    }
#ifdef SHOW_RUN_LOG
    else
    {
        printf("Invalid led color\n");
    }
#endif
}

void t_up(uint speed, uint t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);

    delay(t_time);
}

void t_down(uint speed, uint t_time)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);

    delay(t_time);
}

void t_left(uint speed, uint t_time)
{
    digitalWrite(AIN2, 1);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 1);
    softPwmWrite(PWMB, speed);

    delay(t_time);
}

void t_right(uint speed, uint t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 1);
    softPwmWrite(PWMA, speed);

    digitalWrite(BIN2, 1);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, speed);

    delay(t_time);
}

void t_stop(uint t_time)
{
    digitalWrite(AIN2, 0);
    digitalWrite(AIN1, 0);
    softPwmWrite(PWMA, 0);

    digitalWrite(BIN2, 0);
    digitalWrite(BIN1, 0);
    softPwmWrite(PWMB, 0);
    delayMicroseconds(t_time);
}

void run_forward(uint speed, uint t_time)
{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    softPwmWrite(PWMB, speed);
    delayMicroseconds(t_time);
}

void run_back(uint speed, uint t_time)
{
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    softPwmWrite(PWMA, speed);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    softPwmWrite(PWMB, speed);
    delayMicroseconds(t_time);
}

void run_left(uint speed, uint t_time)
{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    softPwmWrite(PWMA, speed / 3.025f);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    softPwmWrite(PWMB, speed * 2);
    delayMicroseconds(t_time);
}

void run_right(uint speed, uint t_time)
{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    softPwmWrite(PWMA, speed * 2);
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    softPwmWrite(PWMB, speed / 3.025f);
    delayMicroseconds(t_time);
}

void run_brake(uint t_time)
{
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    softPwmWrite(PWMA, 0);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
    softPwmWrite(PWMB, 0);
    delayMicroseconds(t_time);
}

void ultrasonic_init()
{
    pinMode(USECHO, INPUT);
    pinMode(USTRIG, OUTPUT);
    printf("ULTRASONIC_INITIATED\n");
}

float distanceMeasure()
{
    struct timeval tv1, tv2;
    long start, stop;
    float dis;

    digitalWrite(USTRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(USTRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(USTRIG, LOW);

    while (!(digitalRead(USECHO) == 1))
    {
        gettimeofday(&tv1, NULL);
    }
    while (!(digitalRead(USECHO) == 0))
    {
        gettimeofday(&tv2, NULL);
    }

    start = tv1.tv_sec * 1000000 + tv1.tv_usec;
    stop = tv2.tv_sec * 1000000 + tv2.tv_usec;

    dis = (float)(stop - start) / 1000000 * 34000 / 2;
    return dis;
}
