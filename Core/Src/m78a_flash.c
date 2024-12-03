/*
 * m78a_flash.c
 *
 *  Created on: Nov 28, 2024
 *      Author: Bhakti Ramani
 */

#include "m78a_flash.h"

#define SPI_INSTANCE  2                                           /**< SPI instance index. */
//static const nrf_drv_spi_t spi = {.inst_idx = 3,.use_easy_dma=true};
// static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
// nrf_drv_spi_config_t spi_config_flash = NRF_DRV_SPI_DEFAULT_CONFIG;
static volatile bool spi_xfer_done;


uint8_t CMD_READ_DEVICE_ID = 0x9F;
uint8_t CMD_DUMMY_BYTES = 0x00;
uint8_t CMD_PAGE_READ = 0x13;
uint8_t CMD_GET_FEATURE = 0x0F;
uint8_t CMD_STATUS_REG = 0xC0;
uint8_t CMD_READ_FROM_CACHE = 0x03;
uint8_t CMD_WRITE_ENABLE = 0x06;
uint8_t CMD_PROGRAM_EXECUTE = 0x10;
uint8_t CMD_RANDM_PRGM_DATA = 0x84;
uint8_t CMD_LD_PRGM_DATA = 0x02;
uint8_t CMD_SET_FEATURE = 0x1F;


int trial_column_addr = 0x00;
int trial_block_addr = 0x00;
int trial_page_addr = 0x00;
uint8_t trial_data_byte1 = 0x00;
uint8_t trial_data_byte2 = 0x00;

#define M78A_NO_OF_SEC		4 //No.of Sectors per page
#define M78A_NO_OF_PAGES	64//No.of Pages per block
#define M78A_NO_OF_BLOCKS	1024//No.of Blocks in variant M78ATBIG
#define M78A_SECTOR_SIZE    512
#define M78A_PAGE_SIZE 		(M78A_NO_OF_SEC*M78A_SECTOR_SIZE) //Size of the page
#define M78A_BLOCK_SIZE 	(M78A_NO_OF_PAGES*M78A_PAGE_SIZE) //Size of the block
#define M78A_FLASH_SIZE		(M78A_BLOCK_SIZE*M78A_NO_OF_BLOCKS) //Size of the flash
#define M78A_TOTAL_SECTORS	(M78A_NO_OF_SEC*M78A_NO_OF_PAGES*M78A_NO_OF_BLOCKS) //Total no. of sectors
#define M78A_SECTORS_PER_BLOCK	(M78A_NO_OF_PAGES*M78A_NO_OF_SEC) //No. of sectors per block
#define m78a_RD_SREG_CMD_LEN 0x02
#define m78a_MIN_RCV_BYTES_LEN 0x03
#define m78a_PFAIL_STAT (1<<3)

typedef enum {
	WEL_BIT = (1 << 1),
	OIP_BIT = (1 << 0),
	CRBSY = (1 <<7),
	CONTENTS_OUT = 0xFF,
}status_bits_t;

SPI_HandleTypeDef spihandler;

