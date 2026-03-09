/*
 *   sudo chmod a+rw /dev/ttyACM0
 */

/*

python color to display
16-bit 5-6-5 Color to draw text with

def scale(value, bits):
 # def scale_value(value, in_min, in_max, out_min, out_max):
 in_min = 0;
 in_max = 2**8 - 1
 out_min = 0;
 out_max = (2**bits) - 1
 if in_max == in_min:
   raise ValueError("Input min and max cannot be equal")
 return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min)

def rgb_to_hub75(r,g,b):
   rgbhub75 = int(scale(r,5)) << 11
   rgbhub75 |= int(scale(b,6)) << 5
   rgbhub75 |= int(scale(g,5))
   return rgbhub75
*/

/*
SPEED LUT
# 5 steps
for i in range(64):
  print(f'v = {400-(5*i)} : ticks = {int(3.6*150*1000/64/((400-(5*i))/24))} : ({3.6*150*1000/64/(int(3.6*150*1000/64/((400-(5*i))/24)))*24})')

for i in range(64):
  print(f'{int(3.6*150*1000/64/((400-(5*i))/24))},')

# 10 steps
for i in range(32):
  print(f'v = {400-(10*i)} : ticks = {int(3.6*150*1000/64/((400-(10*i))/24))} : ({3.6*150*1000/64/(int(3.6*150*1000/64/((400-(10*i))/24)))*24})')

idx = 0
for i in range(8):
  str = ''
  for j in range(4):
    str += f'{int(3.6*150*1000/64/((400-(10*idx))/24))},\t'
    idx += 1
  print(str)

*/

#include <Arduino.h>
#include <Preferences.h>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include "RotaryEncoder.h"
#include "images.h"

#define UART_RX_START_COUNT 2U
#define UART_RX_PAYLOAD_SIZE 26U
#define UART_RX_FRAME_SIZE (UART_RX_START_COUNT + UART_RX_PAYLOAD_SIZE)

#define UART_HEADER_1 (0xAA)
#define UART_HEADER_2 (0x55)
#define UART_TX_START_COUNT 2U
#define UART_TX_PAYLOAD_SIZE 4U
#define UART_TX_FRAME_SIZE (UART_TX_START_COUNT + UART_TX_PAYLOAD_SIZE)

#define TICKS_SCALE_FACTOR 8.0f
#define SPEED_CONVERSION_KMH 3.6f
#define TRACK_LENGTH_CONSTANT 150000.0f
#define SCALE_24 24.0f
#define SCALE_32 32.0f

#define ENCODER_BUTTON 5

typedef enum
{
  PARSER_WAIT_HEADER1,
  PARSER_WAIT_HEADER2,
  PARSER_PAYLOAD,
  PARSER_CRC_HIGH,
  PARSER_CRC_LOW
} USART_BASE_PARSER_STATE;

typedef enum
{
  RACE_STATE_STARTUP,
  RACE_STATE_RED_FLAG,
  RACE_STATE_YELLOW_FLAG,
  RACE_STATE_GREEN_FLAG,
  RACE_STATE_START_PROC,
  RACE_STATE_JUMP_START,
  RACE_STATE_OPEN
} RACE_STATE;

typedef enum
{
  RACE_ACTIVE,
  RACE_PAUSE,
  RACE_STARTUP
} HUB75_RACE_STATE;

typedef struct
{
  uint32_t start_sequence;
  uint8_t race_state;
  uint8_t start_lights;
  uint8_t jump_start_id;
  uint8_t laps;
  uint32_t track_ticks;
  uint16_t speed_id0_ticks;
  uint16_t speed_id1_ticks;
  uint16_t speed_id2_ticks;
  uint16_t speed_id3_ticks;
  uint16_t speed_id4_ticks;
  uint16_t speed_id5_ticks;
} USART_DISPLAY_DATA;

typedef enum
{
  HUB75_STARTUP_GULF,
  HUB75_SPEED_PAGE_1,
  HUB75_SPEED_PAGE_2,
  HUB75_SENSITIVITY_ADJUSTMENT,
  HUB75_FLL,
  HUB75_SCALE_ON,
  HUB75_SCALE_CHANGE,
  HUB75_FASTER_THEN,
  HUB75_FLASH_OFF,
  HUB75_CHEQUERED,
  HUB75_CHEQUERED_2,
  HUB75_TOP_SPEED,
  HUB75_AVERAGE_SPEED,
  HUB75_CARRERA,
  HUB75_GREEN_FLAG,
  HUB75_GREEN_FLAG_2,
  HUB75_JUMP_START,
  HUB75_MENU
} HUB75_STATE;

typedef enum
{
  HUB75_STATE_1,
  HUB75_STATE_2,
  HUB75_STATE_3
} HUB75_STATE_PAGE_ENUM;

typedef struct
{
  HUB75_STATE_PAGE_ENUM page;
  uint32_t time;
} HUB75_STATE_PAGE;

typedef struct
{
  HUB75_STATE state;
  HUB75_STATE state_previous;
  uint32_t activate_millis;
  uint32_t duration;
  uint8_t refresh;
  HUB75_RACE_STATE race_state;
} HUB75_STATE_HANDLE;

typedef struct
{
  uint8_t id;
  uint8_t active;
  uint16_t speed_in_ticks;
  uint16_t speed_in_ticks_last;
  unsigned long refreshed;
  float speed_in_ms;
  float speed_in_kmh;
  float speed_in_ms24;
  uint16_t speed_in_kmh24;
  float speed_in_ms32;
  uint16_t speed_in_kmh32;
  uint16_t speed_in_ticks_top;
  float speed_in_ms_top;
  float speed_in_kmh_top;
  float speed_in_ms24_top;
  uint16_t speed_in_kmh24_top;
  float speed_in_ms32_top;
  uint16_t speed_in_kmh32_top;
} DRIVER_DATA;

