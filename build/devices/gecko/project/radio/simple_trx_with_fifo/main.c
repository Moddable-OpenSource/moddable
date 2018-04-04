/***************************************************************************//**
 * @file main.c
 * @brief This application demonstrates the simplest exchange of transmit and receive operation in FIFO mode between two nodes.
 * @copyright Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "rail.h"
#include "rail_types.h"
#include "hal_common.h"
#include "rail_config.h"
#include "em_chip.h"
#include "bsp.h"
#include "retargetserial.h"
#include "gpiointerrupt.h"

#define MAX_FRAME_LENGTH (4093)
#define PACKET_HEADER_LEN (2)

#define LED_TX (0)
#define LED_RX  (1)
#define TX_FIFO_SIZE (512)
#define TX_FIFO_THRESHOLD (40)
#define RX_FIFO_THRESHOLD (100)

// Prototypes
void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events);
void radioInit();
void gpioCallback(uint8_t pin);
void printBuffer(uint8_t * buffer, const uint16_t bufferLength);
void buttonInit();
void sendPacket(uint16_t packetLength);
void startReceive();

RAIL_Handle_t railHandle = NULL;

const uint8_t channel = 0;
uint8_t txFifo[TX_FIFO_SIZE];

uint8_t txPtr[MAX_FRAME_LENGTH];
uint8_t rxPtr[MAX_FRAME_LENGTH];

uint16_t txLength;
uint16_t txWritten;
volatile uint16_t rxReceived;
volatile uint16_t rxExpected;
volatile uint8_t fifoReads;
volatile bool packetTx = false; //go into transfer mode
volatile bool packetRx = false;  //go into receive mode

typedef struct ButtonArray{
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} ButtonArray_t;

static const ButtonArray_t buttonArray[BSP_NO_OF_BUTTONS] = BSP_GPIO_BUTTONARRAY_INIT;

static RAIL_Config_t railCfg = {
  .eventsCallback = &RAILCb_Generic,
};

static const RAIL_DataConfig_t dataConfig = {
  .txSource = TX_PACKET_DATA,
  .rxSource = RX_PACKET_DATA,
  .txMethod = FIFO_MODE,
  .rxMethod = FIFO_MODE,
};

int gResetCause = 0;
uint32_t gWakeupPin = 0;
void assertEFM() { }


void modRadioSleep() {
    RAIL_Idle(railHandle, RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS, true);
    gecko_delay(1);
}

void modRadioListen(uint32_t mode) {
	packetRx = true;  //go into receive mode
    startReceive();
}

// modRadioQueueReceivedMessage is built into the xs_gecko.a library if the
// radio module is included. This stub is to allow linking of an app that
// doesn't use the radio.
void modRadioQueueReceivedMessage(void *buffer, uint32_t size) __attribute__ ((weak));
void modRadioQueueReceivedMessage(void *buffer, uint32_t size)
{
#pragma unused(buffer, size);
}

void modRadioPostMessage(void *buffer, uint32_t size) {
    memcpy(&txPtr[PACKET_HEADER_LEN], buffer, size);

    RAIL_Idle(railHandle, RAIL_IDLE, true);
    txPtr[0] = (size) >> 8;
    txPtr[1] = (size) & 0xff;

    txWritten = RAIL_WriteTxFifo(railHandle, txPtr, size + PACKET_HEADER_LEN, true);
    RAIL_StartTx(railHandle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);

    free(buffer);
}

void modRadioWaitUntilIdle() {
    while (RAIL_RF_STATE_IDLE != RAIL_GetRadioState(railHandle))
        gecko_delay(1);
}

uint32_t modRadioSetTxPower(uint32_t txPower) {
	RAIL_SetTxPowerDbm(railHandle, txPower);
}

uint32_t modRadioGetTxPower() {
    return RAIL_GetTxPowerDbm(railHandle);
}

void geckoSleepSensors() {
}

void startReceive()
{
  packetRx = false;
  RAIL_Idle(railHandle, RAIL_IDLE, true);
  RAIL_ResetFifo(railHandle, false, true);
  rxReceived = 0;
  fifoReads = 0;
  rxExpected = 0;
  RAIL_StartRx(railHandle, channel, NULL);
}

void modRadioInit() {
	// Initialize Radio
	radioInit();

	// Enable FIFO mode
	RAIL_ConfigData(railHandle, &dataConfig);

	// Configure RAIL callbacks, with buffer error callbacks
	RAIL_ConfigEvents(railHandle,
					  RAIL_EVENTS_ALL,
					  (RAIL_EVENT_RX_PACKET_RECEIVED
					   | RAIL_EVENT_TX_PACKET_SENT
					   | RAIL_EVENT_RX_FIFO_ALMOST_FULL
					   | RAIL_EVENT_RX_SYNC1_DETECT
					   | RAIL_EVENT_TX_UNDERFLOW
					   | RAIL_EVENT_TX_FIFO_ALMOST_EMPTY));

	// Set TX FIFO
	uint16_t fifoSize = RAIL_SetTxFifo(railHandle, txFifo, 0, TX_FIFO_SIZE);
	if (fifoSize != TX_FIFO_SIZE) {
	  while (1) ;
	}

	// Set FIFO thresholds
	RAIL_SetRxFifoThreshold(railHandle, RX_FIFO_THRESHOLD); //FIFO size is 512B
	RAIL_SetTxFifoThreshold(railHandle, TX_FIFO_THRESHOLD);

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

int main(void)
{
  // Initialize the chip
  CHIP_Init();

  gResetCause = RMU_ResetCauseGet();
  if (gResetCause & (RMU_RSTCAUSE_EM4RST | RMU_RSTCAUSE_EXTRST)) {
	  gWakeupPin = GPIO_EM4GetPinWakeupCause();
  }
  RMU_ResetCauseClear();
  RMU_ResetControl(rmuResetPin, rmuResetModeLimited);	// The CRYOTIMER, DEBUGGER, RTCC, are not reset.
  RMU_ResetControl(rmuResetSys, rmuResetModeLimited);

  // Initialize the system clocks and other HAL components
  halInit();

  // Initialize the BSP
  BSP_Init(BSP_INIT_BCC);

  // Initialize the LEDs on the board
  BSP_LedsInit();

  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);

  // Initialize gpio for buttons
  buttonInit();

//  setupRadio();		// initialized when radio module is loaded

  xs_setup();
  while (1) {
	  xs_loop();
	  if (packetRx) {
		  startReceive();
	  }
  }

/*
  //fill the buffer with dummy data
  for (int i = PACKET_HEADER_LEN; i < MAX_FRAME_LENGTH - PACKET_HEADER_LEN; i++) {
    txPtr[i] = i - PACKET_HEADER_LEN;
  }

  while (1) {
    if (packetTx) {
      packetTx = false;
      txLength = (rand() + 1) % (MAX_FRAME_LENGTH);
      printf("TX(%d): ", txLength - PACKET_HEADER_LEN);
      printBuffer(txPtr + PACKET_HEADER_LEN, txLength - PACKET_HEADER_LEN);
      printf("\n");
      sendPacket(txLength);
    }
    if (packetRx) {
      packetRx = false;
      startReceive();
    }
  }
*/
}

