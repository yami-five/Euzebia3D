#include "sd_driver.h"
#include "IHardware.h"
#include "hardware.h"
#include "pico/time.h"

#define SD_READY_TIMEOUT_MS 500u
#define SD_RESPONSE_TIMEOUT_MS 500u
#define SD_INIT_TIMEOUT_MS 2000u
#define SD_POWER_UP_DELAY_MS 10u
#define SD_CMD0_RETRY_COUNT 100u
#define SD_TRANSFER_BAUDRATE_HZ (4u * 1000u * 1000u)

#define SD_INIT_STAGE_NOT_STARTED 0u
#define SD_INIT_STAGE_STARTUP_CLOCKS 1u
#define SD_INIT_STAGE_CMD0 2u
#define SD_INIT_STAGE_CMD8 3u
#define SD_INIT_STAGE_R7 4u
#define SD_INIT_STAGE_ACMD41 5u
#define SD_INIT_STAGE_CMD58 6u
#define SD_INIT_STAGE_OCR 7u
#define SD_INIT_STAGE_LEGACY_ACMD41 8u
#define SD_INIT_STAGE_LEGACY_CMD1 9u
#define SD_INIT_STAGE_CMD16 10u
#define SD_INIT_STAGE_READY 11u

unsigned char SD_Type = 0; // version of the sd card
volatile uint8_t SD_InitStage = SD_INIT_STAGE_NOT_STARTED;
volatile uint8_t SD_InitLastCmd = 0xff;
volatile uint8_t SD_InitLastR1 = 0xff;
volatile uint8_t SD_InitR7[4] = {0xff, 0xff, 0xff, 0xff};
volatile uint8_t SD_InitOCR[4] = {0xff, 0xff, 0xff, 0xff};
volatile uint8_t SD_LastDataToken = 0xff;
const e3d_IHardware *_hardware;

// data: data to be written to sd card.
// return: data read from sd card.
unsigned char SD_SPI_ReadWriteByte(unsigned char CMD) {
  return _hardware->sd_spi_write_read_byte(CMD);
}

// set spi in low speed mode.
void SD_SPI_SpeedLow(void) {
  _hardware->set_sd_spi_baudrate_hz(400u * 1000u);
}

// set spi in high speed mode.
void SD_SPI_SpeedHigh(void) {
  _hardware->set_sd_spi_baudrate_hz(SD_TRANSFER_BAUDRATE_HZ);
}

// released spi bus
void SD_DisSelect(void) {
  _hardware->write(SD_CS_PIN, 1);
  SD_SPI_ReadWriteByte(0xff); // providing extra 8 clocks
}

// pick sd card and waiting until until it's ready
// return: 0: succed 1: failure
unsigned char SD_Select(void) {
  _hardware->write(SD_CS_PIN, 0);
  if (SD_WaitReady() == 0)
    return 0;
  SD_DisSelect();
  return 1;
}

// waiting for sd card until it's ready
unsigned char SD_WaitReady(void) {
  absolute_time_t deadline = make_timeout_time_ms(SD_READY_TIMEOUT_MS);
  do {
    if (SD_SPI_ReadWriteByte(0XFF) == 0XFF)
      return 0;
  } while (!time_reached(deadline));
  return 1;
}

// waiting for response from sd card.
// Response: expect from sd card.
// return: succeed for 0, fail for other else
// return: 0 for success, other for failure.
unsigned char SD_GetResponse(unsigned char Response) {
  absolute_time_t deadline = make_timeout_time_ms(SD_RESPONSE_TIMEOUT_MS);
  do {
    SD_LastDataToken = SD_SPI_ReadWriteByte(0XFF);
    if (SD_LastDataToken == Response)
      return MSD_RESPONSE_NO_ERROR;
  } while (!time_reached(deadline));

  return MSD_RESPONSE_FAILURE;
}

// read a buffer from sd card.
//*buf: pointer to a buffer.
// len: length of the buffer.
// return: 0 for success, other for failure.
unsigned char SD_RecvData(unsigned char *buf, unsigned short len) {
  if (SD_GetResponse(0xFE))
    return 1;     // waiting for start command send back from sd card.
  while (len--) { // receiving data...
    *buf = SD_SPI_ReadWriteByte(0xFF);
    buf++;
  }

  // send 2 dummy write (dummy CRC)
  SD_SPI_ReadWriteByte(0xFF);
  SD_SPI_ReadWriteByte(0xFF);
  return 0;
}

// write a buffer containing 512 bytes to sd card.
// buf: data buffer
// cmd: command
// return: 0 for success, other for failure.
unsigned char SD_SendBlock(unsigned char *buf, unsigned char cmd) {
  unsigned short t;
  if (SD_WaitReady())
    return 1;
  SD_SPI_ReadWriteByte(cmd);
  if (cmd != 0XFD) {
    for (t = 0; t < 512; t++)
      SD_SPI_ReadWriteByte(buf[t]);
    SD_SPI_ReadWriteByte(0xFF); // ignoring CRC
    SD_SPI_ReadWriteByte(0xFF);
    t = SD_SPI_ReadWriteByte(0xFF);
    if ((t & 0x1F) != 0x05)
      return 2;
  }
  return 0;
}

