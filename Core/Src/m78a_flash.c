/*
 * m78a_flash.c
 *
 *  Created on: Nov 28, 2024
 *      Author: Bhakti Ramani
 */

#include "m78a_flash.h"
#include <stdint.h>
#include "string.h"
#include "main.h"
#include "stm32f4xx_hal_spi.h"


uint8_t CMD_READ_DEVICE_ID = 0x9F;
uint8_t CMD_DUMMY_BYTES = 0x00;
uint8_t CMD_PAGE_READ = 0x13;
uint8_t CMD_GET_FEATURE = 0x0F;
uint8_t CMD_STATUS_REG = 0xC0;
uint8_t CMD_READ_FROM_CACHE = 0x03;
uint8_t CMD_WRITE_ENABLE = 0x06;
uint8_t CMD_PROGRAM_LOAD = 0x02;
uint8_t CMD_PROGRAM_EXECUTE = 0x10;

int trial_column_addr = 0x11;
int trial_block_addr = 0x22;
int trial_page_addr = 0x33;
uint8_t trial_data_byte1 = 0x00;
uint8_t trial_data_byte2 = 0x00;

#define W25N01GW_NO_OF_SEC		4 //No.of Sectors per page
#define W25N01GW_NO_OF_PAGES	64//No.of Pages per block
#define W25N01GW_NO_OF_BLOCKS	1024//No.of Blocks in variant W25N01GWTBIG
#define W25N01GW_PAGE_SIZE 		(W25N01GW_NO_OF_SEC*W25N01GW_SECTOR_SIZE) //Size of the page
#define W25N01GW_BLOCK_SIZE 	(W25N01GW_NO_OF_PAGES*W25N01GW_PAGE_SIZE) //Size of the block
#define W25N01GW_FLASH_SIZE		(W25N01GW_BLOCK_SIZE*W25N01GW_NO_OF_BLOCKS) //Size of the flash
#define W25N01GW_TOTAL_SECTORS	(W25N01GW_NO_OF_SEC*W25N01GW_NO_OF_PAGES*W25N01GW_NO_OF_BLOCKS) //Total no. of sectors
#define W25N01GW_SECTORS_PER_BLOCK	(W25N01GW_NO_OF_PAGES*W25N01GW_NO_OF_SEC) //No. of sectors per block

typedef enum {
	WEL_BIT = (1 << 1),
	OIP_BIT = (1 << 0),
	CONTENTS_OUT = 0xFF,
}status_bits_t;

SPI_HandleTypeDef spihandler;


void m78a_init(SPI_HandleTypeDef *spih)
{
	spihandler = *spih;
	m78a_read_device_manufacturar_id();

	//for(int i = 0; i<100; i++);
	uint8_t data_buffer[2048];
	memset(data_buffer,  0xAA, 2047 );
	m78a_program_load(trial_column_addr,data_buffer );   //colunm, data
	m78a_program_execute(trial_block_addr, trial_page_addr);		//block, page
	for(int i = 0; i<100; i++);
	m78a_page_read(trial_block_addr, trial_page_addr, trial_page_addr);		//block, page, column


}


//reads the memory data by transferring desired address locations's data to cache register
void m78a_page_read(int block, int page, int column)
{
	//sending page read command, 8 dummy bytes for 8 clock cycle, bloack address, page address
	int temp = 0;
	temp = block & 0xF;
	block = block >> 2;
	temp <<= 6;
	page = page | temp;
	uint8_t cmdBuffer[4] = {CMD_PAGE_READ, CMD_DUMMY_BYTES, block, page};
	uint8_t recvBuffer[4] = {0};

	spi(cmdBuffer, recvBuffer, 4, 0);

	//sending get command and status register address to check and recieving the contents of status register, polling the Operation in execution command to see if read is done(data from memory to cache is done)

	m78a_check_status_register(OIP_BIT);

	//reading from cache register(data transferred from desired memory array to cache register, reading only 2 bytes for now)

	uint8_t dummy_and_colunm = column >> 8;
	column = column & 0xFF;

	uint8_t cmd_cache_buffer[] = {CMD_READ_FROM_CACHE, dummy_and_colunm, trial_column_addr, CMD_DUMMY_BYTES};
	uint8_t recv_cache_buffer[2] = {0};
	spi(cmd_cache_buffer, recv_cache_buffer, 4, 2); // Perform SPI transmission


}


