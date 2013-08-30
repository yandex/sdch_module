#ifndef AZLIB_H
#define AZLIB_H

typedef struct {
    unsigned char *next_in;
    unsigned avail_in;
    unsigned long total_in;
    
    unsigned char *next_out;
    unsigned avail_out;
    unsigned long total_out;
} z_stream;

#define Z_NO_FLUSH 0
#define Z_SYNC_FLUSH 2
#define Z_FINISH 4

#define Z_OK 0
#define Z_STREAM_END 1
#define Z_BUF_ERROR (-5)

#endif
