/* Brazo robótico con 4 servomotores, 3 potenciometros y un interruptor para su control manual, y un boton para grabar y reproducir movimientos.
   Idea basa en proyecto de American Hacker.
   Autor: Javier Vargas. El Hormiguero.
   https://creativecommons.org/licenses/by/4.0/
*/
//PINES
#define pinS1 0 //No cambiar direcciones de servos
#define pinS2 1
#define pinS3 2
#define pinS4 3
#define pinPotS1 A0
#define pinPotS2 A1
#define pinPotS3 A2
#define pinCoger 13
#define pinGrabar 2 //Interrupcion
#define pinR 9
#define pinG 10
#define pinB 11

//CONFIGURACION
const int MinServo[] = {0, 180, 0, 100}; //Angulo minimo de los servos
const int MaxServo[] = {180, 0, 180, 180}; //Angulo maximo de los servos
#define SERVOMIN  160 // Anchura minima de señal PWM
#define SERVOMAX  550 // Anchura maxima de señal PWM
#define MemMax 60 //Posiciones maximas a almacenar
#define VelocidadAuto 15 //ms por grado de movimiento grabado
#define Velocidad 8 //ms por grado de movimiento 
#define SensGrab 10 //Sensibilidad analogica con la que se detecta un nuevo movimiento
#define Antirrebotes 200 //ms de antirrebote del boton

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); //Direccion 0x40

int angulo[4]; //Posiciones de los servos
int GrabadoPosicionesS1[MemMax]; //Memoria de posiciones
int GrabadoPosicionesS2[MemMax];
int GrabadoPosicionesS3[MemMax];
int GrabadoPosicionesS4[MemMax];
int addr = 0; //Direcciones de memoria guardadas
byte estado = 0; //Estado del brazo (0 manual, 1 grabando, 2 grabado);
volatile boolean interrupcion = 0;

void setup() {

  //Serial.begin(115200);

  //PinMode
  pinMode(pinCoger, INPUT_PULLUP);
  pinMode(pinGrabar, INPUT_PULLUP);

  //Driver servos
  pwm.begin();
  pwm.setPWMFreq(60);  //Frecuencia de 60Hz en servos analógicos

  //Inicio de servos al min
  for (int i = 0; i < 4; i++) {
    angulo[i] = MinServo[i];
  }

  //Vector de memoria de posiciones a -1 (sin datos)
  for (int i = 0; i < MemMax; i++) {
    GrabadoPosicionesS1[i] = -1;
    GrabadoPosicionesS2[i] = -1;
    GrabadoPosicionesS3[i] = -1;
    GrabadoPosicionesS4[i] = -1;
  }

  RGB(255, 0, 0);
  delay(500);
  RGB(0, 0, 255);
  delay(500);
  RGB(0, 255, 0);
  delay(500);
  RGB(255, 255, 255);

  //Interrupcion
  attachInterrupt(digitalPinToInterrupt(pinGrabar), Interrupcion, CHANGE);
}

//////////////////////////////////
//////////////LOOP////////////////
//////////////////////////////////

