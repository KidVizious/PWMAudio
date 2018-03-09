//==============================================================================
// Copyright © 2018 KidVizious
//==============================================================================
#include "stm32f1xx_hal.h"
#include "Slip.h"

//==============================================================================
// Private defines and constants
//==============================================================================
#define END 0xc0
#define ESC 0xdb
#define ESC_END 0xdc
#define ESC_ESC 0xdd

//==============================================================================
// Private typedefs
//==============================================================================

//==============================================================================
// Private variables
//==============================================================================
static struct pt slipReceivePt;
static struct pt slipSendPt;
static uint8_t *receiveBuffer;
static uint32_t receiveBufferSize;
static uint8_t *sendBuffer;
static uint32_t sendLength;
static SlipDoneCallback_t receiveDoneCallback;
static SlipDoneCallback_t sendDoneCallback;
static uint8_t data;
extern UART_HandleTypeDef huart2;
//==============================================================================
// Private function definitions
//==============================================================================

//------------------------------------------------------------------------------
// This function should be scheduled for each byte received over the UART
//------------------------------------------------------------------------------
static PT_THREAD(SlipReceiveProcess(uint8_t receivedByte))
{
    static uint32_t bytesReceived; // Number of bytes received in this frame
    static uint8_t encodedByte;    // Unescaped byte to be stuffed into the buffer

    PT_BEGIN(&slipReceivePt);

    bytesReceived = 0;
    while ((receivedByte != END) && (bytesReceived < receiveBufferSize))
    {
        // Save received byte in case it is a real value and not escaped
        encodedByte = receivedByte;

        // If this is the ESC code, wait for the next byte and save appropriate value
        if (receivedByte == ESC)
        {
            // Wait to receive another byte, we'll pick up here once one arrives
            PT_YIELD(&slipReceivePt);

            if (receivedByte == ESC_END)
            {
                encodedByte = END;
            }
            else if (receivedByte == ESC_ESC)
            {
                encodedByte = ESC;
            }
            else if (receiveDoneCallback)
            {
                // Protocol error, give a callback but say no bytes received
                receiveDoneCallback(0);
            }
        }

        // Save off whatever byte we just received
        receiveBuffer[bytesReceived++] = encodedByte;

        // Go off and wait for another byte
        PT_YIELD(&slipReceivePt);
    }

    // We got the END byte, if we have received any data, give a callback
    if (bytesReceived)
    {
        if (receiveDoneCallback)
        {
            receiveDoneCallback(bytesReceived);
        }
    }

    PT_END(&slipReceivePt);
}

//------------------------------------------------------------------------------
// This function should be scheduled for each byte to be transmitted over UART
//------------------------------------------------------------------------------
static PT_THREAD(SlipTransmitProcess())
{
    static uint32_t sentBytes;
    static uint8_t dataToSend;

    PT_BEGIN(&slipSendPt);

    sentBytes = 0;
    while (sentBytes < sendLength)
    {
        static uint8_t byteToSend;
        byteToSend = sendBuffer[sentBytes];
        if (byteToSend == END)
        {
            dataToSend = ESC;
            HAL_UART_Transmit_IT(&huart2, &dataToSend, 1);
            PT_YIELD(&slipSendPt);
            dataToSend = ESC_END;
            HAL_UART_Transmit_IT(&huart2, &dataToSend, 1);
        }
        else if (byteToSend == ESC)
        {
            dataToSend = ESC;
            HAL_UART_Transmit_IT(&huart2, &dataToSend, 1);
            PT_YIELD(&slipSendPt);
            dataToSend = ESC_ESC;
            HAL_UART_Transmit_IT(&huart2, &dataToSend, 1);
        }
        else
        {
            HAL_UART_Transmit_IT(&huart2, &byteToSend, 1);
        }
        ++sentBytes;
        PT_YIELD(&slipSendPt);
    }

    dataToSend = END;
    HAL_UART_Transmit_IT(&huart2, &dataToSend, 1);
    PT_END(&slipSendPt);
}

//==============================================================================
// Public function definitions
//==============================================================================

//------------------------------------------------------------------------------
// This function starts the transmission process
//------------------------------------------------------------------------------
bool Slip_Send(uint8_t *data, uint32_t dataLength, SlipDoneCallback_t callback)
{
    sendBuffer = data;
    sendLength = dataLength;
    sendDoneCallback = callback;

    (void)PT_SCHEDULE(SlipTransmitProcess());
}

//------------------------------------------------------------------------------
// This function starts the receive process
//------------------------------------------------------------------------------
bool Slip_Receive(uint8_t *dataBuffer, uint32_t bufferSize, SlipDoneCallback_t callback)
{
    // Save off our state
    receiveBuffer = dataBuffer;
    receiveBufferSize = bufferSize;
    receiveDoneCallback = callback;

    // Set up UART to receive a byte
    HAL_UART_Receive_IT(&huart2, &data, 1);
}

//------------------------------------------------------------------------------
// This function should be called for each byte received over the UART
//------------------------------------------------------------------------------
void Slip_ReceiveByte(void)
{
    (void)PT_SCHEDULE(SlipReceiveProcess(data));
    HAL_UART_Receive_IT(&huart2, &data, 1);
}

//------------------------------------------------------------------------------
// Called by HAL when UART transmit is complete. It will schedule to process to
// send the next byte.
//------------------------------------------------------------------------------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    (void)PT_SCHEDULE(SlipTransmitProcess());
}