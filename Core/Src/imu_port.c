#include "imu_port.h"
#include "main.h"
#include "board_config.h"
#include "cmsis_os2.h"
extern SPI_HandleTypeDef hspi2;
bool ImuPort_Transfer(bool acc, const uint8_t *tx, uint8_t *rx, uint16_t length)
{
    const uint16_t pin = acc ? BOARD_IMU_ACC_CS_PIN : BOARD_IMU_GYRO_CS_PIN;
    HAL_GPIO_WritePin(BOARD_IMU_CS_PORT, pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&BOARD_IMU_SPI,
        (uint8_t *)tx, rx, length, 1U);
    HAL_GPIO_WritePin(BOARD_IMU_CS_PORT, pin, GPIO_PIN_SET);
    return status == HAL_OK;
}
void ImuPort_DelayMs(uint32_t ms) { HAL_Delay(ms); }
uint32_t ImuPort_TimeMs(void) { return osKernelGetTickCount(); }
