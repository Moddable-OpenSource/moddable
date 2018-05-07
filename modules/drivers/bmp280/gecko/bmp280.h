/** mainpage
*
****************************************************************************
* Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
*
* File : bmp280.h
*
* Date : 2016/07/01
*
* Revision : 2.0.5(Pressure and Temperature compensation code revision is 1.1)
*
* Usage: Sensor Driver for BMP280 sensor
*
****************************************************************************
*
* \section License
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
*   Redistributions in binary form must reproduce the above copyright
*   notice, this list of conditions and the following disclaimer in the
*   documentation and/or other materials provided with the distribution.
*
*   Neither the name of the copyright holder nor the names of the
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
* OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*
* The information provided is believed to be accurate and reliable.
* The copyright holder assumes no responsibility
* for the consequences of use
* of such information nor for any infringement of patents or
* other rights of third parties which may result from its use.
* No license is granted by implication or otherwise under any patent or
* patent rights of the copyright holder.
**************************************************************************/

/*! \file bmp280.h
    \brief BMP280 Sensor Driver Support Header File */
#ifndef __BMP280_H__
#define __BMP280_H__

/*!
* @brief The following definition uses for define the data types
*
* @note While porting the API please consider the following
* @note Please check the version of C standard
* @note Are you using Linux platform
*/

/*!
* @brief For the Linux platform support
* Please use the types.h for your data types definitions
*/
#ifdef	__KERNEL__

#include <linux/types.h>
#include <linux/math64.h>
#define BMP280_64BITSUPPORT_PRESENT
/* singed integer type*/
typedef	int8_t s8;/**< used for signed 8bit */
typedef	int16_t s16;/**< used for signed 16bit */
typedef	int32_t s32;/**< used for signed 32bit */
typedef	int64_t s64;/**< used for signed 64bit */

typedef	u_int8_t u8;/**< used for unsigned 8bit */
typedef	u_int16_t u16;/**< used for unsigned 16bit */
typedef	u_int32_t u32;/**< used for unsigned 32bit */
typedef	u_int64_t u64;/**< used for unsigned 64bit */



#else /* ! __KERNEL__ */
/**********************************************************
* These definition uses for define the C
* standard version data types
***********************************************************/
#if defined(__STDC_VERSION__)

/************************************************
 * compiler is C11 C standard
************************************************/
#if (__STDC_VERSION__ == 201112L)

/************************************************/
#include <stdint.h>
/************************************************/

/*unsigned integer types*/
typedef	uint8_t u8;/**< used for unsigned 8bit */
typedef	uint16_t u16;/**< used for unsigned 16bit */
typedef	uint32_t u32;/**< used for unsigned 32bit */
typedef	uint64_t u64;/**< used for unsigned 64bit */

/*signed integer types*/
typedef	int8_t s8;/**< used for signed 8bit */
typedef	int16_t s16;/**< used for signed 16bit */
typedef	int32_t s32;/**< used for signed 32bit */
typedef	int64_t s64;/**< used for signed 64bit */
#define BMP280_64BITSUPPORT_PRESENT
/************************************************
 * compiler is C99 C standard
************************************************/

#elif (__STDC_VERSION__ == 199901L)

/* stdint.h is a C99 supported c library.
which is used to fixed the integer size*/
/************************************************/
#include <stdint.h>
/************************************************/

/*unsigned integer types*/
typedef	uint8_t u8;/**< used for unsigned 8bit */
typedef	uint16_t u16;/**< used for unsigned 16bit */
typedef	uint32_t u32;/**< used for unsigned 32bit */
typedef	uint64_t u64;/**< used for unsigned 64bit */

/*signed integer types*/
typedef int8_t s8;/**< used for signed 8bit */
typedef	int16_t s16;/**< used for signed 16bit */
typedef	int32_t s32;/**< used for signed 32bit */
typedef	int64_t s64;/**< used for signed 64bit */
#define BMP280_64BITSUPPORT_PRESENT
/************************************************
 * compiler is C89 or other C standard
************************************************/

#else /*  !defined(__STDC_VERSION__) */
/*!
* @brief By default it is defined as 32 bit machine configuration
*	define your data types based on your
*	machine/compiler/controller configuration
*/
#define  MACHINE_32_BIT

/*! @brief
 *	If your machine support 16 bit
 *	define the MACHINE_16_BIT
 */
#ifdef MACHINE_16_BIT
#include <limits.h>
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed long int s32;/**< used for signed 32bit */

#if defined(LONG_MAX) && LONG_MAX == 0x7fffffffffffffffL
typedef long int s64;/**< used for signed 64bit */
typedef unsigned long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT
#elif defined(LLONG_MAX) && (LLONG_MAX == 0x7fffffffffffffffLL)
typedef long long int s64;/**< used for signed 64bit */
typedef unsigned long long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT
#else
#warning Either the correct data type for signed 64 bit integer \
could not be found, or 64 bit integers are not supported in your environment.
#warning The API will only offer 32 bit pressure calculation.This will \
slightly impede accuracy(noise of ~1 pascal RMS will be added to output).
#warning If 64 bit integers are supported on your platform, \
please set s64 manually and "#define(BMP280_64BITSUPPORT_PRESENT)" manually.
#endif

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned long int u32;/**< used for unsigned 32bit */

/* If your machine support 32 bit
define the MACHINE_32_BIT*/
#elif defined MACHINE_32_BIT
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed int s32;/**< used for signed 32bit */
typedef	signed long long int s64;/**< used for signed 64bit */

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned int u32;/**< used for unsigned 32bit */
typedef	unsigned long long int u64;/**< used for unsigned 64bit */
/*!
* @brief If your machine support 64 bit
define the BMP280_64BITSUPPORT_PRESENT*/
#define BMP280_64BITSUPPORT_PRESENT

/* If your machine support 64 bit
define the MACHINE_64_BIT*/
#elif defined MACHINE_64_BIT
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed int s32;/**< used for signed 32bit */
typedef	signed long int s64;/**< used for signed 64bit */

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned int u32;/**< used for unsigned 32bit */
typedef	unsigned long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT

#else
#warning The data types defined above which not supported \
define the data types manually
#endif
#endif

/*** This else will execute for the compilers
 *	which are not supported the C standards
 *	Like C89/C99/C11***/
#else
/*!
* @brief By default it is defined as 32 bit machine configuration
*	define your data types based on your
*	machine/compiler/controller configuration
*/
#define  MACHINE_32_BIT

