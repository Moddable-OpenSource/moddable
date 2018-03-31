/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_adc.h"
#include "em_lcd.h"
#include "em_gpio.h"
//#include "rtcdriver.h"

#include "mc.defines.h"

#define ADC_CLOCK               1000000                 /* ADC conversion clock */
#define ADC_CAL_INPUT           adcPosSelAPORT3XCH8     /* PA0 */
#define ADC_NEG_OFFSET_VALUE    0xfff0                  /* Negative offset calibration */
#define ADC_GAIN_CAL_VALUE      0xffd0                  /* Gain calibration */
#define ADC_12BIT_MAX           4096                    /* 2^12 */
#define ADC_16BIT_MAX           65536                   /* 2^16 */
#define ADC_CMP_GT_VALUE        3724                    /* ~3.0V for 3.3V AVDD */
#define ADC_CMP_LT_VALUE        620                     /* ~0.5V for 3.3V AVDD */

#define ADC_INPUT0              adcPosSelAPORT3XCH8     /* PA0 */
//#define ADC_INPUT0				adcPosSelAPORT3YCH23	/* PB7 */

#if !defined(MODDEF_ANALOG_INTERFACE_ADC)
	#define MODDEF_ANALOG_INTERFACE_ADC 0
#endif

#if MODDEF_ANALOG_INTERFACE_ADC == 0
	#define ANALOG_PORT		ADC0
	#define ANALOG_CLOCK	cmuClock_ADC0
	#define ANALOG_IRQ		ADC0_IRQn
#elif MODDEF_ANALOG_INTERFACE_ADC == 1
	#define ANALOG_PORT		ADC1
	#define ANALOG_CLOCK	cmuClock_ADC1
	#define ANALOG_IRQ		ADC1_IRQn
#else
	#error bad Analog port
#endif

/***************************************************************************//**
 * @brief
 *   Calibrate offset and gain for the specified reference.
 *   Supports currently only single ended gain calibration.
 *   Could easily be expanded to support differential gain calibration.
 *
 * @details
 *   The offset calibration routine measures 0 V with the ADC, and adjust
 *   the calibration register until the converted value equals 0.
 *   The gain calibration routine needs an external reference voltage equal
 *   to the top value for the selected reference. For example if the 2.5 V
 *   reference is to be calibrated, the external supply must also equal 2.5V.
 *
 * @param[in] adc
 *   Pointer to ADC peripheral register block.
 *
 * @param[in] ref
 *   Reference used during calibration. Can be both external and internal
 *   references.
 *
 * @return
 *   The final value of the calibration register, note that the calibration
 *   register gets updated with this value during the calibration.
 *   No need to load the calibration values after the function returns.
 ******************************************************************************/