// send a command to sd card
// cmd��command
// arg: parameter
// crc: crc
// return: response sent back from sd card.
unsigned char SD_SendCmd(unsigned char cmd, unsigned int arg,
                         unsigned char crc) {
  unsigned char r1;
  unsigned char Retry = 0;
  SD_InitLastCmd = cmd;
  SD_DisSelect();
  if (SD_Select()) {
    SD_InitLastR1 = 0xff;
    return 0XFF;
  }

  SD_SPI_ReadWriteByte(cmd | 0x40);
  SD_SPI_ReadWriteByte(arg >> 24);
  SD_SPI_ReadWriteByte(arg >> 16);
  SD_SPI_ReadWriteByte(arg >> 8);
  SD_SPI_ReadWriteByte(arg);
  SD_SPI_ReadWriteByte(crc);
  if (cmd == CMD12)
    SD_SPI_ReadWriteByte(0xff); // Skip a stuff byte when stop reading
  Retry = 0X1F;
  do {
    r1 = SD_SPI_ReadWriteByte(0xFF);
  } while ((r1 & 0X80) && Retry--);

  SD_InitLastR1 = r1;
  return r1;
}

// obtain CID including manufacturer informationfrom sd card
//*cid_dat: pointer to the buffer storing CID, at least 16 bytes.
// return: 0 no error  1 error
unsigned char SD_GetCID(unsigned char *cid_data) {
  unsigned char r1;

  r1 = SD_SendCmd(CMD10, 0, 0x01);
  if (r1 == 0x00) {
    r1 = SD_RecvData(cid_data, 16);
  }
  SD_DisSelect();
  if (r1)
    return 1;
  else
    return 0;
}

// obtain CSD including storage and speed.
//*csd_data : pointer to the buffer storing CSD, at least 16 bytes.
// return: 0 no error  1 error
unsigned char SD_GetCSD(unsigned char *csd_data) {
  unsigned char r1;
  r1 = SD_SendCmd(
      CMD9, 0,
      0x01); // ��CMD9�����CSD send CMD9 in order to get CSD
  if (r1 == 0) {
    r1 = SD_RecvData(csd_data, 16);
  }
  SD_DisSelect();
  if (r1)
    return 1;
  else
    return 0;
}

// obtian the totals of sectors of sd card.
// return: 0 error, other else for storage of sd card.
// numbers of bytes of each sector must be 512, otherwise fail to
// initialization.
uint32_t SD_GetSectorCount(void) {
  unsigned char csd[16];
  unsigned int Capacity;
  unsigned char n;
  unsigned short csize;

  if (SD_GetCSD(csd) != 0)
    return 0;
  // calculation for SDHC below
  if ((csd[0] & 0xC0) == 0x40) // V2.00
  {
    csize = csd[9] + ((unsigned short)csd[8] << 8) + 1;
    Capacity = (unsigned int)csize << 10; // totals of sectors
  } else                                  // V1.XX
  {
    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
    csize = (csd[8] >> 6) + ((unsigned short)csd[7] << 2) +
            ((unsigned short)(csd[6] & 3) << 10) + 1;
    Capacity = (unsigned int)csize << (n - 9);
  }
  return Capacity;
}