/* If your machine support 16 bit
define the MACHINE_16_BIT*/
#ifdef MACHINE_16_BIT
#include <limits.h>
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed long int s32;/**< used for signed 32bit */

#if defined(LONG_MAX) && LONG_MAX == 0x7fffffffffffffffL
typedef long int s64;/**< used for signed 64bit */
typedef unsigned long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT
#elif defined(LLONG_MAX) && (LLONG_MAX == 0x7fffffffffffffffLL)
typedef long long int s64;/**< used for signed 64bit */
typedef unsigned long long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT
#else
#warning Either the correct data type for signed 64 bit integer \
could not be found, or 64 bit integers are not supported in your environment.
#warning The API will only offer 32 bit pressure calculation.This will \
slightly impede accuracy(noise of ~1 pascal RMS will be added to output).
#warning If 64 bit integers are supported on your platform, \
please set s64 manually and "#define(BMP280_64BITSUPPORT_PRESENT)" manually.
#endif

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned long int u32;/**< used for unsigned 32bit */

/*! @brief If your machine support 32 bit
define the MACHINE_32_BIT*/
#elif defined MACHINE_32_BIT
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed int s32;/**< used for signed 32bit */
typedef	signed long long int s64;/**< used for signed 64bit */

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned int u32;/**< used for unsigned 32bit */
typedef	unsigned long long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT

/* If your machine support 64 bit
define the MACHINE_64_BIT*/
#elif defined MACHINE_64_BIT
/*signed integer types*/
typedef	signed char  s8;/**< used for signed 8bit */
typedef	signed short int s16;/**< used for signed 16bit */
typedef	signed int s32;/**< used for signed 32bit */
typedef	signed long int s64;/**< used for signed 64bit */

/*unsigned integer types*/
typedef	unsigned char u8;/**< used for unsigned 8bit */
typedef	unsigned short int u16;/**< used for unsigned 16bit */
typedef	unsigned int u32;/**< used for unsigned 32bit */
typedef	unsigned long int u64;/**< used for unsigned 64bit */
#define BMP280_64BITSUPPORT_PRESENT

#else
#warning The data types defined above which not supported \
define the data types manually
#endif
#endif
#endif
/********************************************/
/**\name	ENABLE FLOATING OUTPUT      */
/********************************************/
/*!
* @brief If the user wants to support floating point calculations, please set
* the following define. If floating point calculation is not wanted
* or allowed (e.g. in Linux kernel), please do not set the definition.
*/
#define BMP280_ENABLE_FLOAT

/*!
* @brief If the user wants to support 64 bit integer calculation (needed for
* optimal pressure accuracy) please set the following definition. If
* int64 calculation is not wanted (e.g. because it would include
* large libraries), please do not set the definition.
*/
#define BMP280_ENABLE_INT64
/************************************************************/
/**\name	BUS READ AND WRITE FUNCTION POINTERS        */
/************************************************************/
/*!
	@brief Define the calling convention of YOUR bus communication routine.
	@note This includes types of parameters. This example shows the
	configuration for an SPI bus link.

    If your communication function looks like this:

    write_my_bus_xy(u8 device_addr, u8 register_addr,
    u8 * data, u8 length);

    The BMP280_WR_FUNC_PTR would equal:

	BMP280_WR_FUNC_PTR s8 (* bus_write)(u8,
    u8, u8 *, u8)

    Parameters can be mixed as needed refer to the
    refer BMP280_BUS_WRITE_FUNC  macro.


*/
/* defines the return parameter type of the BMP280_WR_FUNCTION */
#define BMP280_BUS_WR_RETURN_TYPE s8

/* links the order of parameters defined in
BMP280_BUS_WR_PARAM_TYPE to function calls used inside the API*/
#define BMP280_BUS_WR_PARAM_TYPES u8, u8,\
	u8 *, u8

/* links the order of parameters defined in
BMP280_BUS_WR_PARAM_TYPE to function calls used inside the API*/
#define BMP280_BUS_WR_PARAM_ORDER(device_addr, register_addr,\
	register_data, wr_len)

/* never change this line */
#define BMP280_BUS_WRITE_FUNC(device_addr, register_addr,\
register_data, wr_len) bus_write(device_addr, register_addr,\
	register_data, wr_len)
/*!
	@brief link macro between API function calls and bus read function
	@note The bus write function can change since this is a
	system dependant issue.

    If the bus_read parameter calling order is like: reg_addr,
    reg_data, wr_len it would be as it is here.

    If the parameters are differently ordered or your communication
    function like I2C need to know the device address,
    you can change this macro accordingly.


    BMP280_BUS_READ_FUNC(dev_addr, reg_addr, reg_data, wr_len)\
    bus_read(dev_addr, reg_addr, reg_data, wr_len)

    This macro lets all API functions call YOUR communication routine in a
    way that equals your definition in the
    refer BMP280_WR_FUNC_PTR definition.

    @note: this macro also includes the "MSB='1'
    for reading BMP280 addresses.

*/
/* defines the return parameter type of the BMP280_RD_FUNCTION
*/
#define BMP280_BUS_RD_RETURN_TYPE s8

/* defines the calling parameter types of the BMP280_RD_FUNCTION
*/
#define BMP280_BUS_RD_PARAM_TYPES (u8, u8,\
	u8 *, u8)

/* links the order of parameters defined in \
BMP280_BUS_RD_PARAM_TYPE to function calls used inside the API
*/
#define BMP280_BUS_RD_PARAM_ORDER (device_addr, register_addr,\
	register_data)

/* never change this line */
#define BMP280_BUS_READ_FUNC(device_addr, register_addr,\
	register_data, rd_len)bus_read(device_addr, register_addr,\
	register_data, rd_len)
/****************************************/
/**\name	DELAY       */
/****************************************/
/*!
* @brief defines the return parameter type of the BMP280_DELAY_FUNCTION
*/
#define BMP280_DELAY_RETURN_TYPE void

/*!
* @brief defines the calling parameter types of the BMP280_DELAY_FUNCTION
*/
#define BMP280_DELAY_PARAM_TYPES u16
/***************************************************************/
/**\name	GET AND SET BITSLICE FUNCTIONS       */
/***************************************************************/
/* never change this line */
#define BMP280_DELAY_FUNC(delay_in_msec)\
		delay_func(delay_in_msec)

#define BMP280_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMP280_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