uint32_t ADC_Calibration(ADC_TypeDef *adc, ADC_Ref_TypeDef ref)
{
  int32_t  sample;
  uint32_t cal;

  /* Binary search variables */
  uint8_t high;
  uint8_t mid;
  uint8_t low;

  /* Reset ADC to be sure we have default settings and wait for ongoing */
  /* conversions to be complete. */
  ADC_Reset(adc);

  ADC_Init_TypeDef       init       = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

  /* Init common settings for both single conversion and scan mode */
  init.timebase = ADC_TimebaseCalc(0);
  /* Might as well finish conversion as quickly as possibly since polling */
  /* for completion. */
  /* Set ADC clock to 7 MHz, use default HFPERCLK */
  init.prescale = ADC_PrescaleCalc(7000000, 0);

  /* Set an oversampling rate for more accuracy */
  init.ovsRateSel = adcOvsRateSel4096;
  /* Leave other settings at default values */
  ADC_Init(adc, &init);

  /* Init for single conversion use, connect POSSEL and NEGSEL to VSS. */
  singleInit.reference = ref;
  singleInit.posSel    = adcPosSelVSS;
  singleInit.negSel    = adcNegSelVSS;
  singleInit.acqTime   = adcAcqTime16;
  singleInit.fifoOverwrite = true;
  /* Enable oversampling rate */
  singleInit.resolution = adcResOVS;

  ADC_InitSingle(adc, &singleInit);

  /* Positive single ended offset calibration, scan from higher ADC results */
  mid = 15;
  adc->CAL |= ADC_CAL_CALEN;
  adc->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

  while (1)
  {
    /* Write to calibration register */
    cal      = adc->CAL & ~(_ADC_CAL_SINGLEOFFSET_MASK | _ADC_CAL_SCANOFFSET_MASK);
    cal     |= (uint8_t)(mid) << _ADC_CAL_SINGLEOFFSET_SHIFT;
    cal     |= (uint8_t)(mid) << _ADC_CAL_SCANOFFSET_SHIFT;
    adc->CAL = cal;

    /* Start ADC single conversion */
    ADC_Start(ANALOG_PORT, adcStartSingle);
    while ((ANALOG_PORT->IF & ADC_IF_SINGLE) == 0)
      ;

    sample = ADC_DataSingleGet(ANALOG_PORT);
    if ((sample == 0) || (mid == 0))
    {
      break;
    }
    mid--;
  }

  /* Negative single ended offset calibration, scan from lower ADC results */
  mid = 0;
  adc->CAL |= ADC_CAL_OFFSETINVMODE;
  adc->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

  while (1)
  {
    /* Write to calibration register */
    cal      = adc->CAL & ~(_ADC_CAL_SINGLEOFFSETINV_MASK | _ADC_CAL_SCANOFFSETINV_MASK);
    cal     |= (uint8_t)(mid) << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
    cal     |= (uint8_t)(mid) << _ADC_CAL_SCANOFFSETINV_SHIFT;
    adc->CAL = cal;

    /* Start ADC single conversion */
    ADC_Start(ANALOG_PORT, adcStartSingle);
    while ((ANALOG_PORT->IF & ADC_IF_SINGLE) == 0)
      ;

    sample = ADC_DataSingleGet(ANALOG_PORT);
    if ((sample >= ADC_NEG_OFFSET_VALUE) || (mid == 15))
    {
      break;
    }
    mid++;
  }

  /* Positive single ended gain calibration */
  adc->SINGLECTRL &= ~(_ADC_SINGLECTRL_POSSEL_MASK);
  adc->SINGLECTRL |= (ADC_CAL_INPUT<< _ADC_SINGLECTRL_POSSEL_SHIFT);
  adc->CAL &= ~ADC_CAL_OFFSETINVMODE;
  adc->SINGLEFIFOCLEAR = ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;

  high = 128;
  low  = 0;

  /* Do binary search for gain calibration */
  while (low < high)
  {
    /* Calculate midpoint and write to calibration register */
    mid = low + (high - low) / 2;
    cal      = adc->CAL & ~(_ADC_CAL_SINGLEGAIN_MASK | _ADC_CAL_SCANGAIN_MASK);
    cal     |= mid << _ADC_CAL_SINGLEGAIN_SHIFT;
    cal     |= mid << _ADC_CAL_SCANGAIN_SHIFT;
    adc->CAL = cal;

    /* Do a conversion */
    ADC_Start(adc, adcStartSingle);
    while ((ANALOG_PORT->IF & ADC_IF_SINGLE) == 0)
      ;

    /* Get ADC result */
    sample = ADC_DataSingleGet(adc);

    /* Check result and decide in which part to repeat search */
    /* Compare with a value atleast one LSB's less than top to avoid overshooting */
    /* Since oversampling is used, the result is 16 bits, but a couple of lsb's */
    /* applies to the 12 bit result value, if 0xffd is the top value in 12 bit, this */
    /* is in turn 0xffd0 in the 16 bit result. */
    /* Calibration register has negative effect on result */
    if (sample > ADC_GAIN_CAL_VALUE)
    {
      /* Repeat search in top half. */
      low = mid + 1;
    }
    else if (sample < ADC_GAIN_CAL_VALUE)
    {
      /* Repeat search in bottom half. */
      high = mid;
    }
    else
    {
      /* Found it, exit while loop */
      break;
    }
  }

  /* Mask off CALEN bit */
  return(adc->CAL &= ~ADC_CAL_CALEN);
}