reg_contents_t reg;
void m78a_init(SPI_HandleTypeDef *spih)
{
	spihandler = *spih;
	device_info_t info;  // Allocate on stack

	m78a_read_device_manufacturar_id(&info);  // Pass the address of the struct

	//setting the protect register to access all the regions (making it unprotected)
	m78a_writeReg(m78a_PROTECT_REG_ADDR, 0x00);

	//reading the config register value and then just setting the ECEE bit
	uint8_t reg_val = m78a_readReg(m78a_CONFIG_REG_ADDR, &reg);
	m78a_writeReg(m78a_CONFIG_REG_ADDR, (reg_val | (1 << 4)));

	//reading the status register for any errors , checking all registers before starting
    m78a_readReg(m78a_STATUS_REG_ADDR, &reg);
    m78a_readReg(m78a_PROTECT_REG_ADDR, &reg);
    m78a_readReg(m78a_CONFIG_REG_ADDR, &reg);


	uint8_t data_buffer[4] = {0xab, 0xbc, 0xcd, 0xef};

	uint8_t read_buffer[4] = {0};
	m78a_write(data_buffer, 2, 0x0010);
	m78a_read(read_buffer, 2, 0x0010);
//	for(int i = 0; i< 100; i++);
//	 m78a_program_load(trial_column_addr,data_buffer );   //colunm, data
//	 m78a_program_execute(trial_block_addr, trial_page_addr);		//block, page

	//m78a_pageRead(trial_block_addr, trial_page_addr, trial_column_addr);		//block, page, column

}
uint8_t m78a_readReg(m78a_reg_t reg_addr, reg_contents_t *reg)
{
    uint8_t cmdBuffer[m78a_RD_SREG_CMD_LEN] = { 0 };
    uint8_t recv_buff[m78a_MIN_RCV_BYTES_LEN] = {0};

    cmdBuffer[0] = CMD_GET_FEATURE;
    cmdBuffer[1] = reg_addr;

    spi(cmdBuffer,&recv_buff[0],m78a_RD_SREG_CMD_LEN,m78a_MIN_RCV_BYTES_LEN);
    reg->reg_contents = recv_buff[0];
    return recv_buff[m78a_MIN_RCV_BYTES_LEN-1];

    /*Wait until all the device is powered up */
//    while((W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
//    {
//        ;
//    }
    W25N01GW_getManufactureAndDevId(&info);
	W25N01GW_init_protect_reg();
	W25N01GW_write(&data_buffer[0], 2, 0x0000);
	W25N01GW_read(&read_buffer[0], 2, 0x0000);
//

}


void m78a_writeReg(m78a_reg_t reg,uint8_t reg_value)
{
    uint8_t cmdBuffer[3] = { 0 };

    cmdBuffer[0] = CMD_SET_FEATURE;
    cmdBuffer[1] = reg;
    cmdBuffer[2] = reg_value;


    spi(&cmdBuffer[0],NULL,3,0);

}
void m78a_pageRead(int block, int page, int column)
{
    /*Initalise the Protection register*/
	W25N01GW_writeReg(W25N01GW_PROTECT_REG_ADDR, 0);
}
uint8_t W25N01GW_readReg(W25N01GW_reg_t reg)
{

	uint8_t cmdBuffer[4] = {CMD_PAGE_READ, CMD_DUMMY_BYTES, ((pageNum >> 8) & 0xff), (pageNum & 0xff)};
	uint8_t recvBuffer[4] = {0};

	spi(cmdBuffer, recvBuffer, 4, 0);
	while((m78a_readReg(m78a_STATUS_REG_ADDR, &reg)) & (1 << 0))
	{
		;
	}

	return 1;

    return recv_buff[W25N01GW_MIN_RCV_BYTES_LEN-1];

}


void W25N01GW_writeReg(W25N01GW_reg_t reg,uint8_t reg_value)
{
    uint8_t cmdBuffer[3] = { 0 };

    cmdBuffer[0] = W25N01GW_CMD_WR_REG;
    cmdBuffer[1] = reg;
    cmdBuffer[2] = reg_value;

    spi(&cmdBuffer[0],3,NULL,0);

}

