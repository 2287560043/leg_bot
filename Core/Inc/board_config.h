#pragma once
/* Candidate hardware bindings; no actuator is enabled by the skeleton. */
#define BOARD_IMU_SPI hspi2
#define BOARD_WHEEL_UART huart1 /* PB14 TX / PB15 RX */
#define BOARD_LEG_UART huart7  /* PE8 TX / PE7 RX */
#define BOARD_IMU_CS_CONFIGURED 1
#define BOARD_IMU_ACC_CS_PIN GPIO_PIN_0
#define BOARD_IMU_GYRO_CS_PIN GPIO_PIN_3
#define BOARD_IMU_CS_PORT GPIOC
#define APP_CONTROL_MS 2U
#define APP_COMM_MS 5U
#define APP_FEEDBACK_TIMEOUT_MS 100U
/* Servo IDs, limits, motor polarity and IMU chip selects remain unverified. */