/***************************************************************/
/**\name	COMMON USED CONSTANTS       */
/***************************************************************/
/* Constants */
#define BMP280_NULL                          (0)
#define BMP280_RETURN_FUNCTION_TYPE          s8
/* right shift definitions*/
#define BMP280_SHIFT_BIT_POSITION_BY_01_BIT				 (1)
#define BMP280_SHIFT_BIT_POSITION_BY_02_BITS			(2)
#define BMP280_SHIFT_BIT_POSITION_BY_03_BITS			(3)
#define BMP280_SHIFT_BIT_POSITION_BY_04_BITS			(4)
#define BMP280_SHIFT_BIT_POSITION_BY_05_BITS			(5)
#define BMP280_SHIFT_BIT_POSITION_BY_08_BITS			(8)
#define BMP280_SHIFT_BIT_POSITION_BY_11_BITS			(11)
#define BMP280_SHIFT_BIT_POSITION_BY_12_BITS			(12)
#define BMP280_SHIFT_BIT_POSITION_BY_13_BITS			(13)
#define BMP280_SHIFT_BIT_POSITION_BY_14_BITS			(14)
#define BMP280_SHIFT_BIT_POSITION_BY_15_BITS			(15)
#define BMP280_SHIFT_BIT_POSITION_BY_16_BITS			(16)
#define BMP280_SHIFT_BIT_POSITION_BY_17_BITS			(17)
#define BMP280_SHIFT_BIT_POSITION_BY_18_BITS			(18)
#define BMP280_SHIFT_BIT_POSITION_BY_19_BITS			(19)
#define BMP280_SHIFT_BIT_POSITION_BY_25_BITS			(25)
#define BMP280_SHIFT_BIT_POSITION_BY_31_BITS			(31)
#define BMP280_SHIFT_BIT_POSITION_BY_33_BITS			(33)
#define BMP280_SHIFT_BIT_POSITION_BY_35_BITS			(35)
#define BMP280_SHIFT_BIT_POSITION_BY_47_BITS			(47)

/* numeric definitions */
#define	BMP280_PRESSURE_TEMPERATURE_CALIB_DATA_LENGTH		(24)
#define	BMP280_GEN_READ_WRITE_DATA_LENGTH			(1)
#define BMP280_REGISTER_READ_DELAY				(1)
#define	BMP280_TEMPERATURE_DATA_LENGTH				(3)
#define	BMP280_PRESSURE_DATA_LENGTH				(3)
#define	BMP280_ALL_DATA_FRAME_LENGTH				(6)
#define	BMP280_INIT_VALUE					(0)
#define	BMP280_CHIP_ID_READ_COUNT				(5)
#define	BMP280_CHIP_ID_READ_SUCCESS				(0)
#define	BMP280_CHIP_ID_READ_FAIL				((s8)-1)
#define	BMP280_INVALID_DATA					(0)

/************************************************/
/**\name	ERROR CODES      */
/************************************************/
#define	SUCCESS			((u8)0)
#define E_BMP280_NULL_PTR         ((s8)-127)
#define E_BMP280_COMM_RES         ((s8)-1)
#define E_BMP280_OUT_OF_RANGE     ((s8)-2)
#define ERROR                     ((s8)-1)
/************************************************/
/**\name	CHIP ID DEFINITION       */
/***********************************************/
#define BMP280_CHIP_ID1		(0x56)
#define BMP280_CHIP_ID2		(0x57)
#define BMP280_CHIP_ID3		(0x58)
/************************************************/
/**\name	I2C ADDRESS DEFINITION       */
/***********************************************/
#define BMP280_I2C_ADDRESS1                  (0x76)
#define BMP280_I2C_ADDRESS2                  (0x77)
/************************************************/
/**\name	POWER MODE DEFINITION       */
/***********************************************/
/* Sensor Specific constants */
#define BMP280_SLEEP_MODE                    (0x00)
#define BMP280_FORCED_MODE                   (0x01)
#define BMP280_NORMAL_MODE                   (0x03)
#define BMP280_SOFT_RESET_CODE               (0xB6)
/************************************************/
/**\name	STANDBY TIME DEFINITION       */
/***********************************************/
#define BMP280_STANDBY_TIME_1_MS              (0x00)
#define BMP280_STANDBY_TIME_63_MS             (0x01)
#define BMP280_STANDBY_TIME_125_MS            (0x02)
#define BMP280_STANDBY_TIME_250_MS            (0x03)
#define BMP280_STANDBY_TIME_500_MS            (0x04)
#define BMP280_STANDBY_TIME_1000_MS           (0x05)
#define BMP280_STANDBY_TIME_2000_MS           (0x06)
#define BMP280_STANDBY_TIME_4000_MS           (0x07)
/************************************************/
/**\name	OVERSAMPLING DEFINITION       */
/***********************************************/
#define BMP280_OVERSAMP_SKIPPED          (0x00)
#define BMP280_OVERSAMP_1X               (0x01)
#define BMP280_OVERSAMP_2X               (0x02)
#define BMP280_OVERSAMP_4X               (0x03)
#define BMP280_OVERSAMP_8X               (0x04)
#define BMP280_OVERSAMP_16X              (0x05)
/************************************************/
/**\name	WORKING MODE DEFINITION       */
/***********************************************/
#define BMP280_ULTRA_LOW_POWER_MODE          (0x00)
#define BMP280_LOW_POWER_MODE	             (0x01)
#define BMP280_STANDARD_RESOLUTION_MODE      (0x02)
#define BMP280_HIGH_RESOLUTION_MODE          (0x03)
#define BMP280_ULTRA_HIGH_RESOLUTION_MODE    (0x04)

#define BMP280_ULTRALOWPOWER_OVERSAMP_PRESSURE          BMP280_OVERSAMP_1X
#define BMP280_ULTRALOWPOWER_OVERSAMP_TEMPERATURE       BMP280_OVERSAMP_1X

#define BMP280_LOWPOWER_OVERSAMP_PRESSURE	         BMP280_OVERSAMP_2X
#define BMP280_LOWPOWER_OVERSAMP_TEMPERATURE	         BMP280_OVERSAMP_1X

#define BMP280_STANDARDRESOLUTION_OVERSAMP_PRESSURE     BMP280_OVERSAMP_4X
#define BMP280_STANDARDRESOLUTION_OVERSAMP_TEMPERATURE  BMP280_OVERSAMP_1X

#define BMP280_HIGHRESOLUTION_OVERSAMP_PRESSURE         BMP280_OVERSAMP_8X
#define BMP280_HIGHRESOLUTION_OVERSAMP_TEMPERATURE      BMP280_OVERSAMP_1X

