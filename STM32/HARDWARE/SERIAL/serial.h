#ifndef __SERIAL_H
#define __SERIAL_H
#include "sys.h"

typedef struct 
{
    float LastP;//上次估算协方差 初始化值为0.02
    float Now_P;//当前估算协方差 初始化值为0
    float out;//卡尔曼滤波器输出 初始化值为0
    float Kg;//卡尔曼增益 初始化值为0
    float Q;//过程噪声协方差 初始化值为0.001
    float R;//观测噪声协方差 初始化值为0.543
}KFP;//Kalman Filter parameter
float kalmanFilter(KFP *kfp,float input);
void KFP_init(float LastP, float Now_P, float out, float Kg, float Q, float R, KFP* KFP);
void receivedata(void);

#endif


