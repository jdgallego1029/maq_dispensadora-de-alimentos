/*
  Práctica #8 Laboratiro Fundamentos de Electrónica Digital

  Universidad de Antioquia

  Desarrollado por

  JUAN  DAVID GALLEGO GARCÍA
  CC. 1035914506
 
  ESTEBAN PULGARIN ARANGO
  CC. 1037617009
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

/*Define los estados de la Máquina de Estados Finitos (MEF)*/
enum STATE{estInicial,estVenta,estVuelta,estPass,estConf,estConfProd,estConfPass,estProdCant,estProdValor};
STATE mef = estInicial; //Estado inicial

//Variables
/*Pulsadores para ingresar valor en pesos 
 * en orden: 
 * $50, $100, $200, $500
 */
int pulsA = 4, pulsB = 5 , pulsC = 6, pulsD = 7;
/* 
 *  Led para devolver valor en pesos 
 *  En orden:
  * $50, $100, $200, $500
 */
int ledA = 14, ledB = 15, ledC = 16;
/*
 * Otras Variabes
 */
char cadena[10]; // Para guardar el código ascii de las teclas del teclado
char prod [10];//Array donde se almacena caracteres ingresados para producto
int dir = 0;
unsigned long tiempo = 0;
unsigned long t_actualizado = 0;
int datoH = 0, datoL = 0;
int fila = 0, columna = 0, posicion = 0;
int pass = 0, clave = 0, newPass = 0;
int precio = 0, costo = 0, auxCosto = 0, newCosto = 0, cant = 0, total = 0, cont = 0;
int newCant = 0;
int dato = 0;
//bool check;
bool flag = HIGH;
bool inConf = HIGH, menuConf = LOW; //Indica cuando se puede ingresar a configuracion
bool inPass = LOW;
bool inVenta = LOW;
bool outVuelta = LOW;
bool noProd = LOW;
bool ledEstado = HIGH;

/* Funciones utilizadas*/
void modoConf();
void pulsDinero();
void confProducto();
void confContrasena();
void addProducto();
bool checkProd();
bool checkPass();
int checkSalir();
void onLedVuelta();
int checkConf();
int checkConfProd();

//void addProducto();
void setup() {
  
  /* Configuración de pines*/
  for(int puls = 4; puls < 8; puls++){
    pinMode(puls,INPUT);
  }
  for(int led = 14; led < 17; led++){
    pinMode(led,OUTPUT);
    digitalWrite(led, HIGH);  
  }
  Serial.begin(9600);
  lcd.begin(20, 4); 
  
  /*Interrupciones*/
  attachInterrupt(0, modoConf, RISING); //Para ingresar a modo configuración
  attachInterrupt(1, pulsDinero, RISING);  //Para saber cuando se ingresa una meneda   
  //addProducto();
}

