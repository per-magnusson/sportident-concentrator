// Concentrator for Sportident stations. Four serial port inputs, 
// one long wire input, one serial port output and one USB output.

// Documentation for PIO UART:
// https://arduino-pico.readthedocs.io/en/latest/piouart.html

#include <LiquidCrystal.h>
#include <elapsedMillis.h>
#include "SerialInput.h"
#include "lcd_util.h"
#include "debug.h"

// Configure the code for use as a triple long-wire receiver rather than the normal concentrator box
#define RECEIVE_ONLY 0

static const uint32_t POWER_BANK_PULSE_MS = 1100;         // Length of power bank keep-alive pulse
static const uint32_t POWER_BANK_PULSE_PERIOD_MS = 25000; // Time between power bank keep-alive pulses


// Use only software serial ports to allow for different baud rates on TD1/RD1 and 
// make all accesses to the ports identical.
// The USB port is used as an additional serial output.
static const int TD1_Pin = 0;
static const int RD1_Pin = 1;
static const int RD2_Pin = 13;
static const int RD3_Pin = 11;
static const int RD4_Pin = 9;
static const int RD5_Pin = 15;

static const int LED_Pin = 25;
static const int LCD_Backlight_Pin = 5;
static const int SW1_Pin = 6;
static const int SW2_Pin = 7;
static const int SW3_Pin = 8;
static const int SW4_Pin = 10;
static const int Button_Pin = 14;
static const int LCD_RS_Pin = 21;
static const int LCD_EN_Pin = 20;
static const int LCD_D4_Pin = 19;
static const int LCD_D5_Pin = 18;
static const int LCD_D6_Pin = 17;
static const int LCD_D7_Pin = 16;
static const int Batt_Pin = 28;
static const int Resistor_Pin = 22; // To periodically pull power from the power bank so that it does not power off

static const int N_INPUTS = 5; // Number of serial port inputs, must remain at 5 even in RECEIVE_ONLY as a lot is hard-coded for that

static const int LOW_BAUD = 5000;      // Low baud rate, nominal is 4800, but slightly higher works better for some reason
static const int LOW_BAUD_TX = 4800;   // Low baud rate for transmission
static const int HIGH_BAUD = 38400;    // High baud rate


// PIO serial ports
SerialPIO serTX(TD1_Pin, SerialPIO::NOPIN);
SerialPIO ser1(SerialPIO::NOPIN, RD1_Pin);
SerialPIO ser2(SerialPIO::NOPIN, RD2_Pin);
SerialPIO ser3(SerialPIO::NOPIN, RD3_Pin);
SerialPIO ser4(SerialPIO::NOPIN, RD4_Pin);
SerialPIO ser5(SerialPIO::NOPIN, RD5_Pin); // Long wire input
SerialPIO *serial_ports[N_INPUTS+1]; // Array of pointers to the serial input ports so that we can index them

// Serial port protocol handlers
SerialInput serial_input[N_INPUTS + 1]; // use index 1 to N_INPUTS for clearer code (wasting 0)


LiquidCrystal lcd(LCD_RS_Pin, LCD_EN_Pin, LCD_D4_Pin, LCD_D5_Pin, LCD_D6_Pin, LCD_D7_Pin);

elapsedMillis global_time;

uint32_t resistor_time;
uint32_t blink_time;

void lcd_show_splash();


void setup() {
  analogReadResolution(12);
  pinMode(LED_Pin, OUTPUT);

  digitalWrite(Resistor_Pin, LOW);
  pinMode(Resistor_Pin, OUTPUT);
  resistor_time = global_time;
  blink_time = global_time;

  pinMode(LCD_Backlight_Pin, OUTPUT);

  Serial.begin();
  Serial.ignoreFlowControl(true); // This may be necessary e.g. when talking to MeOS as it does not seem to assert the virtual DTR

  lcd.begin(8, 2);
  lcd_define_glyphs();
  lcd_show_splash();

  // Reboot life sign
  int ii;
  ii = 0;
  while (ii++ < 5) {
    digitalWrite(LED_Pin, HIGH);
    digitalWrite(LCD_Backlight_Pin, HIGH);
    sleep_ms(50);
    digitalWrite(LED_Pin, LOW);
    digitalWrite(LCD_Backlight_Pin, LOW);
    sleep_ms(50);
  }

  digitalWrite(LCD_Backlight_Pin, HIGH);

  // Initialize the serial ports array
  serial_ports[0] = NULL; // 0 is not used, start with index 1 for clearer code
  serial_ports[1] = &ser1;
  serial_ports[2] = &ser2;
  serial_ports[3] = &ser3;
  serial_ports[4] = &ser4;
  serial_ports[5] = &ser5;
  int speed;
#if RECEIVE_ONLY
  // Start with 4800 in long-wire receive mode
  speed = LOW_BAUD;
#else
  // Start with 38400 on the serial port inputs
  speed = HIGH_BAUD;
#endif
  for(ii = 1; ii <= 4; ii++) {
    serial_input[ii].set_speed(speed);
    serial_ports[ii]->begin(speed);
  }
  speed = LOW_BAUD;
  serial_input[5].set_speed(speed);
  serial_ports[5]->begin(speed); // Long wire
  // Use 4800 for the long wire
  speed = LOW_BAUD_TX;
  serTX.begin(speed);
}


