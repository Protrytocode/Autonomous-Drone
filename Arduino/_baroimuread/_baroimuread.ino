#include <Wire.h>

// ==========================================================
// I2C PINS
// ==========================================================

#define SDA_PIN 22
#define SCL_PIN 21

// ==========================================================
// DEVICE ADDRESSES
// ==========================================================

#define MPU6050_ADDR 0x68

// BME280 can be 0x76 or 0x77
uint8_t bmeAddr = 0x76;

// ==========================================================
// MPU6050 REGISTERS
// ==========================================================

#define MPU_WHO_AM_I   0x75
#define MPU_PWR_MGMT_1 0x6B
#define MPU_ACCEL_XOUT 0x3B
#define MPU_GYRO_XOUT  0x43
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_GYRO_CONFIG  0x1B

// ==========================================================
// BME280 REGISTERS
// ==========================================================

#define BME_ID         0xD0
#define BME_RESET      0xE0
#define BME_CTRL_HUM   0xF2
#define BME_STATUS     0xF3
#define BME_CTRL_MEAS  0xF4
#define BME_CONFIG     0xF5
#define BME_PRESS_MSB  0xF7
#define BME_TEMP_MSB   0xFA
#define BME_CALIB00    0x88
#define BME_CALIB26    0xE1

// ==========================================================
// BME280 CALIBRATION DATA
// ==========================================================

uint16_t dig_T1;
int16_t  dig_T2, dig_T3;

uint16_t dig_P1;
int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
int16_t  dig_P6, dig_P7, dig_P8, dig_P9;

uint8_t dig_H1;
int16_t dig_H2;
uint8_t dig_H3;
int16_t dig_H4;
int16_t dig_H5;
int8_t  dig_H6;

int32_t t_fine;

// ==========================================================
// BASELINE PRESSURE
// ==========================================================

float baselinePressure = 0;
bool baselineSet = false;

// ==========================================================
// I2C HELPERS
// ==========================================================

bool i2cWriteByte(uint8_t addr, uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data);

    return Wire.endTransmission() == 0;
}

bool i2cReadBytes(
    uint8_t addr,
    uint8_t reg,
    uint8_t *buffer,
    uint8_t length
)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
        return false;

    uint8_t received = Wire.requestFrom(
        (int)addr,
        (int)length
    );

    if (received != length)
        return false;

    for (uint8_t i = 0; i < length; i++)
    {
        buffer[i] = Wire.read();
    }

    return true;
}

// ==========================================================
// I2C SCANNER
// ==========================================================

void scanI2C()
{
    Serial.println();
    Serial.println("========== I2C SCAN ==========");

    uint8_t found = 0;

    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        uint8_t error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("Found device at 0x");

            if (address < 16)
                Serial.print("0");

            Serial.println(address, HEX);

            found++;
        }
    }

    if (found == 0)
        Serial.println("No I2C devices found.");

    Serial.println("==============================");
}

// ==========================================================
// MPU6050
// ==========================================================

bool initMPU6050()
{
    uint8_t whoami;

    if (!i2cReadBytes(
        MPU6050_ADDR,
        MPU_WHO_AM_I,
        &whoami,
        1
    ))
    {
        return false;
    }

    Serial.print("MPU6050 WHO_AM_I = 0x");
    Serial.println(whoami, HEX);

    if (whoami != 0x68)
    {
        Serial.println("WARNING: Unexpected MPU6050 ID.");
    }

    // Wake MPU6050
    if (!i2cWriteByte(
        MPU6050_ADDR,
        MPU_PWR_MGMT_1,
        0x00
    ))
    {
        return false;
    }

    delay(100);

    // ±2g accelerometer
    i2cWriteByte(
        MPU6050_ADDR,
        MPU_ACCEL_CONFIG,
        0x00
    );

    // ±250 deg/s gyro
    i2cWriteByte(
        MPU6050_ADDR,
        MPU_GYRO_CONFIG,
        0x00
    );

    return true;
}

bool readMPU6050(
    float &ax,
    float &ay,
    float &az,
    float &gx,
    float &gy,
    float &gz
)
{
    uint8_t data[14];

    if (!i2cReadBytes(
        MPU6050_ADDR,
        MPU_ACCEL_XOUT,
        data,
        14
    ))
    {
        return false;
    }

    int16_t rawAx =
        ((int16_t)data[0] << 8) | data[1];

    int16_t rawAy =
        ((int16_t)data[2] << 8) | data[3];

    int16_t rawAz =
        ((int16_t)data[4] << 8) | data[5];

    int16_t rawGx =
        ((int16_t)data[8] << 8) | data[9];

    int16_t rawGy =
        ((int16_t)data[10] << 8) | data[11];

    int16_t rawGz =
        ((int16_t)data[12] << 8) | data[13];

    // ±2g => 16384 LSB/g
    ax = rawAx / 16384.0;
    ay = rawAy / 16384.0;
    az = rawAz / 16384.0;

    // ±250 deg/s => 131 LSB/(deg/s)
    gx = rawGx / 131.0;
    gy = rawGy / 131.0;
    gz = rawGz / 131.0;

    return true;
}