W25N01GW_errorCode_t W25N01GW_eraseBlock(uint32_t pos,uint32_t len)
{
    uint8_t cmdBuffer[4] = { 0 };

    W25N01GW_errorCode_t res = W25N01GW_ERROR;
    uint16_t pageNum = 0;
    uint8_t reg_value = 0;
    pos = pos-1;

    if (pos%W25N01GW_BLOCK_SIZE != 0 || len%W25N01GW_BLOCK_SIZE != 0) {
        res = W25N01GW_ERR_LOCATION_INVALID;
        return res;
      }
    while(len>0)
    {
        pageNum = pos/W25N01GW_PAGE_SIZE;

        /* Enable write */
        cmdBuffer[0] = W25N01GW_CMD_WR_ENABLE;

       spi(cmdBuffer[0],1,NULL,0);


        /*Block Erase*/
        cmdBuffer[0] = W25N01GW_CMD_128KB_BLCK_ERASE;
        cmdBuffer[1] = 0; // Dummy Cycle
        cmdBuffer[2] = ((pageNum >> 8) & 0xff);     //Page Address
        cmdBuffer[3] = (pageNum & 0xff);        //Page Address

        spi(&cmdBuffer[0],4,NULL,0);

        /*Wait until all the requested blocks are erase*/
        while((reg_value=W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
        {
            ;
        }

        if((reg_value&W25N01GW_PFAIL_STAT)||(reg_value&W25N01GW_EFAIL_STAT))
        {
            return W25N01GW_ERASE_FAILURE;
        }

        pos += W25N01GW_BLOCK_SIZE;
        len -= W25N01GW_BLOCK_SIZE;

    }

    res = W25N01GW_ERASE_SUCCESS;
    return res;

}


W25N01GW_errorCode_t W25N01GW_pageRead(uint16_t pageNum)
{
    uint8_t dummy_byte=0;
    uint8_t cmdBuffer[4] = { 0 };

    cmdBuffer[0] = W25N01GW_CMD_PAGE_DATA_RD;
    cmdBuffer[1] = dummy_byte;
    cmdBuffer[2] = ((pageNum >> 8) & 0xff);     //Page Address
    cmdBuffer[3] = (pageNum & 0xff);        //Page Address

    spi(&cmdBuffer[0],4,NULL,0);

    /*Wait until Ready*/
    while(W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR)&W25N01GW_BUSY_STAT)
    {
        ;
    }
    return W25N01GW_READ_SUCCESS;
}

W25N01GW_errorCode_t W25N01GW_read_spare(uint8_t* dataPtr,int8_t noOfbytesToRead,uint16_t pageNum,uint16_t pageOff)
{
    uint8_t cmdBuffer[4] = { 0 };

    uint8_t dummy_byte=0,reg_val = 0;

    uint8_t rcv_buff[256] = {0};

    if(dataPtr==NULL)
    {
        return W25N01GW_ERR_BUFFER_INVALID;
    }
    if(noOfbytesToRead==0)
    {
        return W25N01GW_ERR_BYTE_LEN_INVALID;
    }
    /*Page read*/
    W25N01GW_pageRead(pageNum);
    /* Read to the data buffer*/
    cmdBuffer[0] = W25N01GW_CMD_RD_DATA;
    cmdBuffer[3] = dummy_byte;
    cmdBuffer[1] = ((pageOff >> 8) & 0xff);
    cmdBuffer[2] = (pageOff & 0xff);

    spi(&cmdBuffer[0],4,rcv_buff,noOfbytesToRead+4);

    memcpy(dataPtr,&rcv_buff[4],noOfbytesToRead);
    reg_val = W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR);
    if((reg_val&W25N01GW_ECC0_STAT)||(reg_val&W25N01GW_ECC0_STAT))
    {
        return W25N01GW_ECC_FAILURE;
    }
    return W25N01GW_READ_SUCCESS;

}