typedef struct
{
  uint16_t ticks;
  float speed_kmh;
  float speed_kmh24;
  float speed_kmh32;
  uint8_t refreshed;
  uint16_t show_time;
} SPEED_TRAP_SENSITIVITY;

typedef struct
{
  uint8_t scaled;
  uint8_t scaled_refreshed;
  uint16_t scaled_show_time;
  uint8_t scale;
  uint8_t scale_refreshed;
  uint16_t scale_show_time;
} INPUTS_HUB75;

typedef struct
{
  uint8_t on;
  uint8_t faster_then;
} SPEED_TRAP_SETTINGS;

typedef struct
{
  uint8_t scale;       // 0: real / 1: 1/24 / 2: 1/32
  uint8_t sensitivity; // 0 - 31 (max size speedtrap_speed_lut - 1)
  uint8_t flash_mode;  // 0: ft / 1: st
} GLOBAL_SETTINGS;

typedef enum
{
  UI_IDLE,
  UI_MENU,
  UI_SUBMENU
} UI_STATE;

typedef enum
{
  MENU_SCALE = 0,
  MENU_SENSITIVITY,
  MENU_FLASH_MODE,
  MENU_COUNT
} MENU_ITEM;

typedef struct
{
  UI_STATE state;
  MENU_ITEM selected_item;
  uint32_t last_action;
} UI_CONTEXT;

UI_CONTEXT ui = {
    .state = UI_IDLE,
    .selected_item = MENU_SCALE,
    .last_action = 0};

// Extremly sensitve lut
// Direct comparison with ticks in stm32
// To recalculate use python code above and
// alter max in ui menu state machine.
static uint16_t speedtrap_speed_lut[32] = {
    506, 519, 532, 547,
    562, 578, 595, 613,
    632, 653, 675, 698,
    723, 750, 778, 810,
    843, 880, 920, 964,
    1012, 1065, 1125, 1191,
    1265, 1350, 1446, 1557,
    1687, 1840, 2024, 2250};

/*
ID 0: RED
ID 1: YELLOW
ID 0: GREEN
ID 0: BLUE
ID 0: GREY / PURPLE
ID 0: BLACK / WHITE
*/
const uint16_t color_driver_lut[] = {
    0xF800, 0xF81F, 0x001F,
    0x07E0, 0xFFE0, 0xFFFF};

Preferences global_settings_preferences;
GLOBAL_SETTINGS global_settings = {0};
GLOBAL_SETTINGS global_settings_t = {0};

HardwareSerial &uart = Serial1;
uint8_t rx_payload[UART_RX_PAYLOAD_SIZE];
uint8_t rx_frame[UART_RX_FRAME_SIZE];
USART_DISPLAY_DATA uart_data = {0};
USART_DISPLAY_DATA uart_data_t = {0};
uint32_t uart_tx_timer = 0;

MatrixPanel_I2S_DMA *dma_display = nullptr;

DRIVER_DATA driver[6] = {0};
SPEED_TRAP_SENSITIVITY sensitiviy = {0};
INPUTS_HUB75 inputs = {0};
SPEED_TRAP_SETTINGS st_settings = {0};
HUB75_STATE_HANDLE hub75_handle;
HUB75_STATE_PAGE hub75_state_page;

float top_speed = 0.0f;
float average_speed = 0.0f;
uint32_t average_speed_count = 0;

uint8_t hub75_chequered_state = 0;
uint32_t hub75_chequered_millis = 0;

uint32_t millis_t = 0;
uint32_t paging_timeout = 3000;
uint32_t page = 0;

uint32_t msc_active = 0;
uint32_t msc_finished = 0;
uint32_t msc_millis = 0;
int16_t msc_x_pos = 0;

RotaryEncoder encoder(SCL, SDA, RotaryEncoder::LatchMode::FOUR3);
long encoder_position = 0;
int8_t encoder_delta = 0;
uint8_t encoder_button_pressed = 0;
uint8_t encoder_button = 0;
uint8_t encoder_button_flag = 0;
uint32_t encoder_button_debounce = 0;
uint32_t encoder_debug_serial_counter = 0;

static inline uint16_t crc16_ccitt_init(void)
{
  return 0xFFFF;
}

static inline uint16_t crc16_ccitt_update(uint8_t byte, uint16_t crc)
{
  int i;
  int xor_flag;

  /* For each bit in the data byte, starting from the leftmost bit */
  for (i = 7; i >= 0; i--)
  {
    /* If leftmost bit of the CRC is 1, we will XOR with
     * the polynomial later */
    xor_flag = crc & 0x8000;

    /* Shift the CRC, and append the next bit of the
     * message to the rightmost side of the CRC */
    crc <<= 1;
    crc |= (byte & (1 << i)) ? 1 : 0;

    /* Perform the XOR with the polynomial */
    if (xor_flag)
      crc ^= 0x1021;
  }

  return crc;
}

static inline uint16_t crc16_ccitt_finalize(uint16_t crc)
{
  int i;

  /* Augment 16 zero-bits */
  for (i = 0; i < 2; i++)
  {
    crc = crc16_ccitt_update(0, crc);
  }

  return crc;
}

uint16_t crc_calculate_algorithm(const void *data, const uint8_t len)
{
  uint8_t *b = (uint8_t *)data;
  uint8_t i;
  uint16_t crc;

  crc = crc16_ccitt_init();

  /* Update the CRC using the data */
  for (i = 0; i < len; i++)
  {
    crc = crc16_ccitt_update(b[i], crc);
  }

  crc = crc16_ccitt_finalize(crc);

  return crc;
}

