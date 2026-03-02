#define USE_HEADLESS_INTERFACE
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#define KEYFILE "/dev/input/event0" // Use your keyboard's event file
#define VAL_KEY_UP (0)
#define VAL_KEY_DOWN (1)
#define VAL_KEY_REPEAT (2)

// TODO abstract away all SDL-specific stuff to an SDL-based interface header, 
//      which in turn will be selected in interface.h.
/**********************
 * Rendering
 *********************/
static struct termios tty_attr;
static FILE *file = NULL;

class Input {
  
  // open up the keyboard file in read binary mode so we can accept key presses.
  file = fopen(KEYFILE, "rb"); 
  assert(file);

  // Set terminal settings: 
  struct termios new_tty_attr = tty_attr;
  new_tty_attr.c_lflag &= ~(ICANON | ECHO); // turn off canonic mode and echo

  // Apply new terminal settings
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tty_attr);
};


void processKeys() {
  tcsetattr(STDIN_FILENO, TCSANOW, &tty_attr);
  // Keyboard button-release events (only process this if engine is unpaused)
  // NOTE: You can implement a partial pause with any one of the other keys.
  // Grab each key press event
  struct input_event ev;
  while ( fread((void *)&ev, sizeof(ev), 1, file) != 0 ) {

    if ((ev.type == EV_KEY) && (ev.value != VAL_KEY_REPEAT))
    {
#ifndef NDEBUG
      printf("type: %d, val: %d, code: %d\n", ev.type, ev.value, ev.code); // EV_KEY corresponds to key press events
#endif
      if ( ev.value == VAL_KEY_UP ) {
        switch( ev.code ) {
          case KEY_a_:      _keyState &= ~BTN_PRESSED_a; break;
          case KEY_b_:      _keyState &= ~BTN_PRESSED_b; break;
          case KEY_c_:      _keyState &= ~BTN_PRESSED_c; break;
          case KEY_d_:      _keyState &= ~BTN_PRESSED_d; break;
          case KEY_e_:      _keyState &= ~BTN_PRESSED_e; break;
          case KEY_f_:      _keyState &= ~BTN_PRESSED_f; break;
          case KEY_g_:      _keyState &= ~BTN_PRESSED_g; break;
          case KEY_h_:      _keyState &= ~BTN_PRESSED_h; break;
          case KEY_i_:      _keyState &= ~BTN_PRESSED_i; break;
          case KEY_j_:      _keyState &= ~BTN_PRESSED_j; break;
          case KEY_k_:      _keyState &= ~BTN_PRESSED_k; break;
          case KEY_l_:      _keyState &= ~BTN_PRESSED_l; break;
          case KEY_m_:      _keyState &= ~BTN_PRESSED_m; break;
          case KEY_n_:      _keyState &= ~BTN_PRESSED_n; break;
          case KEY_o_:      _keyState &= ~BTN_PRESSED_o; break;
          case KEY_p_:      _keyState &= ~BTN_PRESSED_p; break;
          case KEY_q_:      _keyState &= ~BTN_PRESSED_q; break;
          case KEY_r_:      _keyState &= ~BTN_PRESSED_r; break;
          case KEY_s_:      _keyState &= ~BTN_PRESSED_s; break;
          case KEY_t_:      _keyState &= ~BTN_PRESSED_t; break;
          case KEY_u_:      _keyState &= ~BTN_PRESSED_u; break;
          case KEY_v_:      _keyState &= ~BTN_PRESSED_v; break;
          case KEY_w_:      _keyState &= ~BTN_PRESSED_w; break;
          case KEY_x_:      _keyState &= ~BTN_PRESSED_x; break;
          case KEY_y_:      _keyState &= ~BTN_PRESSED_y; break;
          case KEY_z_:      _keyState &= ~BTN_PRESSED_z; break;
          case KEY_SPACE_:  _keyState &= ~BTN_PRESSED_SPACE; break;
          case KEY_LSHIFT_: _keyState &= ~BTN_PRESSED_LSHIFT; break;
          case KEY_ESCAPE_: _keyState &= ~BTN_PRESSED_ESCAPE; break;
          default: break;
        }  // switch-case for keys released
      } // if key-up event
        // Keyboard button-press events
      else if ( ev.value == VAL_KEY_DOWN ) {
        switch( ev.code ) {
          case KEY_a_:      _keyState |=  BTN_PRESSED_a; break;
          case KEY_b_:      _keyState |=  BTN_PRESSED_b; break;
          case KEY_c_:      _keyState |=  BTN_PRESSED_c; break;
          case KEY_d_:      _keyState |=  BTN_PRESSED_d; break;
          case KEY_e_:      _keyState |=  BTN_PRESSED_e; break;
          case KEY_f_:      _keyState |=  BTN_PRESSED_f; break;
          case KEY_g_:      _keyState |=  BTN_PRESSED_g; break;
          case KEY_h_:      _keyState |=  BTN_PRESSED_h; break;
          case KEY_i_:      _keyState |=  BTN_PRESSED_i; break;
          case KEY_j_:      _keyState |=  BTN_PRESSED_j; break;
          case KEY_k_:      _keyState |=  BTN_PRESSED_k; break;
          case KEY_l_:      _keyState |=  BTN_PRESSED_l; break;
          case KEY_m_:      _keyState |=  BTN_PRESSED_m; break;
          case KEY_n_:      _keyState |=  BTN_PRESSED_n; break;
          case KEY_o_:      _keyState |=  BTN_PRESSED_o; break;
          case KEY_p_:      _keyState |=  BTN_PRESSED_p; break;
          case KEY_q_:      _keyState |=  BTN_PRESSED_q; break;
          case KEY_r_:      _keyState |=  BTN_PRESSED_r; break;
          case KEY_s_:      _keyState |=  BTN_PRESSED_s; break;
          case KEY_t_:      _keyState |=  BTN_PRESSED_t; break;
          case KEY_u_:      _keyState |=  BTN_PRESSED_u; break;
          case KEY_v_:      _keyState |=  BTN_PRESSED_v; break;
          case KEY_w_:      _keyState |=  BTN_PRESSED_w; break;
          case KEY_x_:      _keyState |=  BTN_PRESSED_x; break;
          case KEY_y_:      _keyState |=  BTN_PRESSED_y; break;
          case KEY_z_:      _keyState |=  BTN_PRESSED_z; break;
          case KEY_SPACE_:  _keyState |=  BTN_PRESSED_SPACE; break;
          case KEY_LSHIFT_: _keyState |=  BTN_PRESSED_LSHIFT; break;
          case KEY_ESCAPE_: _keyState |=  BTN_PRESSED_ESCAPE; break;
        }  // switch-case for keys pressed
      }  // if key-down event
    }
  }
}
