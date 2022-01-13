
#include <math.h>
#include <SPI.h>
#include <U8g2lib.h>     /* LCD */
#include "ClickEncoder.h"
#include "Menu.h"
#include "Thermistor.h"
#include "pin_map.h"
#include "RTClib.h"


bool Running = true;
int units = 0; // Celcius
char *menu_current[20];
uint8_t menu_item_current  = 0;
uint8_t menu_length_current  = 0;
uint8_t menu_redraw_required = 0;
int16_t last, value;                  // store position of encoder

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, DOGLCD_SCK , DOGLCD_MOSI, DOGLCD_CS ); // Initialize graphics controller
ClickEncoder *encoder;                //Create instance of ClickEncoder class
RTC_DS1307 rtc;


// ----------------------------------------------------------------------------


void setup() {
  Serial.begin(115200);
  Serial.println("RAMPS TEST");
  rtc.begin();
  
  //set up Menu
  memcpy(menu_current, menu_setup, sizeof(menu_setup));
  menu_length_current = *(&menu_setup + 1) - menu_setup ;
  menu_redraw_required = 1;     // force initial redraw

  // set up click encoder
  encoder = new ClickEncoder(BTN_EN2, BTN_EN1, BTN_ENC);
   // encoder = new ClickEncoder(D33, D31, D35);
  last = -1;

  // initialize timers
  noInterrupts();           // disable all interrupts

  // encoder read timer
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  OCR2A = round(16000000 / 1024 / 1000);        // compare match register e.g. 16MHz/1024/100Hz
  TCCR2B |= (1 << WGM12);   // CTC mode
  TCCR2B |= (1 << CS12) | (1 << CS10);  // 1024 prescaler
  TIMSK2 |= (1 << OCIE2A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts


  // Menu splash screen
  u8g2.begin(); // Write startup message to LCD
  u8g2.setFontPosTop(); // references position to top of font
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(10, 10, "Dinheiros!!");
  } while ( u8g2.nextPage() );
  delay(500);
}

// ----------------------------------------------------------------------------
// TIMER ISRs
// ----------------------------------------------------------------------------

ISR(TIMER2_COMPA_vect)
{
   encoder->service();
}

// ----------------------------------------------------------------------------






void loop() {
  if (Running) {
    u8g2.firstPage();
    do {
      u8g2.setCursor(25, 35);
      DateTime now = rtc.now();
      u8g2.setFont(u8g2_font_fub17_tf);
      u8g2.print(now.hour());
      u8g2.print(':');
      u8g2.print(now.minute());
      u8g2.print(':');
      u8g2.print(now.second());
    } while ( u8g2.nextPage() );

  } else {    //Draw Menu
    if (menu_redraw_required != 0) {
      u8g2.firstPage();
      do  {
        drawMenu(menu_setup, *(&menu_setup + 1) - menu_setup  );
      } while ( u8g2.nextPage() );
      menu_redraw_required = 0;
    }
  }

  // ClickEncoder
  value += encoder->getValue();
  if (value < 0) {
    value = 0;
  }
  if (value > menu_length_current - 1) {
    value = menu_length_current - 1;
  }
  if (value != last) {
    Serial.print("Encoder Value: ");
    Serial.println(value);
    updateMenu(value);                            // update menu bar
    last = value;
  }


  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b){
      case ClickEncoder::Clicked:
        Serial.println("ClickEncoder::Clicked");
        if (!Running && value == 3 ) {
          menu_redraw_required = 1;
          menuClick(value);
        } 
        break;
      case ClickEncoder::Pressed:
        Serial.println("ClickEncoder::Pressed");
        break;
      case ClickEncoder::DoubleClicked:
        Running = 0;
        Serial.println("ClickEncoder::DoubleClicked");
        break;
    }
  }

}


// ----------------------------------------------------------------------------
// MENU HANDLING ROUTINES
// ----------------------------------------------------------------------------
void drawMenu(const char *menu[], uint8_t menu_len) {
  uint8_t i, h;
  u8g2_uint_t w, d;
  u8g2.setFont(u8g_font_6x13);
  u8g2.setFontRefHeightText();
  h = u8g2.getFontAscent() - u8g2.getFontDescent();
  w = u8g2.getWidth();
  for (  i = 0; i < menu_len; i++ ) {
    d = 10; // menu indent
    if ( i == menu_item_current ) {
      u8g2.drawBox(0, i  * h  , w, h);
      u8g2.setDrawColor(0);
      u8g2.drawStr(d, i * h , menu[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(d, i * h , menu[i]);
    }
  }
}


void updateMenu(int i) {
  menu_item_current  = i;
  menu_redraw_required = 1;
}



void   menuClick( uint8_t _value) {
  Serial.print("menuClick\t");
  Serial.println(_value);
  units = _value;
  Running = true;
}