/******************************************************************************
 * Configuration Utility Functions
 *****************************************************************************/
void sendPacket(uint16_t packetLength)
{
  RAIL_Idle(railHandle, RAIL_IDLE, true);
  txPtr[0] = (packetLength - PACKET_HEADER_LEN) >> 8;
  txPtr[1] = (packetLength - PACKET_HEADER_LEN) & 0xff;

  txWritten = RAIL_WriteTxFifo(railHandle, txPtr, packetLength, true);
  RAIL_StartTx(railHandle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
}

void buttonInit()
{
  // Enable the buttons on the board
  for (int i = 0; i < BSP_NO_OF_BUTTONS; i++) {
    GPIO_PinModeSet(buttonArray[i].port, buttonArray[i].pin, gpioModeInputPull, 1);
  }

  // Button Interrupt Config
  GPIOINT_Init();
  GPIOINT_CallbackRegister(buttonArray[0].pin, gpioCallback);
  GPIOINT_CallbackRegister(buttonArray[1].pin, gpioCallback);
  GPIO_IntConfig(buttonArray[0].port, buttonArray[0].pin, false, true, true);
  GPIO_IntConfig(buttonArray[1].port, buttonArray[1].pin, false, true, true);
}

void radioInit()
{
  railHandle = RAIL_Init(&railCfg, NULL);
  if (railHandle == NULL) {
    while (1) ;
  }
  RAIL_Idle(railHandle, RAIL_IDLE, true);
  RAIL_ConfigCal(railHandle, RAIL_CAL_ALL);

  // Set us to a valid channel for this config and force an update in the main
  // loop to restart whatever action was going on
  RAIL_ConfigChannels(railHandle, channelConfigs[0], NULL);
}

void gpioCallback(uint8_t pin)
{
  BSP_LedToggle(LED_TX);
  packetTx = true;
}

void printBuffer(uint8_t * buffer, const uint16_t bufferLength)
{
  for (uint16_t i = 0; i < bufferLength; ++i) {
    printf("0x%X, ", buffer[i]);
  }
}

/******************************************************************************
 * RAIL Callback Implementation
 *****************************************************************************/
void storeReceivedPackets(uint16_t bytesAvailable)
{
  if (rxExpected == 0) {
    rxReceived = RAIL_ReadRxFifo(railHandle, rxPtr, bytesAvailable);
    rxExpected = (rxPtr[0] << 8) + rxPtr[1] + PACKET_HEADER_LEN; //received length + length of header
  } else {
    uint16_t _rxReceived = rxReceived;
    uint16_t _rxExpected = rxExpected;
    rxReceived += RAIL_ReadRxFifo(railHandle,
                                  rxPtr + _rxReceived,
                                  _rxExpected - _rxReceived);
  }
}

void printReceivedPackets()
{
  printf("RX(%d): ", rxReceived - PACKET_HEADER_LEN);
  printBuffer(rxPtr + PACKET_HEADER_LEN, rxReceived - PACKET_HEADER_LEN);
  printf("\n");
}

void RAILCb_Generic(RAIL_Handle_t railHandle, RAIL_Events_t events)
{
  if (events & RAIL_EVENT_TX_PACKET_SENT) {
    BSP_LedToggle(LED_TX);
    packetRx = true;
  }
  if (events & RAIL_EVENT_TX_UNDERFLOW) {
    packetRx = true;
  }
  if (events & RAIL_EVENT_TX_FIFO_ALMOST_EMPTY) {
    if ( txLength - txWritten > 0) {
      txWritten += RAIL_WriteTxFifo(railHandle,
                                    txPtr + txWritten,
                                    txLength - txWritten,
                                    false);
    }
  }
  if (events & RAIL_EVENT_RX_SYNC1_DETECT) {
    BSP_LedToggle(LED_RX);
  }
  if (events & RAIL_EVENT_RX_PACKET_RECEIVED) {
    BSP_LedToggle(LED_RX);

    storeReceivedPackets(RAIL_GetRxFifoBytesAvailable(railHandle));
//    printReceivedPackets();
    modRadioQueueReceivedMessage(rxPtr + PACKET_HEADER_LEN, rxReceived - PACKET_HEADER_LEN);
    packetRx = true;
  }
  if (events & RAIL_EVENT_RX_FIFO_ALMOST_FULL) {
    storeReceivedPackets(RAIL_GetRxFifoBytesAvailable(railHandle));
  }
  if (events & RAIL_EVENT_RX_FIFO_OVERFLOW) {
    printf("overflow\n");
  }
}

