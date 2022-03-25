/* This is the source code for the VEFXIO.DLL that ships with Bemanitools 5.

   If you want to add on some minor functionality like a custom 16seg display
   or a customer slider board then feel free to extend this code with support
   for your custom device.

   This DLL is only used by the generic input variant of IIDXIO.DLL that ships
   with Bemanitools by default. If you want to create a custom IO board that
   provides all of the inputs for IIDX then you should replace IIDXIO.DLL
   instead and not call into this DLL at all. */

#include <windows.h>
#include <mmsystem.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bemanitools/vefxio.h"
#include "bemanitools/input.h"

#define MSEC_PER_NOTCH       128

enum iidx_slider_bits {
    VEFX_IO_S1_UP   = 0x20,
    VEFX_IO_S1_DOWN = 0x21,

    VEFX_IO_S2_UP   = 0x22,
    VEFX_IO_S2_DOWN = 0x23,

    VEFX_IO_S3_UP   = 0x24,
    VEFX_IO_S3_DOWN = 0x25,

    VEFX_IO_S4_UP   = 0x26,
    VEFX_IO_S4_DOWN = 0x27,

    VEFX_IO_S5_UP   = 0x28,
    VEFX_IO_S5_DOWN = 0x29,
};

HANDLE *hComm;
boolean Status;

//ticker
char prevChar[9];
boolean ticker = false;


struct vefx_io_slider_inputs {
    uint8_t slider_up;
    uint8_t slider_down;
};

struct vefx_io_slider {
    uint32_t last_notch;
    int8_t pos;
    int8_t last_dir;
};

static const struct vefx_io_slider_inputs vefx_io_slider_inputs[5] = {
    { VEFX_IO_S1_UP, VEFX_IO_S1_DOWN },
    { VEFX_IO_S2_UP, VEFX_IO_S2_DOWN },
    { VEFX_IO_S3_UP, VEFX_IO_S3_DOWN },
    { VEFX_IO_S4_UP, VEFX_IO_S4_DOWN },
    { VEFX_IO_S5_UP, VEFX_IO_S5_DOWN },
};

static struct vefx_io_slider vefx_io_slider[5];

static void vefx_io_slider_update(uint64_t* ppad);

/* Uncomment these if you need them. */

#if 0
static log_formatter_t vefx_io_log_misc;
static log_formatter_t vefx_io_log_info;
static log_formatter_t vefx_io_log_warning;
static log_formatter_t vefx_io_log_fatal;
#endif

void vefx_io_set_loggers(log_formatter_t misc, log_formatter_t info,
        log_formatter_t warning, log_formatter_t fatal)
{
    /* Uncomment this block if you have something you'd like to log.

       You should probably return false from the appropriate function instead
       of calling the fatal logger yourself though. */
#if 0
    vefx_io_log_misc = misc;
    vefx_io_log_info = info;
    vefx_io_log_warning = warning;
    vefx_io_log_fatal = fatal;
#endif
}

bool vefx_io_init(thread_create_t thread_create, thread_join_t thread_join,
        thread_destroy_t thread_destroy)
{
    /* geninput should have already been initted be now so we don't do it */
    vefx_io_slider[0].pos = 15;
    vefx_io_slider[1].pos = 15;
    vefx_io_slider[2].pos = 15;
    vefx_io_slider[3].pos = 15;
    vefx_io_slider[4].pos = 15;

    /* Initialize your own IO devices here. Log something and then return
       false if the initialization fails. */

    ticker = init_ticker();

    return Status;



    return true;
}

bool vefx_io_recv(uint64_t* ppad)
{
    vefx_io_slider_update(ppad);

    return true;
}

void vefx_io_fini(void)
{
    /* This function gets called as IIDX shuts down after an Alt-F4. Close your
       connections to your IO devices here. */
    if(ticker){
        close_ticker();
    }
}

