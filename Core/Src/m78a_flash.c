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


#define W25N01GW_CMD_RD_REG          0x0F
#define W25N01GW_CMD_WR_REG          0x1F
#define W25N01GW_CMD_WR_ENABLE           0x06
#define W25N01GW_CMD_WR_DISABLE          0x04
#define W25N01GW_CMD_128KB_BLCK_ERASE    0xD8
#define W25N01GW_CMD_LD_PRGM_DATA        0x02
#define W25N01GW_CMD_RANDM_PRGM_DATA     0x84
#define W25N01GW_CMD_QUAD_LD_PRGM_DATA       0x32
#define W25N01GW_CMD_QUAD_RANDM_PRGM_DATA    0x34
#define W25N01GW_CMD_PRGM_EXEC           0x10
#define W25N01GW_CMD_PAGE_DATA_RD        0x13
#define W25N01GW_CMD_RD_DATA             0x03
#define W25N01GW_CMD_FAST_RD             0x0B

SPI_HandleTypeDef spihandler;

#define W25N01GW_BUSY_STAT  (1 << 0)
#define W25N01GW_WEL_STAT   (1 << 1)
#define W25N01GW_EFAIL_STAT (1 << 2)
#define W25N01GW_PFAIL_STAT (1 << 3)
#define W25N01GW_ECC0_STAT (1 << 4)
#define W25N01GW_ECC1_STAT (1 << 5)

#define W25N01GW_REG_CONF_BUF (1 << 3)
#define W25N01GW_REG_CONF_ECCE (1 << 4)

#define W25N01GW_CMD_JDEC_ID     0x9F
#define W25N01GW_CMD_RESET       0xFF
/*COMMAND LENGTH FOR READ STATUS REG COMMAND*/
#define W25N01GW_RD_SREG_CMD_LEN 0x02
#define W25N01GW_MIN_RCV_BYTES_LEN  0x03/*First two bytes will receive nothing */


#define W25N01GW_DEVICE_ID 0xBA21
#define W25M01GW_DEVICE_ID 0xBB21
#define W25N01GW_MANUFACTURER_ID 0xEF
W25N01GW_errorCode_t W25N01GW_initStatus = W25N01GW_UNINITIALIZED;

//Return '1' if the bit value at position y within x is '1' and '0' if it's 0 by ANDing x with a bit mask where the bit in y's position is '1' and '0' elsewhere and comparing it to all 0's.  Returns '1' in least significant bit position if the value of the bit is '1', '0' if it was '0'.
#define READ(x,y) ((0u == (x & (1<<y)))?0u:1u)
/**********************************************************************************/
/* static function declaration */
/**********************************************************************************/

/**********************************************************************************/
/* functions */
/**********************************************************************************/

/**********************************************************************************/