void loop() {
  /*Maquina de estado finita */
  switch(mef){
     case estInicial: //Estado inicial
        inConf = HIGH; //Variable bandera que permite leer interrupcion de pulsConf para entrar a modo configuracion
        if(flag){
          Serial.println(" "); 
          Serial.println("BIENVENIDO"); 
          Serial.println("Seleccionar producto. Formato X#");
          lcd.clear();
          lcd.setCursor(5,0);
          lcd.print("BIENVENIDO");
          lcd.setCursor(0,1);
          lcd.print("SELECCIONAR PRODUCTO");
          flag = LOW; 
        }
        if(Serial.available()>0 && !flag){ //Comprobamos si en el buffer hay datos
          bool check = checkProd(); //Verifica que el código de procuto es correcto
          if(check && EEPROM.read(posicion) > 0){ //Codigo correcto y hay existencia
            mef = estVenta; //Nuevo estado
            inVenta = HIGH;  
          }else{
            Serial.println("ERROR");
            mef = estInicial; //Se queda en estado actual
            tiempo = 0;
            while(tiempo < 150){ //pequeño ciclo para mostrar mensaje en LCD
              tiempo++;
              lcd.setCursor(0,2);
              lcd.print("PRODUCTO INCORRECTO");
              lcd.setCursor(1,3);
              lcd.print("INTENTA DE NUEVO");
            }
            tiempo = 0;  
          }
          flag = HIGH;
        }
        if(inPass){//En caso de ser presionado pulsConf
          mef = estPass; //nuevo estado
          flag = HIGH;
        }
      break;
      case estVenta: //Estado de venta
        if(flag){
          Serial.println(" ");
          Serial.print("Producto: ");
          Serial.println(prod);
          Serial.print("Precio de producto: $");
          Serial.println(costo);
          Serial.println(" ");
          Serial.println("Esperando  que ingresen dinero");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("SELECCION");
          lcd.setCursor(0,1);
          lcd.print("PRODUCTO: ");
          lcd.print(prod);
          lcd.setCursor(0,2);
          lcd.print("PRECIO:   $");
          lcd.print(costo);
          flag = LOW;
          Serial.println("Ingresar CC para cancelar compra");
        }
        lcd.setCursor(0,3);
        lcd.print("TOTAL:    $"); //Total ingresado con los pulsA-D
        lcd.print(total);
        if(Serial.available()>0 && !flag){ //Comprobamos si en el buffer hay datos
           int dato = checkSalir();
           if(dato == 134 && total > 0){ //Valor entero de sumar ASCII de C + C 
              Serial.println("OK");
              mef = estVuelta; //Nuevo estado para reembolsar lo ingresado
              noProd = HIGH;
           }
           else if (dato == 134){//Si no ingreso dinero psa a estado inicial
              Serial.println("OK");
              mef = estInicial; //Nuevo estado
           }else{//En caso de ingresar incorrecta/. CC
              Serial.println("ERROR");
              mef = estVenta;
           }
           flag = HIGH;
        }
        if(total >= costo){ //Ingresado con pulsA-D supera el precio del producto
          Serial.print("Total ingresado: $");
          Serial.println(total);
          inVenta = LOW;
          flag = HIGH;
          total = total - costo; //Dinero a devolver
          mef = estVuelta;  //Nuevo estado
        }
        if(inPass){//En caso de ser presionado pulsConf 
          Serial.println(" ");
          Serial.println("POR FAVOR TERMINAR PROCESO DE VENTA");
          inPass = LOW; //No permite realizar función de interrupción pin 2
          flag = HIGH; 
          mef = estVenta; //Se queda en este estado
        }
      break;
      
      case estVuelta: //Estado de reembolso
        if(flag){
          Serial.println(" ");
          lcd.clear();
          if(noProd){ //En caso que reembolso sea por cancelar producto
            lcd.setCursor(0,0);
            lcd.print("PRODUCTO CANCELADO");
          }
          else{//En caso de que el reembolso sea porque el producto fue entregado
            lcd.setCursor(0,0);
            lcd.print("DISFRUTA TU PRODUCTO");
            lcd.setCursor(0,2);
            lcd.print("PRODUCTO:  ");
            lcd.print(prod);
          }
          lcd.setCursor(0,1);
          lcd.print("REEMBOLSO DE DINERO");
          lcd.setCursor(0,3);
          lcd.print("REEMBOLSO: $");
          lcd.print(total);
          Serial.print("Reembolso $");
          Serial.println(total);
          flag = LOW;
          
        }
        onLedVuelta(); //Prende los led's correspondientes al reembolso 
        if(total == 0 ){ //Realizó el reembolso 
          tiempo = millis();
          if( tiempo - t_actualizado >= 1000){//Para seguir mostrando en pantalla el mensaje de reembolso
            t_actualizado = tiempo;
            if(!outVuelta){
              outVuelta = HIGH;
            }else{
              flag = HIGH; //Para mostrar mensaje de estInicial
              if(noProd){//En caso de reembolso por cancelar no descuenta cantidad
                noProd = LOW;
              }else{//El reembolso es por venta y descuenta cantidad
                EEPROM.update(posicion,cant-1);
              }
              mef = estInicial; //Para volver estInicial
            }
          }    
        }
        else{
            mef = estVuelta; //Continua en este estado
            outVuelta = LOW; 
        }
        if(inPass){ //No entra a modo configuración mientras esta en proceso de venta
          Serial.println(" ");
          Serial.println("POR FAVOR TERMINAR PROCESO DE VENTA");
          inPass = LOW;
        } 
      break;
      case estPass: //Estado para verificar contraseña para ingresar a modo configuracion
        if(inPass){
          Serial.println(" ");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("MODO CONFIGURACION");
          lcd.setCursor(0,1);
          lcd.print("INGRESAR CONTRASENA");
          Serial.println("Modo Configuracion");
          Serial.println("Por favor ingresar contrasena numerica de 4 digitos");
          inPass = LOW;  
        }
        if(Serial.available()>0 && !inPass){ //Comprobamos si en el buffer hay datos
          bool check = checkPass(); //Comprueba que la contraseña sea igual a la guardada en EEPROM
           if(check){
              mef = estConf; //nuevo estado
              flag =  HIGH; 
           }else{
            cont++;
            if(cont < 3){//3 intentos para ingresar la contraseña
              lcd.setCursor(0,2);
              lcd.print("ERROR CONTRASENA");
              lcd.setCursor(0,3);
              lcd.print("INTENTAR DE NUEVO");
              Serial.println(" ");
              Serial.println("ERROR CONTRASENA INCORRECTA");
              Serial.print("Intento # ");
              Serial.println(cont);
              Serial.println("Intentar de nuevo");
              mef = estPass;
            }else{
              Serial.println("Supero numero de intentos");
              mef = estInicial;
              flag = HIGH; 
            }
            
           }
        } 
      break;

      case estConf: //Estado configuración
        inConf = LOW; //Para deshabilitar interrupción pulsador configuración
        if(flag){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("MODO CONFIGURACION");
          lcd.setCursor(0,1);
          lcd.print("INGRESAR OPCION");
          lcd.setCursor(0,3);
          lcd.print("EXIT PARA SALIR");
          Serial.println(" ");
          Serial.println("Modo configuracion. Elegir opcion");
          Serial.println("Codigo X# de producto a configurar");
          Serial.println("Nueva contrasena con PWD_XXXX ");
          Serial.println("EXIT para salir");
          flag = LOW;
          menuConf = HIGH;
        } 
        if(Serial.available()>0 && !flag){ //Comprobamos si en el buffer hay datos
          dato = checkConf(); //Verifica que los comandos sean ingresados correctamente y recibe un dato para ejecutar una acción
          switch(dato){
             case 1:
              mef = estConfProd; //nuevo estado comando X#
              flag = HIGH;
             break;
             case 2:
              mef = estConfPass; //nuevo estado comando PWD_XXXX
              flag = HIGH;  
             break;
             case 3:
                mef = estInicial; //nuevo estado comando EXIT
                flag = HIGH;
             break;
             case 0: 
                mef = estConf; //Estado actual
                Serial.println(" ");
                Serial.println("ERROR");
                Serial.println("Por favor verifica de nuevo");
                flag = HIGH;
             break;
             default:
                mef = estConf; //Estado actual
                Serial.println("ERROR");
                Serial.println("Por favor verifica de nuevo");
                flag = HIGH;
             break;
          }
        }
      break;
      case estConfProd: //Estado configuración de producto
        if(flag){
          Serial.println(" ");
          Serial.print("Producto: ");
          Serial.println(prod);
          Serial.print("Precio ACTUAL: $");
          Serial.println(auxCosto);
          Serial.print("Cantidad ACTUAL: ");
          Serial.println(cant);
          Serial.println(" ");
          Serial.println("Ingresar opcion ");
          Serial.println("Formato C_XX para ingresar cantidad");
          Serial.println("NOTA: La cantidad a ingresar no puede ser mayor a 15");
          Serial.println("EJEMPLO: C_01, C_05 , C_08 , C_12 , C_14");
          Serial.println(" ");
          Serial.println("Formato P_XXXX para ingresar precio");
          Serial.println("NOTA: El precio debe ser multiplo de 50");
          Serial.println("EJEMPLO: P_1200 , P_1450 , P_1300 , P_1550");
          Serial.println(" ");
          Serial.println("Formato TP para salir");
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("INGRESAR OPCION ");
          lcd.setCursor(0,1);
          lcd.print("PRODUCTO: ");
          lcd.print(prod);
          lcd.setCursor(0,2);
          lcd.print("PRECIO:   $");
          lcd.print(auxCosto);
          lcd.setCursor(0,3);
          lcd.print("CANTIDAD: ");
          lcd.print(cant);
          flag = LOW;
        }
        if(Serial.available()>0 && !flag){ //Comprobamos si en el buffer hay datos
          dato = checkConfProd(); //Verifica que los comandos sean bien ingresos y recibe un dato para realizar una acción
          switch(dato){
            case 1: // Comando C_XX
              if(newCant+cant <= 15){ //Verifica que nueva cantidad + cantidad actual sea menor a 15
                 cant += newCant; 
                 EEPROM.update(posicion, cant);
                 Serial.println(" ");
                 Serial.print("Cantidad ACTUALIZADA: ");
                 Serial.println(EEPROM.read(posicion));
                 flag = HIGH;
                 
              }else{//En caso que suma sea mayor a 15
                Serial.println(" ");
                Serial.println("ERROR");
                Serial.println("La cantidad no debe superar el total de 15");
                mef = estConfProd;
                flag = HIGH;  
              }
            break;
            case 2: //Comando P_XXXX guarda el nuevo precio en la EEPROM
              EEPROM.update(posicion+1,newCosto >> 8);
              EEPROM.update(posicion+2,newCosto & 0XFF);
              auxCosto = EEPROM.read(posicion+1)<<8;
              auxCosto |= EEPROM.read(posicion+2);
              Serial.print("Precio ACTUALIZADO: $");
              Serial.println(auxCosto);
              flag = HIGH;
            break;
            case 3: //Comando TP
             mef = estConf; //nuevo estado
             flag = HIGH;
            break;
            case 0: //En caso de ingresar un comando erroneo
             Serial.println(" ");
             Serial.println("ERROR");
             Serial.println("Por favor verifica de nuevo");
             mef = estConfProd; //Estado actual
             flag = HIGH;
            break;
            default: //En caso de ingresar un comando erroneo
             Serial.println(" ");
             Serial.println("ERROR");
             Serial.println("Por favor verifica de nuevo");
             mef = estConfProd; //Estado actual
             flag = HIGH;
            break;  
          }
        }
      break;
      case estConfPass: //Estado configuración de contraseña
        Serial.println(" ");
        Serial.println("Contrasena actualizada correctamente. NUNCA LA OLVIDES");
        /*Se guarda la nueva contraseña en la EEPROM*/
        EEPROM.update(60, newPass >> 8);
        EEPROM.update(61, newPass & 0XFF);
        mef = estConf; //Nuevo estado
        flag = HIGH;
        clave = EEPROM.read(60)<<8;
        clave |= EEPROM.read(61);
        Serial.print("Clave NUEVA ");
        Serial.println(clave);
      break;
      default: //Estado Finito cuando en estado inicial no se hace nada
        mef = estInicial;
        flag = HIGH;
      break;
  }
}