void global_settings_load(void)
{
  // open
  global_settings_preferences.begin("GLOBAL", true);

  // if first run, init preferences
  bool first_init = global_settings_preferences.isKey("first_init");
  if (!first_init)
  {
    global_settings_preferences.end();
    global_settings_preferences.begin("GLOBAL", false);

    global_settings_preferences.putBool("first_init", true);
    global_settings_preferences.putUChar("turning_on_counter", 1U);
    global_settings_preferences.putUChar("scale", 1U);
    global_settings_preferences.putUChar("sensitivity", 1U);
    global_settings_preferences.putUChar("flash_mode", 1U);

    global_settings_preferences.end();
    global_settings_preferences.begin("GLOBAL", true);
  }

  // read preferences
  global_settings.scale = global_settings_preferences.getUChar("scale", 1U);
  global_settings.sensitivity = global_settings_preferences.getUChar("sensitivity", 1U);
  global_settings.flash_mode = global_settings_preferences.getUChar("flash_mode", 1U);

  // close
  global_settings_preferences.end();
}

void global_settings_update_sensitivity(void)
{
  global_settings_preferences.begin("GLOBAL", false);
  global_settings_preferences.putUChar("sensitivity", global_settings.sensitivity);
  global_settings_preferences.end();
}

void global_settings_update_scale(void)
{
  global_settings_preferences.begin("GLOBAL", false);
  global_settings_preferences.putUChar("scale", global_settings.scale);
  global_settings_preferences.end();
}

void global_settings_update_flash_mode(void)
{
  global_settings_preferences.begin("GLOBAL", false);
  global_settings_preferences.putUChar("flash_mode", global_settings.flash_mode);
  global_settings_preferences.end();
}

void hub75_next_state(HUB75_STATE next, uint32_t duration)
{
  hub75_handle.state = next;
  hub75_handle.duration = duration;
  hub75_handle.refresh = 1;
}

void ui_state_machine(void)
{

  uint8_t encoder_rotary_flag = 0; // read encoder
  encoder_delta = 0;
  if (encoder.getPosition() != encoder_position)
  {
    encoder_position = encoder.getPosition();
    encoder_rotary_flag = 1;
    encoder_delta = (int8_t)encoder.getDirection();

    Serial.print("Encoder Position: ");
    Serial.print((uint32_t)encoder_position);
    Serial.print(" Encoder Delta: ");
    Serial.println(encoder_delta);
  }

  encoder_button_pressed = encoder_button > encoder_button_flag ? 1 : 0;
  encoder_button_flag = encoder_button;
  if (encoder_button_pressed)
    Serial.println(" Encoder Button Pressed");

  // if (encoder_rotary_flag || encoder_button) // remain in menu and reset counter
  //   hub75_next_state(HUB75_MENU, 10000);

  if ((ui.state != UI_IDLE) && (hub75_handle.state != HUB75_MENU)) // go back to default state
  {
    ui.state = UI_IDLE;
  }

  switch (ui.state)
  {
  case UI_IDLE:
    if (encoder_button_pressed)
    {
      hub75_next_state(HUB75_MENU, 10000);
      ui.state = UI_MENU;
      ui.selected_item = MENU_SCALE;

      global_settings_t.flash_mode = global_settings.flash_mode;
      global_settings_t.scale = global_settings.scale;
      global_settings_t.sensitivity = global_settings.sensitivity;
      sensitiviy.ticks = global_settings_t.sensitivity;
      uint32_t ticks_t = (uint32_t)speedtrap_speed_lut[sensitiviy.ticks];
      if (ticks_t < 32)
      {
        sensitiviy.speed_kmh = SPEED_CONVERSION_KMH * TRACK_LENGTH_CONSTANT / ((float)(((uint32_t)ticks_t) << 6));
        sensitiviy.speed_kmh24 = sensitiviy.speed_kmh * SCALE_24;
        sensitiviy.speed_kmh32 = sensitiviy.speed_kmh * SCALE_32;
      }
    }
    break;
  case UI_MENU:
    if (encoder_delta != 0)
    {
      hub75_next_state(HUB75_MENU, 10000);
      int new_item = (int)ui.selected_item + encoder_delta;

      if (new_item < 0)
        new_item = MENU_COUNT - 1;
      if (new_item >= MENU_COUNT)
        new_item = 0;

      ui.selected_item = (MENU_ITEM)new_item;
      hub75_handle.refresh = 1;
    }

    if (encoder_button_pressed)
    {
      hub75_next_state(HUB75_MENU, 10000);
      ui.state = UI_SUBMENU;
      hub75_handle.refresh = 1;
    }

    break;

  case UI_SUBMENU:

    if (encoder_delta != 0)
    {
      hub75_next_state(HUB75_MENU, 10000);
      switch (ui.selected_item)
      {
      case MENU_SCALE:
        if ((encoder_delta > 0) && (2 > global_settings_t.scale))
          global_settings_t.scale++;
        if ((encoder_delta < 0) && (0 < global_settings_t.scale))
          global_settings_t.scale--;
        break;

      case MENU_SENSITIVITY:
        if ((encoder_delta > 0) && (31 > global_settings_t.sensitivity))
          global_settings_t.sensitivity++;
        else if ((encoder_delta < 0) && (0 < global_settings_t.sensitivity))
          global_settings_t.sensitivity--;

        if (encoder_delta != 0)
        {
          sensitiviy.ticks = global_settings_t.sensitivity;
          uint32_t ticks_t = (uint32_t)speedtrap_speed_lut[sensitiviy.ticks];
          sensitiviy.speed_kmh = SPEED_CONVERSION_KMH * TRACK_LENGTH_CONSTANT / ((float)(((uint32_t)ticks_t) << 6));
          sensitiviy.speed_kmh24 = sensitiviy.speed_kmh * SCALE_24;
          sensitiviy.speed_kmh32 = sensitiviy.speed_kmh * SCALE_32;
        }
        break;

      case MENU_FLASH_MODE:
        if (encoder_rotary_flag)
          global_settings_t.flash_mode ^= 1;
        break;

      default:
        break;
      }

      hub75_handle.refresh = 1;
    }

    if (encoder_button_pressed)
    {
      hub75_next_state(HUB75_MENU, 10000);
      switch (ui.selected_item)
      {
      case MENU_SCALE:
        if (global_settings.scale != global_settings_t.scale)
        {
          global_settings.scale = global_settings_t.scale;
          global_settings_update_scale();
        }
        break;
      case MENU_SENSITIVITY:
        if (global_settings.sensitivity != global_settings_t.sensitivity)
        {
          global_settings.sensitivity = global_settings_t.sensitivity;
          global_settings_update_sensitivity();
        }
        break;
      case MENU_FLASH_MODE:
        if (global_settings.flash_mode != global_settings_t.flash_mode)
        {
          global_settings.flash_mode = global_settings_t.flash_mode;
          global_settings_update_flash_mode();
        }
        break;
      }
      ui.state = UI_MENU; // go back
      hub75_handle.refresh = 1;
    }

    break;

  default:
    ui.state = UI_IDLE;
    break;
  }
}