// ==========================================================
// BME280 CALIBRATION
// ==========================================================

bool readBME280Calibration()
{
    uint8_t data[26];

    if (!i2cReadBytes(
        bmeAddr,
        BME_CALIB00,
        data,
        26
    ))
    {
        return false;
    }

    dig_T1 = (uint16_t)data[1] << 8 | data[0];
    dig_T2 = (int16_t)((uint16_t)data[3] << 8 | data[2]);
    dig_T3 = (int16_t)((uint16_t)data[5] << 8 | data[4]);

    dig_P1 = (uint16_t)data[7] << 8 | data[6];
    dig_P2 = (int16_t)((uint16_t)data[9] << 8 | data[8]);
    dig_P3 = (int16_t)((uint16_t)data[11] << 8 | data[10]);
    dig_P4 = (int16_t)((uint16_t)data[13] << 8 | data[12]);
    dig_P5 = (int16_t)((uint16_t)data[15] << 8 | data[14]);
    dig_P6 = (int16_t)((uint16_t)data[17] << 8 | data[16]);
    dig_P7 = (int16_t)((uint16_t)data[19] << 8 | data[18]);
    dig_P8 = (int16_t)((uint16_t)data[21] << 8 | data[20]);
    dig_P9 = (int16_t)((uint16_t)data[23] << 8 | data[22]);

    dig_H1 = data[25];

    uint8_t humData[7];

    if (!i2cReadBytes(
        bmeAddr,
        BME_CALIB26,
        humData,
        7
    ))
    {
        return false;
    }

    dig_H2 =
        (int16_t)((uint16_t)humData[1] << 8 | humData[0]);

    dig_H3 = humData[2];

    dig_H4 =
        (int16_t)((humData[3] << 4) | (humData[4] & 0x0F));

    dig_H5 =
        (int16_t)((humData[5] << 4) | (humData[4] >> 4));

    dig_H6 =
        (int8_t)humData[6];

    return true;
}

// ==========================================================
// BME280 INIT
// ==========================================================

bool initBME280()
{
    uint8_t id;

    if (!i2cReadBytes(
        bmeAddr,
        BME_ID,
        &id,
        1
    ))
    {
        return false;
    }

    Serial.print("BME sensor ID at 0x");
    Serial.print(bmeAddr, HEX);
    Serial.print(" = 0x");
    Serial.println(id, HEX);

    if (id == 0x60)
    {
        Serial.println("BME280 detected.");
    }
    else if (id == 0x58)
    {
        Serial.println("BMP280 detected (no humidity sensor).");
    }
    else
    {
        Serial.println("Unknown pressure sensor.");
        return false;
    }

    // Read calibration constants
    if (!readBME280Calibration())
        return false;

    // Humidity oversampling ×1
    i2cWriteByte(
        bmeAddr,
        BME_CTRL_HUM,
        0x01
    );

    // Temperature ×1
    // Pressure ×1
    // Normal mode
    i2cWriteByte(
        bmeAddr,
        BME_CTRL_MEAS,
        0b00100111
    );

    // Standby 1000 ms, filter off
    i2cWriteByte(
        bmeAddr,
        BME_CONFIG,
        0b10100000
    );

    delay(100);

    return true;
}

// ==========================================================
// BME280 TEMPERATURE + PRESSURE
// ==========================================================