bool checkPass (){ //Verifica la contraseña guardada en la EEPROM con la ingresa
  bool passCheck;
  int dir = 0;
  memset(cadena, 0,sizeof(cadena));//memset borra el contenido del array  "cadena" desde la posición 0 hasta el final sizeof
  while(Serial.available()>0){ //Mientras haya datos en el buffer ejecuta la función
    delay(5); //Poner un pequeño delay para mejorar la recepción de datos
    cadena[dir] = Serial.read();//Lee un carácter del string "cadena" de la "posicion", luego lee el siguiente carácter con "posicion++"
    dir++;
  }
  dir = 0;
  pass = atoi(cadena);//Convertimos la cadena de caracteres en 
  clave = EEPROM.read(60)<<8;
  clave |= EEPROM.read(61);
  if(clave == pass){
    passCheck = HIGH;  
  }else{
     passCheck = LOW;   
  }
  return passCheck;
}

bool checkProd(){ //Verifica que el codigo X# sea ingresado correctamente y guarda todos sus datos en variables globales
  bool prodCheck;
  dir = 0;
  memset(prod, 0,sizeof(prod));//memset borra el contenido del array  "cadena" desde la posición 0 hasta el final sizeof
  while(Serial.available()>0){ //Mientras haya datos en el buffer ejecuta la función
    delay(5); //Poner un pequeño delay para mejorar la recepción de datos
    prod[dir] = Serial.read();//Lee un carácter del string "cadena" de la "posicion", luego lee el siguiente carácter con "posicion++"
    dir++;
  }
  dir = 0;
  fila = prod[0] - 65; //Para obtener la fila
  columna = atoi(&prod[1])-1;//Para obtener la columna, convierte array en decimal
  posicion = (12*fila) + (3*columna); //Obtiene posicion de cada producto    
  cant = EEPROM.read(posicion);
  if((columna >= 0 && columna <=3)&&(fila >=0 && fila <= 4)){
    prodCheck = true;
    costo =  EEPROM.read(posicion+1)<<8;
    costo |= EEPROM.read(posicion+2);
  }else{
    prodCheck = false;  
  }
  return prodCheck;      
}