void msc_letters();

void update_inputs(void)
{
  uint8_t encoder_button_t = !digitalRead(ENCODER_BUTTON); // read button
  if (encoder_button_t)
  {
    if (encoder_button_debounce < 10)
      encoder_button_debounce++;
  }
  else
  {
    if (encoder_button_debounce > 0)
      encoder_button_debounce--;
  }

  if (encoder_button_debounce)
    encoder_button = 1;
  else
    encoder_button = 0;
}

bool uart_receive_frame(void)
{
  static USART_BASE_PARSER_STATE state = PARSER_WAIT_HEADER1;
  static uint8_t payload_index = 0;
  static uint8_t frame_index = 0;

  static uint8_t crc_high = 0x00;
  static uint8_t crc_low = 0x00;

  while (uart.available() > 0)
  {

    uint8_t byte_in = uart.read();
    // Serial.println(byte_in);

    switch (state)
    {
    // Waiting for 4 consecutive 0xAA bytes
    case PARSER_WAIT_HEADER1:
      frame_index = 0;
      if (UART_HEADER_1 == byte_in)
      {
        rx_frame[frame_index++] = UART_HEADER_1;
        state = PARSER_WAIT_HEADER2;
      }
      else
      {
        state = PARSER_WAIT_HEADER1;
      }
      break;
    case PARSER_WAIT_HEADER2:
      if (UART_HEADER_2 == byte_in)
      {
        rx_frame[frame_index++] = UART_HEADER_2;
        state = PARSER_PAYLOAD;
      }
      else
      {
        state = PARSER_WAIT_HEADER1;
      }
      break;
    case PARSER_PAYLOAD: // Receiving payload
      rx_frame[frame_index++] = byte_in;
      if (frame_index >= UART_RX_FRAME_SIZE)
        state = PARSER_CRC_HIGH;
      break;
    case PARSER_CRC_HIGH:
      crc_high = byte_in;
      state = PARSER_CRC_LOW;
      break;
    case PARSER_CRC_LOW:
      state = PARSER_WAIT_HEADER1;

      crc_low = byte_in;
      uint16_t received_crc = ((uint16_t)crc_high << 8) | crc_low;
      uint16_t calculated_crc = crc_calculate_algorithm(rx_frame, UART_RX_FRAME_SIZE);

      if (received_crc == calculated_crc)
      {
        // Valid frame
        for (uint8_t idx = 0; idx < UART_RX_PAYLOAD_SIZE; idx++)
          rx_payload[idx] = rx_frame[2 + idx];
        return true;
      }
      break;
    }
  }

  return false;
}

void uart_transmit_frame(void)
{
  // prepare frame
  uint8_t uart_tx_frame[UART_TX_FRAME_SIZE] = {0};
  uart_tx_frame[0] = UART_HEADER_1;
  uart_tx_frame[1] = UART_HEADER_2;
  uart_tx_frame[2] = 0x17;
  uart_tx_frame[3] = global_settings.flash_mode;
  uint16_t speed_t = speedtrap_speed_lut[global_settings.sensitivity];
  uart_tx_frame[4] = (uint8_t)(speed_t >> 8); // high byte
  uart_tx_frame[5] = (uint8_t)(speed_t & 0x00FF); // low byte

  // calculate crc
  uint16_t crc = crc_calculate_algorithm(uart_tx_frame, UART_TX_FRAME_SIZE);
  uint8_t crc_high = (uint8_t)(crc >> 8);
  uint8_t crc_low = (uint8_t)(crc & 0x00FF);

  // transmit
  for (uint8_t frame_idx = 0; frame_idx < UART_TX_FRAME_SIZE; frame_idx++)
    uart.write(uart_tx_frame[frame_idx]);
  uart.write(crc_high);
  uart.write(crc_low);
}

void compute_scaled_speeds(DRIVER_DATA *drv)
{
  drv->speed_in_ms24 = drv->speed_in_ms * SCALE_24;
  drv->speed_in_ms32 = drv->speed_in_ms * SCALE_32;
  drv->speed_in_kmh = drv->speed_in_ms * SPEED_CONVERSION_KMH;
  drv->speed_in_kmh24 = (uint16_t)(drv->speed_in_kmh * SCALE_24);
  drv->speed_in_kmh32 = (uint16_t)(drv->speed_in_kmh * SCALE_32);
}

