/* 
 * RTSP recorder
 * 
 * F8 Network (c) 2014, jack139
 *
 * Version: 1.0
 *
 */
#include <stdint.h>

struct RTP_header{
  /* byte 0 */
  unsigned short cc     :4; /* CSRC count */
  unsigned short x      :1; /* header extension flag */
  unsigned short p      :1; /* padding flag */
  unsigned short version:2; /* protocol version */
  /* byte 1 */
  unsigned short pt     :7; /* payload type */
  unsigned short marker :1; /* marker bit */
  /* bytes 2, 3 */
  uint16_t seqno __attribute__((packed)); /* sequence number */
  /* bytes 4-7 */
  uint32_t ts    __attribute__((packed)); /* timestamp in ms */
  /* bytes 8-11 */
  uint32_t ssrc  __attribute__((packed)); /* synchronization source */
} __attribute__((packed));

struct RTSP_header{
  uint8_t  dollar;     /* 1 byte, $:dollar sign(24 decimal) */
  uint8_t  channel_id; /* 1 byte, channel id */
  uint16_t reserved      __attribute__((packed));  /* 2 bytes, reseved */
  uint32_t payload_len   __attribute__((packed));  /* 4 bytes, payload length */
  struct RTP_header rtp_head; /* 12 bytes, rtp head */
} __attribute__((packed));

struct RTP_extension{
  uint16_t cust_def      __attribute__((packed));  /* 2 bytes, defined by profile */
  uint16_t length        __attribute__((packed));  /* 2 bytes, length */
} __attribute__((packed));

struct RTSP_interleaved{
  uint8_t  dollar;     /* 1 byte, $:dollar sign(24 decimal) */
  uint8_t  channel_id; /* 1 byte, channel id */
  uint16_t payload_len   __attribute__((packed));  /* 2 bytes, length */
  struct RTP_header rtp_head; /* 12 bytes, rtp head */
} __attribute__((packed));

enum STATE{
  IDLE, 
  CONNECT,
  RECORDING,
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  CLOSE,
  SENDING,
  WAIT_RESP,
  HEARTBEAT
};

extern uint32_t max_size;

void *snap_thread(void);