double read_batt()
{
  // Read the battery voltage
  int acc = 0;
  const int niter = 10;
  double volt;

  for(int ii = 0; ii < niter; ii++) {
    // Average a few times
    acc += analogRead(Batt_Pin);
  }
  // ADC max = 4095 at 3.3 V
  // 1:2 voltage divider
  volt = 2* 3.3 * acc / (double) niter / 4095.0;
  return volt;
}



// Show some information on the LCD as soon as possible after power up.
void lcd_show_splash()
{
  char str[16];

  lcd.clear();
  lcd.print("SIC v1.0");
  // Show the battery voltage
  lcd.setCursor(0, 1); // bottom left
  sprintf(str, "  %.2f V", read_batt());
  lcd.print(str);
}


void led_heartbeat()
{
  static int led_state = 0;

  if(global_time > blink_time + 100) {
    if(led_state) {
      digitalWrite(LED_Pin, LOW);
    } else {
      digitalWrite(LED_Pin, HIGH);
    }
    led_state = !led_state;
    blink_time = global_time + 100;
  }
}


void powerbank_keepalive()
{
  static int state = 0;

  if(state == 0) {
    // Drawing extra power from the power bank
    digitalWrite(Resistor_Pin, HIGH);
    digitalWrite(LED_Pin, HIGH);
    if(global_time > resistor_time + POWER_BANK_PULSE_MS) {
      resistor_time = global_time;
      state = 1;
      digitalWrite(Resistor_Pin, LOW);
      digitalWrite(LED_Pin, LOW);
    }
  } else {
    // Between pulses
    digitalWrite(Resistor_Pin, LOW);
    digitalWrite(LED_Pin, LOW);
    if(global_time > resistor_time + POWER_BANK_PULSE_PERIOD_MS) {
      resistor_time = global_time;
      state = 0;
      digitalWrite(Resistor_Pin, HIGH);
      digitalWrite(LED_Pin, HIGH);
    }
  }
}


void switch_baud(int n)
{
  int new_speed;

  if(serial_input[n].get_speed() < 2*LOW_BAUD) {
    new_speed = HIGH_BAUD;
  } else {
    new_speed = LOW_BAUD;
  }

  debug_printf("Trying %d baud on port %d\n", new_speed, n);
  serial_ports[n]->end();
  serial_ports[n]->begin(new_speed, SERIAL_8N1);
  // Consume garbage
  delay(10);
  while(serial_ports[n]->available() > 0) {
    char c;
    c = serial_ports[n]->read();
    c = c; // To avoid warning about variable that is set but not used when DEBUG is 0
    debug_printf("eating '0x%x' ", (int)c);
    delay(10);
  }
  serial_input[n].set_speed(new_speed);
  debug_printf("\n%d in effect\n", new_speed);
  serial_input[n].reset_link_status();
}


void receive_from_inputs()
{
  uint8_t c;

  for(int ii = 1; ii <= N_INPUTS; ii++) {
    if(serial_ports[ii]->available() > 0) {
      c = serial_ports[ii]->read();
      serial_input[ii].new_byte(c);
    } else {
      serial_input[ii].ping();
    }
    if(!serial_input[ii].link_ok_p()) {
      // Got some invalid data
      // Have we ever got something valid at this speed on this port?
      if(!serial_input[ii].get_ever_ok()) {
        // No, try the other speed
        switch_baud(ii);
      }
    }
  }
}


// Transmit data from the packet buffer, if data is available and the serial port is ready.
void transmit()
{
  static uint8_t *buf = NULL;
  static int buflen = 0;
  static int bufidx = 0;

  if(!buf) {
    buf = packet_buffer.get_packet();
    if(buf) {
      buflen = packet_buffer.get_packet_len();
      bufidx = 0;
      if(buflen < 6) {
        // This should not happen
        debug_println("Short packet!");
        packet_buffer.pop_packet(); // Done with this packet
        return;
      }
    } else {
      // No packet available
      return;
    }
  }

  while(serTX.availableForWrite()) {
    serTX.write(buf[bufidx]);
    if(DEBUG) {
      debug_printf("0x%x\n", buf[bufidx]); // Debug on USB output
    } else {
      Serial.write(buf[bufidx]);
    }
    bufidx++;
    if(bufidx >= buflen) {
      // We have emptied this buffer
      packet_buffer.pop_packet(); // Done with this packet
      debug_println("Pop!");
      buf = NULL;
      buflen = 0;
      bufidx = 0;
      break;
    }
  }
}