/**************************************************************************//**
 * @brief Initialize ADC for single and scan conversion
 *****************************************************************************/
void adcSetup(void)
{
  /* Enable ADC clock */
  CMU_ClockEnable(ANALOG_CLOCK, true);

#if 0
  /* Select AUXHFRCO for ADC ASYNC mode so that ADC can run on EM2 */
  CMU->ADCCTRL = CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;

  /* Initialize compare threshold for both single and scan conversion */
  ADC0->CMPTHR = _ADC_CMPTHR_RESETVALUE;
  ADC0->CMPTHR = (ADC_CMP_GT_VALUE << _ADC_CMPTHR_ADGT_SHIFT) +
                 (ADC_CMP_LT_VALUE << _ADC_CMPTHR_ADLT_SHIFT);
#endif
}

void adcTerminate(void) {
	CMU_ClockEnable(ANALOG_CLOCK, false);
}

/**************************************************************************//**
 * @brief Reset ADC related registers and parameters to default values.
 *****************************************************************************/
void adcReset(void)
{
#if 0
  /* Switch the ADCCLKMODE to SYNC */
  NVIC_DisableIRQ(ANALOG_IRQ);
  ANALOG_PORT->CTRL &= ~ADC_CTRL_ADCCLKMODE_ASYNC;
#endif

  /* Rest ADC registers */
  ADC_Reset(ANALOG_PORT);

  /* Reset AUXHFRCO to default */
  CMU_AUXHFRCOFreqSet(cmuAUXHFRCOFreq_19M0Hz);
}

#if MODDEF_ANALOG_INTERFACE_ADC == 0
void ADC0_IRQHandler(void)
#elif MODDEF_ANALOG_INTERFACE_ADC == 1
void ADC1_IRQHandler(void)
#endif
{
	ADC_IntClear(ANALOG_PORT, ADC_IEN_SINGLE);
}

/**************************************************************************//**
 * @brief ADC single and scan conversion example (Single-ended mode)
 * @param[in] ovs
 *   False for 12 bit ADC, True for 16 bit oversampling ADC.
 *****************************************************************************/
uint32_t adcSingle(bool ovs, uint32_t inputChan, uint32_t reference)
{
  uint32_t sample;

  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

  /* Init common issues for both single conversion and scan mode */
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(ADC_CLOCK, 0);
  if (ovs)
  {
    /* Set oversampling rate */
    init.ovsRateSel = adcOvsRateSel256;
  }
  ADC_Init(ANALOG_PORT, &init);

  /* Initialize for single conversion */
  singleInit.reference = reference;
  singleInit.posSel = inputChan;
  singleInit.negSel = adcNegSelVSS;
  if (ovs)
  {
    /* Enable oversampling rate */
    singleInit.resolution = adcResOVS;
  }
ADC_IntEnable(ANALOG_PORT, ADC_IEN_SINGLE);
NVIC_ClearPendingIRQ(ANALOG_IRQ);
  ADC_InitSingle(ANALOG_PORT, &singleInit);


  /* Start ADC single conversion */
  ADC_Start(ANALOG_PORT, adcStartSingle);
  while ((ANALOG_PORT->IF & ADC_IF_SINGLE) == 0)
#if 0
{
//	SCB->SCR = 0x10;
//	__WFE();
	EMU_EnterEM1();
	ADC_IntClear(ANALOG_PORT, ADC_IEN_SINGLE);
	NVIC_ClearPendingIRQ(ANALOG_IRQ);
}
#else
	;
#endif



ADC_IntDisable(ANALOG_PORT, ADC_IEN_SINGLE);
  /* Get ADC single result */
  sample = ADC_DataSingleGet(ANALOG_PORT);

  adcReset();
  return sample;
}