void copy_scaled_speeds_highscore(DRIVER_DATA *drv)
{
  drv->speed_in_ms_top = drv->speed_in_ms;
  drv->speed_in_ms24_top = drv->speed_in_ms24;
  drv->speed_in_ms32_top = drv->speed_in_ms32;
  drv->speed_in_kmh_top = drv->speed_in_kmh;
  drv->speed_in_kmh24_top = drv->speed_in_kmh24;
  drv->speed_in_kmh32_top = drv->speed_in_kmh32;
}

void reset_drivers(void)
{
  for (int i = 0; i < 6; i++)
  {
    DRIVER_DATA *drv = &driver[i];
    drv->active = 0;
    drv->refreshed = millis();
    drv->speed_in_ticks_last = drv->speed_in_ticks;

    drv->speed_in_ms = 0.0f;
    compute_scaled_speeds(drv);

    drv->speed_in_ticks_top = 0;
    copy_scaled_speeds_highscore(drv);
  }
}

void reset_switch_to_green(void)
{
  if ((hub75_handle.state != HUB75_GREEN_FLAG) && (hub75_handle.state != HUB75_GREEN_FLAG_2))
    hub75_next_state(HUB75_GREEN_FLAG, 20000);
}

void reset(void)
{
  uint32_t idle = 0;
  for (int i = 0; i < 6; i++)
    if (driver[i].active)
      idle++;

  if (idle)
    reset_drivers();

  reset_switch_to_green();
}

void update_driver_data(void)
{
  for (int i = 0; i < 6; i++)
  {
    DRIVER_DATA *drv = &driver[i];
    if (drv->speed_in_ticks != drv->speed_in_ticks_last)
    {
      drv->speed_in_ticks_last = drv->speed_in_ticks;
      drv->active = 1;
      drv->refreshed = millis();

      if (drv->speed_in_ticks > 0)
      {
        float ticks_scaled = (float)drv->speed_in_ticks * TICKS_SCALE_FACTOR;
        drv->speed_in_ms = (TRACK_LENGTH_CONSTANT) / ticks_scaled;
      }
      else
      {
        drv->speed_in_ms = 0.0f;
      }
      compute_scaled_speeds(drv);

      if ((drv->speed_in_ticks_top == 0) || (drv->speed_in_ticks_top > drv->speed_in_ticks))
      {
        drv->speed_in_ticks_top = drv->speed_in_ticks;
        copy_scaled_speeds_highscore(drv);
      }

      if (top_speed < drv->speed_in_kmh)
        top_speed = drv->speed_in_kmh;
      average_speed_count++;
      average_speed += (drv->speed_in_kmh - average_speed) / (float)average_speed_count;

      if (hub75_handle.race_state == RACE_PAUSE)
      {
        hub75_handle.race_state = RACE_ACTIVE;
        hub75_next_state(HUB75_SPEED_PAGE_1, 2000);
      }
    }
  }
}

void uart_parse(void)
{
  uart_data.race_state = rx_payload[0];
  uart_data.start_lights = rx_payload[1];
  uart_data.jump_start_id = rx_payload[2];
  uart_data.laps = rx_payload[3];
  uart_data.track_ticks = (0x000000FF & rx_payload[4]) << 24;
  uart_data.track_ticks |= (0x000000FF & rx_payload[5]) << 16;
  uart_data.track_ticks |= (0x000000FF & rx_payload[6]) << 8;
  uart_data.track_ticks |= (0x000000FF & rx_payload[7]);
  uart_data.speed_id0_ticks = (0x00FF & rx_payload[8]) << 8;
  uart_data.speed_id0_ticks |= (0x00FF & rx_payload[9]);
  uart_data.speed_id1_ticks = (0x00FF & rx_payload[10]) << 8;
  uart_data.speed_id1_ticks |= (0x00FF & rx_payload[11]);
  uart_data.speed_id2_ticks = (0x00FF & rx_payload[12]) << 8;
  uart_data.speed_id2_ticks |= (0x00FF & rx_payload[13]);
  uart_data.speed_id3_ticks = (0x00FF & rx_payload[14]) << 8;
  uart_data.speed_id3_ticks |= (0x00FF & rx_payload[15]);
  uart_data.speed_id4_ticks = (0x00FF & rx_payload[16]) << 8;
  uart_data.speed_id4_ticks |= (0x00FF & rx_payload[17]);
  uart_data.speed_id5_ticks = (0x00FF & rx_payload[18]) << 8;
  uart_data.speed_id5_ticks |= (0x00FF & rx_payload[19]); 

  // 20 til 25 reserved for specific car information

  driver[0].speed_in_ticks = uart_data.speed_id0_ticks;
  driver[1].speed_in_ticks = uart_data.speed_id1_ticks;
  driver[2].speed_in_ticks = uart_data.speed_id2_ticks;
  driver[3].speed_in_ticks = uart_data.speed_id3_ticks;
  driver[4].speed_in_ticks = uart_data.speed_id4_ticks;
  driver[5].speed_in_ticks = uart_data.speed_id5_ticks;
}

void uart_process(void)
{
  if (0 == uart_data.start_lights)
  {
    update_driver_data();
  }

  if (uart_data.race_state == RACE_STATE_START_PROC)
  {
    reset();
  }
}