#define BMP280_ULTRAHIGHRESOLUTION_OVERSAMP_PRESSURE       BMP280_OVERSAMP_16X
#define BMP280_ULTRAHIGHRESOLUTION_OVERSAMP_TEMPERATURE    BMP280_OVERSAMP_2X
/************************************************/
/**\name	FILTER DEFINITION       */
/***********************************************/
#define BMP280_FILTER_COEFF_OFF               (0x00)
#define BMP280_FILTER_COEFF_2                 (0x01)
#define BMP280_FILTER_COEFF_4                 (0x02)
#define BMP280_FILTER_COEFF_8                 (0x03)
#define BMP280_FILTER_COEFF_16                (0x04)
/************************************************/
/**\name	DELAY TIME DEFINITION       */
/***********************************************/
#define T_INIT_MAX					(20)
/* 20/16 = 1.25 ms */
#define T_MEASURE_PER_OSRS_MAX				(37)
/* 37/16 = 2.3125 ms*/
#define T_SETUP_PRESSURE_MAX				(10)
/* 10/16 = 0.625 ms */
/************************************************/
/**\name	CALIBRATION PARAMETERS DEFINITION       */
/***********************************************/
/*calibration parameters */
#define BMP280_TEMPERATURE_CALIB_DIG_T1_LSB_REG             (0x88)
#define BMP280_TEMPERATURE_CALIB_DIG_T1_MSB_REG             (0x89)
#define BMP280_TEMPERATURE_CALIB_DIG_T2_LSB_REG             (0x8A)
#define BMP280_TEMPERATURE_CALIB_DIG_T2_MSB_REG             (0x8B)
#define BMP280_TEMPERATURE_CALIB_DIG_T3_LSB_REG             (0x8C)
#define BMP280_TEMPERATURE_CALIB_DIG_T3_MSB_REG             (0x8D)
#define BMP280_PRESSURE_CALIB_DIG_P1_LSB_REG                (0x8E)
#define BMP280_PRESSURE_CALIB_DIG_P1_MSB_REG                (0x8F)
#define BMP280_PRESSURE_CALIB_DIG_P2_LSB_REG                (0x90)
#define BMP280_PRESSURE_CALIB_DIG_P2_MSB_REG                (0x91)
#define BMP280_PRESSURE_CALIB_DIG_P3_LSB_REG                (0x92)
#define BMP280_PRESSURE_CALIB_DIG_P3_MSB_REG                (0x93)
#define BMP280_PRESSURE_CALIB_DIG_P4_LSB_REG                (0x94)
#define BMP280_PRESSURE_CALIB_DIG_P4_MSB_REG                (0x95)
#define BMP280_PRESSURE_CALIB_DIG_P5_LSB_REG                (0x96)
#define BMP280_PRESSURE_CALIB_DIG_P5_MSB_REG                (0x97)
#define BMP280_PRESSURE_CALIB_DIG_P6_LSB_REG                (0x98)
#define BMP280_PRESSURE_CALIB_DIG_P6_MSB_REG                (0x99)
#define BMP280_PRESSURE_CALIB_DIG_P7_LSB_REG                (0x9A)
#define BMP280_PRESSURE_CALIB_DIG_P7_MSB_REG                (0x9B)
#define BMP280_PRESSURE_CALIB_DIG_P8_LSB_REG                (0x9C)
#define BMP280_PRESSURE_CALIB_DIG_P8_MSB_REG                (0x9D)
#define BMP280_PRESSURE_CALIB_DIG_P9_LSB_REG                (0x9E)
#define BMP280_PRESSURE_CALIB_DIG_P9_MSB_REG                (0x9F)
/************************************************/
/**\name	REGISTER ADDRESS DEFINITION       */
/***********************************************/
#define BMP280_CHIP_ID_REG                   (0xD0)  /*Chip ID Register */
#define BMP280_RST_REG                       (0xE0) /*Softreset Register */
#define BMP280_STAT_REG                      (0xF3)  /*Status Register */
#define BMP280_CTRL_MEAS_REG                 (0xF4)  /*Ctrl Measure Register */
#define BMP280_CONFIG_REG                    (0xF5)  /*Configuration Register */
#define BMP280_PRESSURE_MSB_REG              (0xF7)  /*Pressure MSB Register */
#define BMP280_PRESSURE_LSB_REG              (0xF8)  /*Pressure LSB Register */
#define BMP280_PRESSURE_XLSB_REG             (0xF9)  /*Pressure XLSB Register */
#define BMP280_TEMPERATURE_MSB_REG           (0xFA)  /*Temperature MSB Reg */
#define BMP280_TEMPERATURE_LSB_REG           (0xFB)  /*Temperature LSB Reg */
#define BMP280_TEMPERATURE_XLSB_REG          (0xFC)  /*Temperature XLSB Reg */
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION      */
/***********************************************/
/* Status Register */
#define BMP280_STATUS_REG_MEASURING__POS           (3)
#define BMP280_STATUS_REG_MEASURING__MSK           (0x08)
#define BMP280_STATUS_REG_MEASURING__LEN           (1)
#define BMP280_STATUS_REG_MEASURING__REG           (BMP280_STAT_REG)

