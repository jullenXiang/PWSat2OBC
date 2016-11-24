#include "adxrs453.h"
#include "spidrv.h"
#include "base/os.h"
#include "system.h"
#include "io_map.h"
#include "logger/logger.h"
#include <em_gpio.h>
#include <stdint.h>



void GenerateCommand(uint8_t commandByte , uint8_t registerAddress, uint16_t registerValue, uint8_t * sendBuffer)
{
	uint32_t  command       = 0;
	sendBuffer[0] = commandByte | (registerAddress >> 7);
	sendBuffer[1] = (registerAddress << 1);
	if(commandByte == ADXRS453_WRITE)
	{
		sendBuffer[1] |=  (registerValue >> 15);
		sendBuffer[2] = (registerValue >> 7);
		sendBuffer[3] = (registerValue << 1);
	}

	command = ((uint32_t)sendBuffer[0] << 24) |
	          ((uint32_t)sendBuffer[1] << 16) |
	          ((uint16_t)sendBuffer[2] << 8) |
	          sendBuffer[3];
	sendBuffer[3] |=!__builtin_parity(command);

}


Ecode_t SPISendB(ADXRS453_PinLocations_t *locations , SPIDRV_Handle_t 	handle,
		const void * 	buffer,
		uint8_t 	length )
{
	Ecode_t resultCode;
	System.SleepTask(50);
	GPIO_PinOutClear((GPIO_Port_TypeDef)locations->csPortLocation,locations->csPinLocation);
	resultCode=SPIDRV_MTransmitB( handle,buffer,length);
	GPIO_PinOutSet((GPIO_Port_TypeDef)locations->csPortLocation,locations->csPinLocation);
	System.SleepTask(50);
	return resultCode;
}

Ecode_t SPIRecvB(ADXRS453_PinLocations_t *locations, SPIDRV_Handle_t 	handle,
		void * 	buffer,
		uint8_t 	length )
{
	Ecode_t resultCode;
	System.SleepTask(50);
	GPIO_PinOutClear((GPIO_Port_TypeDef)locations->csPortLocation,locations->csPinLocation);
	resultCode=SPIDRV_MReceiveB(handle,buffer,length);
	GPIO_PinOutSet((GPIO_Port_TypeDef)locations->csPortLocation,locations->csPinLocation);
	System.SleepTask(50);
	return resultCode;
}

SPI_TransferPairResultCode_t SPISendRecvB(ADXRS453_PinLocations_t *locations, SPIDRV_Handle_t 	handle,
		void * 	buffer,
		uint8_t 	length )
{
	SPI_TransferPairResultCode_t pairResultCode;
	pairResultCode.resultCodeWrite= SPISendB(locations, handle, buffer, length );
	pairResultCode.resultCodeRead= SPIRecvB(locations, handle, buffer, length);
	return pairResultCode;
}

void ADXRS453Spi_Init(ADXRS453_Obj_t *gyro) {
	 GPIO_PinModeSet((GPIO_Port_TypeDef)(gyro->pinLocations).csPortLocation, (gyro->pinLocations).csPinLocation ,gpioModePushPull, 1 );
	 GPIO_PinOutSet((GPIO_Port_TypeDef)(gyro->pinLocations).csPortLocation,(gyro->pinLocations).csPinLocation);
}

void ADXRS453_Init(ADXRS453_Obj_t *gyro, SPIDRV_Handle_t handle)
{
	uint8_t  dataBuffer[4] = {0x20, 0x00, 0x00, 0x03};
    ADXRS453Spi_Init(gyro);
    gyro->interface.writeProc(&(gyro->pinLocations), handle, dataBuffer, 4 );
    dataBuffer[3]=0x00;
    gyro->interface.writeProc(&(gyro->pinLocations), handle, dataBuffer, 4 );
    gyro->interface.writeProc(&(gyro->pinLocations), handle, dataBuffer, 4 );
    gyro->interface.writeProc(&(gyro->pinLocations), handle, dataBuffer, 4 );
    gyro->interface.writeProc(&(gyro->pinLocations), handle, dataBuffer, 4 );
}