W25N01GW_errorCode_t W25N01GW_read(uint8_t* dataPtr, uint32_t noOfbytesToRead, uint32_t readLoc)
{
    uint8_t cmdBuffer[4] = {0};
    uint16_t pageNum=0,pageOff=0;
    uint16_t rd_len_page = 0;
    uint8_t rd_len = 0;
    uint8_t dummy_byte=0;

    uint8_t rcv_buff[256] = {0};

    if(dataPtr==NULL)
    {
        return W25N01GW_ERR_BUFFER_INVALID;
    }
    if(noOfbytesToRead==0)
    {
        return W25N01GW_ERR_BYTE_LEN_INVALID;
    }
    if(readLoc>W25N01GW_FLASH_SIZE)
    {
        return W25N01GW_ERR_LOCATION_INVALID;
    }

    while(noOfbytesToRead>0)
    {
        //if(W25N01GW_readReg(W25N01GW_CONFIG_REG_ADDR)&W25N01GW_REG_CONF_ECCE) // Check if ECC-E flag is set. If set page size is 1024 else page size is 2048+64, as the bytes used to store
        pageNum = readLoc/(W25N01GW_PAGE_SIZE);
        pageOff = readLoc%(W25N01GW_PAGE_SIZE);

        rd_len_page = MIN(noOfbytesToRead, W25N01GW_PAGE_SIZE-pageOff+1);

        /*Page read, program Load operation*/
        m78a_page_read(pageNum);

        /*program execute operation*/
        /* Read to the data buffer*/
        cmdBuffer[0] = W25N01GW_CMD_RD_DATA;
        cmdBuffer[3] = dummy_byte;

        while(rd_len_page>0)
        {
            dataPtr += rd_len;
            cmdBuffer[1] = ((pageOff >> 8) & 0xff);
            cmdBuffer[2] = (pageOff & 0xff);
            rd_len = MIN(rd_len_page,251);//Since every read cycle has four dummy bytes added in front.
            //Driver supports maximum reading of 256 bytes. Hence update the page Offset and read 256 bytes in a cycle.
			spi(cmdBuffer, rcv_buff, 4, rd_len + 4);

            memcpy(dataPtr,&rcv_buff[4],rd_len);
            rd_len_page -= rd_len;
            pageOff += rd_len;
            noOfbytesToRead -=rd_len;
            readLoc += rd_len;
        }

    }
    m78a_readReg(m78a_STATUS_REG_ADDR, &reg);

    return 1;
}



W25N01GW_errorCode_t W25N01GW_write(const uint8_t* dataPtr, uint32_t noOfbytesToWrite, uint32_t writeLoc)
{
    uint8_t cmdBuffer[4] = { 0 };
    uint16_t pageNum=0,pageOff=0;
    uint16_t wr_len_page = 0;
    uint8_t dummy_byte = 0;
    uint8_t tx_buf[3+128] = {0};
    uint8_t reg_value=0;
    uint16_t txn_off,txn_len = 0;
    uint8_t reg_val = 0;


    if(writeLoc==1)
    {
        reg_value=0;
    }
    if(dataPtr==NULL)
    {
        return W25N01GW_ERR_BUFFER_INVALID;
    }
    if(noOfbytesToWrite==0)
    {
        return W25N01GW_ERR_BYTE_LEN_INVALID;
    }
    if(writeLoc>W25N01GW_FLASH_SIZE)
    {
        return W25N01GW_ERR_LOCATION_INVALID;
    }
    while(noOfbytesToWrite>0)
    {
        pageNum = writeLoc/W25N01GW_PAGE_SIZE;
        pageOff = writeLoc%W25N01GW_PAGE_SIZE;

        wr_len_page = MIN(noOfbytesToWrite, W25N01GW_PAGE_SIZE+1 - pageOff);

        if(wr_len_page!=W25N01GW_PAGE_SIZE)
        {
            /*If ECC is enabled */
            m78a_page_read(pageNum);
            tx_buf[0] = CMD_RANDM_PRGM_DATA;
        }
        else
        {
            tx_buf[0] = W25N01GW_CMD_LD_PRGM_DATA;
        }
        /* Enable write */



//		m78a_write_enable();
        reg_val = m78a_readReg(m78a_STATUS_REG_ADDR, &reg);
        m78a_writeReg(m78a_STATUS_REG_ADDR, (reg_val | (1 << 1)) );
        m78a_readReg(m78a_STATUS_REG_ADDR, &reg);



        /*Load the program into Databuffer*/
           for (txn_off = 0, txn_len = 0; txn_off < wr_len_page;txn_off += txn_len)
           {
              txn_len = MIN(128, wr_len_page - txn_off);
              tx_buf[1] = (pageOff + txn_off) >> 8;
              tx_buf[2] = (pageOff + txn_off) & 0xff;
              memcpy(tx_buf + 3, dataPtr, txn_len);
			  spi(tx_buf, rx_dummy_buff, 3 + txn_len, 0 );

              tx_buf[0] = CMD_RANDM_PRGM_DATA;
              dataPtr += txn_len;
            }

           /*Execute the program*/
            cmdBuffer[0] = W25N01GW_CMD_PRGM_EXEC;
            cmdBuffer[1] = dummy_byte;//Dummy byte
            cmdBuffer[2] = ((pageNum >> 8) & 0xff);
            cmdBuffer[3] = (pageNum & 0xff);

//            nrf_drv_spi_transfer(&spi,&cmdBuffer[0],4,NULL,0);
			spi(cmdBuffer, NULL, 4, 0 );


            /*Wait until data is written to the flash*/
			while((reg_value = m78a_readReg(m78a_STATUS_REG_ADDR, &reg)) & (1 << 0))
			{
				;
			}
            if(reg_value&m78a_PFAIL_STAT)
            {
                return -1;
            }

           writeLoc += wr_len_page;
           noOfbytesToWrite -= wr_len_page;
    }

    return W25N01GW_WRITE_SUCCESS;
}