int checkSalir(){ //Verifica que comando CC sea ingresado correctamente
  dir = 0;
  memset(cadena, 0,sizeof(cadena));//memset borra el contenido del array  "cadena" desde la posición 0 hasta el final sizeof
  while(Serial.available()>0){ //Mientras haya datos en el buffer ejecuta la función
    delay(5); //Poner un pequeño delay para mejorar la recepción de datos
    cadena[dir] = Serial.read();//Lee un carácter del string "cadena" de la "posicion", luego lee el siguiente carácter con "posicion++"
    dir++;
  }
  dir = 0;
  return cadena[0] + cadena[1];
}

void onLedVuelta(){ //Prende los led's para indicar la devolución de dinero
  /*
    Cuando el total ingresado supero los $500 y multiplo de este, 
    entrega primero monedas de 500 y luego las otras,
    si es el caso
  */
  if(total > 200){ 
    tiempo = millis();
    if( tiempo - t_actualizado >= 500){
      t_actualizado = tiempo;
      if(ledEstado){
        ledEstado = LOW;
      }else{
        ledEstado = HIGH;
        total -= 200; 
      }
      digitalWrite(ledC,ledEstado);
    }
   }else{ //entrega las otras cuando el reembolso es menor a 500
    switch(total){
      case 50: //moneda de $50
        tiempo = millis();
        if( tiempo - t_actualizado >= 500){
          t_actualizado = tiempo;
          if(ledEstado){
            ledEstado = LOW;
          }else{
            ledEstado = HIGH;
            total = 0;
          }
          digitalWrite(ledA,ledEstado);
        }
      break;
      case 100: //moneda de $100
        tiempo = millis();
        if( tiempo - t_actualizado >= 500){
          t_actualizado = tiempo;
          if(ledEstado){
            ledEstado = LOW;
          }else{
            ledEstado = HIGH;
            total = 0;
          }
          digitalWrite(ledB,ledEstado);
        }
      break;
      case 150: //Devuelve $100 y resta el total ingresado
        tiempo = millis();
        if( tiempo - t_actualizado >= 500){
          t_actualizado = tiempo;
          if(ledEstado){
            ledEstado = LOW;
          }else{
            ledEstado = HIGH;
            total -= 100;
          }
          digitalWrite(ledB,ledEstado);
        }
      break;
      case 200://Moneda de $200
        tiempo = millis();
        if( tiempo - t_actualizado >= 500){
          t_actualizado = tiempo;
          if(ledEstado){
            ledEstado = LOW;
          }else{
            ledEstado = HIGH;
            total = 0;
          }
          digitalWrite(ledC,ledEstado);
        }
  
      break;
    }
   }
}