static void vefx_io_slider_update(uint64_t* ppad)
{
    uint32_t delta;
    int sno;
    int8_t sdir;
    uint32_t now;
    uint64_t pad;

    pad = *ppad;
    now = timeGetTime();

    for (sno = 0 ; sno < 5 ; sno++) {
        if (pad & (1ULL << vefx_io_slider_inputs[sno].slider_up)) {
            sdir = +1;
        } else if (pad & (1ULL << vefx_io_slider_inputs[sno].slider_down)) {
            sdir = -1;
        } else {
            sdir = 0;
        }

        /* Update slider based on current direction */

        if (sdir != vefx_io_slider[sno].last_dir) {
            /* Just started (or stopped). Give the slider a big push to make sure
            it begins to register straight from the first frame */

            vefx_io_slider[sno].pos += sdir;
            vefx_io_slider[sno].last_notch = now;
        } else if (sdir != 0) {
            /* Roll slider forward by an appropriate number of notches, given the
            elapsed time. Roll `last_notch' forward by an appropriate number
            of msec to ensure partial notches get counted properly */

            delta = now - vefx_io_slider[sno].last_notch;
            if (delta >= MSEC_PER_NOTCH) {
                vefx_io_slider[sno].pos += sdir;
                vefx_io_slider[sno].last_notch = now;
            }
        }
        vefx_io_slider[sno].last_dir = sdir;
        if (vefx_io_slider[sno].pos < 0) {
            vefx_io_slider[sno].pos = 0;
        }
        if (vefx_io_slider[sno].pos > 15) {
            vefx_io_slider[sno].pos = 15;
        }
    }
}

uint8_t vefx_io_get_slider(uint8_t slider_no)
{
    if (slider_no > 4) {
        return 0;
    }

    return vefx_io_slider[slider_no].pos;
}

bool vefx_io_write_16seg(const char *text)
{
    /* Insert code to write to your 16seg display here.
       Log something and return false if you encounter an IO error. */
    if(ticker){
        if(!write_ticker(text)){
            return ticker = false;
        }
    }
    return true;
}
//Arduino LCD Ticker
//By Kasaski
//2019-04-29

bool write_ticker(const char *text){

    char send[9];
    strncpy(send,text, 9); //this is probs useless but whatever
    send[9] = 0x00;
    DWORD c;
    //don't send to ticker if no change
    if(!(strcmp(prevChar, send)==0)){
        strcpy(prevChar, send);
        printf("%s\n", send);
        return WriteFile(hComm, send, strlen(send), &c, 0); //send to arduino
    }else{
        return true;
    }

}


bool init_ticker(FILE *port) {
    hComm = CreateFile("\\\\.\\COM2",               //port name
                      GENERIC_READ | GENERIC_WRITE, //Read/Write
                      0,                            // No Sharing
                      NULL,                         // No Security
                      OPEN_EXISTING,                // Open existing port only
                      0,                            // Non Overlapped I/O
                      NULL);                        // Null for Comm Devices

    if (hComm == INVALID_HANDLE_VALUE){
        printf("Error in opening serial port\n");
        return false;
    }
    else{
        printf("opening serial port successful\n");
    }
    DCB dcbSerialParams = { 0 }; // Initializing DCB structure
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_9600;  // Setting BaudRate = 9600
    dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
    dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
    dcbSerialParams.Parity   = NOPARITY;  // Setting Parity = None

    //setting DCB setting on the serial port
    SetCommState(hComm, &dcbSerialParams);

    char lpBuffer[] = "IIDX Init\n";
    DWORD dNoOFBytestoWrite;         // No of bytes to write into the port
    DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port
    dNoOFBytestoWrite = sizeof(lpBuffer);

    Status = WriteFile(hComm,        // Handle to the Serial port
                   lpBuffer,     // Data to be written to the port
                   dNoOFBytestoWrite,  //No of bytes to write
                   &dNoOfBytesWritten, //Bytes written
                   NULL);
    return true;
}

bool close_ticker() {
    char lpBuffer[] = "IIDX closing\0";
    DWORD dNoOFBytestoWrite;         // No of bytes to write into the port
    DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port
    dNoOFBytestoWrite = sizeof(lpBuffer);
    Status = WriteFile(hComm,        // Handle to the Serial port
                   lpBuffer,     // Data to be written to the port
                   dNoOFBytestoWrite,  //No of bytes to write
                   &dNoOfBytesWritten, //Bytes written
                   NULL);
    printf("closing Arduino COMM port");
    CloseHandle(hComm);//Closing the Serial Port
    Sleep(1000);
    return 0;
}