void W25N01GW_getManufactureAndDevId(W25N01GW_deviceInfo_t* info)
{
    uint8_t cmdBuffer[2] = { 0 };
    uint8_t dummy_byte = 0;
    uint8_t recv_buff[5];
    /*TOBE REMOVED*/
    cmdBuffer[0] = W25N01GW_CMD_JDEC_ID;
    cmdBuffer[1] = dummy_byte;

    memset(recv_buff,0,5);
    spi(&cmdBuffer[0],W25N01GW_RD_SREG_CMD_LEN,&recv_buff[0],5);

    info->mfgId = recv_buff[2];
    info->deviceId = recv_buff[3] << 8 | recv_buff[4];

}

W25N01GW_errorCode_t W25N01GW_getDeviceInitStatus()
{
    return W25N01GW_initStatus;
}

void  W25N01GW_getMemoryParams(W25N01GW_memoryParams_t* flashMemoryParams)
{


    /* Get the memorySize from the macro defined for the flash module */
        flashMemoryParams->eraseBlockUnits = 1;
        flashMemoryParams->memorySize = W25N01GW_FLASH_SIZE;
        flashMemoryParams->noOfSectors = W25N01GW_TOTAL_SECTORS;
        flashMemoryParams->sectorSize = W25N01GW_SECTOR_SIZE;

}
void m78a_read_device_manufacturar_id(device_info_t *info)
{
	uint8_t transmit_commands_for_device_id[2] = {CMD_READ_DEVICE_ID, CMD_DUMMY_BYTES};
	uint8_t rcv_buffer[2] = {0};


	spi(transmit_commands_for_device_id, rcv_buffer, 2, 2);
	info -> manufacturar_id = rcv_buffer[0];
	info -> voltage_spec = rcv_buffer[1];

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
void spi(uint8_t *transmit_data,  int transmit_size, uint8_t *recieve_data,int recieve_size)
{
    spi_enable();                          /* Assert CS */
    if(transmit_data!=NULL)
    {
    	HAL_SPI_Transmit(&spihandler, transmit_data, transmit_size, SPI_TIMEOUT);
    }
    if(recieve_data!=NULL)
    {
        spi_recieve(recieve_data, recieve_size);      /* Receive data */
    }
    for(int i = 0; i< 100; i++);          /* Short delay for stability */
    spi_disable();                         /* De-assert CS */
}