// Convert a link status to a character to show on the LCD
char link_stat_to_char(ser_link_status_t stat)
{
  switch(stat) {
    case LS_ZERO: return '?';
    case LS_FAILED: return 'f';
    case LS_TIMEOUT: return 't';
    case LS_UNEXPECTED: return 'u';
    case LS_AUTOSEND: return TICK_CHAR;
    case LS_OK: return TICK_CHAR;
    case LS_LEGACY: return LEGACY_CHAR;
    default: return 'q';
  }
}


// Check to see if any input has received a new valid packet, starting with current_port.
// Return the port number of the next input with a new packet.
int check_inputs(int current_port)
{
  int port;

  port = current_port;
  for(int ii=1; ii <= N_INPUTS; ii++) {
    if(port < N_INPUTS) {
      port++;
    } else {
      port = 1;
    }
    if(serial_input[port].check_new_data()) {
      return port;
    }
  }
  return 0;
}


void update_LCD()
{
  static char ser_stat[N_INPUTS+1]; // Status characters for the input ports, 0 is for port 1, end in '\0'
  char ser_speed[N_INPUTS+1]; // Speed characters for the input ports, 0 is for port 1, end in '\0', 3 == 38400, 4 = 4800
  char new_stat;
  static int current_port = 0; // Which input we are currently showing recent data from, 0 means none
  static uint32_t last_LCD_update = global_time;
  static uint32_t backlight_off_time = global_time + 10000;
  static int state = 1; // 0 => recent punch info, 1 => link status screen, 2 => speed screen
  int card, code;
  char str[16];

  current_port = check_inputs(current_port);
  if(current_port > 0) {
    // New data on current_port, show it on the LCD
    card = serial_input[current_port].get_last_card();
    code = serial_input[current_port].get_last_code();
    serial_input[current_port].ack_new_data();
    lcd.setCursor(0, 0);
    sprintf(str, "%d: c=%3d", current_port, code);
    lcd.print(str);
    lcd.setCursor(0, 1);
    sprintf(str, "%8d", card);
    lcd.print(str);
    digitalWrite(LCD_Backlight_Pin, HIGH);
    last_LCD_update = global_time;
    backlight_off_time = global_time + 10000;
    state = 0;
  }

  if(((state == 0 && (global_time > last_LCD_update + 5000))) ||
     ((state > 0) && (global_time > last_LCD_update + 1000))) {
    // We have not updated the LCD for a while, update and show link status or speed
    // Check the status of the different ports
    for(int ii=1; ii <= N_INPUTS; ii++) {
      new_stat = link_stat_to_char(serial_input[ii].get_link_status());
      if(ser_stat[ii-1] != new_stat) {
        digitalWrite(LCD_Backlight_Pin, HIGH);
        backlight_off_time = global_time + 5000;
      }
      ser_stat[ii-1] = new_stat;

      if(serial_input[ii].get_speed() == LOW_BAUD) {
        ser_speed[ii-1] = '4';
      } else {
        ser_speed[ii-1] = '3';
      }
    }
    ser_speed[N_INPUTS] = '\0';
    ser_stat[N_INPUTS] = '\0'; // NUL terminated string
    lcd.setCursor(0, 0);
    if(state <= 1) {
#if RECEIVE_ONLY
      sprintf(str, "%c%c%c     ", ser_stat[0], ser_stat[1], ser_stat[2]);
#else
      sprintf(str, "%c%c%c%c %c  ", ser_stat[0], ser_stat[1], ser_stat[2], ser_stat[3], ser_stat[4]);
#endif
      state = 2;
    } else {
#if RECEIVE_ONLY
      sprintf(str, "%c%c%c     ", ser_speed[0], ser_speed[1], ser_speed[2]);
#else
      sprintf(str, "%c%c%c%c %c  ", ser_speed[0], ser_speed[1], ser_speed[2], ser_speed[3], ser_speed[4]);
#endif
      state = 1;
    }
    lcd.print(str);
    lcd.setCursor(0, 1);
    sprintf(str, "  %.2f V", read_batt());
    lcd.print(str);
    last_LCD_update = global_time;
  }

  if(global_time > backlight_off_time) {
    digitalWrite(LCD_Backlight_Pin, LOW);
  }
}


void loop() 
{
  powerbank_keepalive();
  receive_from_inputs();
  transmit();
  update_LCD();
}