bool readBME280(
    float &temperature,
    float &pressure
)
{
    uint8_t data[6];

    if (!i2cReadBytes(
        bmeAddr,
        BME_PRESS_MSB,
        data,
        6
    ))
    {
        return false;
    }

    // 20-bit raw pressure
    int32_t adc_P =
        ((int32_t)data[0] << 12) |
        ((int32_t)data[1] << 4) |
        (data[2] >> 4);

    // 20-bit raw temperature
    int32_t adc_T =
        ((int32_t)data[3] << 12) |
        ((int32_t)data[4] << 4) |
        (data[5] >> 4);

    // ---------------------------
    // Temperature compensation
    // ---------------------------

    int32_t var1 =
        ((((adc_T >> 3) -
        ((int32_t)dig_T1 << 1))) *
        ((int32_t)dig_T2)) >> 11;

    int32_t var2 =
        (((((adc_T >> 4) -
        ((int32_t)dig_T1)) *
        ((adc_T >> 4) -
        ((int32_t)dig_T1))) >> 12) *
        ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;

    int32_t T =
        (t_fine * 5 + 128) >> 8;

    temperature = T / 100.0;

    // ---------------------------
    // Pressure compensation
    // ---------------------------

    int64_t p_var1 =
        ((int64_t)t_fine) - 128000;

    int64_t p_var2 =
        p_var1 * p_var1 * (int64_t)dig_P6;

    p_var2 +=
        ((p_var1 * (int64_t)dig_P5) << 17);

    p_var2 +=
        ((int64_t)dig_P4 << 35);

    p_var1 =
        ((p_var1 * p_var1 * (int64_t)dig_P3) >> 8) +
        ((p_var1 * (int64_t)dig_P2) << 12);

    p_var1 =
        (((((int64_t)1) << 47) + p_var1) *
        (int64_t)dig_P1) >> 33;

    if (p_var1 == 0)
        return false;

    int64_t p =
        1048576 - adc_P;

    p =
        (((p << 31) - p_var2) * 3125) /
        p_var1;

    p_var1 =
        ((int64_t)dig_P9 *
        (p >> 13) *
        (p >> 13)) >> 25;

    p_var2 =
        ((int64_t)dig_P8 * p) >> 19;

    p =
        ((p + p_var1 + p_var2) >> 8) +
        ((int64_t)dig_P7 << 4);

    pressure = p / 256.0;

    // Returned in Pa
    return true;
}

// ==========================================================
// SETUP
// ==========================================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("   MPU6050 + BME280 I2C TEST");
    Serial.println("========================================");

    // Start I2C
    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    Wire.setClock(400000);

    Serial.println("I2C started:");
    Serial.print("SDA = GPIO");
    Serial.println(SDA_PIN);
    Serial.print("SCL = GPIO");
    Serial.println(SCL_PIN);

    // Scan
    scanI2C();

    // ======================================================
    // MPU
    // ======================================================

    Serial.println();
    Serial.println("Initializing MPU6050...");

    if (initMPU6050())
    {
        Serial.println(
            "[SUCCESS] MPU6050 initialized"
        );
    }
    else
    {
        Serial.println(
            "[ERROR] MPU6050 not responding"
        );
    }

    // ======================================================
    // BME280
    // ======================================================

    Serial.println();
    Serial.println("Searching for BME280...");

    // First try 0x76
    bmeAddr = 0x76;

    uint8_t testID;

    if (!i2cReadBytes(
        bmeAddr,
        BME_ID,
        &testID,
        1
    ))
    {
        // Try 0x77
        bmeAddr = 0x77;

        if (!i2cReadBytes(
            bmeAddr,
            BME_ID,
            &testID,
            1
        ))
        {
            Serial.println(
                "[ERROR] No BME/BMP280 found"
            );
        }
        else
        {
            Serial.println(
                "Found pressure sensor at 0x77"
            );

            initBME280();
        }
    }
    else
    {
        Serial.println(
            "Found pressure sensor at 0x76"
        );

        initBME280();
    }

    Serial.println();
    Serial.println("========================================");
    Serial.println("           SENSOR TEST RUNNING");
    Serial.println("========================================");
}

// ==========================================================
// LOOP
// ==========================================================

void loop()
{
    // ======================================================
    // MPU6050
    // ======================================================

    float ax, ay, az;
    float gx, gy, gz;

    bool mpuOK = readMPU6050(
        ax, ay, az,
        gx, gy, gz
    );

    // ======================================================
    // BME280
    // ======================================================

    float temperature = 0;
    float pressure = 0;

    bool bmeOK = readBME280(
        temperature,
        pressure
    );

    // ======================================================
    // PRINT
    // ======================================================

    Serial.println();
    Serial.println("----------------------------------------");

    if (mpuOK)
    {
        Serial.println("MPU6050:");

        Serial.printf(
            "  Accel: X=%7.3f g  Y=%7.3f g  Z=%7.3f g\n",
            ax, ay, az
        );

        Serial.printf(
            "  Gyro : X=%7.2f dps Y=%7.2f dps Z=%7.2f dps\n",
            gx, gy, gz
        );
    }
    else
    {
        Serial.println(
            "MPU6050: READ FAILED"
        );
    }

    if (bmeOK)
    {
        // Set first pressure as reference
        if (!baselineSet)
        {
            baselinePressure = pressure;
            baselineSet = true;

            Serial.printf(
                "  BASELINE PRESSURE = %.2f Pa\n",
                baselinePressure
            );
        }

        // Relative altitude:
        // h = 44330 * (1 - P/P0)^0.1903
        float relativeAltitude =
            44330.0 *
            (1.0 -
            pow(
                pressure / baselinePressure,
                0.1903
            ));

        Serial.println("BME280:");

        Serial.printf(
            "  Temperature: %.2f C\n",
            temperature
        );

        Serial.printf(
            "  Pressure:    %.2f Pa (%.2f hPa)\n",
            pressure,
            pressure / 100.0
        );

        Serial.printf(
            "  Relative Alt: %.2f m\n",
            relativeAltitude
        );
    }
    else
    {
        Serial.println(
            "BME280: READ FAILED"
        );
    }

    delay(500);
}