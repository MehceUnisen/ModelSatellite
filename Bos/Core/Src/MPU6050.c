/*
library name: 	MPU6050 6 axis module
written by: 		T.Jaber
Date Written: 	25 Mar 2019
Last Modified: 	20 April 2019 by Mohamed Yaqoob
Description: 		MPU6050 Module Basic Functions Device Driver library that use HAL libraries.
References:
								- MPU6050 Registers map: https://www.invensense.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
								- Jeff Rowberg MPU6050 library: https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050

* Copyright (C) 2019 - T. Jaber
   This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
   of the GNU General Public Licenseversion 3 as published by the Free Software Foundation.

   This software library is shared with puplic for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
   or indirectly by this software, read more about this on the GNU General Public License.
*/

//Header files
#include "MPU6050.h"

static I2C_HandleTypeDef i2cHandler;
static float accelScalingFactor, gyroScalingFactor;
static int16_t GyroRW[3];

//Fucntion Definitions
//1- i2c Handler
void MPU6050_Init(I2C_HandleTypeDef *I2Chnd)
{
	//Copy I2C CubeMX handle to local library
	memcpy(&i2cHandler, I2Chnd, sizeof(*I2Chnd));
}

//2- i2c Read
void I2C_Read(uint8_t ADDR, uint8_t *i2cBif, uint8_t NofData)
{
	uint8_t i2cBuf[2];
	uint8_t MPUADDR;
	//Need to Shift address to make it proper to i2c operation
	MPUADDR = (MPU_ADDR<<1);
	i2cBuf[0] = ADDR;
	HAL_I2C_Master_Transmit(&i2cHandler, MPUADDR, i2cBuf, 1, 10);
	HAL_I2C_Master_Receive(&i2cHandler, MPUADDR, i2cBif, NofData, 100);
}

//3- i2c Write
void I2C_Write8(uint8_t ADDR, uint8_t data)
{
	uint8_t i2cData[2];
	i2cData[0] = ADDR;
	i2cData[1] = data;
	uint8_t MPUADDR = (MPU_ADDR<<1);
	HAL_I2C_Master_Transmit(&i2cHandler, MPUADDR, i2cData, 2,100);
}

//4- MPU6050 Initialaztion Configuration
void MPU6050_Config(MPU_ConfigTypeDef *config)
{
	uint8_t Buffer = 0;
	I2C_Write8(PWR_MAGT_1_REG, 0x80);
	HAL_Delay(100);
	Buffer = config ->ClockSource & 0x07;
	Buffer |= (config ->Sleep_Mode_Bit << 6) &0x40;
	I2C_Write8(PWR_MAGT_1_REG, Buffer);
	HAL_Delay(100);

	Buffer = 0;
	Buffer = config->CONFIG_DLPF & 0x07;
	I2C_Write8(CONFIG_REG, Buffer);

	Buffer = 0;
	Buffer = (config->Gyro_Full_Scale << 3) & 0x18;
	I2C_Write8(GYRO_CONFIG_REG, Buffer);

	Buffer = 0;
	Buffer = (config->Accel_Full_Scale << 3) & 0x18;
	I2C_Write8(ACCEL_CONFIG_REG, Buffer);

	MPU6050_Set_SMPRT_DIV(0x04);

	switch (config->Accel_Full_Scale)
	{
		case AFS_SEL_2g:
			accelScalingFactor = 0.00006103515625;
			break;

		case AFS_SEL_4g:
			accelScalingFactor = 0.0001220703125;
				break;

		case AFS_SEL_8g:
			accelScalingFactor = 0.000244140625;
			break;

		case AFS_SEL_16g:
			accelScalingFactor = 0.00048828125;
			break;

		default:
			break;
	}

	switch (config->Gyro_Full_Scale)
	{
		case FS_SEL_250:
			gyroScalingFactor = 0.00763358778626;
			break;

		case FS_SEL_500:
			gyroScalingFactor = 0.01526717557252;
			break;

		case FS_SEL_1000:
			gyroScalingFactor = 0.030487804878;
			break;

		case FS_SEL_2000:
			gyroScalingFactor = 0.0609756097561;
			break;

		default:
			break;
	}

}

uint8_t MPU6050_Get_SMPRT_DIV(void)
{
	uint8_t Buffer = 0;

	I2C_Read(SMPLRT_DIV_REG, &Buffer, 1);
	return Buffer;
}

void MPU6050_Set_SMPRT_DIV(uint8_t SMPRTvalue)
{
	I2C_Write8(SMPLRT_DIV_REG, SMPRTvalue);
}

uint8_t MPU6050_Get_FSYNC(void)
{
	uint8_t Buffer = 0;

	I2C_Read(CONFIG_REG, &Buffer, 1);
	Buffer &= 0x38;
	return (Buffer>>3);
}