void show_driver(DRIVER_DATA *drv, uint8_t y_value)
{
  dma_display->setCursor(0, y_value);
  dma_display->setTextColor(color_driver_lut[drv->id]);
  dma_display->print("D");
  dma_display->print(drv->id + 1);
  dma_display->print(" ");
  dma_display->setCursor(20, y_value);
  if (drv->active)
  {
    switch (global_settings.scale)
    {
    case 0:
      dma_display->print((uint8_t)drv->speed_in_kmh);
      break;
    case 1:
      dma_display->print(drv->speed_in_kmh24);
      break;
    case 2:
      dma_display->print(drv->speed_in_kmh32);
      break;
    }
  }
  else
  {
    dma_display->print("---");
  }
  dma_display->setCursor(40, y_value);
  dma_display->print("km/h");
}

void show_hub75_display(void)
{
  /*switch (hub75_handle.state)
  {
    case HUB75_SENSITIVITY_ADJUSTMENT:
      showSensitivityAdjustment();
      break;

    case HUB75_SCALE_CHANGE:
      showScaleChange();
      break;

    case HUB75_SPEED_PAGE_1:
      showSpeedPage(0);
      break;

    case HUB75_SPEED_PAGE_2:
      showSpeedPage(1);
      break;

  }*/
}

void show_hub75(void)
{
  // a digit is WxH 6x8 (5x7)

  uint16_t y_value = 4;
  if (HUB75_MENU == hub75_handle.state)
  {
    dma_display->clearScreen();
    dma_display->setTextColor(0x003F);

    switch (ui.state)
    {
    case UI_MENU:
      dma_display->setCursor(4, 0);
      dma_display->print("FLASH MODE");
      dma_display->setCursor(4, 10);
      dma_display->print("SCALE FACT");
      dma_display->setCursor(4, 20);
      dma_display->print("SENSITIVE");
      switch (ui.selected_item)
      {
      case MENU_FLASH_MODE:
        dma_display->fillCircle(1, 0 + 5, 1, 0xffff);
        break;
      case MENU_SCALE:
        dma_display->fillCircle(1, 10 + 5, 1, 0xffff);
        break;
      case MENU_SENSITIVITY:
        dma_display->fillCircle(1, 20 + 5, 1, 0xffff);
        break;
      default:
        break;
      }
      break;
    case UI_SUBMENU:
      switch (ui.selected_item)
      {
      case MENU_FLASH_MODE:
        dma_display->setCursor(4, 0);
        dma_display->print("FASTER");
        dma_display->setCursor(4, 10);
        dma_display->print("SLOWER");
        switch (global_settings_t.flash_mode)
        {
        case 0:
          dma_display->fillCircle(1, 0 + 5, 1, 0xffff);
          break;
        case 1:
          dma_display->fillCircle(1, 10 + 5, 1, 0xffff);
          break;
        default:
          break;
        }
        break;
      case MENU_SCALE:
        dma_display->setCursor(4, 0);
        dma_display->print("REAL");
        dma_display->setCursor(4, 10);
        dma_display->print("1/24");
        dma_display->setCursor(4, 20);
        dma_display->print("1/32");
        switch (global_settings_t.scale)
        {
        case 0:
          dma_display->fillCircle(1, 0 + 5, 1, 0xffff);
          break;
        case 1:
          dma_display->fillCircle(1, 10 + 5, 1, 0xffff);
          break;
        case 2:
          dma_display->fillCircle(1, 20 + 5, 1, 0xffff);
          break;
        default:
          break;
        }
        break;
      case MENU_SENSITIVITY:
        dma_display->setCursor(0, 0);
        dma_display->print("REAL ");
        dma_display->setCursor(38, 0);
        dma_display->print(sensitiviy.speed_kmh, 1);

        dma_display->setCursor(0, 11);
        dma_display->print("24 ");
        dma_display->setCursor(20, 11);
        dma_display->print((uint16_t)sensitiviy.speed_kmh24);
        dma_display->setCursor(40, 11);
        dma_display->print("km/h");

        dma_display->setCursor(0, 22);
        dma_display->print("32 ");
        dma_display->setCursor(20, 22);
        dma_display->print((uint16_t)sensitiviy.speed_kmh32);
        dma_display->setCursor(40, 22);
        dma_display->print("km/h");
        break;
      default:
        break;
      }
      break;
    default:
      dma_display->setCursor(4, 0);
      dma_display->print("UI STATE");
      dma_display->setCursor(4, 12);
      dma_display->print("ISSUE");
      break;
    }
  }
  else if (HUB75_SPEED_PAGE_1 == hub75_handle.state)
  {
    dma_display->clearScreen();

    dma_display->drawCircle(56, 1, 1, 0xffff);
    dma_display->drawPixel(56, 1, 0xffff);
    dma_display->drawLine(58, 2, 60, 0, 0x79ef);
    dma_display->drawCircle(62, 1, 1, 0x79ef);

    uint8_t driver_counter = 0;
    for (uint8_t idx = 0; idx < 6; idx++)
    {
      if (driver[idx].active)
      {
        driver_counter++;
        if ((driver_counter > 0) && (driver_counter < 4))
          show_driver(&driver[idx], y_value);
        if ((driver_counter > 0) && (3 > driver_counter))
        {
          dma_display->drawFastHLine(0, y_value + 8, 64, 40, 40, 40);
          y_value += 10;
        }
      }
    }
  }
  else if (HUB75_SPEED_PAGE_2 == hub75_handle.state)
  {
    dma_display->clearScreen();

    dma_display->drawCircle(56, 1, 1, 0x79ef);
    dma_display->drawLine(58, 2, 60, 0, 0x79ef);
    dma_display->drawCircle(62, 1, 1, 0xffff);
    dma_display->drawPixel(62, 1, 0xffff);

    uint8_t driver_counter = 0;
    for (uint8_t idx = 0; idx < 6; idx++)
    {
      if (driver[idx].active)
      {
        driver_counter++;
        if (driver_counter > 3)
        {
          show_driver(&driver[idx], y_value);
          if ((driver_counter > 3) && (6 > driver_counter))
          {
            dma_display->drawFastHLine(0, y_value + 8, 64, 40, 40, 40);
            y_value += 10;
          }
        }
      }
    }
  }
  else if (HUB75_FLL == hub75_handle.state)
  {
    dma_display->clearScreen();
    dma_display->drawBitmap(0, 0, Fast_Lap_Lights_64x32, 64, 32, 0xffff);

    uint16_t x_value_lights = 6;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xf800);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);

    x_value_lights += 10;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xf800);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);

    x_value_lights += 10;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xf800);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);

    x_value_lights += 10;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xF81F);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);

    x_value_lights += 10;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xF81F);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);

    x_value_lights += 10;
    dma_display->fillCircle(x_value_lights, 15, 2, 0xF81F);
    dma_display->drawCircle(x_value_lights, 15, 3, 0xffff);
  }
  else if (HUB75_STARTUP_GULF == hub75_handle.state)
  {

    dma_display->clearScreen();
    dma_display->drawRGBBitmap(0, 0, gulf_logo, 64, 32);
  }
  else if (HUB75_CARRERA == hub75_handle.state)
  {
    dma_display->clearScreen();
    dma_display->drawRGBBitmap(0, 0, logo_carrera, 64, 32);
  }
  else if (HUB75_CHEQUERED == hub75_handle.state)
  {
    dma_display->clearScreen();
    for (uint8_t idx = 0; idx < 4; idx++)
    {
      uint8_t multipl = 16 * idx;
      dma_display->fillRect(multipl, 0, 8, 8, 0xffff);
      dma_display->fillRect(multipl, 16, 8, 8, 0xffff);
      dma_display->fillRect(multipl + 8, 8, 8, 8, 0xffff);
      dma_display->fillRect(multipl + 8, 24, 8, 8, 0xffff);
    }
  }
  else if (HUB75_CHEQUERED_2 == hub75_handle.state)
  {
    dma_display->clearScreen();
    for (uint8_t idx = 0; idx < 4; idx++)
    {
      uint8_t multipl = 16 * idx;
      dma_display->fillRect(multipl + 8, 0, 8, 8, 0xffff);
      dma_display->fillRect(multipl + 8, 16, 8, 8, 0xffff);
      dma_display->fillRect(multipl, 8, 8, 8, 0xffff);
      dma_display->fillRect(multipl, 24, 8, 8, 0xffff);
    }
  }
  else if (HUB75_TOP_SPEED == hub75_handle.state)
  {
    dma_display->clearScreen();
    y_value = 0;
    dma_display->setCursor(5, y_value);
    dma_display->setTextColor(0xFFFF);
    dma_display->print("TOP SPEED");
    y_value += 12;
    dma_display->setCursor(0, y_value);
    dma_display->setTextColor(0xf815);
    // 7 px each digit width
    switch (global_settings.scale)
    {
    case 0:
      dma_display->setCursor(18, y_value);
      dma_display->print(top_speed, 2);
      break;
    case 1:
      if (top_speed * SCALE_24 >= 100.0)
        dma_display->setCursor(14, y_value);
      else
        dma_display->setCursor(18, y_value);
      dma_display->print(top_speed * SCALE_24, 1);
      break;
    case 2:
      if (top_speed * SCALE_32 >= 100.0)
        dma_display->setCursor(14, y_value);
      else
        dma_display->setCursor(18, y_value);
      dma_display->print(top_speed * SCALE_32, 1);
      break;
    }
    dma_display->setTextColor(0xFFFF);
    y_value += 12;
    dma_display->setCursor(0, y_value);
    dma_display->print("   km/h   ");
  }
  else if (HUB75_AVERAGE_SPEED == hub75_handle.state)
  {
    dma_display->clearScreen();
    y_value = 0;
    dma_display->setCursor(5, y_value);
    dma_display->setTextColor(0xFFFF);
    dma_display->print("AVG SPEED");
    y_value += 12;
    dma_display->setCursor(0, y_value);
    dma_display->setTextColor(0xf815);
    dma_display->print("    ");
    switch (global_settings.scale)
    {
    case 0:
      dma_display->print(average_speed, 2);
      break;
    case 1:
      dma_display->print(average_speed * SCALE_24, 1);
      break;
    case 2:
      dma_display->print(average_speed * SCALE_32, 1);
      break;
    }
    dma_display->setTextColor(0xFFFF);
    y_value += 12;
    dma_display->setCursor(0, y_value);
    dma_display->print("   km/h   ");
  }
  else if (HUB75_GREEN_FLAG == hub75_handle.state)
  {
    dma_display->fillScreen(color_driver_lut[2]);
  }
  else if (HUB75_GREEN_FLAG_2 == hub75_handle.state)
  {
    dma_display->clearScreen();
  }
}