int m78a_write(const uint8_t*dataPtr, uint32_t no_of_bytes_to_write, uint32_t write_loc)
{
	uint8_t cmdBuffer[4] = { 0 };
    uint16_t pageNum=0,pageOff=0;
    uint16_t wr_len_page = 0;
    uint8_t dummy_byte = 0;
    uint8_t tx_buf[3+128] = {0};
    uint8_t reg_value=0;
    uint16_t txn_off,txn_len = 0;

	if(writeLoc==1)
    {
        reg_value=0;
    }
    if(dataPtr==NULL)
    {
        return -1;
    }
    if(noOfbytesToWrite==0)
    {
        return -1;
    }
    if(writeLoc>W25N01GW_FLASH_SIZE)
    {
        return -1;
    }
	while(noOfbytesToWrite>0)
    {
        pageNum = writeLoc/W25N01GW_PAGE_SIZE;
        pageOff = writeLoc%W25N01GW_PAGE_SIZE;

        wr_len_page = MIN(noOfbytesToWrite, W25N01GW_PAGE_SIZE+1 - pageOff);

        if(wr_len_page!=W25N01GW_PAGE_SIZE)
        {
            /*If ECC is enabled */
            W25N01GW_pageRead(pageNum);
            tx_buf[0] = W25N01GW_CMD_RANDM_PRGM_DATA;
        }
        else
        {
            tx_buf[0] = W25N01GW_CMD_LD_PRGM_DATA;
        }
        /* Enable write */
        cmdBuffer[0] = W25N01GW_CMD_WR_ENABLE;

                nrf_drv_spi_transfer(&spi,&cmdBuffer[0],1,NULL,0);



        /*Load the program into Databuffer*/
           for (txn_off = 0, txn_len = 0; txn_off < wr_len_page;txn_off += txn_len)
           {
              txn_len = MIN(128, wr_len_page - txn_off);
              tx_buf[1] = (pageOff + txn_off) >> 8;
              tx_buf[2] = (pageOff + txn_off) & 0xff;
              memcpy(tx_buf + 3, dataPtr, txn_len);
              nrf_drv_spi_transfer(&spi,&tx_buf[0],3 + txn_len,NULL,0);
              tx_buf[0] = W25N01GW_CMD_RANDM_PRGM_DATA;
              dataPtr += txn_len;
            }

           /*Execute the program*/
            cmdBuffer[0] = W25N01GW_CMD_PRGM_EXEC;
            cmdBuffer[1] = dummy_byte;//Dummy byte
            cmdBuffer[2] = ((pageNum >> 8) & 0xff);
            cmdBuffer[3] = (pageNum & 0xff);

            nrf_drv_spi_transfer(&spi,&cmdBuffer[0],4,NULL,0);

            /*Wait until data is written to the flash*/
            while((reg_value=W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
            {
                ;
            }
            if(reg_value&W25N01GW_PFAIL_STAT)
            {
                return W25N01GW_WRITE_FAILURE;
            }

           writeLoc += wr_len_page;
           noOfbytesToWrite -= wr_len_page;
    }
}
//passing the colunm in page and data buffer array to write at that address, will be sawved into cache register
//pass colunm and 2 data bytes
void m78a_program_load(int column, uint8_t *data_byte_buffer)
{
	m78a_write_enable();
	//Sending the command for program load, dummy bits , plane bit(0), 12 column address bits and data buffer 
    uint8_t dummy_and_colunm = column >> 8;
	column = column & 0xFF;
	uint8_t program_load_cmd_buffer[] = {CMD_PROGRAM_LOAD, dummy_and_colunm, column };
	uint8_t dummy_buffer[0];
	spi(program_load_cmd_buffer, dummy_buffer, 3 , 0);
	spi(data_byte_buffer, dummy_buffer, 2047, 0);
}

//transferring from cache register to
//pass block and page address to write it
void m78a_program_execute(int block, int page)
{
	int temp = 0;
	temp = block & 0xF;
	block = block >> 2;
	temp <<= 6;
	page = page | temp;
	uint8_t program_execute_cmd_buffer[] = {CMD_PROGRAM_EXECUTE, CMD_DUMMY_BYTES,  block, page};
	uint8_t recieved_data_bytes[4] = {0};

	spi(program_execute_cmd_buffer, recieved_data_bytes, 4, 0);

	//sending get command and status register address to check and recieving the contents of status register, polling the Operation in execution command to see if read is done(data from memory to cache is done)
	m78a_check_status_register(OIP_BIT);



}

//OIP bit = 0 means device is ready for nextr command
//WEL bit = 1 means device is ready for write operations in memory
void m78a_check_status_register(int number_of_bit)
{
	uint8_t status_reg_check_cmd_buffer[2] = {CMD_GET_FEATURE, CMD_STATUS_REG};  // TX commands
	uint8_t recieved_state_status_reg[2] = {}; // Recieved_state of status register
	spi(status_reg_check_cmd_buffer, recieved_state_status_reg, 2, 2); // Perform SPI transmission

	switch(number_of_bit)
	{

		case WEL_BIT:
			while(!(recieved_state_status_reg[0] & WEL_BIT))
		
			break;

		case OIP_BIT:
	
			while(recieved_state_status_reg[0] & OIP_BIT) //bit not set
			{
		
					//update the register
					//spi(status_reg_check_cmd_buffer, recieved_state_status_reg, 2, 2);

	
			}
			//retransmitting the status register and reading its contents to varify if OIP bit is actually free
			//spi(status_reg_check_cmd_buffer, recieved_state_status_reg, 2, 2); // Perform SPI transmission

			break;
		case CONTENTS_OUT:
			break;
		default:
			break;
	}
}


void m78a_write_enable(void)
{
	//sending write enable and polling the status register to check if WEL bit is actually set
	spi(&CMD_WRITE_ENABLE, &CMD_DUMMY_BYTES, 1, 0);
	m78a_check_status_register(WEL_BIT);

}
void m78a_read_device_manufacturar_id()
{
	uint8_t transmit_commands_for_device_id[2] = {CMD_READ_DEVICE_ID, CMD_DUMMY_BYTES};
	uint8_t recieved_device_id[2] = {0};

	spi(transmit_commands_for_device_id, recieved_device_id, 2, 2);
}
/* Timeout value for SPI operations in milliseconds */
#define SPI_TIMEOUT 1000



/**
 * @brief Enable SPI communication by asserting CS pin
 * 
 * Activates the SPI slave device by pulling the chip select (CS)
 * pin low on GPIOA pin 4
 */
void spi_enable()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}

/**
 * @brief Disable SPI communication by de-asserting CS pin
 * 
 * Deactivates the SPI slave device by pulling the chip select (CS)
 * pin high on GPIOA pin 4
 */
void spi_disable()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
 * @brief Transmit data over SPI
 * @param transmit_data Pointer to data buffer to transmit
 * @param size Number of bytes to transmit
 * 
 * Sends data over SPI using HAL library with specified timeout
 */
void spi_transmit(uint8_t *transmit_data, int size)
{
    HAL_SPI_Transmit(&spihandler, transmit_data, size, SPI_TIMEOUT);
}

/**
 * @brief Receive data over SPI
 * @param recieve_data Pointer to buffer for received data
 * @param size Number of bytes to receive
 * 
 * Receives data over SPI if size is non-zero
 * @note Function returns immediately if size is 0
 */
void spi_recieve(uint8_t *recieve_data, int size)
{
    if(size == 0)
    {
        /* No data to receive */
        return;
    }
    else
    {
        HAL_SPI_Receive(&spihandler, recieve_data, size, SPI_TIMEOUT);
    }
}

/**
 * @brief Complete SPI transaction with transmit and receive
 * @param transmit_data Pointer to data to transmit
 * @param recieve_data Pointer to buffer for received data
 * @param transmit_size Number of bytes to transmit
 * @param recieve_size Number of bytes to receive
 * 
 * Performs a complete SPI transaction:
 * 1. Enables SPI by asserting CS
 * 2. Transmits data
 * 3. Receives data (if any)
 * 4. Small delay for stability
 * 5. Disables SPI by de-asserting CS
 */
void spi(uint8_t *transmit_data, uint8_t *recieve_data, int transmit_size, int recieve_size)
{
    spi_enable();                          /* Assert CS */
    spi_transmit(transmit_data, transmit_size);   /* Send data */
    spi_recieve(recieve_data, recieve_size);      /* Receive data */
    for(int i = 0; i< 100; i++);          /* Short delay for stability */
    spi_disable();                         /* De-assert CS */
}
