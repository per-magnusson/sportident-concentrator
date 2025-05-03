#include "SerialInput.h"
#include "debug.h"

PacketBuffer packet_buffer;

// Some Sportident protocol characters
static const uint8_t P_STX = 0x02; // Start of transmission
static const uint8_t P_ETX = 0x03; // End of transmission
static const uint8_t P_DLE = 0x10; // Delimiter to put before 0x00-0x1F in basic protocol
static const uint8_t P_AUTOSEND = 0xD3; // Autosend packet
static const uint8_t P_WAKEUP = 0xFF; // Sent to station from a computer, but should not really appear in autosend data from a station
static const uint8_t P_LEGACY_AUTOSEND = 0x53; // Autosend packet, legacy
static const uint8_t P_LEGACY_AUTOSEND1 = 0x33; // Autosend packet, very old BSF3 stations
static const uint8_t P_LEGACY_AUTOSEND2 = 0x54; // Autosend packet, legacy lightbeam
static const uint8_t P_LEGACY_LIMIT = 0x80; // Command codes below this are in the legacy protocol


static const uint8_t AUTOSEND_SIZE = 0x0D; // Size of an autosend packet

static const int TIMEOUT = 1000; // If no bytes are received within this many milliseconds, 
                                 // go back to the SER_IDLE state, waiting for a new start byte.


elapsedMillis ser_global_time;

/*
 Extended prototcol example package:

 card 425074, control, code 31
 0x2 0xd3 0xd 0x0 0x1f 0x0 0x4 0x61 0xf2 0x10 0x9b 0x1c 0xc8 0x0 0x4 0x8 0xc0 0xf 0x3 

 Legacy protocol example packages:
 card 425074, finish, code 10

 0x2 0x53 0x10 0x11 0x10 0xa 0x10 0x4 0x61 0xf2 0x10 0x0 0x10 0x1 0x38 0x10 0x0 0x22 0x25 0x3
 0x2 0x53 0x10 0x11 0x10 0xa 0x10 0x4 0x61 0xf2 0x10 0x0 0x10 0x1 0xac 0x10 0xc 0xda 0x10 0x4 0x3 
 */