int checkConf(){//Verifica que comando X#, PWD_XXXX o EXIT sean correctos
  int opcion = 0;
  dir = 0;
  memset(cadena, 0,sizeof(cadena));//memset borra el contenido del array  "cadena" desde la posición 0 hasta el final sizeof
  while(Serial.available()>0){ //Mientras haya datos en el buffer ejecuta la función
    delay(5); //Poner un pequeño delay para mejorar la recepción de datos
    cadena[dir] = Serial.read();//Lee un carácter del string "cadena" de la "posicion", luego lee el siguiente carácter con "posicion++"
    dir++;
  }
  dir = 0;
  if(cadena[0]-65 >= 0 && cadena[0]-65 <= 4){ //Se granatiza que columna X sea el primer dato ingresado sea A-E
    fila = cadena[0] - 65; //Para obtener la fila
    columna = atoi(&cadena[1])-1;//Para obtener la columna, convierte array en decimal
    posicion = (12*fila) + (3*columna); //Obtiene posicion de cada producto
     if((columna >= 0 && columna <=3)&&(fila >=0 && fila <= 4)){ //Comprueba que el producto sea correcto
      opcion = 1;
      cant = EEPROM.read(posicion);
      auxCosto =  EEPROM.read(posicion+1)<<8;
      auxCosto |= EEPROM.read(posicion+2);
       for(dir = 0 ; dir <= 10 ; dir++){
          prod[dir] = cadena[dir];
       }  
     }else{
      opcion = 0;
     }
  }
  if(cadena[0]==80 && cadena[1]==87 && cadena[2]==68 && cadena[3]==95){//Codigo PWD_
    dir = 4;
    while(dir >= 4 && dir < 8) {//Para leer datos depues de PWD_
        if(cadena[dir] >= 48 && cadena[dir] <= 57){ //Se garantiza que XXXX sean números
          opcion = 2;
          newPass = atoi(&cadena[4]); 
        }else{
          opcion = 0;
          dir = 8;
          newPass = clave;
        }
        dir++;
    }
  }
  if(cadena[0]==69 && cadena[1]==88 && cadena[2]==73 &&cadena[3]==84){//Comando EXIT
    opcion = 3; 
  }
  return opcion;
}