void update_hub75(uint32_t now)
{

  /*
  duration is a countdown until the next state change.
  It's possible to force a state change by settin duration = 0;
  */
  if (hub75_handle.duration > 0)
    hub75_handle.duration--;

  // if a transition is due, check if we are in a race
  /*
  Pause is if all drivers were refreshed min 120s ago
  */
  uint8_t driver_active = 0;
  uint8_t pause_check = 0;
  uint32_t last_refresh_t = 0;
  for (uint8_t idx = 0; idx < 6; idx++)
  {
    if (driver[idx].active)
      driver_active++;

    if (driver[idx].refreshed > last_refresh_t)
      if (driver[idx].active)
        last_refresh_t = driver[idx].refreshed;
  }
  if ((now - last_refresh_t) > 120000)
    pause_check = 1;

  // if pause detected, swith to pause state
  if ((1 == pause_check) || (driver_active == 0))
  {
    if (hub75_handle.race_state != RACE_PAUSE)
    {
      hub75_handle.race_state = RACE_PAUSE;
      hub75_handle.state = HUB75_CHEQUERED;
      hub75_handle.duration = 2000;
    }
  }

  // catch should not happen case
  // automatically reset to race_active state

  // state machine
  switch (hub75_handle.state)
  {
  case HUB75_SPEED_PAGE_1:
    if (hub75_handle.duration > 0)
      break;               // break if no update
    if (3 < driver_active) // goto page 2 if more then 3 drivers active
      hub75_next_state(HUB75_SPEED_PAGE_2, 2000);
    else
      hub75_next_state(HUB75_SPEED_PAGE_1, 2000);
    break;
  case HUB75_SPEED_PAGE_2:
    if (hub75_handle.duration > 0)
      break; // break if no update
    hub75_next_state(HUB75_SPEED_PAGE_1, 2000);
    break;
  case HUB75_CHEQUERED:
    if (hub75_handle.duration > 0)
    {
      if (now - hub75_chequered_millis > 250)
      {
        hub75_chequered_millis = now;
        hub75_handle.state = HUB75_CHEQUERED_2;
        hub75_handle.refresh = 1;
      }
    }
    else
    {
      hub75_next_state(HUB75_FLL, 5000);
    }
    break;
  case HUB75_CHEQUERED_2:
    if (now - hub75_chequered_millis > 250)
    {
      hub75_chequered_millis = now;
      hub75_handle.state = HUB75_CHEQUERED;
      hub75_handle.refresh = 1;
    }
    break;
  case HUB75_FLL:
    if (hub75_handle.duration > 0)
      break; // break if no update
    hub75_next_state(HUB75_TOP_SPEED, 5000);
    break;
  case HUB75_TOP_SPEED:
    if (hub75_handle.duration > 0)
      break; // break if no update
    hub75_next_state(HUB75_AVERAGE_SPEED, 5000);
    break;
  case HUB75_AVERAGE_SPEED:
    if (hub75_handle.duration > 0)
      break; // break if no update
    hub75_next_state(HUB75_CARRERA, 5000);
    break;
  case HUB75_CARRERA:
    if (hub75_handle.duration > 0)
      break;       // break if no update
    msc_x_pos = 0; // relates to msc
    hub75_next_state(HUB75_FLL, 5000);
    break;
  case HUB75_GREEN_FLAG:
    if (hub75_handle.duration > 0)
    {
      if ((now - hub75_state_page.time) > 250)
      {
        hub75_state_page.time = now;
        hub75_handle.state = HUB75_GREEN_FLAG_2;
        hub75_handle.refresh = 1;
      }
    }
    else
    {
      hub75_next_state(HUB75_FLL, 5000);
    }
    break;
  case HUB75_GREEN_FLAG_2:
    if ((now - hub75_state_page.time) > 250)
    {
      hub75_state_page.time = now;
      hub75_handle.state = HUB75_GREEN_FLAG;
      hub75_handle.refresh = 1;
    }
    break;
  default:
    if (hub75_handle.duration == 0)
      if (hub75_handle.race_state != RACE_PAUSE)
        hub75_next_state(HUB75_SPEED_PAGE_1, 2000);
      else
        hub75_next_state(HUB75_CHEQUERED, 5000);
    break;
  }
}