void SerialInput::new_byte(uint8_t c)
{
  if(packet_state != PS_IDLE && ser_global_time > last_byte_time + TIMEOUT) {
    // Time out! 
    link_status = LS_TIMEOUT;
    packet_state = PS_IDLE; // Be ready for a new packet
    debug_println(" Timeout");
  }

  last_byte_time = ser_global_time;

  if(packet_state != PS_IDLE || c != 0) {
    debug_printf("%d 0x%x ", packet_state, c);
  }

  if(packet_state == PS_IDLE) {
    if(c == P_STX) {
      buf_idx = 0;
      buf[buf_idx++] = c;
      packet_state = PS_CMD;
    } else if(c == P_WAKEUP) {
      // This should not really happen, but ignore if it does.
    } else if(c == 0) {
      // 0 seems to be coming in from the software serial ports, ignore
    } else {
      // Not a start of packet byte when expecting one. This is an error. Maybe wrong speed?
      packet_state = PS_GARBAGE; // A timeout will take us out of this state
      link_status = LS_FAILED;
    }
  } else if(packet_state == PS_CMD) {
    buf[buf_idx++] = c;
    if(c < P_LEGACY_LIMIT) {
      // Legacy protocol packet. This is not expected, but is handled properly.
      packet_state = PS_LEGACY;
      link_status = LS_LEGACY;
      legacy_buf[0] = buf[0]; // STX
      legacy_buf[1] = buf[1]; // CMD
      legacy_buf_idx = 2;
    } else {
      packet_state = PS_SIZE;
      if(c == P_AUTOSEND) {
        link_status = LS_AUTOSEND;
      } else {
        // Some unexpected kind of packet!
        link_status = LS_UNEXPECTED;
      }
    }
  } else if(packet_state == PS_LEGACY) {
    buf[buf_idx++] = c;
    if(c == P_DLE) {
      // Do not store DLE in legacy_buf
      packet_state = PS_LEGACY_DLE;
    } else {
      legacy_buf[legacy_buf_idx++] = c;
      if(c == P_ETX) {
        // End of legacy packet
        packet_state = PS_IDLE;
        // Decode card and control code, if possible
        if(legacy_buf[1] == P_LEGACY_AUTOSEND) {
          last_code = legacy_buf[3];
          last_card = (legacy_buf[5] << 8) + legacy_buf[6];
          if(legacy_buf[4] <= 4) {
            // Weird SI5 card number encoding
            last_card += legacy_buf[4]*100000;
          } else {
            // Normal card number encoding
            last_card += legacy_buf[4]<<16;
          }
        } else {
          // Don't know how to interpret this kind of package
          last_code = 0;
          last_card = 0;
        }
        new_data = true;
        ever_ok = true;
        debug_println(" Legacy OK");
        // Put the packet in the TX buffer
        packet_buffer.push_packet(buf, buf_idx);
      }
    }
  } else if(packet_state == PS_LEGACY_DLE) {
    buf[buf_idx++] = c;
    legacy_buf[legacy_buf_idx++] = c;
    packet_state = PS_LEGACY;
  } else if(packet_state == PS_SIZE) {
    buf[buf_idx++] = c;
    packet_size = c;
    packet_state = PS_DATA;
    if(packet_size != AUTOSEND_SIZE) {
      link_status = LS_UNEXPECTED;
      if(packet_size == 0) {
        packet_state = PS_CRC1;
      }
    }
  } else if(packet_state == PS_DATA) {
    buf[buf_idx++] = c;
    packet_size--;
    if(packet_size == 0) {
      packet_state = PS_CRC1;
    }
  } else if(packet_state == PS_CRC1) {
    buf[buf_idx++] = c;
    packet_state = PS_CRC2;
  } else if(packet_state == PS_CRC2) {
    buf[buf_idx++] = c;
    packet_state = PS_ETX;
    // Do not bother to check the CRC?
  } else if(packet_state == PS_ETX) {
    buf[buf_idx++] = c;
    if(c == P_ETX) {
      if(link_status == LS_AUTOSEND) {
        // We got a complete autosend package, extract station code and card number
        last_code = (buf[3]<<8) + buf[4];
        last_card = (buf[7]<<8) + buf[8];
        if(buf[6] < 5) {
          /* This is an SI 5 card with weird encoding:
              - byte 0:   always 0 (not stored on the card)
              - byte 1:   card series (stored on the card as CNS), to be multiplied by 100000, unless it is 0 or 1
              - byte 2, 3:card number
              - printed:  100'000*CNS + card number
              - nr range: 1-65'000 + 200'001-265'000 + 300'001-365'000 + 400'001-465'000
              Card series 0 and 1 do not have the 0/1 printed on the card
              */
          last_card += 100000 * buf[6];
        } else {
          last_card += (buf[6]<<16) + (buf[5] << 24);
        }
        link_status = LS_OK;
      } else {
        // Some unexpected kind of packet
        last_code = 0;
        last_card = 0;
      }
      new_data = true;
      ever_ok = true;
      debug_println(" OK");
      // Put the packet in the TX buffer
      packet_buffer.push_packet(buf, buf_idx);
    } else {
      // Did not get an ETX at the end of the packet
      link_status = LS_FAILED;
      debug_println(" Missing ETX!");
    }
    packet_state = PS_IDLE;
  }
}


// No byte received, check for timeout
void SerialInput::ping()
{
  if(packet_state != PS_IDLE && ser_global_time > last_byte_time + TIMEOUT) {
    // Time out! 
    link_status = LS_TIMEOUT;
    packet_state = PS_IDLE; // Be ready for a new packet
    debug_println(" Timeout");
  }
}


bool PacketBuffer::push_packet(uint8_t *buf, int len)
{
  for(int ii=0; ii < len && ii < PB_BUFLEN; ii++) {
    buffers[next_free_buffer][ii] = buf[ii];
  }
  buflens[next_free_buffer] = len;
  next_free_buffer++;
  if(next_free_buffer >= N_BUFFERS) {
    next_free_buffer = 0;
  }
  if(next_free_buffer == current_buffer) {
    // Ooops, we are out of buffers. This should not happen.
    // Not handled here, just tell the calling function.
    return false;
  }
  return true;
}


uint8_t *PacketBuffer::get_packet()
{
  if(current_buffer != next_free_buffer) {
    // We have at least one buffer that is filled, return a pointer to it.
    return buffers[current_buffer];
  }
  // No buffer with valid data.
  return NULL;
}


int PacketBuffer::get_packet_len()
{
  if(current_buffer != next_free_buffer) {
    // We have at least one buffer that is filled, return its length.
    return buflens[current_buffer];
  }
  // No buffer with valid data.
  return 0;
}


void PacketBuffer::pop_packet()
{
  if(current_buffer != next_free_buffer) {
    // We have at least one buffer that is filled. Free it.
    buflens[current_buffer] = 0;
    current_buffer++;
    if(current_buffer >= N_BUFFERS) {
      current_buffer = 0;
    }
  }
}