SPI_TransferReturn_t ADXRS453_GetRegisterValue(ADXRS453_Obj_t *gyro,
									SPIDRV_Handle_t handle,
									uint8_t registerAddress)
{
	SPI_TransferReturn_t transferReturn;
	uint8_t sendBuffer[4] = {0, 0, 0, 0};
    uint16_t registerValue = 0;

    GenerateCommand(ADXRS453_READ,registerAddress,0,sendBuffer);
    transferReturn.resultCodes=gyro->interface.readProc(&(gyro->pinLocations), handle, sendBuffer, 4 );
    registerValue = ((uint16_t)sendBuffer[1] << 11) |
                    ((uint16_t)sendBuffer[2] << 3) |
                    (sendBuffer[3] >> 5);
    transferReturn.result.dataResult=registerValue;
    return transferReturn;
}

SPI_TransferReturn_t ADXRS453_SetRegisterValue(ADXRS453_Obj_t *gyro,
								SPIDRV_Handle_t 	handle,
                     			uint8_t registerAddress,
                                uint16_t registerValue)
{
	SPI_TransferReturn_t transferReturn;
	uint8_t sendBuffer[4] = {0, 0, 0, 0};
    GenerateCommand(ADXRS453_WRITE,registerAddress,registerValue,sendBuffer);
    transferReturn.resultCodes.resultCodeWrite= gyro->interface.writeProc(&(gyro->pinLocations), handle, sendBuffer, 4 );
    return transferReturn;


}

SPI_TransferReturn_t ADXRS453_GetSensorData(ADXRS453_Obj_t *gyro,SPIDRV_Handle_t 	handle)
{
	SPI_TransferReturn_t transferReturn;
	uint8_t sendBuffer[4] = {0, 0, 0, 0};
    uint32_t registerValue = 0;
    GenerateCommand(ADXRS453_SENSOR_DATA,0,0,sendBuffer);
    transferReturn.resultCodes=gyro->interface.readProc(&(gyro->pinLocations), handle, sendBuffer, 4 );
    registerValue = ((uint32_t)sendBuffer[0] << 24) |
                    ((uint32_t)sendBuffer[1] << 16) |
                    ((uint16_t)sendBuffer[2] << 8) |
					sendBuffer[3];
    transferReturn.result.dataResult=registerValue;
    return transferReturn;
}

SPI_TransferReturn_t ADXRS453_GetRate(ADXRS453_Obj_t *gyro,SPIDRV_Handle_t 	handle)
{
	SPI_TransferReturn_t transferReturn;
    uint16_t registerValue = 0;
    int16_t          rate          = 0.0;
    
    transferReturn = ADXRS453_GetRegisterValue(gyro, handle, ADXRS453_REG_RATE);
    registerValue = transferReturn.result.sensorResult;
   
    if(transferReturn.resultCodes.resultCodeWrite==ECODE_OK && transferReturn.resultCodes.resultCodeRead==ECODE_OK)
    {

    if(registerValue < 0x8000)
    {
        rate = ((int16_t)registerValue / 80);
    }

    else
    {
        rate = (-1) * ((int16_t)(0xFFFF - registerValue + 1) / 80);
    }
    transferReturn.result.sensorResult= rate;
    }
    else
    {
    	LOGF(LOG_LEVEL_ERROR, "[adxrs] Unable to get rate, write code %d read code %d", transferReturn.resultCodes.resultCodeWrite,
    			transferReturn.resultCodes.resultCodeRead);
    }
    return transferReturn;
}

SPI_TransferReturn_t ADXRS453_GetTemperature(ADXRS453_Obj_t *gyro,SPIDRV_Handle_t 	handle)
{
	SPI_TransferReturn_t transferReturn;
    uint32_t registerValue = 0;
    int16_t          temperature   = 0;
    
    transferReturn = ADXRS453_GetRegisterValue(gyro, handle, ADXRS453_REG_TEM);
    if(transferReturn.resultCodes.resultCodeWrite==ECODE_OK && transferReturn.resultCodes.resultCodeRead==ECODE_OK)
       {
    	registerValue = transferReturn.result.dataResult;
    	registerValue = (registerValue >> 6) - 0x31F;
    	temperature = (int16_t) registerValue / 5;
    	transferReturn.result.sensorResult= temperature;
       }
    else
       {
       	LOGF(LOG_LEVEL_ERROR, "[adxrs] Unable to get temperature, write code %d read code %d", transferReturn.resultCodes.resultCodeWrite,
       			transferReturn.resultCodes.resultCodeRead);
       }
    return transferReturn;
}