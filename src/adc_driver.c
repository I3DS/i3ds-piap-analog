#include "adc_driver.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


/*******************************************************************************
* Registers from xparameters.h
*******************************************************************************/
#define XPAR_THERMISTOR_ADC_IF_S00_AXI_BASEADDR 0x80000000
#define XPAR_THERMISTOR_ADC_IF_S00_AXI_HIGHADDR 0x8000FFFF

#define XPAR_TACTILE_ADC_IF_S00_AXI_BASEADDR 0x80010000
#define XPAR_TACTILE_ADC_IF_S00_AXI_HIGHADDR 0x8001FFFF

#define XPAR_TORQUE_ADC_IF_S00_AXI_BASEADDR 0x80020000
#define XPAR_TORQUE_ADC_IF_S00_AXI_HIGHADDR 0x8002FFFF

// Memdev calculations
#define PHYSICAL_ADDRESS XPAR_THERMISTOR_ADC_IF_S00_AXI_BASEADDR
#define ADDRESS_RANGE (XPAR_TORQUE_ADC_IF_S00_AXI_HIGHADDR - PHYSICAL_ADDRESS)

#define ADC_INTERFACE_OFFSET (XPAR_TACTILE_ADC_IF_S00_AXI_BASEADDR - XPAR_THERMISTOR_ADC_IF_S00_AXI_BASEADDR)

#define ADC_ID_OFFSET 4

// Global variables
void *_adcBaseAddr = NULL;
int _fdmemHandle;
bool _adc_initialized = false;
const char _memDevice[] = "/dev/mem";

// Helper functions
#define ADC_GET_UINT32P(X) (uint32_t *)(_adcBaseAddr + X)

// Initialize the adc driver
int adc_initialize() {

  // Open /dev/mem
  _fdmemHandle = open( _memDevice, O_RDWR | O_SYNC );

  if (_fdmemHandle < 0)
  {
    printf("Failed to open the file /dev/mem.\r\n");
    return -1;
  }
  else
  {
    printf("Opened file /dev/mem successfully.\r\n");
  }

  // mmap() /dev/mem using the physical base address provide in the #defines
  _adcBaseAddr = mmap(0, ADDRESS_RANGE, PROT_READ, MAP_SHARED, _fdmemHandle, PHYSICAL_ADDRESS);

  if (_adcBaseAddr == MAP_FAILED) {
    printf("Could not initialize ADC driver\r\n");
    perror("Error occurred when mmapping /dev/mem\r\n");
    return -1;
  }

  _adc_initialized = true;
  return 0;
}

uint32_t adc_read_value(ADC_INTERFACE_ID adc_id, uint8_t adc_num) {
  if (!_adc_initialized) {
    printf("Error: the ADC is not initialized.\r\n");
    return -1;
  }

  if (adc_id > ADC_NUMBER_OF_ADCS) {
    printf("Error: Invalid ADC interface id: %d.\r\n", adc_id);
    return -1;
  }

  if (adc_num > ADC_MAX_INDEX) {
    printf("Error: Invalid ADC index: %d\r\n", adc_num);
    return -1;
  }

  // Read the value from the mmaped device
  return *ADC_GET_UINT32P(
    ADC_INTERFACE_OFFSET * adc_id + adc_num * ADC_ID_OFFSET
    );
}

// Deinitialize the adc driver
void adc_deinitialize() {
  _adc_initialized = false;

  // De-allocate
  if (munmap(_adcBaseAddr, ADDRESS_RANGE) == -1)
  {
    perror("Error occurred when un-mmapping /dev/mem\r\n");
  }

  _adcBaseAddr = NULL;

  // Close the character device
  close(_fdmemHandle);
}
