// Libraries #########################################################################################

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // version 2.8
#include <IRremote.h> //version 1.1.2
#include <arduino-timer.h>

// Pins ##############################################################################################

int receptor_pin = 6;
int relay_pin = 7 ;
int ldr_pin = A0;

// Remote DEC Codes ##################################################################

int b_MENU   = 25245;
int b_NEXT   = 765;
int b_PREV   = 8925;
int b_OK     = -15811;
int b_BACK   = -28561;
int b_MORE   = -22441;
int b_LESS  = -8161;
 

// Parameters ########################################################################################

LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv irrecv(receptor_pin);
decode_results results;

auto timer_diagnostic = timer_create_default();
auto timer_screen = timer_create_default();

// Variables ####s####################################################################################


int lux_limit = 100;  //Maximum Value in LUX to turn on relay
int lux = 0; //Momentary value in LUX
int R_div = 1000; // Resistance of Voltage Diviser

bool diagonostic_result = true; // Diagonostic Result
String state = "";
bool state_overide = false;
String mode = "";
int menu = 0;
int remote = 0;
int error = 0;



// Functions ##########################################################################################

void screen (int cursor1, String line1, int cursor2, String line2){
  lcd.clear();
  lcd.setCursor(cursor1,0);
  lcd.print(line1);
  lcd.setCursor(cursor2,1);
  lcd.print(line2);
}


void ldr_mesure(){
  int ldr_value = analogRead (ldr_pin); //LDR terminal reading (0-1012) 
  float Vout = float(ldr_value) * (float(5)/float(1023));  // Voltage conversion
  float R_ldr = (R_div * (5 - Vout))/Vout; //Value of LDR Resistance
  lux = 500/(R_ldr/1000); //LUX conversion
}


bool diagonostic(void*) {
    screen (2, "Diagonostic", 0, "");
    delay(600);
    ldr_mesure();
  
  if (lux < 5 && lux > 0){
    diagonostic_result = false;
    error = 1;
    state_overide = true;
    }

  else if (lux < 0){
    diagonostic_result = false;
    error = 2;
    state_overide = true;
  }

  else{
    diagonostic_result = true;
    error = 0;
  }
  Serial.println(diagonostic_result);
  return true;
}

bool screen_main_menu(void*) {
  if (state_overide){
    state = "OFF";    
  }
  else{
    state = "ON";
  }

  
  switch (menu) {
    case 0:
      if (diagonostic_result){
      screen(0, "mode: " + mode, 0, String(lux) + " lux");
      }
      else if (!diagonostic_result){
        switch(error) {
          case 1:
            screen(5, "error 1", 4, "LDR open");
            break;
          case 2:
            screen(5, "error 2", 4, "LDR open");
            break;
        }
      }
      break;
    case 1:
      menu = 2;
      break;
    case 2:
      screen(0, ">state: " + state , 1 , "lim: " + String(lux_limit) + " lux");
      break;
    case 3:
      screen(1, "state: " + state , 0 , ">lim: " + String(lux_limit) + " lux");
      break;
    case 4:
      screen(1, "lim: " + String(lux_limit) + " lux" , 0 , ">diagonostic");
      break;
    case 5 ... 9:
      menu = 4;
      break;

  }
  return true; //Keep the timer active    
}




// Void setup & loop ################################################################################################

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  screen(2, "initializing", 0, "");
  irrecv.enableIRIn();
  pinMode(relay_pin, OUTPUT);

  timer_diagnostic.every(3600000, diagonostic); // Run every hour
  timer_screen.every(800, screen_main_menu);

}

void loop() {
  timer_diagnostic.tick();
  timer_screen.tick();
  ldr_mesure();

  if(state_overide){
    digitalWrite (relay_pin, HIGH);
    mode = "OFF";
  }

  
  if (irrecv.decode(&results)) {
    irrecv.resume();
    remote = results.value;
    
    
    if (remote == b_MENU){
      menu = 2;
      }
    else if (remote == b_BACK && menu > 0){
      menu = 0;
    }
    else if (remote == b_NEXT && menu > 0){
      menu++;
      }
    else if (remote == b_PREV && menu > 0){
      menu--;
    }
    else if (remote == b_OK && menu > 0){
      switch(menu){
        case 2:
          if (state_overide == false){
            state_overide = true;
            digitalWrite (relay_pin, HIGH);
            mode = "OFF";
          }
          else{
            state_overide = false;
          }
        break; 
        case 4:
          timer_diagnostic.in(0,diagonostic);
          menu=0;
          break;
      }      
    }
    else if (remote == b_MORE && menu == 3){
      lux_limit = lux_limit + 5;
    }

    else if (remote == b_LESS && menu == 3){
      lux_limit = lux_limit - 5;
    }
    Serial.println(menu);
    Serial.println(remote);
    irrecv.resume();
  }

  
  else if (diagonostic_result && menu == 0 && !state_overide){
    if (lux < lux_limit){
      digitalWrite (relay_pin, LOW);
      state = "ON";
      mode = "Night";
      
     }
    else{
    digitalWrite (relay_pin, HIGH);
    state = "OFF";
    mode = "Day";
    }    
  }
  
  delay(100);
}
