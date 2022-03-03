#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Created on 2022-01-13 16:20
The last revision time on 2022-01-14 18:10

@author: ZhangKaiyang
@QQ: 3173244086
"""

import cv2
import serial
import tkinter
import numpy as np

a = tkinter.Tk(screenName=':1.0')  # 树莓派默认显示设备
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)  # 使用USB连接串行口,发送数据给stm32


lower_blue = np.array([50, 50, 200])  # 规定寻找物体色彩最低阈值
upper_blue = np.array([130, 255, 255])  # 规定寻找物体色彩最高阈值


def Coordinate(frame):  # 返回图像中心坐标
    # 寻找边缘
    contours, hierarchy = cv2.findContours(frame, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:      # 没找到，返回固定坐标
        cv2.imshow("RGB_Mirror", RGB_Mirror)
        Coordinate_X = 320
        Coordinate_Y = 340
    else:               # 找到边缘
#   以下注释部分在OPENCV4中不需要有，而在OPENCV2、3中需要
#        e = len(contours)       # 测量边缘长度
#        for i in range(e):
#            if cv2.contourArea(contours[i]) > cv2.contourArea(contours[0]):
#               contours[0] = contours[i]               # 获取边缘数据
        x, y, w, h = cv2.boundingRect(contours[0])      # 获取最小包围矩形的边长及坐标数据
        # 写出绘制边框要用的数组
        brcnt = np.array([[x, y], [x + w, y], [x + w, y + h], [x, y + h]])
        # 在原图上绘制边框
        cv2.drawContours(RGB_Mirror, [brcnt], -1, (255, 255, 255), 2)
        # 处理返回坐标
        Coordinate_X = x
        Coordinate_Y = y
        # 显示图像
        cv2.imshow('RGB_Mirror', RGB_Mirror)
    return Coordinate_X, Coordinate_Y


def SerialTransmission():   # 数据发送函数
    # 由于传输的是两个数据，要让STM32能够识别到分别是X, Y的数据，必须加上相关标志位，然后在STM32上对标志位写处理协议
    data = '#' + str(X) + '$' + str(Y) + '\r\n'
    # 发送数据
    ser.write(data.encode())


if __name__ == "__main__":  # 主运行函数
    cap = cv2.VideoCapture(0)  # 打开摄像头
#    ser.open()  # 打开串口
    while cap.isOpened():  # 当摄像头成功打开后
        ret, RGB = cap.read()  # 读取图像,捕捉帧，捕捉成功后ret为True，否则为False，RGB为读取的图像
        RGB_Mirror = cv2.flip(RGB, 1)  # 对图像取镜像，保证画面一致性
        # 将RGB图像转化为HSV图像格式
        HSV = cv2.cvtColor(RGB_Mirror, cv2.COLOR_BGR2HSV)
        # 将所选颜色区域加上掩膜，将所选颜色区域筛选出来
        mask = cv2.inRange(HSV, lower_blue, upper_blue)
        # 反过来，掩膜其他区域，即得到了目标区域
        target = cv2.bitwise_and(RGB_Mirror, RGB_Mirror, mask=mask)
        # 画面滤波
        target_optimize = cv2.bilateralFilter(target, 5, 100, 100)
        # 画面运算-开运算
        k = np.ones([12, 12], np.uint8)
        target_open = cv2.morphologyEx(target_optimize, cv2.MORPH_OPEN, k)
        # 画面运算-闭运算
        n = np.ones([10, 10], np.uint8)
        target_close = cv2.morphologyEx(target_open, cv2.MORPH_CLOSE, n, iterations=5)
        # cv2.imshow('target_close', target_close)
        gray = cv2.cvtColor(target_close, cv2.COLOR_BGR2GRAY)   # 图像处理为灰度图，否则无法寻找边缘
        # 获取坐标数据
        X, Y = Coordinate(gray)
        # 发送数据
        SerialTransmission()
        # 监视器打印物体坐标
        print([X, Y])       
        c = cv2.waitKey(1)
        if c == 27:
            break
    ser.close()  # 关闭串口
    cap.release()   # 释放视频
    cv2.destroyAllWindows()     # 关闭所有窗口
