// Bibliotecas #########################################################################################

#include <Wire.h>
#include <LiquidCrystal_I2C.h> //versão 2.8
#include <IRremote.h> //versão 1.1.2
#include <arduino-timer.h>

// Pinos ##############################################################################################

int pino_recetor = 6;
int pino_rele = 7 ;
int pino_ldr = A0;

// Códigos DEC dos butões do comando ##################################################################

int b_MENU   = 25245;
int b_PROX   = 765;
int b_ANTE   = 8925;
int b_OK     = -15811;
int b_SAIR   = -28561;
int b_MAIS   = -22441;
int b_MENOS  = -8161;
 

// Parâmetros ########################################################################################

LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv irrecv(pino_recetor);
decode_results results;

auto timer_diagonostico = timer_create_default();
auto timer_ecra = timer_create_default();

// Variáveis ####s####################################################################################


int limite_lux = 100;  // limite lux LDR para ligar o relé
int lux = 0; // Valor em lux
int R_div = 1000; // Resistencia do divisor de tensão

bool resultado_diagonostico = true; // Resultado do diagonóstico
String estado = "";
bool estado_overide = false;
String modo = "";
int menu = 0;
int comando = 0;
int erro = 0;



// Funções ##########################################################################################

void ecra (int cursor1, String linha1, int cursor2, String linha2){
  lcd.clear();
  lcd.setCursor(cursor1,0);
  lcd.print(linha1);
  lcd.setCursor(cursor2,1);
  lcd.print(linha2);
}


void medicao_ldr(){
  int valor_ldr = analogRead (pino_ldr); // Leitura aos terminais do LDR (0-1012)
  float Vout = float(valor_ldr) * (float(5)/float(1023));  // Conversão para tensão
  float R_ldr = (R_div * (5 - Vout))/Vout; //Cálculo do valor da resistência do LDR;
  lux = 500/(R_ldr/1000); //conversao para lux
}


bool diagonostico(void*) {
    ecra (3, "A realizar", 2, "diagonostico");
    delay(600);
    medicao_ldr();
  
  if (lux < 5 && lux > 0){
    resultado_diagonostico = false;
    erro = 1;
    estado_overide = true;
    }

  else if (lux < 0){
    resultado_diagonostico = false;
    erro = 2;
    estado_overide = true;
  }

  else{
    resultado_diagonostico = true;
    erro = 0;
  }
  Serial.println(resultado_diagonostico);
  return true;
}

bool ecra_menu_principal(void*) {
  if (estado_overide){
    estado = "OFF";    
  }
  else{
    estado = "ON";
  }

  
  switch (menu) {
    case 0:
      if (resultado_diagonostico){
      ecra(0, "Modo: " + modo, 0, String(lux) + " lux");
      }
      else if (!resultado_diagonostico){
        switch(erro) {
          case 1:
            ecra(5, "Erro 1", 0, "Bloco LDR aberto");
            break;
          case 2:
            ecra(5, "Erro 2", 0, "Bloco LDR aberto");
            break;
        }
      }
      break;
    case 1:
      menu = 2;
      break;
    case 2:
      ecra(0, ">Estado: " + estado , 1 , "Lim: " + String(limite_lux) + " lux");
      break;
    case 3:
      ecra(1, "Estado: " + estado , 0 , ">Lim: " + String(limite_lux) + " lux");
      break;
    case 4:
      ecra(1, "Lim: " + String(limite_lux) + " lux" , 0 , ">Diagonostico");
      break;
    case 5 ... 9:
      menu = 4;
      break;

  }
  return true; //Mantem o timer ativo
}




// Void setup & loop ################################################################################################

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  ecra(3, "a iniciar", 0, "");
  irrecv.enableIRIn();
  pinMode(pino_rele, OUTPUT);

  timer_diagonostico.every(3600000, diagonostico);
  timer_ecra.every(800, ecra_menu_principal);

}

void loop() {
  timer_diagonostico.tick();
  timer_ecra.tick();
  medicao_ldr();

  if(estado_overide){
    digitalWrite (pino_rele, HIGH);
    modo = "OFF";
  }

  
  if (irrecv.decode(&results)) {
    irrecv.resume();
    comando = results.value;
    
    
    if (comando == b_MENU){
      menu = 2;
      }
    else if (comando == b_SAIR && menu > 0){
      menu = 0;
    }
    else if (comando == b_PROX && menu > 0){
      menu++;
      }
    else if (comando == b_ANTE && menu > 0){
      menu--;
    }
    else if (comando == b_OK && menu > 0){
      switch(menu){
        case 2:
          if (estado_overide == false){
            estado_overide = true;
            digitalWrite (pino_rele, HIGH);
            modo = "OFF";
          }
          else{
            estado_overide = false;
          }
        break; 
        case 4:
          timer_diagonostico.in(0,diagonostico);
          menu=0;
          break;
      }      
    }
    else if (comando == b_MAIS && menu == 3){
      limite_lux = limite_lux + 5;
    }

    else if (comando == b_MENOS && menu == 3){
      limite_lux = limite_lux - 5;
    }
    Serial.println(menu);
    Serial.println(comando);
    irrecv.resume();
  }

  
  else if (resultado_diagonostico && menu == 0 && !estado_overide){
    if (lux < limite_lux){
      digitalWrite (pino_rele, LOW);
      estado = "ON";
      modo = "Noite";
      
     }
    else{
    digitalWrite (pino_rele, HIGH);
    estado = "OFF";
    modo = "Dia";
    }    
  }
  
  delay(100);
}
