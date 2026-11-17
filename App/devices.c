#include "devices.h"
#include "imu_port.h"
#include <string.h>

static ImuDiagnostics imu_status;
static bool transfer(bool acc, uint8_t reg, uint8_t *data, uint16_t count, bool read)
{
    uint8_t tx[12] = {0}, rx[12] = {0};
    const uint16_t offset = read ? (acc ? 2U : 1U) : 1U;
    /* Private callers read at most 8 bytes; both SPI layouts fit these buffers. */
    tx[0] = read ? (reg | 0x80U) : (reg & 0x7FU);
    if (!read) memcpy(tx + 1, data, count);
    if (!ImuPort_Transfer(acc, tx, rx, count + offset)) {
        imu_status.error = 1; imu_status.errors++; return false;
    }
    if (read) memcpy(data, rx + offset, count);
    return true;
}
static bool read_reg(bool acc, uint8_t reg, uint8_t *data)
{ return transfer(acc, reg, data, 1, true); }
static bool write_reg(bool acc, uint8_t reg, uint8_t value)
{
    if (!transfer(acc, reg, &value, 1, false)) return false;
    ImuPort_DelayMs(5);
    return true;
}
static bool configure(bool acc, uint8_t reg, uint8_t value)
{
    uint8_t actual = 0;
    if (!write_reg(acc, reg, value) || !read_reg(acc, reg, &actual)) return false;
    if (actual != value) { imu_status.error = 3; imu_status.errors++; return false; }
    return true;
}
void Devices_Init(void)
{
    imu_status = (ImuDiagnostics){0};
    ImuPort_DelayMs(50);
    uint8_t value = 0;
    /* First ACC read only selects SPI mode; ACC reads have an extra dummy byte. */
    if (!read_reg(true, 0x00, &value)) return;
    if (!write_reg(true, 0x7E, 0xB6)) return;
    ImuPort_DelayMs(50);
    if (!write_reg(false, 0x14, 0xB6)) return;
    ImuPort_DelayMs(50);
    if (!read_reg(true, 0x00, &value)) return;
    if (!read_reg(true, 0x00, &value)) return;
    imu_status.accel_id = value;
    if (!read_reg(false, 0x00, &value)) return;
    imu_status.gyro_id = value;
    if (imu_status.accel_id != 0x1E || imu_status.gyro_id != 0x0F) {
        imu_status.error = 2; imu_status.errors++; return;
    }
    /* ACC 800 Hz / normal bandwidth / +/-6g; GYRO 1 kHz /116 Hz / +/-2000 dps. */
    if (!configure(true, 0x7C, 0x00) || !configure(true, 0x7D, 0x04) ||
        !configure(true, 0x40, 0xAB) || !configure(true, 0x41, 0x01) ||
        !configure(false, 0x0F, 0x00) || !configure(false, 0x10, 0x82) ||
        !configure(false, 0x11, 0x00)) return;
    if (!read_reg(true, 0x02, &value)) return;
    if (value) { imu_status.error = 4; imu_status.errors++; return; }
    imu_status.ready = 1;
}
void Imu_GetDiagnostics(ImuDiagnostics *diagnostics) { *diagnostics = imu_status; }
static int16_t signed_le(const uint8_t *b)
{ return (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8)); }
DeviceResult Imu_Read(ImuData *sample)
{
    *sample = (ImuData){0};
    if (!imu_status.ready) return DEVICE_NOT_READY;
    uint8_t acc_status = 0, acc[6], gyro[8];
    if (!read_reg(true, 0x03, &acc_status))
        return DEVICE_ERROR;
    if (!(acc_status & 0x80)) return DEVICE_NOT_READY;
    if (!transfer(true, 0x12, acc, 6, true) || !transfer(false, 0x00, gyro, 8, true))
        return DEVICE_ERROR;
    /* GYRO DRDY is a short pulse, unsuitable for 2 ms polling. Its 1 kHz
     * output is sampled with each new ACC reading; no hardware synchronization. */
    if (gyro[0] != 0x0F) { imu_status.error = 2; imu_status.errors++; return DEVICE_ERROR; }
    for (unsigned i = 0; i < 3; i++) {
        sample->accel_m_s2[i] = signed_le(acc + 2*i) * (6.0f * 9.80665f / 32768.0f);
        sample->gyro_rad_s[i] = signed_le(gyro + 2 + 2*i) * (2000.0f * 0.01745329252f / 32768.0f);
    }
    sample->header = (DataHeader){true, ImuPort_TimeMs()};
    imu_status.samples++; imu_status.error = 0;
    return DEVICE_OK;
}
/* Wheel/leg drivers remain disabled while IMU-only observation is in use. */
DeviceResult Wheels_Read(WheelFeedback *s) { *s = (WheelFeedback){0}; return DEVICE_NOT_READY; }
DeviceResult Legs_Read(LegFeedback *s) { *s = (LegFeedback){0}; return DEVICE_NOT_READY; }
DeviceResult Wheels_Write(const WheelTarget *s) { (void)s; return DEVICE_NOT_READY; }
DeviceResult Legs_Write(const LegTarget *s) { (void)s; return DEVICE_NOT_READY; }