void loop() {

  //MODO GRABAR
  if (ModoGrabar() == 2) {
    PotToServo(pinPotS1, pinS1);
    PotToServo(pinPotS2, pinS2);
    PotToServo(pinPotS3, pinS3);
    BotToServo(pinCoger, pinS4);
    GrabarMovimientos();
    estado = 1;
  }

  //MODO AUTOMÁTICO
  else if (ModoGrabar() == 1) {
    estado = 2;
    RepetirMovimientos();
  }

  //MODO MANUAL
  else {
    estado = 0;
    PotToServo(pinPotS1, pinS1);
    PotToServo(pinPotS2, pinS2);
    PotToServo(pinPotS3, pinS3);
    BotToServo(pinCoger, pinS4);
  }

  delay(5);
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void RepetirMovimientos() {

  RGB(255, 255, 0);
  delay(1000);
  RGB(0, 255, 0);

  //Recorremos las posiciones guardadas hacia adelante
  for (int x = 0; x <= addr; x++) {
    if (ModoGrabar() != 1) break;
    if (GrabadoPosicionesS4[x] != -1) SetServo(pinS4,  GrabadoPosicionesS4[x], VelocidadAuto);
    if (GrabadoPosicionesS3[x] != -1) SetServo(pinS3,  GrabadoPosicionesS3[x], VelocidadAuto);
    if (GrabadoPosicionesS2[x] != -1) SetServo(pinS2,  GrabadoPosicionesS2[x], VelocidadAuto);
    if (GrabadoPosicionesS1[x] != -1) SetServo(pinS1,  GrabadoPosicionesS1[x], VelocidadAuto);
  }
}


void GrabarMovimientos() {

  static int a1, a2, a3, a4 = 0;
  static byte e = 0;

  //Borrado de memoria la primera vez que entramos en el modo grabar
  if (estado != 1) {
    while (addr > 0) {
      addr--;
      GrabadoPosicionesS1[addr] = -1;
      GrabadoPosicionesS2[addr] = -1;
      GrabadoPosicionesS3[addr] = -1;
      GrabadoPosicionesS4[addr] = -1;
    }
    a1 = analogRead(pinPotS1);
    a2 = analogRead(pinPotS2);
    a3 = analogRead(pinPotS3);
    a4 = digitalRead(pinCoger);

    GrabadoPosicionesS1[addr] = angulo[0];
    GrabadoPosicionesS2[addr] = angulo[1];
    GrabadoPosicionesS3[addr] = angulo[2];
    GrabadoPosicionesS4[addr] = angulo[3];

    e = 0;
  }

  //Cambio en servo 1
  if (e == 1) a1 = analogRead(pinPotS1), GrabadoPosicionesS1[addr] = angulo[0];
  else if (Diferente(a1, analogRead(pinPotS1), SensGrab)) e = 1, addr++;

  //Cambio en servo 2
  if (e == 2) a2 = analogRead(pinPotS2), GrabadoPosicionesS2[addr] = angulo[1];
  else if (Diferente(a2, analogRead(pinPotS2), SensGrab)) e = 2, addr++;

  //Cambio en servo 3
  if (e == 3) a3 = analogRead(pinPotS3), GrabadoPosicionesS3[addr] = angulo[2];
  else if (Diferente(a3, analogRead(pinPotS3), SensGrab)) e = 3, addr++;

  //Cambio en servo 4
  if (e == 4) a4 = digitalRead(pinCoger), GrabadoPosicionesS4[addr] = angulo[3];
  else if (Diferente(a4, digitalRead(pinCoger), 0)) e = 4, addr++;
}

void Interrupcion() {
  static unsigned long tPuls = 0;
  if (millis() > tPuls + Antirrebotes) {
    tPuls = millis();
    if (!digitalRead(pinGrabar)) interrupcion = 1; //Flanco de subida
  }
}

byte ModoGrabar() {
  static boolean e = 0;
  static byte out = 0;
  static unsigned long m = 0;

  //Estado no pulsado
  if (e == 0 && interrupcion) {
    e = 1;
    interrupcion = 0;
    m = millis() + 2000;
  }

  //Estado pulsado
  if (e == 1) {
    if (millis() > m) {
      out = 2, RGB(255, 0, 0); //Devuelve 2 (modo grabar) tras 2 segundos pulsado
      e = 0;
    }
    else if (digitalRead(pinGrabar)) { //Se suelta el boton
      if (out == 0) out = 1, RGB(0, 255, 0); //Devuelve 1 (modo auto)
      else out = 0, RGB(255, 255, 255); //Devuelve 0 (modo manual)
      e = 0;
    }
  }

  return out;
}

void PotToServo(int pot, int servo) {
  int anguloFinal = map(analogRead(pot), 0, 1023, MinServo[servo], MaxServo[servo]);
  SetServo(servo, anguloFinal, Velocidad);
}

void BotToServo(int bot, int servo) {
  int anguloFinal = map(digitalRead(bot), 0, 1, MinServo[servo], MaxServo[servo]);
  SetServo(servo, anguloFinal, Velocidad);
}
boolean Diferente(int valor1, int valor2, int margen) {
  if (valor1 > valor2 + margen || valor1 < valor2 - margen) return 1;
  else return 0;
}

void SetServo(int servo, int anguloFinal, int t) {
  while (angulo[servo] != anguloFinal) {
    if (anguloFinal > angulo[servo]) angulo[servo]++;
    else angulo[servo]--;
    pwm.setPWM(servo, 0, map(angulo[servo], 0, 180, SERVOMIN, SERVOMAX));
    delay(t);
  }
}

void RGB(int R, int G, int B) {
  analogWrite(pinR, R);
  analogWrite(pinG, G);
  analogWrite(pinB, B);
}