// initialize sd card
unsigned char SD_Initialize(const e3d_IHardware *hardware) {
  unsigned char r1;
  unsigned short retry;
  unsigned char buf[4];
  unsigned short i;
  _hardware = hardware;

  SD_Type = SD_TYPE_ERR;
  SD_InitStage = SD_INIT_STAGE_STARTUP_CLOCKS;
  SD_InitLastCmd = 0xff;
  SD_InitLastR1 = 0xff;
  for (i = 0; i < 4; i++) {
    SD_InitR7[i] = 0xff;
    SD_InitOCR[i] = 0xff;
  }

  // The reader on the display board shares SPI0 with the external reader.
  _hardware->write(SD_CS_PIN, 1);
  _hardware->write(ONBOARD_SD_CS_PIN, 1);
  SD_SPI_SpeedLow();
  _hardware->delay_ms(SD_POWER_UP_DELAY_MS);
  for (i = 0; i < 20; i++)
    SD_SPI_ReadWriteByte(0XFF);

  SD_InitStage = SD_INIT_STAGE_CMD0;
  absolute_time_t init_deadline = make_timeout_time_ms(SD_INIT_TIMEOUT_MS);
  retry = SD_CMD0_RETRY_COUNT;
  do {
    r1 = SD_SendCmd(CMD0, 0, 0x95); // enter to idle state
    if (r1 != 0X01)
      _hardware->delay_ms(1);
  } while ((r1 != 0X01) && retry-- && !time_reached(init_deadline));

  if (r1 == 0X01) {
    SD_InitStage = SD_INIT_STAGE_CMD8;
    r1 = SD_SendCmd(CMD8, 0x1AA, 0x87);
    if (r1 == 1) // SD V2.0
    {
      SD_InitStage = SD_INIT_STAGE_R7;
      for (i = 0; i < 4; i++)
        SD_InitR7[i] = buf[i] =
            SD_SPI_ReadWriteByte(0XFF); // Get trailing return value of R7 resp
      if (buf[2] == 0X01 && buf[3] == 0XAA) // is it support of 2.7~3.6V
      {
        SD_InitStage = SD_INIT_STAGE_ACMD41;
        init_deadline = make_timeout_time_ms(SD_INIT_TIMEOUT_MS);
        do {
          SD_SendCmd(CMD55, 0, 0X01);
          r1 = SD_SendCmd(CMD41, 0x40000000, 0X01);
          if (r1)
            _hardware->delay_ms(1);
        } while (r1 && !time_reached(init_deadline));

        if (!r1) {
          SD_InitStage = SD_INIT_STAGE_CMD58;
          r1 = SD_SendCmd(CMD58, 0, 0X01);
        }
        if (!r1) {
          SD_InitStage = SD_INIT_STAGE_OCR;
          for (i = 0; i < 4; i++)
            SD_InitOCR[i] = buf[i] = SD_SPI_ReadWriteByte(0XFF); // get OCR
          if (buf[0] & 0x40)
            SD_Type = SD_TYPE_V2HC; // check CCS
          else
            SD_Type = SD_TYPE_V2;
        }
      }
    } else // SD V1.x/ MMC	V3
    {
      SD_InitStage = SD_INIT_STAGE_LEGACY_ACMD41;
      SD_SendCmd(CMD55, 0, 0X01);
      r1 = SD_SendCmd(CMD41, 0, 0X01);
      if (r1 <= 1) {
        SD_Type = SD_TYPE_V1;
        init_deadline = make_timeout_time_ms(SD_INIT_TIMEOUT_MS);
        do // exit idle state
        {
          SD_SendCmd(CMD55, 0, 0X01);
          r1 = SD_SendCmd(CMD41, 0, 0X01);
          if (r1)
            _hardware->delay_ms(1);
        } while (r1 && !time_reached(init_deadline));
      } else {
        SD_Type = SD_TYPE_MMC; // MMC V3
        SD_InitStage = SD_INIT_STAGE_LEGACY_CMD1;
        init_deadline = make_timeout_time_ms(SD_INIT_TIMEOUT_MS);
        do {
          r1 = SD_SendCmd(CMD1, 0, 0X01);
          if (r1)
            _hardware->delay_ms(1);
        } while (r1 && !time_reached(init_deadline));
      }
      if (!r1) {
        SD_InitStage = SD_INIT_STAGE_CMD16;
        r1 = SD_SendCmd(CMD16, 512, 0X01);
      }
      if (r1)
        SD_Type = SD_TYPE_ERR;
    }
  }
  SD_DisSelect();
  if (SD_Type) {
    SD_InitStage = SD_INIT_STAGE_READY;
    SD_SPI_SpeedHigh();
    return 0;
  } else if (r1) {
    return r1;
  }
  return 0xaa;
}

// read SD card
// buf: data buffer
// sector: sector
// cnt: totals of sectors]
// return: 0 ok, other for failure
uint8_t SD_ReadDisk(uint8_t *buf, uint32_t sector, uint8_t cnt) {
  unsigned char r1;
  if (SD_Type != SD_TYPE_V2HC)
    sector <<= 9;
  if (cnt == 1) {
    r1 = SD_SendCmd(CMD17, sector, 0X01);
    if (r1 == 0) {
      r1 = SD_RecvData(buf, 512);
    }
  } else {
    r1 = SD_SendCmd(CMD18, sector, 0X01);
    do {
      r1 = SD_RecvData(buf, 512);
      buf += 512;
    } while (--cnt && r1 == 0);
    SD_SendCmd(CMD12, 0, 0X01);
  }
  SD_DisSelect();
  return r1; //
}

// write sd card
// buf: data buffer
// sector: start sector
// cnt: totals of sectors]
// return: 0 ok, other for failure
uint8_t SD_WriteDisk(uint8_t *buf, uint32_t sector, uint8_t cnt) {
  unsigned char r1;
  if (SD_Type != SD_TYPE_V2HC)
    sector *= 512;
  if (cnt == 1) {
    r1 = SD_SendCmd(CMD24, sector, 0X01);
    if (r1 == 0) {
      r1 = SD_SendBlock(buf, 0xFE);
    }
  } else {
    if (SD_Type != SD_TYPE_MMC) {
      SD_SendCmd(CMD55, 0, 0X01);
      SD_SendCmd(CMD23, cnt, 0X01);
    }
    r1 = SD_SendCmd(CMD25, sector, 0X01);
    if (r1 == 0) {
      do {
        r1 = SD_SendBlock(buf, 0xFC);
        buf += 512;
      } while (--cnt && r1 == 0);
      r1 = SD_SendBlock(0, 0xFD);
    }
  }
  SD_DisSelect();
  return r1;
}
