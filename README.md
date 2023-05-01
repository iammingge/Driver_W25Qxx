# Driver_W25Qxx
### *Usage*

#### Step 1 ：User add SPI port function

```c
/**
 * @brief W25Qxx BUS Port
 */
typedef struct
{
    void (*spi_delayms)(uint32_t ms);
    void (*spi_cs_H)(void);
    void (*spi_cs_L)(void);
    uint8_t(*spi_rw)(uint8_t data);
} W25Qxx_PORT_t;
```

#### Step 2 ：Init W25Qxx Device （Mounted Devices）

```c
/* W25Qxx Init (Mounted Devices) */
testdev.port.spi_delayms = SPI5_DelayMS;
testdev.port.spi_rw = SPI5_RW;
testdev.port.spi_cs_H = SPI5_CS_H;
testdev.port.spi_cs_L = SPI5_CS_L;
W25Qxx_config(&testdev, &err);
if (err != W25Qxx_ERR_NONE) while (1);
```

#### Step 3 ：Test code

```c
char test[] = "Hello World !!!";
char buff[20] = "";

/* Write */
W25Qxx_Program(&testdev, test, 0x00124567, strlen(test), &err);  
if (err != W25Qxx_ERR_NONE) while (1);

/* Read */
W25Qxx_Read(&testdev, buff, 0x00124567, strlen(test), &err);	 
if (err != W25Qxx_ERR_NONE) while (1);
```



