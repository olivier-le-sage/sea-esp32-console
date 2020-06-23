/*
 * Console command for loading a bitstream from a file and programming
 *   the Xilinx Spartan-7 (xc7s15ftgb196-1) FPGA present on the Seeedstudio SEA
 *
 * Author: Olivier Lesage
 */

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmd_load_bitstream.h"
#include "spartan-edge-esp32-boot.h" // include the dev board library

static const char *TAG = "cmd_load_bitstream";

// initialize the spartan_edge_esp32_boot library
spartan_edge_esp32_boot esp32Cla;

// the bitstream name which we are loading
#define OVERLAY_FOLDER "/overlay/"
#define DEFAULT_FILE OVERLAY_FOLDER "default.bit"

// argtable
static struct {
    struct arg_str *filename;
    struct arg_end *end;
} load_bitstream_args;

/* Load the bitstream from the file system and program the FPGA.
    The filename can be specified. If the file isn't found, the command fails.
    If no filename is specified, a default bitstream file is loaded.
*/
static int load_bitstream(int argc, char *argv[]) {
    // check for errors in input args
    int nerrors = arg_parse(argc, argv, (void **) &load_bitstream_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, load_bitstream_args.end, argv[0]);
        return 1;
    }

    // library initialization
    if (0 != esp32Cla.begin()) {
        ESP_LOGE(TAG, "Failed to initialize SEA library.");
        return 1;
    };


    // XFPGA pin Initialize
    esp32Cla.xfpgaGPIOInit();

    if (load_bitstream_args.filename->count == 1) {

        // load the bitstream onto the FPGA
        char temp[256];
        snprintf(temp, sizeof(temp), "%s%s", OVERLAY_FOLDER, load_bitstream_args.filename->sval[0]);
        esp32Cla.xlibsSstream(temp);
    } else {
        // load the default bitstream onto the FPGA
        esp32Cla.xlibsSstream(DEFAULT_FILE);
    }
    return 0;
}

void register_load_bitstream() {
    load_bitstream_args.filename =
        arg_strn(NULL, NULL, "[file]", 0, 1, "input bitstream file name");
    load_bitstream_args.end =
        arg_end(1);

    const esp_console_cmd_t cmd = {
        .command = "load_bitstream",
        .help = "Program Spartan-7 FPGA with a bitstream from the SD card. "
                "Will only search in the /overlay/ folder.",
        .hint = NULL,
        .func = &load_bitstream,
        .argtable = &load_bitstream_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
