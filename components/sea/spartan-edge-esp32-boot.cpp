/* Spartan Edge Accelerator board library for loading bitstreams to the FPGA */
/* Modified by: Olivier Lesage */

#include <dirent.h>
#include "spartan-edge-esp32-boot.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "driver/gpio.h" // GPIO driver

#define LOW 0
#define HIGH 1

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13
#endif //USE_SPI_MODE

static const char *TAG = "spartan-edge-esp32-boot";

// initialization
int spartan_edge_esp32_boot::begin(void) {
  // Mount SD Card
  ESP_LOGI(TAG, "Initializing SD card");

#ifndef USE_SPI_MODE
  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // To use 1-line SD mode, uncomment the following line:
  // slot_config.width = 1;

  // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
  // Internal pull-ups are not sufficient. However, enabling internal pull-ups
  // does make a difference some boards, so we do that here.
  gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
  gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
  gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

#else
  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = PIN_NUM_MISO;
  slot_config.gpio_mosi = PIN_NUM_MOSI;
  slot_config.gpio_sck  = PIN_NUM_CLK;
  slot_config.gpio_cs   = PIN_NUM_CS;
  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
#endif //USE_SPI_MODE

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 10,
      .allocation_unit_size = 16 * 1024
  };

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount filesystem. Please reboot the board.");
      } else {
          ESP_LOGE(TAG, "Failed to initialize the card (%s). "
              "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      }
      return -1;
  }

  // Card has been initialized, print its properties
  ESP_LOGI(TAG, "Printing SD card properties:");
  sdmmc_card_print_info(stdout, card);
  return 0; // success
}

// unmount (clean up)
void spartan_edge_esp32_boot::end(void) {
    // unmount partition and disable SDMMC (or SPI peripheral)
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(TAG, "SD Card unmounted");
    return;
}

// XFPGA pin Initialize
void spartan_edge_esp32_boot::xfpgaGPIOInit(void) {
  // GPIO Initialize
  gpio_set_direction(XFPGA_INTB_PIN, GPIO_MODE_INPUT);
  gpio_set_direction(XFPGA_DONE_PIN, GPIO_MODE_INPUT);
  gpio_set_direction(XFPGA_PROGRAM_PIN, GPIO_MODE_OUTPUT);

  // FPGA configuration start sign
  gpio_set_level(XFPGA_PROGRAM_PIN, LOW);
  gpio_set_direction(XFPGA_CCLK_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(XFPGA_CCLK_PIN, LOW);
  gpio_set_level(XFPGA_PROGRAM_PIN, HIGH);

  // wait until fpga reports reset complete
  while(gpio_get_level(XFPGA_INTB_PIN) == 0);
}

// loading the FPGA LOGIC
int spartan_edge_esp32_boot::xlibsSstream(const char* path) {
  unsigned char byte_buff[1024] = {0};
  int byte_len;
  unsigned byte;
  int i = 0;
  int j = 0;

  // open the file using C standard library functions
  char full_path[256];
  snprintf(full_path, sizeof(full_path), "/sdcard%s", path);
  FILE* fp = fopen(full_path, "rb"); // bitstream requires binary mode

  if(!fp) {
    ESP_LOGE(TAG, "Failed to open file %s. Please check that the file exists.", full_path);

    // list files in overlay directory
    struct dirent *de; // directory pointer
    DIR* dr = opendir("/sdcard/overlay");
    if (!dr) {
        ESP_LOGE(TAG, "Couldn't open /overlay/ directory.");
        return -1;
    }
    ESP_LOGE(TAG, "Valid options found in /overlay/ are:");
    while ((de = readdir(dr)) != NULL) ESP_LOGI(TAG, "%s", de->d_name);
    return -1;
  }

  /* read data from bitstream */
  byte_len = fread(byte_buff, sizeof(char), READ_SIZE, fp);

  // find the raw bits
  if(byte_buff[0] != 0xff)
  {
    // skip header
    i = ((byte_buff[0]<<8) | byte_buff[1]) + 4;

    // find the 'e' record
    while(byte_buff[i] != 0x65)
    {
        // skip the record
        i += (byte_buff[i+1]<<8 | byte_buff[i+2]) + 3;
        // exit if the next record isn't within the buffer
        if(i>= byte_len) {
            ESP_LOGE(TAG, "DEV ERROR: Next record not within byte buffer.");
            return -1;
        }
    }
    // skip the field name and bitstream length
    i += 5;
  } // else it's already a raw bin file

  /* put pins down for Configuration */
  gpio_set_direction(XFPGA_DIN_PIN, GPIO_MODE_OUTPUT);

   /*
   * loading the bitstream
   * If you want to know the details,you can Refer to the following documentation
   * https://www.xilinx.com/support/documentation/user_guides/ug470_7Series_Config.pdf
   */
  int k = 0;
  int dot_num = 1;
  while ((byte_len != 0)&&(byte_len != -1)) {

    // update a status message every 5 writes to show write is in progress
    if (k==0) {
        if (dot_num == 1) printf("\rWriting.   ");
        else if (dot_num == 2) printf("\rWriting..  ");
        else if (dot_num == 3) printf("\rWriting... ");
        else if (dot_num == 4) printf("\rWriting....");
        dot_num =  dot_num % 4 + 1; // 1,2,3,4,1,2,3,4...
    }

    for ( ;i < byte_len;i++) {
      byte = byte_buff[i];

      for(j = 0;j < 8;j++) {
        REG_WRITE(GPIO_OUT_W1TC_REG, (1<<XFPGA_CCLK_PIN));
        REG_WRITE((byte&0x80)?GPIO_OUT_W1TS_REG:GPIO_OUT_W1TC_REG, (1<<XFPGA_DIN_PIN));
        byte = byte << 1;
        REG_WRITE(GPIO_OUT_W1TS_REG, (1<<XFPGA_CCLK_PIN));
      }
    }
    // byte_len = file.read(byte_buff, READ_SIZE); // Arduino
    byte_len = fread(byte_buff, sizeof(char), READ_SIZE, fp);
    i = 0;
    k = (k+1)%15;
  }
  printf("\n");
  gpio_set_level(XFPGA_CCLK_PIN, LOW);

  if(byte_len == -1) ESP_LOGE(TAG, "bitstream read error");

  fclose(fp);

  // check the result
  // if(0 == digitalRead(XFPGA_DONE_PIN)) { // Arduino
  if (0 == gpio_get_level(XFPGA_DONE_PIN)) {
    ESP_LOGE(TAG, "FPGA Configuration Failed");
  }
  else {
    ESP_LOGI(TAG, "FPGA Configuration Success!");
  }
  return 0;
}