// void spi_event_handler(nrfx_spim_evt_t const * p_event,
//                         void * p_context)
// {
//     spi_xfer_done = true;
// }
W25N01GW_errorCode_t W25N01GW_Init(SPI_HandleTypeDef *spih)
{
	spihandler = *spih;
    W25N01GW_errorCode_t ret_code = W25N01GW_INITIALIZATION_FAILED;
    uint8_t reg_val = 0;
    W25N01GW_deviceInfo_t info;

    /*Wait until all the device is powered up */
//    while((W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
//    {
//        ;
//    }
    W25N01GW_getManufactureAndDevId(&info);
     	uint8_t data_buffer[4];
     	memset(data_buffer,  0xAA, 2);
        uint8_t read_buffer[4] = {0};
     	W25N01GW_write(data_buffer, 2, 0x0000);
     	W25N01GW_read(read_buffer, 2, 0x0000);

     	for(int i = 0; i< 100; i++);


//
//    W25N01GW_getManufactureAndDevId(&info);
//
//    if (((info.deviceId == W25N01GW_DEVICE_ID)||(info.deviceId == W25M01GW_DEVICE_ID)) && (info.mfgId == W25N01GW_MANUFACTURER_ID))
//    {
//        /*Initalise the Protection register*/
//        W25N01GW_writeReg(W25N01GW_PROTECT_REG_ADDR,0);
//
//
//        /*Initalise the Config register
//         * 1)Read from Config Register.
//         * 2)Set the ECC-E and BUF
//         * */
//        reg_val = W25N01GW_readReg(W25N01GW_CONFIG_REG_ADDR);
//
//        W25N01GW_writeReg(W25N01GW_CONFIG_REG_ADDR,(reg_val | W25N01GW_REG_CONF_BUF) & ~(W25N01GW_REG_CONF_ECCE));
//        W25N01GW_initStatus = W25N01GW_INITIALIZED;
//        ret_code = W25N01GW_INITIALIZED;
//
//    }
//    else
//    {
//        ret_code = W25N01GW_INITIALIZATION_FAILED;
//    }
//    return ret_code;

}
uint8_t W25N01GW_readReg(W25N01GW_reg_t reg)
{
    uint8_t cmdBuffer[W25N01GW_RD_SREG_CMD_LEN] = { 0 };
    uint8_t recv_buff[W25N01GW_MIN_RCV_BYTES_LEN] = {0};

    cmdBuffer[0] = W25N01GW_CMD_RD_REG;
    cmdBuffer[1] = reg;

    //nrf_drv_spi_transfer(&spi,&cmdBuffer[0],W25N01GW_RD_SREG_CMD_LEN,&recv_buff[0],W25N01GW_MIN_RCV_BYTES_LEN);
	spi(&cmdBuffer[0],&recv_buff[0],W25N01GW_RD_SREG_CMD_LEN , W25N01GW_MIN_RCV_BYTES_LEN );

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
    uint8_t cmdBuffer[4] = { 0 };
    uint16_t pageNum=0,pageOff=0;
    uint16_t rd_len_page = 0;
    uint8_t rd_len = 0;
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

        /*Page read*/
        W25N01GW_pageRead(pageNum);

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
            spi(&cmdBuffer[0],4,rcv_buff,rd_len+4);
            memcpy(dataPtr,&rcv_buff[4],rd_len);
            rd_len_page -= rd_len;
            pageOff += rd_len;
            noOfbytesToRead -=rd_len;
            readLoc += rd_len;
        }

    }
    reg_val = W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR);
    if((reg_val&W25N01GW_ECC0_STAT)||(reg_val&W25N01GW_ECC0_STAT))
    {
        return W25N01GW_ECC_FAILURE;
    }
    return W25N01GW_READ_SUCCESS;
}