void MPU6050_Set_FSYNC(enum EXT_SYNC_SET_ENUM ext_Sync)
{
	uint8_t Buffer = 0;
	I2C_Read(CONFIG_REG, &Buffer,1);
	Buffer &= ~0x38;

	Buffer |= (ext_Sync <<3);
	I2C_Write8(CONFIG_REG, Buffer);

}

void MPU6050_Get_Accel_RawData(RawData_Def *rawDef)
{
	uint8_t i2cBuf[2];
	uint8_t AcceArr[6], GyroArr[6];

	I2C_Read(INT_STATUS_REG, &i2cBuf[1],1);
	if((i2cBuf[1]&&0x01))
	{
		I2C_Read(ACCEL_XOUT_H_REG, AcceArr,6);

		//Accel Raw Data
		rawDef->x = ((AcceArr[0]<<8) + AcceArr[1]); // x-Axis
		rawDef->y = ((AcceArr[2]<<8) + AcceArr[3]); // y-Axis
		rawDef->z = ((AcceArr[4]<<8) + AcceArr[5]); // z-Axis
		//Gyro Raw Data
		I2C_Read(GYRO_XOUT_H_REG, GyroArr,6);
		GyroRW[0] = ((GyroArr[0]<<8) + GyroArr[1]);
		GyroRW[1] = (GyroArr[2]<<8) + GyroArr[3];
		GyroRW[2] = ((GyroArr[4]<<8) + GyroArr[5]);
	}
}

void MPU6050_Get_Gyro_RawData(RawData_Def *rawDef)
{

	//Accel Raw Data
	rawDef->x = GyroRW[0];
	rawDef->y = GyroRW[1];
	rawDef->z = GyroRW[2];

}

void MPU6050_Get_Cal()
{
	for(int i=0;i<SAMPLE_NUM;i++)
	{
		MPU6050_Get_Accel_RawData(&myAccelRaw);
		MPU6050_Get_Gyro_RawData(&myGyroRaw);
		myGyroCal.x  += ((float)myGyroRaw.x * gyroScalingFactor);
		myGyroCal.y  += ((float)myGyroRaw.y * gyroScalingFactor);
		myGyroCal.z  += ((float)myGyroRaw.z * gyroScalingFactor);
		myAccelCal.x += ((float)myAccelRaw.x * accelScalingFactor);
		myAccelCal.y += ((float)myAccelRaw.y * accelScalingFactor);
		myAccelCal.z += ((float)myAccelRaw.z * accelScalingFactor);
		HAL_Delay(3);
	}
	myGyroCal.x  /= SAMPLE_NUM;
	myGyroCal.y  /= SAMPLE_NUM;
	myGyroCal.z  /= SAMPLE_NUM;
	myAccelCal.x /= SAMPLE_NUM;
	myAccelCal.y /= SAMPLE_NUM;
	myAccelCal.z /= SAMPLE_NUM;
}

void MPU6050_Read_Value()
{
	MPU6050_Get_Accel_RawData(&myAccelRaw);
	MPU6050_Get_Gyro_RawData(&myGyroRaw);
	myAccelMean.x = ((float)myAccelRaw.x * accelScalingFactor) - myAccelCal.x;
	myAccelMean.y = ((float)myAccelRaw.y * accelScalingFactor) - myAccelCal.y;
    myAccelMean.z = (((float)myAccelRaw.z * accelScalingFactor) - myAccelCal.z) + 1;
	myGyroMean.x  = (((float)myGyroRaw.x  * gyroScalingFactor) - myGyroCal.x) * 0.004;
	myGyroMean.y  = (((float)myGyroRaw.y  * gyroScalingFactor) - myGyroCal.y) * 0.004;
    myGyroMean.z  = (((float)myGyroRaw.z  * gyroScalingFactor) - myGyroCal.z) * 0.004;

    myGyroMean.x += myGyroMean.y * sin(myGyroMean.z);
    myGyroMean.y -= myGyroMean.x * sin(myGyroMean.z);

    accel.pitch = atan(-1*(myAccelMean.x)/sqrt(pow((myAccelMean.y),2) + pow((myAccelMean.z),2)))*RAD_TO_DEG;
	accel.roll  = atan((myAccelMean.y)/sqrt(pow((myAccelMean.x),2) + pow((myAccelMean.z),2)))*RAD_TO_DEG;

	mpu6050.pitch = (0.99 * (mpu6050.pitch + myGyroMean.y)) + (0.01 * accel.pitch);
	mpu6050.roll  = (0.99 * (mpu6050.roll + myGyroMean.x)) + (0.01 * accel.roll);
}



