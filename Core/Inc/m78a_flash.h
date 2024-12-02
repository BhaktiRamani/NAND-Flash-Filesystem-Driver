/*
 * m78a_flash.h
 *
 *  Created on: Nov 28, 2024
 *      Author: Bhakti Ramani
 */

#ifndef INC_M78A_FLASH_H_
#define INC_M78A_FLASH_H_

#include "stm32f4xx_hal.h"
#include <stddef.h>
#include <stdint.h>
#include "string.h"
#include "main.h"
#include "stm32f4xx_hal_spi.h"

#define MIN(a, b) (a < b ? a : b)

void m78a_read_device_manufacturar_id();
void m78a_init(SPI_HandleTypeDef *spih);
void m78a_page_read(uint16_t pageNum);
void m78a_program_load(int column, uint8_t *data_byte_buffer);
void m78a_program_execute(int block, int page);
void m78a_write_enable(void);
void m78a_check_status_register(int number_of_bit);

int m78a_write(const uint8_t* dataPtr, uint32_t noOfbytesToWrite, uint32_t writeLoc);
int m78a_read(uint8_t* dataPtr, uint32_t noOfbytesToRead, uint32_t readLoc);

void spi_recieve(uint8_t *recieve_data, int size);
void spi_transmit(uint8_t *transmit_data, int size);
void spi_disable();
void spi_enable();
void spi(uint8_t *transmit_data, uint8_t *recieve_data, int transmit_size, int recieve_size);

#endif /* INC_M78A_FLASH_H_ */
