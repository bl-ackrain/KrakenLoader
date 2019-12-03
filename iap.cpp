#include <stdio.h>
#include <stdint.h>
#include <iap.h>
#include "LPC11U6x.h"
//#include "PokittoDisk.h"

#define TICKRATE_HZ (10)	/* 10 ticks per second */
/* SystemTick Counter */
static volatile uint32_t sysTick;

/* LPC1347 IAP entry address */
#define IAP_LOCATION 0x1fff1ff1

#define last_sector_flash  0x00038000  //0x0000F000
#define IAP_LAST_SECTOR 28 /* Page number 896 - 1023, 0x00038000 - 0x0003FFFF */
#define IAP_NUM_BYTES_TO_WRITE 256
#define WRITECOUNT (IAP_NUM_BYTES_TO_WRITE / 4) /* when data array is in uint32_t */

#define IAP_PREWRRITE_CMD 50 /* Prepare sector for write operation command */
#define IAP_WRISECTOR_CMD 51
#define IAP_ERSSECTOR_CMD 52
#define IAP_REPID_CMD 54

/* IAP command variables */
static unsigned int command[5], result[4];

/* IAP entry function */
typedef int (*IAP)(unsigned int[], unsigned int[]);
IAP iap_entry = (IAP) IAP_LOCATION;

int CopyPageToFlash (uint32_t address, uint8_t* data) {
    IAP iap_call = (IAP) IAP_LOCATION;
    //uint32_t writecount=0;
	asm volatile ("cpsid i" : : : "memory");

    unsigned int sector;
    bool firstpage=false;

    //DEBUG//
    //for (int i=0;i<256;i++) data[i]=0xBB;

    /* Calculate sector based on address */
    if (address < 0x18000) sector = address/0x1000; // sectors go in 4 k's
    else if (address >= 0x38000) sector = 28;
    else if (address >= 0x30000) sector = 27;
    else if (address >= 0x28000) sector = 26;
    else if (address >= 0x20000) sector = 25;
    else sector = 24;

    /* Check is it the first page in the sector */
    if (sector < 24) {
        /* 4KB addresses cover 0x000 - 0xFFF range */
        firstpage = ((address & 0x0FFF) == 0);
    } else {
        /* 32KB addresses cover 0x0000 - 0x7FFF and 0x8000 - 0xFFFF range */
        firstpage = ((address & 0x7FFF) == 0);
    }

	/* Prepare the sector for writing */
	command[0] = IAP_PREWRRITE_CMD;						/* Prepare to write/erase command code */
	command[1] = sector;         							/* Start Sector Number */
	command[2] = sector;		        					/* End Sector Number */
	iap_call(command, result);
    if (result[0]) return 1;

    /* wipe pages when writing the loader */
    //if (address==0x39000) {
    //        erasepage=true;
    //}

    /* do sector erase only when writing first page of given sector */
    if (firstpage) {
        /* Erase the last sector */
        command[0] = IAP_ERSSECTOR_CMD;	   					/* Erase command code*/
        command[1] = sector;             						/* Start Sector Number */
        command[2] = sector;            						/* End Sector Number */
        command[3] = SystemCoreClock / 1000UL;	/* Core clock frequency in kHz */
        iap_call(command, result);
        if (result[0]) return 1;
        /* Prepare to write/erase the last sector, needs to be done again because succesful erase re-locks sectors */
        command[0] = IAP_PREWRRITE_CMD;						/* Prepare to write/erase command code */
        command[1] = sector;			            				/* Start Sector Number */
        command[2] = sector;        							/* Start Sector Number */
        iap_call(command, result);
        if (result[0]) return 1;
    }

    /* page erase for bootloader area */
	/* 
    if (erasepage) {
        command[0] = 59; //erase page command
        command[1] = 896;
        command[2] = 1023;
        command[3] = SystemCoreClock / 1000UL;	// Core clock frequency in kHz 
        iap_call(command, result);
        if (result[0]) return 1;
        // Prepare to write/erase the last sector, needs to be done again because succesful erase re-locks sectors 
        command[0] = IAP_PREWRRITE_CMD;						// Prepare to write/erase command code 
        command[1] = sector;			            				// Start Sector Number 
        command[2] = sector;        							// Start Sector Number 
        iap_call(command, result);
        if (result[0]) return 1;
    }
	*/

	/* Write data to the sectors */
	command[0] = IAP_WRISECTOR_CMD;						/* Write command code */
	command[1] = (uint32_t) (uint32_t*) address;              		    /* Destination Flash Address */
	command[2] = (uint32_t) data;	    				/* Source RAM Address */
	command[3] = 0x100;             					/* Number of Bytes to be written */
	command[4] = SystemCoreClock / 1000;				/* System clock frequency */
	iap_call(command, result);
    if (result[0]) return 1;

	/* Re-enable interrupt mode */
	asm volatile ("cpsie i" : : : "memory");

    return 0; /*succesful write*/

}


//1) EEprom Write
//
//Command code: 61
//Param0: eeprom address (byte, half-word or word aligned)
//Param1: RAM address (byte, half-word or word aligned)
//Param2: Number of bytes to be written ( Byte, Half-words write are ok)
//Param3: System Clock Frequency (CCLK) in kHz
//
//Return Code CMD_SUCCESS | SRC_ADDR_NOT_MAPPED | DST_ADDR_NOT_MAPPED
void writeEEPROM( uint16_t* eeAddress, uint8_t* buffAddress, uint32_t byteCount )
{
	unsigned int command[5], result[4];

	command[0] = 61;
	command[1] = (uint32_t) eeAddress;
	command[2] = (uint32_t) buffAddress;
	command[3] = byteCount;
	command[4] = SystemCoreClock/1000;

	/* Invoke IAP call...*/
#if (EEPROM_PROFILE!=0)

	asm volatile ("cpsid i" : : : "memory");
  	iap_entry(command, result);
  	asm volatile ("cpsie i" : : : "memory");
#else
    asm volatile ("cpsid i" : : : "memory");
  	iap_entry(command, result);
  	asm volatile ("cpsie i" : : : "memory");
#endif
	if (0 != result[0])
	{
		//Trap error
		while(1);
	}
	return;
}

//2) EEprom Read
//Command code: 62
//Param0: eeprom address (byte, half-word or word aligned)
//Param1: RAM address (byte, half-word or word aligned)
//Param2: Number of bytes to be read ( Byte, Half-words read are ok)
//Param3: System Clock Frequency (CCLK) in kHz
//
//Return Code CMD_SUCCESS | SRC_ADDR_NOT_MAPPED | DST_ADDR_NOT_MAPPED
void readEEPROM( uint16_t* eeAddress, uint8_t* buffAddress, uint32_t byteCount )
{
	unsigned int command[5], result[4];

	command[0] = 62;
	command[1] = (uint32_t) eeAddress;
	command[2] = (uint32_t) buffAddress;
	command[3] = byteCount;
	command[4] = SystemCoreClock/1000;

	/* Invoke IAP call...*/
	asm volatile ("cpsid i" : : : "memory");
  	iap_entry(command, result);
  	asm volatile ("cpsie i" : : : "memory");
	if (0 != result[0])
	{
		//Trap error
		while(1);
	}
	return;
}


uint8_t eeprom_read_byte(uint16_t* index) {
    uint8_t val;
    readEEPROM(index,&val,1);
    return val;
}

void eeprom_write_byte(uint16_t* index , uint8_t val) {
    writeEEPROM(index,&val,1);
}