int checkConfProd(){//Verifica que comando C_XX P_XXXX o TP sean ingresados correctamente
  int opcion = 0;
  dir = 0;
  memset(cadena, 0,sizeof(cadena));//memset borra el contenido del array  "cadena" desde la posición 0 hasta el final sizeof
  while(Serial.available()>0){ //Mientras haya datos en el buffer ejecuta la función
    delay(5); //Poner un pequeño delay para mejorar la recepción de datos
    cadena[dir] = Serial.read();//Lee un carácter del string "cadena" de la "posicion", luego lee el siguiente carácter con "posicion++"
    dir++;
  }
  if(cadena[0] == 84 && cadena[1] == 80 ){ //Comando TP
    opcion = 3; //Terminar configuración de producto
  }
  else if(cadena[0] == 67 && cadena[1] == 95){ //Comando C_
    dir = 2;
    while(dir >= 2 && dir < 4) {
        if(cadena[dir] >= 48 && cadena[dir] <= 57){ //Parantiza que XX sean números
          opcion = 1;
          newCant = atoi(&cadena[2]); 
        }else{
          opcion = 0;
          newCant = 0;
          dir = 4;
        }
        dir++;
    }
  }
  else if(cadena[0] == 80 && cadena[1] == 95){ //Comando P_
    dir = 2;
    while(dir >= 2 && dir < 6) {
        if(cadena[dir] >= 48 && cadena[dir] <= 57){ //Parantiza que XXXXX sean números
          newCosto = atoi(&cadena[2]);
          if(newCosto % 50 == 0){//Garantiza que precio sea multiplo de 50
            opcion = 2;  
          }else{
            opcion = 0;  
          }
        }else{
          opcion = 0;
          dir = 6;
        }
        dir++;
    }
  }
  else{
    opcion = 0;
  }
  return opcion;
}

/*Interrupcione¨*/
/*Modo configuracion por medio de interrupcion flanco de subida
  Pin2
*/
void modoConf(){
  if(inConf){
    inPass = HIGH;
    inConf = LOW;
  }
}

/*Leer estado de pulsador para leer dinero ingresado Pin3
*/ 
void pulsDinero(){
  if(inVenta){
   tiempo = millis();
   if( tiempo - t_actualizado >= 500){
      t_actualizado = tiempo;
      if(digitalRead(pulsA)){//Moneda de $50
        total += 50;  
      }
      if(digitalRead(pulsB)){//Moneda de $100
        total += 100;  
      }
      if(digitalRead(pulsC)){//Moneda de $200
        total += 200;  
      }
      if(digitalRead(pulsD)){//Moneda de $500
        total += 500;  
      }
    }
  }
}


//void addProducto(){
//  posicion = 57;
//  cant = 11;
//  precio = 1950;
//  datoH = precio>>8;  //Bits mas significativos de la contraseña
//  datoL = precio & 0XFF; //Bits menos significativos de la contraseña
//  EEPROM.update(posicion, cant);
//  Serial.println(cant);
//  EEPROM.update(posicion+1, datoH);
//  EEPROM.update(posicion+2, datoL);
//  int otro = EEPROM.read(posicion); 
//  costo =  EEPROM.read(posicion+1)<<8;
//  costo |= EEPROM.read(posicion+2);
//  Serial.print("Cantidad ");
//  Serial.println(otro);
//  Serial.print("Costo ");
//  Serial.println(costo);  
//}
