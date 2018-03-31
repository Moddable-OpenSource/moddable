/***************************************************************************//**
 * @file main.c
 * @brief Simple RAIL application which does not include hal
 * @copyright Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com
 ******************************************************************************/
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_rmu.h"

#include "rail.h"
#include "rail_config.h"

#include "hal-config.h"
#include "gpiointerrupt.h"

// Memory manager configuration
#define MAX_BUFFER_SIZE  256

int receiveBuffer[MAX_BUFFER_SIZE];
uint8_t radioStarted = 0;
uint32_t radioTxPower;
uint16_t rxOutstanding = 0;
uint16_t txOutstanding = 0;
uint8_t channel = 0;

// volatile uint32_t msTickCount = 0;
int gResetCause = 0;
uint32_t wakeupPin = 0;


void assertEFM() { }

// Prototypes
void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events);

RAIL_Handle_t railHandle;

static RAIL_Config_t railCfg = {
  .eventsCallback = &RAILCb_Generic,
};

void initRadio()
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

  EMU_DCDCInit(&dcdcInit);
  CMU_HFXOInit(&hfxoInit);

  /* Switch HFCLK to HFXO and disable HFRCO */
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

  railHandle = RAIL_Init(&railCfg, NULL);
  if (railHandle == NULL) {
    while (1) ;
  }
  RAIL_ConfigCal(railHandle, RAIL_CAL_ALL);

  // Set us to a valid channel for this config and force an update in the main
  // loop to restart whatever action was going on
  RAIL_ConfigChannels(railHandle, channelConfigs[0], NULL);

  // Initialize the PA now that the HFXO is up and the timing is correct
  RAIL_TxPowerConfig_t txPowerConfig = {
#if HAL_PA_2P4_LOWPOWER
    .mode = RAIL_TX_POWER_MODE_2P4_LP,
#else
    .mode = RAIL_TX_POWER_MODE_2P4_HP,
#endif
    .voltage = HAL_PA_VOLTAGE,
    .rampTime = HAL_PA_RAMP,
  };
  if (channelConfigs[0]->configs[0].baseFrequency < 1000000UL) {
    // Use the Sub-GHz PA if required
    txPowerConfig.mode = RAIL_TX_POWER_MODE_SUBGIG;
  }
  if (RAIL_ConfigTxPower(railHandle, &txPowerConfig) != RAIL_STATUS_NO_ERROR) {
    // Error: The PA could not be initialized due to an improper configuration.
    // Please ensure your configuration is valid for the selected part.
    while (1) ;
  }
  RAIL_SetTxPower(railHandle, HAL_PA_POWER);
}

extern uint32_t gModIdleSleep;
void modRadioInit() {
    RAIL_StateTransitions_t txTransitions, rxTransitions;

	if (radioStarted)
		return;

	initRadio();

    radioStarted = true;
    gModIdleSleep = 1;

     // Configure RAIL callbacks
    RAIL_ConfigEvents(railHandle,
                        RAIL_EVENTS_ALL,
                        (RAIL_EVENT_RX_PACKET_RECEIVED
                         | RAIL_EVENT_TX_PACKET_SENT
                         | RAIL_EVENT_TX_UNDERFLOW));

    txTransitions.success = RAIL_RF_STATE_RX;
    txTransitions.error = RAIL_RF_STATE_RX;
    rxTransitions.success = RAIL_RF_STATE_RX;
    rxTransitions.error = RAIL_RF_STATE_RX;
    RAIL_SetTxTransitions(railHandle, &txTransitions);
    RAIL_SetRxTransitions(railHandle, &rxTransitions);

}

void modRadioSleep() {
    while (txOutstanding)
        gecko_delay(1);
    RAIL_Idle(railHandle, RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS, true);
    gecko_delay(1);
}

void modRadioListen(uint32_t mode) {
    RAIL_Idle(railHandle, RAIL_IDLE, true);
    RAIL_StartRx(railHandle, channel, NULL);
}

// modRadioQueueReceivedMessage() is defined in the radio module.
// It is included "weak" here so that this main can be used for
// apps that don't use the radio module.
void modRadioQueueReceivedMessage(void *buffer, uint32_t size) __attribute__ ((weak));
void modRadioQueueReceivedMessage(void *buffer, uint32_t size)
{
#pragma unused(buffer, size);
}

void modRadioPostMessage(void *buffer, uint32_t size) {
	uint8_t ret;

    RAIL_Idle(railHandle, RAIL_IDLE, true);
    RAIL_SetTxFifo(railHandle, buffer, size, size);
    ret = RAIL_StartTx(railHandle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);

    if (0 == ret)
        txOutstanding = true;
    rxOutstanding++;
    free(buffer);
}

void modRadioWaitUntilIdle() {

    while (RAIL_RF_STATE_IDLE != RAIL_GetRadioState(railHandle))
        gecko_delay(1);
}

uint32_t modRadioSetTxPower(uint32_t txPower) {
    radioTxPower = txPower;
}

uint32_t modRadioGetTxPower() {
    if (radioStarted)
        return RAIL_GetTxPowerDbm(railHandle);
    else
        return radioTxPower;
}

void geckoSleepSensors() {
}


int main(void)
{
  CHIP_Init();
 // initRadio();

  gResetCause = RMU_ResetCauseGet();
  if (gResetCause & (RMU_RSTCAUSE_EM4RST | RMU_RSTCAUSE_SYSREQRST | RMU_RSTCAUSE_EXTRST)) {
	  wakeupPin = GPIO_EM4GetPinWakeupCause();
  }
  RMU_ResetCauseClear();
//  RMU_ResetControl(rmuResetPin, rmuResetModeLimited);
  RMU_ResetControl(rmuResetSys, rmuResetModeLimited);

  xs_setup();

  while (1) {
  	  xs_loop(NULL);
  }
  return 0;
}

void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events)
{
    if (events & RAIL_EVENT_RX_PACKET_RECEIVED) {
        RAIL_RxPacketHandle_t packet;
        RAIL_RxPacketInfo_t packetInfo;

        packet = RAIL_GetRxPacketInfo(railHandle, RAIL_RX_PACKET_HANDLE_OLDEST, &packetInfo);
        if (RAIL_RX_PACKET_READY_SUCCESS == packetInfo.packetStatus) {
            if (packetInfo.firstPortionBytes != packetInfo.packetBytes) {
                    // data is split between firstPortionData and lastPortionData
            }
            else {
            	modRadioQueueReceivedMessage(packetInfo.firstPortionData, packetInfo.packetBytes);
            }

            RAIL_ReleaseRxPacket(railHandle, packet);
            --rxOutstanding;
        }
    }
    if (events & RAIL_EVENT_TX_PACKET_SENT) {
        txOutstanding = false;
    }
    if (events & RAIL_EVENT_TX_UNDERFLOW) {
//    packetRx = true;
    }
}




