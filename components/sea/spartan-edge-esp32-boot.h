/*
 * spartan_edge_esp32_boot.h - for spartan_edge board
 * Modified by: Olivier Lesage
 */

// ensure this library description is only included once
#ifndef SPARTAN_EDGE_ESP32_BOOT_H
#define SPARTAN_EDGE_ESP32_BOOT_H

/* pin define */
#define XFPGA_CCLK_PIN GPIO_NUM_17
#define XFPGA_DIN_PIN GPIO_NUM_27
#define XFPGA_PROGRAM_PIN GPIO_NUM_25
#define XFPGA_INTB_PIN GPIO_NUM_26
#define XFPGA_DONE_PIN GPIO_NUM_34

// the size of a single read
// If this value is too high, which will result result in failure
#define READ_SIZE 256

class spartan_edge_esp32_boot {
  public:
    // initialization
    int begin(void);
    void end(void);

    // loading bitstream
    void xfpgaGPIOInit(void);
    int xlibsSstream(const char* path);
  private:
};
#endif