W25N01GW_errorCode_t W25N01GW_write_spare(const uint8_t* dataPtr,uint8_t noOfbytesToWrite,uint16_t pageNum,uint16_t pageOff)
{
    uint8_t cmdBuffer[4] = { 0 };
    uint8_t dummy_byte = 0;
    uint8_t tx_buf[3+128] = {0};
    uint8_t reg_value=0;

    if(dataPtr==NULL)
    {
        return W25N01GW_ERR_BUFFER_INVALID;
    }
    if(noOfbytesToWrite==0 || noOfbytesToWrite>4)
    {
        return W25N01GW_ERR_BYTE_LEN_INVALID;
    }

    /* Enable write */
    cmdBuffer[0] = W25N01GW_CMD_WR_ENABLE;

    spi(&cmdBuffer[0],1,NULL,0);


    /*Load the program into Databuffer*/
      tx_buf[0] = W25N01GW_CMD_RANDM_PRGM_DATA;
      tx_buf[1] = (pageOff) >> 8;
      tx_buf[2] = (pageOff) & 0xff;
      memcpy(tx_buf + 3, dataPtr, noOfbytesToWrite);
      spi(&tx_buf[0],3 + noOfbytesToWrite,NULL,0);

       /*Execute the program*/
        cmdBuffer[0] = W25N01GW_CMD_PRGM_EXEC;
        cmdBuffer[1] = dummy_byte;//Dummy byte
        cmdBuffer[2] = ((pageNum >> 8) & 0xff);
        cmdBuffer[3] = (pageNum & 0xff);

        spi(&cmdBuffer[0],4,NULL,0);

        /*Wait until data is written to the flash*/
        while((reg_value=W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
        {
            ;
        }
        if(reg_value&W25N01GW_PFAIL_STAT)
        {
            return W25N01GW_WRITE_FAILURE;
        }

    return W25N01GW_WRITE_SUCCESS;

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
            W25N01GW_pageRead(pageNum);
            tx_buf[0] = W25N01GW_CMD_RANDM_PRGM_DATA;
        }
        else
        {
            tx_buf[0] = W25N01GW_CMD_LD_PRGM_DATA;
        }
        /* Enable write */
        cmdBuffer[0] = W25N01GW_CMD_WR_ENABLE;

                spi(&cmdBuffer[0],1,NULL,0);



        /*Load the program into Databuffer*/
           for (txn_off = 0, txn_len = 0; txn_off < wr_len_page;txn_off += txn_len)
           {
              txn_len = MIN(128, wr_len_page - txn_off);
              tx_buf[1] = (pageOff + txn_off) >> 8;
              tx_buf[2] = (pageOff + txn_off) & 0xff;
              memcpy(tx_buf + 3, dataPtr, txn_len);
              spi(&tx_buf[0],3 + txn_len,NULL,0);
              tx_buf[0] = W25N01GW_CMD_RANDM_PRGM_DATA;
              dataPtr += txn_len;
            }

           /*Execute the program*/
            cmdBuffer[0] = W25N01GW_CMD_PRGM_EXEC;
            cmdBuffer[1] = dummy_byte;//Dummy byte
            cmdBuffer[2] = ((pageNum >> 8) & 0xff);
            cmdBuffer[3] = (pageNum & 0xff);

            spi(&cmdBuffer[0],4,NULL,0);

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


// uint8_t CMD_READ_DEVICE_ID = 0x9F;
// uint8_t CMD_DUMMY_BYTES = 0x00;
// uint8_t CMD_PAGE_READ = 0x13;
// uint8_t CMD_GET_FEATURE = 0x0F;
// uint8_t CMD_STATUS_REG = 0xC0;
// uint8_t CMD_READ_FROM_CACHE = 0x03;
// uint8_t CMD_WRITE_ENABLE = 0x06;
// uint8_t CMD_PROGRAM_EXECUTE = 0x10;
// uint8_t CMD_RANDM_PRGM_DATA = 0x84;
// uint8_t CMD_LD_PRGM_DATA = 0x02;
// uint8_t CMD_SET_FEATURE = 0x1F;
// uint8_t CMD_BLOCK_LOCK_ADDR = 0xA0;
// uint8_t CMD_ALL_BLOCK_UNLOCK = 0x00;

// int trial_column_addr = 0x00;
// int trial_block_addr = 0x00;
// int trial_page_addr = 0x00;
// uint8_t trial_data_byte1 = 0x00;
// uint8_t trial_data_byte2 = 0x00;

// #define M78A_NO_OF_SEC		4 //No.of Sectors per page
// #define M78A_NO_OF_PAGES	64//No.of Pages per block
// #define M78A_NO_OF_BLOCKS	1024//No.of Blocks in variant M78ATBIG
// #define M78A_SECTOR_SIZE    512
// #define M78A_PAGE_SIZE 		(M78A_NO_OF_SEC*M78A_SECTOR_SIZE) //Size of the page
// #define M78A_BLOCK_SIZE 	(M78A_NO_OF_PAGES*M78A_PAGE_SIZE) //Size of the block
// #define M78A_FLASH_SIZE		(M78A_BLOCK_SIZE*M78A_NO_OF_BLOCKS) //Size of the flash
// #define M78A_TOTAL_SECTORS	(M78A_NO_OF_SEC*M78A_NO_OF_PAGES*M78A_NO_OF_BLOCKS) //Total no. of sectors
// #define M78A_SECTORS_PER_BLOCK	(M78A_NO_OF_PAGES*M78A_NO_OF_SEC) //No. of sectors per block

// typedef enum {
// 	WEL_BIT = (1 << 1),
// 	OIP_BIT = (1 << 0),
// 	CRBSY = (1 <<7),
// 	CONTENTS_OUT = 0xFF,
// }status_bits_t;

// SPI_HandleTypeDef spihandler;

// void m78a_block_unlock()
// {
// 	uint8_t cmd_buffer[3] = {CMD_SET_FEATURE, CMD_BLOCK_LOCK_ADDR, CMD_ALL_BLOCK_UNLOCK};
// 	uint8_t rcv_buffer[0] = {};
// 	spi(cmd_buffer, rcv_buffer, 3, 0);

// }
// void m78a_init(SPI_HandleTypeDef *spih)
// {
// 	spihandler = *spih;
// 	m78a_read_device_manufacturar_id();
//     m78a_check_status_register(0xFF);
// //   m78a_check_status_register(OIP_BIT);
// 	//for(int i = 0; i<100; i++);
// 	m78a_block_unlock();
// 	uint8_t data_buffer[4];
// 	memset(data_buffer,  0xAA, 2);
// 	uint8_t read_buffer[4] = {0};
// 	m78a_write(data_buffer, 2, 0x0000);
// 	for(int i = 0; i<100; i++);
// 	m78a_check_status_register(0xFF);
// 	m78a_read(read_buffer, 2, 0x0000);
// 	for(int i = 0; i< 100; i++);
// //	 m78a_program_load(trial_column_addr,data_buffer );   //colunm, data
// //	 m78a_program_execute(trial_block_addr, trial_page_addr);		//block, page
// //
// //	m78a_pageRead(trial_block_addr, trial_page_addr, trial_column_addr);		//block, page, column


// }

// void m78a_pageRead(int block, int page, int column)
// {
// 	//sending page read command, 8 dummy bytes for 8 clock cycle, bloack address, page address
// 	 int temp = 0;
// 	 temp = block & 0xF;
// 	 block = block >> 2;
// 	 temp <<= 6;
// 	 page = page | temp;
// 	uint8_t cmdBuffer[4] = {CMD_PAGE_READ, CMD_DUMMY_BYTES, block, page};
// 	uint8_t recvBuffer[4] = {0};

// 	spi(cmdBuffer, recvBuffer, 4, 0);

// 	 //sending get command and status register address to check and recieving the contents of status register, polling the Operation in execution command to see if read is done(data from memory to cache is done)

// 	m78a_check_status_register(OIP_BIT);

// 	 //reading from cache register(data transferred from desired memory array to cache register, reading only 2 bytes for now)

// 	 uint8_t dummy_and_colunm = column >> 8;
// 	 column = column & 0xFF;

// 	 uint8_t cmd_cache_buffer[] = {CMD_READ_FROM_CACHE, dummy_and_colunm, trial_column_addr, CMD_DUMMY_BYTES};
// 	 uint8_t recv_cache_buffer[2] = {0};
// 	 spi(cmd_cache_buffer, recv_cache_buffer, 4, 2); // Perform SPI transmission

// }

// //reads the memory data by transferring desired address locations's data to cache register
// void m78a_page_read(uint16_t pageNum)
// {
// 	//sending page read command, 8 dummy bytes for 8 clock cycle, bloack address, page address

// 	uint8_t cmdBuffer[4] = {CMD_PAGE_READ, CMD_DUMMY_BYTES, ((pageNum >> 8) & 0xff), (pageNum & 0xff)};
// 	uint8_t recvBuffer[4] = {0};

// 	spi(cmdBuffer, recvBuffer, 4, 0);

// 	// sending get command and status register address to check and recieving the contents of status register, polling the Operation in execution command to see if read is done(data from memory to cache is done)

// 	m78a_check_status_register(OIP_BIT);




// }

// int m78a_read(uint8_t* dataPtr, uint32_t noOfbytesToRead, uint32_t readLoc)
// {
//     uint8_t cmdBuffer[4] = { 0 };
//     uint16_t pageNum=0,pageOff=0;
//     uint16_t rd_len_page = 0;
//     uint8_t rd_len = 0;
//     uint8_t dummy_byte=0,reg_val = 0;

//     uint8_t rcv_buff[256] = {0};

//     if(dataPtr==NULL)
//     {
//         return -1;
//     }
//     if(noOfbytesToRead==0)
//     {
//         return -1;
//     }
//     if(readLoc>M78A_FLASH_SIZE)
//     {
//         return -1;
//     }

//     while(noOfbytesToRead>0)
//     {
//         //if(M78A_readReg(M78A_CONFIG_REG_ADDR)&M78A_REG_CONF_ECCE) // Check if ECC-E flag is set. If set page size is 1024 else page size is 2048+64, as the bytes used to store
//         pageNum = readLoc/(M78A_PAGE_SIZE);
//         pageOff = readLoc%(M78A_PAGE_SIZE);

//         rd_len_page = MIN(noOfbytesToRead, M78A_PAGE_SIZE-pageOff+1);

//         /*Page read*/
//         m78a_page_read(pageNum);

//         /* Read to the data buffer*/
//         cmdBuffer[0] = CMD_READ_FROM_CACHE;
//         cmdBuffer[3] = dummy_byte;

//         while(rd_len_page>0)
//         {
//             dataPtr += rd_len;
//             cmdBuffer[1] = ((pageOff >> 8) & 0xff);
//             cmdBuffer[2] = (pageOff & 0xff);
//             rd_len = MIN(rd_len_page,251);//Since every read cycle has four dummy bytes added in front.
//             //Driver supports maximum reading of 256 bytes. Hence update the page Offset and read 256 bytes in a cycle.
// 			spi(cmdBuffer, rcv_buff, 4, rd_len + 4);
//             //nrf_drv_spi_transfer(&spi,&cmdBuffer[0],4,rcv_buff,rd_len+4);
//             memcpy(dataPtr,&rcv_buff[4],rd_len);
//             rd_len_page -= rd_len;
//             pageOff += rd_len;
//             noOfbytesToRead -=rd_len;
//             readLoc += rd_len;
//         }

//     }

//     //reg_val = M78A_readReg(M78A_STATUS_REG_ADDR);
//     // if((reg_val&M78A_ECC0_STAT)||(reg_val&M78A_ECC0_STAT))
//     // {
//     //     return M78A_ECC_FAILURE;
//     // }
//     return 1;
// }


// int m78a_write(const uint8_t* dataPtr, uint32_t noOfbytesToWrite, uint32_t writeLoc)
// {
//     uint8_t cmdBuffer[4] = { 0 };
//     uint16_t pageNum=0,pageOff=0;
//     uint16_t wr_len_page = 0;
//     uint8_t dummy_byte = 0;
//     uint8_t tx_buf[3+128] = {0};
// 	uint8_t rx_dummy_buff[] = {0};
//     uint8_t reg_value=0;
//     uint16_t txn_off,txn_len = 0;


//     if(writeLoc==1)
//     {
//         reg_value=0;
//         (void)reg_value;
//     }
//     if(dataPtr==NULL)
//     {
//         return -1;
//     }
//     if(noOfbytesToWrite==0)
//     {
//         return -1;
//     }
//     if(writeLoc>M78A_FLASH_SIZE)
//     {
//         return -1;
//     }
//     while(noOfbytesToWrite>0)
//     {
//         pageNum = writeLoc/M78A_PAGE_SIZE;
//         pageOff = writeLoc%M78A_PAGE_SIZE;

//         wr_len_page = MIN(noOfbytesToWrite, M78A_PAGE_SIZE+1 - pageOff);

//         if(wr_len_page!=M78A_PAGE_SIZE)
//         {
//             /*If ECC is enabled */
//             //M78A_pageRead(pageNum);
//             tx_buf[0] = CMD_RANDM_PRGM_DATA;
//         }
//         else
//         {
//             tx_buf[0] = CMD_LD_PRGM_DATA;
//         }
//         /* Enable write */
//         // cmdBuffer[0] = M78A_CMD_WR_ENABLE;

//         //     nrf_drv_spi_transfer(&spi,&cmdBuffer[0],1,NULL,0);
// 			m78a_write_enable();



//         /*Load the program into Databuffer*/
//            for (txn_off = 0, txn_len = 0; txn_off < wr_len_page;txn_off += txn_len)
//            {
//               txn_len = MIN(128, wr_len_page - txn_off);
//               tx_buf[1] = (pageOff + txn_off) >> 8;
//               tx_buf[2] = (pageOff + txn_off) & 0xff;
//               memcpy(tx_buf + 3, dataPtr, txn_len);
// 			  spi(tx_buf, rx_dummy_buff, 3 + txn_len, 0 );
//               //nrf_drv_spi_transfer(&spi,&tx_buf[0],3 + txn_len,NULL,0);
//               tx_buf[0] = CMD_RANDM_PRGM_DATA;
//               dataPtr += txn_len;
//             }

//            /*Execute the program*/
//             cmdBuffer[0] = CMD_PROGRAM_EXECUTE;
//             cmdBuffer[1] = dummy_byte;//Dummy byte
//             cmdBuffer[2] = ((pageNum >> 8) & 0xff);
//             cmdBuffer[3] = (pageNum & 0xff);

// //            nrf_drv_spi_transfer(&spi,&cmdBuffer[0],4,NULL,0);
// 			spi(cmdBuffer, rx_dummy_buff, 4, 0 );


//             /*Wait until data is written to the flash*/
// 		    m78a_check_status_register(OIP_BIT);
//             // while((reg_value=W25N01GW_readReg(W25N01GW_STATUS_REG_ADDR))&W25N01GW_BUSY_STAT)
//             // {
//             //     ;
//             // }
//             // if(reg_value&M78A_PFAIL_STAT)
//             // {
//             //     return -1;
//             // }

//            writeLoc += wr_len_page;
//            noOfbytesToWrite -= wr_len_page;
//     }

//     return 1;
// }
// //passing the colunm in page and data buffer array to write at that address, will be sawved into cache register
// //pass colunm and 2 data bytes
// void m78a_program_load(int column, uint8_t *data_byte_buffer)
// {
// 	m78a_write_enable();
// 	//Sending the command for program load, dummy bits , plane bit(0), 12 column address bits and data buffer 
//     uint8_t dummy_and_colunm = column >> 8;
// 	column = column & 0xFF;
// 	uint8_t program_load_cmd_buffer[] = {CMD_LD_PRGM_DATA, dummy_and_colunm, column };
// 	uint8_t dummy_buffer[0];
// 	spi(program_load_cmd_buffer, dummy_buffer, 3 , 0);
// 	spi(data_byte_buffer, dummy_buffer, 2047, 0);
// }

// //transferring from cache register to
// //pass block and page address to write it
// void m78a_program_execute(int block, int page)
// {
// 	int temp = 0;
// 	temp = block & 0xF;
// 	block = block >> 2;
// 	temp <<= 6;
// 	page = page | temp;
// 	uint8_t program_execute_cmd_buffer[] = {CMD_PROGRAM_EXECUTE, CMD_DUMMY_BYTES,  block, page};
// 	uint8_t recieved_data_bytes[4] = {0};

// 	spi(program_execute_cmd_buffer, recieved_data_bytes, 4, 0);

// 	//sending get command and status register address to check and recieving the contents of status register, polling the Operation in execution command to see if read is done(data from memory to cache is done)
// 	m78a_check_status_register(OIP_BIT);



// }

// //OIP bit = 0 means device is ready for nextr command
// //WEL bit = 1 means device is ready for write operations in memory
// void m78a_check_status_register(int number_of_bit)
// {
// 	uint8_t status_reg_check_cmd_buffer[2] = {CMD_GET_FEATURE, CMD_STATUS_REG};  // TX commands
// 	uint8_t recieved_state_status_reg[2] = {}; // Recieved_state of status register
// 	spi(status_reg_check_cmd_buffer, recieved_state_status_reg, 2, 2); // Perform SPI transmission

// 	switch(number_of_bit)
// 	{

// 		case WEL_BIT:
// 			while(!(recieved_state_status_reg[0] & WEL_BIT));
		
// 			break;

// 		case OIP_BIT:
// //			int time = 1000;
// //			while((recieved_state_status_reg[0] & OIP_BIT) && (time != 0 )) //bit not set
// //			{
// //				time = time - 1;
// //			}
// 			while(recieved_state_status_reg[0] & OIP_BIT)
// 			{
// 				;
// 			}
// 		case CRBSY:
// 			while(recieved_state_status_reg[0] & CRBSY)
// 			{
// 				;
// 			}
	

		
// 			//retransmitting the status register and reading its contents to varify if OIP bit is actually free
// 			//spi(status_reg_check_cmd_buffer, recieved_state_status_reg, 2, 2); // Perform SPI transmission

// 			break;
// 		case CONTENTS_OUT:
// 			break;
// 		default:
// 			break;
// 	}
// }


// void m78a_write_enable(void)
// {
// 	//sending write enable and polling the status register to check if WEL bit is actually set
// 	spi(&CMD_WRITE_ENABLE, &CMD_DUMMY_BYTES, 1, 0);
// 	m78a_check_status_register(WEL_BIT);

// }
// void m78a_read_device_manufacturar_id()
// {
// 	uint8_t transmit_commands_for_device_id[2] = {CMD_READ_DEVICE_ID, CMD_DUMMY_BYTES};
// 	uint8_t recieved_device_id[2] = {0};

// 	spi(transmit_commands_for_device_id, recieved_device_id, 2, 2);
// }
// /* Timeout value for SPI operations in milliseconds */

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
    spi_transmit(transmit_data, transmit_size);   /* Send data */
    spi_recieve(recieve_data, recieve_size);      /* Receive data */
    for(int i = 0; i< 100; i++);          /* Short delay for stability */
    spi_disable();                         /* De-assert CS */
}
