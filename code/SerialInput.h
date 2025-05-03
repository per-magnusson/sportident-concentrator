#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <elapsedMillis.h>


static const int BUFLEN = 300;      // A packet from an SI station cannot be this long. Max 255+5 = 260 bytes

class PacketBuffer;

extern PacketBuffer packet_buffer;

extern elapsedMillis ser_global_time;


enum ser_link_status_t {
  LS_ZERO,       // no bytes received yet
  LS_FAILED,     // invalid packet received
  LS_TIMEOUT,    // Too long gap between bytes in packet
  LS_UNEXPECTED, // unexpected, but valid, packet received
  LS_AUTOSEND,   // recieving an autosend packet, looking OK so far
  LS_LEGACY,     // legacy packet received
  LS_OK,         // valid packet received
};


enum ser_packet_state_t {
  PS_IDLE,
  PS_CMD,
  PS_SIZE,
  PS_DATA,
  PS_CRC1,
  PS_CRC2,
  PS_ETX,
  PS_LEGACY,
  PS_LEGACY_DLE,
  PS_GARBAGE
};


// Class to handle the reception of serial autosend packets with punching information
// from a SportIdent station.
class SerialInput
{
  public:
  SerialInput() : buf_idx(0), legacy_buf_idx(0), packet_size(0), link_status(LS_ZERO), packet_state(PS_IDLE), 
  last_byte_time(0), last_card(0), last_code(0), new_data(false), speed(38400), ever_ok(false)
  { }

  void new_byte(uint8_t c); // Handle a new byte that was (just) received
  void ping(); // No new byte, just check for timeout
  ser_link_status_t get_link_status() {return link_status;};
  void reset_link_status() {link_status = LS_ZERO; packet_state = PS_IDLE;};
  bool link_ok_p() {return (link_status != LS_FAILED) && (link_status != LS_TIMEOUT);};
  int get_last_card() {return last_card;};
  int get_last_code() {return last_code;};
  bool check_new_data() {return new_data;};
  void ack_new_data() {new_data = false;};
  void set_speed(int speed_a) {speed = speed_a; ever_ok = false; link_status = LS_ZERO;};
  int get_speed() {return speed;};
  bool get_ever_ok() {return ever_ok;};


  private:
  uint8_t buf[BUFLEN];
  uint8_t legacy_buf[BUFLEN]; // To store unescaped (no DLE) legacy data to make decoding easier
  int buf_idx;
  int legacy_buf_idx;
  int packet_size;
  ser_link_status_t link_status;
  ser_packet_state_t packet_state;
  uint32_t last_byte_time;
  int last_card;
  int last_code;
  bool new_data;
  int speed; // To store the speed of the serial port, 38400 or 4800
  bool ever_ok; // Has a packet ever been successfully received at this speed?
};


// Class to buffer received packets until they have been sent out.
// A circular buffer of buffers is used. Should be large enough for 
// all realistic scenarios.
class PacketBuffer
{
  public:
  PacketBuffer() : current_buffer(0), next_free_buffer(0)
  { }

  bool push_packet(uint8_t *buf, int len);
  uint8_t *get_packet(); // Returns NULL if no packet is available
  int get_packet_len();
  void pop_packet();

  private:
  static const int PB_BUFLEN = BUFLEN; // Length of each buffer
  static const int N_BUFFERS = 100;    // Number of buffers, 100 is severe overkill, but we have the memory
  uint8_t buffers[N_BUFFERS][PB_BUFLEN];
  uint8_t buflens[N_BUFFERS];
  int current_buffer;
  int next_free_buffer;
};