void setup()
{
  // serial console
  Serial.begin(9600);

  // setup uart
  uart.begin(19200, SERIAL_8N1, RX, TX);

  // read global settings
  global_settings_load();

  // set presets
  sensitiviy.ticks = global_settings.sensitivity;
  uint32_t ticks_t = (uint32_t)speedtrap_speed_lut[sensitiviy.ticks];
  sensitiviy.speed_kmh = SPEED_CONVERSION_KMH * TRACK_LENGTH_CONSTANT / ((float)(((uint32_t)ticks_t) << 6));
  sensitiviy.speed_kmh24 = sensitiviy.speed_kmh * SCALE_24;
  sensitiviy.speed_kmh32 = sensitiviy.speed_kmh * SCALE_32;

  // setup HUB75
  HUB75_I2S_CFG mxconfig(/* width = */ 64, /* height = */ 32, /* chain = */ 1);
  mxconfig.clkphase = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(255);
  dma_display->clearScreen();
  dma_display->setLatBlanking(4);

  hub75_handle.state = HUB75_STARTUP_GULF;
  hub75_handle.state_previous = HUB75_STARTUP_GULF;
  hub75_handle.duration = 2000;
  hub75_handle.activate_millis = 0;
  hub75_handle.refresh = 1;
  hub75_handle.race_state = RACE_PAUSE;

  hub75_state_page.page = HUB75_STATE_1;
  hub75_state_page.time = 0;

  // setup inputs
  pinMode(ENCODER_BUTTON, INPUT);

  for (int i = 0; i < 6; i++)
  {
    driver[i].id = i;
    driver[i].active = 0;
    driver[i].refreshed = 0;
    driver[i].speed_in_ticks = 0;
  }
}

void loop()
{
  uint32_t now = millis();

  // read buttons
  update_inputs();

  // write uart with 2 hz
  if ((now - uart_tx_timer) > 500){
    uart_tx_timer = now;
    uart_transmit_frame();
  }

  // read uart
  if (uart_receive_frame())
  {
    uart_parse();
    uart_process();
  }

  // handle encoder
  encoder.tick();
  ui_state_machine();

  // handle hub75 once a millisecond
  if (now != millis_t)
  {
    millis_t = now;
    update_hub75(now);
  }

  // if refresh is triggred
  if (hub75_handle.refresh)
  {
    hub75_handle.refresh = 0;
    show_hub75();
  }
}