#define BMP280_STATUS_REG_IM_UPDATE__POS            (0)
#define BMP280_STATUS_REG_IM_UPDATE__MSK            (0x01)
#define BMP280_STATUS_REG_IM_UPDATE__LEN            (1)
#define BMP280_STATUS_REG_IM_UPDATE__REG           (BMP280_STAT_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR TEMPERATURE OVERSAMPLING */
/***********************************************/
/* Control Measurement Register */
#define BMP280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__POS             (5)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__MSK             (0xE0)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__LEN             (3)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__REG             \
(BMP280_CTRL_MEAS_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR PRESSURE OVERSAMPLING */
/***********************************************/
#define BMP280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__POS             (2)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__MSK             (0x1C)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__LEN             (3)
#define BMP280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__REG             \
(BMP280_CTRL_MEAS_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR POWER MODE */
/***********************************************/
#define BMP280_CTRL_MEAS_REG_POWER_MODE__POS              (0)
#define BMP280_CTRL_MEAS_REG_POWER_MODE__MSK              (0x03)
#define BMP280_CTRL_MEAS_REG_POWER_MODE__LEN              (2)
#define BMP280_CTRL_MEAS_REG_POWER_MODE__REG             (BMP280_CTRL_MEAS_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR STANDBY DURATION */
/***********************************************/
/* Configuration Register */
#define BMP280_CONFIG_REG_STANDBY_DURN__POS                 (5)
#define BMP280_CONFIG_REG_STANDBY_DURN__MSK                 (0xE0)
#define BMP280_CONFIG_REG_STANDBY_DURN__LEN                 (3)
#define BMP280_CONFIG_REG_STANDBY_DURN__REG                 (BMP280_CONFIG_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR IIR FILTER */
/***********************************************/
#define BMP280_CONFIG_REG_FILTER__POS              (2)
#define BMP280_CONFIG_REG_FILTER__MSK              (0x1C)
#define BMP280_CONFIG_REG_FILTER__LEN              (3)
#define BMP280_CONFIG_REG_FILTER__REG              (BMP280_CONFIG_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR SPI ENABLE*/
/***********************************************/
#define BMP280_CONFIG_REG_SPI3_ENABLE__POS             (0)
#define BMP280_CONFIG_REG_SPI3_ENABLE__MSK             (0x01)
#define BMP280_CONFIG_REG_SPI3_ENABLE__LEN             (1)
#define BMP280_CONFIG_REG_SPI3_ENABLE__REG             (BMP280_CONFIG_REG)
/************************************************/
/**\name	BIT LENGTH,POSITION AND MASK DEFINITION
FOR PRESSURE AND TEMPERATURE DATA REGISTERS */
/***********************************************/
/* Data Register */
#define BMP280_PRESSURE_XLSB_REG_DATA__POS         (4)
#define BMP280_PRESSURE_XLSB_REG_DATA__MSK         (0xF0)
#define BMP280_PRESSURE_XLSB_REG_DATA__LEN         (4)
#define BMP280_PRESSURE_XLSB_REG_DATA__REG         (BMP280_PRESSURE_XLSB_REG)

#define BMP280_TEMPERATURE_XLSB_REG_DATA__POS      (4)
#define BMP280_TEMPERATURE_XLSB_REG_DATA__MSK      (0xF0)
#define BMP280_TEMPERATURE_XLSB_REG_DATA__LEN      (4)
#define BMP280_TEMPERATURE_XLSB_REG_DATA__REG      (BMP280_TEMPERATURE_XLSB_REG)
/************************************************/
/**\name	BUS READ AND WRITE FUNCTION POINTERS */
/***********************************************/
#define BMP280_WR_FUNC_PTR	s8 (*bus_write)(u8, u8, u8 *, u8)

#define BMP280_RD_FUNC_PTR	s8 (*bus_read)(u8, u8, u8 *, u8)

#define BMP280_MDELAY_DATA_TYPE u32
/****************************************************/
/**\name	DEFINITIONS FOR ARRAY SIZE OF DATA   */
/***************************************************/
#define	BMP280_TEMPERATURE_DATA_SIZE		(3)
#define	BMP280_PRESSURE_DATA_SIZE		(3)
#define	BMP280_DATA_FRAME_SIZE			(6)
#define	BMP280_CALIB_DATA_SIZE			(24)

#define	BMP280_TEMPERATURE_MSB_DATA		(0)
#define	BMP280_TEMPERATURE_LSB_DATA		(1)
#define	BMP280_TEMPERATURE_XLSB_DATA		(2)

#define	BMP280_PRESSURE_MSB_DATA		(0)
#define	BMP280_PRESSURE_LSB_DATA		(1)
#define	BMP280_PRESSURE_XLSB_DATA		(2)

#define	BMP280_DATA_FRAME_PRESSURE_MSB_BYTE	(0)
#define	BMP280_DATA_FRAME_PRESSURE_LSB_BYTE	(1)
#define	BMP280_DATA_FRAME_PRESSURE_XLSB_BYTE	(2)
#define	BMP280_DATA_FRAME_TEMPERATURE_MSB_BYTE	(3)
#define	BMP280_DATA_FRAME_TEMPERATURE_LSB_BYTE	(4)
#define	BMP280_DATA_FRAME_TEMPERATURE_XLSB_BYTE	(5)

/****************************************************/
/**\name	ARRAY PARAMETER FOR CALIBRATION     */
/***************************************************/
#define	BMP280_TEMPERATURE_CALIB_DIG_T1_LSB	(0)
#define	BMP280_TEMPERATURE_CALIB_DIG_T1_MSB	(1)
#define	BMP280_TEMPERATURE_CALIB_DIG_T2_LSB	(2)
#define	BMP280_TEMPERATURE_CALIB_DIG_T2_MSB	(3)
#define	BMP280_TEMPERATURE_CALIB_DIG_T3_LSB	(4)
#define	BMP280_TEMPERATURE_CALIB_DIG_T3_MSB	(5)
#define	BMP280_PRESSURE_CALIB_DIG_P1_LSB	(6)
#define	BMP280_PRESSURE_CALIB_DIG_P1_MSB	(7)
#define	BMP280_PRESSURE_CALIB_DIG_P2_LSB	(8)
#define	BMP280_PRESSURE_CALIB_DIG_P2_MSB	(9)
#define	BMP280_PRESSURE_CALIB_DIG_P3_LSB	(10)
#define	BMP280_PRESSURE_CALIB_DIG_P3_MSB	(11)
#define	BMP280_PRESSURE_CALIB_DIG_P4_LSB	(12)
#define	BMP280_PRESSURE_CALIB_DIG_P4_MSB	(13)
#define	BMP280_PRESSURE_CALIB_DIG_P5_LSB	(14)
#define	BMP280_PRESSURE_CALIB_DIG_P5_MSB	(15)
#define	BMP280_PRESSURE_CALIB_DIG_P6_LSB	(16)
#define	BMP280_PRESSURE_CALIB_DIG_P6_MSB	(17)
#define	BMP280_PRESSURE_CALIB_DIG_P7_LSB	(18)
#define	BMP280_PRESSURE_CALIB_DIG_P7_MSB	(19)
#define	BMP280_PRESSURE_CALIB_DIG_P8_LSB	(20)
#define	BMP280_PRESSURE_CALIB_DIG_P8_MSB	(21)
#define	BMP280_PRESSURE_CALIB_DIG_P9_LSB	(22)
#define	BMP280_PRESSURE_CALIB_DIG_P9_MSB	(23)
/**************************************************************/
/**\name	STRUCTURE DEFINITIONS                         */
/**************************************************************/
/*!
 * @brief This structure holds all device specific calibration parameters
 */
struct bmp280_calib_param_t {
	u16 dig_T1;/**<calibration T1 data*/
	s16 dig_T2;/**<calibration T2 data*/
	s16 dig_T3;/**<calibration T3 data*/
	u16 dig_P1;/**<calibration P1 data*/
	s16 dig_P2;/**<calibration P2 data*/
	s16 dig_P3;/**<calibration P3 data*/
	s16 dig_P4;/**<calibration P4 data*/
	s16 dig_P5;/**<calibration P5 data*/
	s16 dig_P6;/**<calibration P6 data*/
	s16 dig_P7;/**<calibration P7 data*/
	s16 dig_P8;/**<calibration P8 data*/
	s16 dig_P9;/**<calibration P9 data*/

	s32 t_fine;/**<calibration t_fine data*/
};
/*!
 * @brief This structure holds BMP280 initialization parameters
 */
struct bmp280_t {
	struct bmp280_calib_param_t calib_param;/**<calibration data*/

	u8 chip_id;/**< chip id of the sensor*/
	u8 dev_addr;/**< device address of the sensor*/

	u8 oversamp_temperature;/**< temperature over sampling*/
	u8 oversamp_pressure;/**< pressure over sampling*/

	BMP280_WR_FUNC_PTR;/**< bus write function pointer*/
	BMP280_RD_FUNC_PTR;/**< bus read function pointer*/
	void (*delay_msec)(BMP280_MDELAY_DATA_TYPE);/**< delay function pointer*/
};
/**************************************************************/
/**\name	FUNCTION DECLARATIONS                         */
/**************************************************************/
/**************************************************************/
/**\name	FUNCTION FOR  INTIALIZATION                       */
/**************************************************************/
/*!
 *	@brief This function is used for initialize
 *	the bus read and bus write functions
 *  and assign the chip id and I2C address of the BMP280 sensor
 *	chip id is read in the register 0xD0 bit from 0 to 7
 *
 *	@param *bmp280 structure pointer.
 *
 *	@note While changing the parameter of the p_bmp280
 *	@note consider the following point:
 *	Changing the reference value of the parameter
 *	will changes the local copy or local reference
 *	make sure your changes will not
 *	affect the reference value of the parameter
 *	(Better case don't change the reference value of the parameter)
 *
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_init(struct bmp280_t *bmp280);
/**************************************************************/
/**\name	FUNCTION FOR READ UNCOMPENSATED TEMPERATURE     */
/**************************************************************/
/*!
 *	@brief This API is used to read uncompensated temperature
 *	in the registers 0xFA, 0xFB and 0xFC
 *	@note 0xFA -> MSB -> bit from 0 to 7
 *	@note 0xFB -> LSB -> bit from 0 to 7
 *	@note 0xFC -> LSB -> bit from 4 to 7
 *
 *	@param v_uncomp_temperature_s32 : The uncompensated temperature.
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_read_uncomp_temperature(
		s32 *v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ TRUE TEMPERATURE S32 OUTPUT    */
/**************************************************************/
/*!
 *	@brief Reads actual temperature
 *	from uncompensated temperature
 *	@note Returns the value in 0.01 degree Centigrade
 *	@note Output value of "5123" equals 51.23 DegC.
 *
 *
 *
 *  @param v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *
 *  @return Actual temperature output as s32
 *
*/
s32 bmp280_compensate_temperature_int32(s32 v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ UNCOMPENSATED PRESSURE     */
/**************************************************************/
/*!
 *	@brief This API is used to read uncompensated pressure.
 *	in the registers 0xF7, 0xF8 and 0xF9
 *	@note 0xF7 -> MSB -> bit from 0 to 7
 *	@note 0xF8 -> LSB -> bit from 0 to 7
 *	@note 0xF9 -> LSB -> bit from 4 to 7
 *
 *
 *
 *	@param v_uncomp_pressure_s32 : The value of uncompensated pressure
 *
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_read_uncomp_pressure(
		s32 *v_uncomp_pressure_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ TRUE PRESSURE S32 OUTPUT    */
/**************************************************************/
/*!
 *	@brief Reads actual pressure from uncompensated pressure
 *	and returns the value in Pascal(Pa)
 *	@note Output value of "96386" equals 96386 Pa =
 *	963.86 hPa = 963.86 millibar
 *
 *
 *
 *
 *  @param  v_uncomp_pressure_s32: value of uncompensated pressure
 *
 *
 *
 *  @return Returns the Actual pressure out put as s32
 *
*/
u32 bmp280_compensate_pressure_int32(s32 v_uncomp_pressure_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ UNCOMPENSATED TEMPERATURE AND PRESSURE  */
/**************************************************************/
/*!
 * @brief reads uncompensated pressure and temperature
 *
 *
 * @param  v_uncomp_pressure_s32: The value of uncompensated pressure.
 * @param  v_uncomp_temperature_s32: The value of uncompensated temperature.
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_read_uncomp_pressure_temperature(
		s32 *v_uncomp_pressure_s32, s32 *v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ TRUE TEMPERATURE AND PRESSURE    */
/**************************************************************/
/*!
 * @brief This API reads the true pressure and temperature
 *
 *
 *  @param  v_pressure_u32 : The value of compensated pressure.
 *  @param  v_temperature_s32 : The value of compensated temperature.
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_read_pressure_temperature(
		u32 *v_pressure_u32, s32 *v_pressure_s32);
/**************************************************************/
/**\name	FUNCTION FOR READ CALIBRATION DATA    */
/**************************************************************/
/*!
 *	@brief This API is used to
 *	calibration parameters used for calculation in the registers
 *
 *  parameter | Register address |   bit
 *------------|------------------|----------------
 *	dig_T1    |  0x88 and 0x89   | from 0 : 7 to 8: 15
 *	dig_T2    |  0x8A and 0x8B   | from 0 : 7 to 8: 15
 *	dig_T3    |  0x8C and 0x8D   | from 0 : 7 to 8: 15
 *	dig_P1    |  0x8E and 0x8F   | from 0 : 7 to 8: 15
 *	dig_P2    |  0x90 and 0x91   | from 0 : 7 to 8: 15
 *	dig_P3    |  0x92 and 0x93   | from 0 : 7 to 8: 15
 *	dig_P4    |  0x94 and 0x95   | from 0 : 7 to 8: 15
 *	dig_P5    |  0x96 and 0x97   | from 0 : 7 to 8: 15
 *	dig_P6    |  0x98 and 0x99   | from 0 : 7 to 8: 15
 *	dig_P7    |  0x9A and 0x9B   | from 0 : 7 to 8: 15
 *	dig_P8    |  0x9C and 0x9D   | from 0 : 7 to 8: 15
 *	dig_P9    |  0x9E and 0x9F   | from 0 : 7 to 8: 15
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_calib_param(void);
/**************************************************************/
/**\name	FUNCTION FOR OVERSAMPLING TEMPERATURE AND PRESSURE    */
/**************************************************************/
/*!
 *	@brief This API is used to get
 *	the temperature oversampling setting in the register 0xF4
 *	bits from 5 to 7
 *
 *        value             | Temperature oversampling
 *  ------------------------|------------------------------
 *       0x00               |  BMP280_OVERSAMP_SKIPPED
 *       0x01               |  BMP280_OVERSAMP_1X
 *       0x02               |  BMP280_OVERSAMP_2X
 *       0x03               |  BMP280_OVERSAMP_4X
 *       0x04               |  BMP280_OVERSAMP_8X
 *       0x05,0x06 and 0x07 |  BMP280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 :The value of temperature over sampling
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_oversamp_temperature(u8 *v_value_u8);
/*!
 *	@brief This API is used to set
 *	the temperature oversampling setting in the register 0xF4
 *	bits from 5 to 7
 *
 *        value             | Temperature oversampling
 *  ------------------------|------------------------------
 *       0x00               |  BMP280_OVERSAMP_SKIPPED
 *       0x01               |  BMP280_OVERSAMP_1X
 *       0x02               |  BMP280_OVERSAMP_2X
 *       0x03               |  BMP280_OVERSAMP_4X
 *       0x04               |  BMP280_OVERSAMP_8X
 *       0x05,0x06 and 0x07 |  BMP280_OVERSAMP_16X
 *
 *
 *  @param v_value_u8 :The value of temperature over sampling
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_oversamp_temperature(u8 v_value_u8);
/*!
 *	@brief This API is used to get
 *	the pressure oversampling setting in the register 0xF4
 *	bits from 2 to 4
 *
 *        value             | Pressure oversampling
 *  ------------------------|------------------------------
 *       0x00               |  BMP280_OVERSAMP_SKIPPED
 *       0x01               |  BMP280_OVERSAMP_1X
 *       0x02               |  BMP280_OVERSAMP_2X
 *       0x03               |  BMP280_OVERSAMP_4X
 *       0x04               |  BMP280_OVERSAMP_8X
 *       0x05,0x06 and 0x07 |  BMP280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of pressure over sampling
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_oversamp_pressure(u8 *v_value_u8);
/*!
 *	@brief This API is used to set
 *	the pressure oversampling setting in the register 0xF4
 *	bits from 2 to 4
 *
 *        value             | Pressure oversampling
 *  ------------------------|------------------------------
 *       0x00               |  BMP280_OVERSAMP_SKIPPED
 *       0x01               |  BMP280_OVERSAMP_1X
 *       0x02               |  BMP280_OVERSAMP_2X
 *       0x03               |  BMP280_OVERSAMP_4X
 *       0x04               |  BMP280_OVERSAMP_8X
 *       0x05,0x06 and 0x07 |  BMP280_OVERSAMP_16X
 *
 *
 *  @param  v_value_u8 : The value of pressure over sampling
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_oversamp_pressure(u8 v_value_u8);
/**************************************************************/
/**\name	FUNCTION FOR POWER MODE    */
/**************************************************************/
/*!
 *	@brief This API used to get the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode value
 *  value            |   Power mode
 * ------------------|------------------
 *	0x00             | BMP280_SLEEP_MODE
 *	0x01 and 0x02    | BMP280_FORCED_MODE
 *	0x03             | BMP280_NORMAL_MODE
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_power_mode(u8 *v_power_mode_u8);
/*!
 *	@brief This API used to set the
 *	Operational Mode from the sensor in the register 0xF4 bit 0 and 1
 *
 *
 *
 *	@param v_power_mode_u8 : The value of power mode value
 *  value            |   Power mode
 * ------------------|------------------
 *	0x00             | BMP280_SLEEP_MODE
 *	0x01 and 0x02    | BMP280_FORCED_MODE
 *	0x03             | BMP280_NORMAL_MODE
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_power_mode(u8 v_power_mode_u8);
/**************************************************************/
/**\name	FUNCTION FOR SOFT RESET   */
/**************************************************************/
/*!
 * @brief Used to reset the sensor
 * The value 0xB6 is written to the
 * 0xE0 register the device is reset using the
 * complete power-on-reset procedure.
 * Soft reset can be easily set using bmp280_set_softreset().
 *
 * @note Usage Hint : bmp280_set_softreset()
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_soft_rst(void);
/**************************************************************/
/**\name	FUNCTION FOR SPI ENABLE    */
/**************************************************************/
/*!
 *	@brief This API used to get the sensor
 *	SPI mode(communication type) in the register 0xF5 bit 0
 *
 *
 *
 *	@param v_enable_disable_u8 : The spi3 enable or disable state
 *    value    | Description
 *  -----------|---------------
 *     0       | Disable
 *     1       | Enable
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_spi3(u8 *v_enable_disable_u8);
/*!
 *	@brief This API used to set the sensor
 *	SPI mode(communication type) in the register 0xF5 bit 0
 *
 *
 *
 *	@param v_enable_disable_u8 : The spi3 enable or disable state
 *    value    | Description
 *  -----------|---------------
 *     0       | Disable
 *     1       | Enable
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_spi3(u8 v_enable_disable_u8);
/**************************************************************/
/**\name	FUNCTION FOR IIR FILTER SETTING   */
/**************************************************************/
/*!
 *	@brief This API is used to reads filter setting
 *	in the register 0xF5 bit 3 and 4
 *
 *
 *
 *	@param v_value_u8 : The value of filter coefficient
 *	value	    |	Filter coefficient
 * -------------|-------------------------
 *	0x00        | BMP280_FILTER_COEFF_OFF
 *	0x01        | BMP280_FILTER_COEFF_2
 *	0x02        | BMP280_FILTER_COEFF_4
 *	0x03        | BMP280_FILTER_COEFF_8
 *	0x04        | BMP280_FILTER_COEFF_16
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_filter(u8 *v_value_u8);
/*!
 *	@brief This API is used to write filter setting
 *	in the register 0xF5 bit 3 and 4
 *
 *
 *
 *	@param v_value_u8 : The value of filter coefficient
 *	value	    |	Filter coefficient
 * -------------|-------------------------
 *	0x00        | BMP280_FILTER_COEFF_OFF
 *	0x01        | BMP280_FILTER_COEFF_2
 *	0x02        | BMP280_FILTER_COEFF_4
 *	0x03        | BMP280_FILTER_COEFF_8
 *	0x04        | BMP280_FILTER_COEFF_16
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_filter(u8 v_value_u8);
/**************************************************************/
/**\name	FUNCTION FOR STANDBY DURATION   */
/**************************************************************/
/*!
 *	@brief This API used to Read the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *
 *	@param v_standby_durn_u8 : The standby duration time value.
 *  value     |  standby duration
 * -----------|--------------------
 *    0x00    | BMP280_STANDBYTIME_1_MS
 *    0x01    | BMP280_STANDBYTIME_63_MS
 *    0x02    | BMP280_STANDBYTIME_125_MS
 *    0x03    | BMP280_STANDBYTIME_250_MS
 *    0x04    | BMP280_STANDBYTIME_500_MS
 *    0x05    | BMP280_STANDBYTIME_1000_MS
 *    0x06    | BMP280_STANDBYTIME_2000_MS
 *    0x07    | BMP280_STANDBYTIME_4000_MS
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_standby_durn(u8 *v_standby_durn_u8);
/*!
 *	@brief This API used to Read the
 *	standby duration time from the sensor in the register 0xF5 bit 5 to 7
 *	@note Normal mode comprises an automated perpetual cycling between an (active)
 *	Measurement period and an (inactive) standby period.
 *	@note The standby time is determined by the contents of the register t_sb.
 *	Standby time can be set using BMP280_STANDBYTIME_125_MS.
 *
 *	@note bmp280_set_standby_durn(BMP280_STANDBYTIME_125_MS)
 *
 *
 *
 *	@param v_standby_durn_u8 : The standby duration time value.
 *  value     |  standby duration
 * -----------|--------------------
 *    0x00    | BMP280_STANDBYTIME_1_MS
 *    0x01    | BMP280_STANDBYTIME_63_MS
 *    0x02    | BMP280_STANDBYTIME_125_MS
 *    0x03    | BMP280_STANDBYTIME_250_MS
 *    0x04    | BMP280_STANDBYTIME_500_MS
 *    0x05    | BMP280_STANDBYTIME_1000_MS
 *    0x06    | BMP280_STANDBYTIME_2000_MS
 *    0x07    | BMP280_STANDBYTIME_4000_MS
 *
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_standby_durn(u8 v_standby_durn_u8);
/**************************************************************/
/**\name	FUNCTION FOR WORK MODE   */
/**************************************************************/
/*!
 *	@brief This API is used to write
 *	 the working mode of the sensor
 *
 *
 *  @param v_work_mode_u8 : The value of work mode
 *   value      |  mode
 * -------------|-------------
 *    0         | BMP280_ULTRA_LOW_POWER_MODE
 *    1         | BMP280_LOW_POWER_MODE
 *    2         | BMP280_STANDARD_RESOLUTION_MODE
 *    3         | BMP280_HIGH_RESOLUTION_MODE
 *    4         | BMP280_ULTRA_HIGH_RESOLUTION_MODE
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_set_work_mode(u8 v_work_mode_u8);
/**************************************************************/
/**\name	FUNCTION FOR FORCE MODE READING    */
/**************************************************************/
/*!
 *	@brief This API used to read both
 *	uncompensated pressure and temperature in forced mode
 *
 *
 *  @param  v_uncomp_pressure_s32: The value of uncompensated pressure.
 *  @param  v_uncomp_temperature_s32: The value of uncompensated temperature
 *
 *
 *  @return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMP280_RETURN_FUNCTION_TYPE bmp280_get_forced_uncomp_pressure_temperature(
		s32 *v_uncomp_pressure_s32, s32 *v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR COMMON READ AND WRITE    */
/**************************************************************/
/*!
 * @brief
 *	This API write the data to
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BMP280_RETURN_FUNCTION_TYPE bmp280_write_register(u8 v_addr_u8,
		u8 *v_data_u8, u8 v_len_u8);
/*!
 * @brief
 *	This API reads the data from
 *	the given register
 *
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
 */
BMP280_RETURN_FUNCTION_TYPE bmp280_read_register(u8 v_addr_u8,
		u8 *v_data_u8, u8 v_len_u8);
/**************************************************************/
/**\name	FUNCTION FOR TRUE TEMPERATURE CALCULATION   */
/**************************************************************/
#ifdef BMP280_ENABLE_FLOAT
/*!
 * @brief This API used to read
 * actual temperature from uncompensated temperature
 * @note Returns the value in Degree centigrade
 * @note Output value of "51.23" equals 51.23 DegC.
 *
 *
 *
 *  @param v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *
 *  @return
 *	Actual temperature in floating point
 *
*/
double bmp280_compensate_temperature_double(s32 v_uncomp_temperature_s32);
/**************************************************************/
/**\name	FUNCTION FOR TRUE PRESSURE CALCULATION   */
/**************************************************************/
/*!
 *	@brief Reads actual pressure from uncompensated pressure
 *	and returns pressure in Pa as double.
 *	@note Output value of "96386.2"
 *	equals 96386.2 Pa = 963.862 hPa.
 *
 *
 *
 *  @param v_uncomp_pressure_s32 : value of uncompensated pressure
 *
 *
 *
 *  @return
 *	Actual pressure in floating point
 *
*/
double bmp280_compensate_pressure_double(s32 v_uncomp_pressure_s32);
#endif
#if defined(BMP280_ENABLE_INT64) && defined(BMP280_64BITSUPPORT_PRESENT)
/*!
 * @brief This API used to read actual pressure from uncompensated pressure
 * @note returns the value in Pa as unsigned 32 bit
 * integer in Q24.8 format (24 integer bits and
 * 8 fractional bits). Output value of "24674867"
 * represents 24674867 / 256 = 96386.2 Pa = 963.862 hPa
 *
 *
 *
 *  @param v_uncomp_pressure_s32 : value of uncompensated pressure
 *
 *
 *
 *  @return actual pressure as 64bit output
 *
*/
u32 bmp280_compensate_pressure_int64(s32 v_uncomp_pressure_s32);
#endif
/**************************************************************/
/**\name	FUNCTION FOR DELAY CALCULATION DURING FORCEMODE  */
/**************************************************************/
/*!
 * @brief Computing waiting time for sensor data read
 *
 *
 *
 *
 *  @param v_delaytime_u8r: The value of delay time
 *
 *
 *  @return 0
 *
 *
 */
BMP280_RETURN_FUNCTION_TYPE bmp280_compute_wait_time(u8
		*v_delaytime_u8r);
#endif
