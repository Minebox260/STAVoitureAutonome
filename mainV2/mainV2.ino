#include <util/atomic.h>
//#include "MeOrion.h"
#include <MeMegaPi.h>
#include <Wire.h>


#define DEBUG false


double angleWrap(double angle);
// A class to compute the control signal
class SimplePID{
  private:
    float kp, kd, ki, umax; // Parameters
    float eprev, eintegral; // Storage

  public:
  // Constructor
  SimplePID() : kp(1), kd(0), ki(0), umax(255), eprev(0.0), eintegral(0.0){}

  // A function to set the parameters
  void setParams(float kpIn, float kdIn, float kiIn, float umaxIn){
    kp = kpIn; kd = kdIn; ki = kiIn; umax = umaxIn;
  }

// A function to compute the control signal
double evalu(int value, int target, float deltaT){
    // error
    int e = target - value;
    //Serial.print("Error:");
    //Serial.println(e );
  
    // derivative
    float dedt = (e-eprev)/(deltaT);

    //Serial.print("KD:");
    //Serial.println(dedt );
  
    // integral
    eintegral = eintegral + e*deltaT;
    eintegral = constrain(abs(eintegral),-45,46);
    //Serial.print("KI:");
    //Serial.println(eintegral );

  
    // control signal
    double u = kp*e + kd*dedt + ki*eintegral;
  
    // store previous error
    eprev = e;

    return u;
  }  
};

void acknowledge() {
  delay(50);
  Serial.println("ACK");
  delay(50);
};


void blink(int times, int pause) {
  for(int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(pause);
    digitalWrite(LED_BUILTIN, LOW);
    delay(pause);
  }
}

void emptySerial() {
  while(Serial.available() > 0) {
    char _ = Serial.read();
  }
}

// How many motors
#define NMOTORS 2

// Pins
const int enca[] = {18,19};
const int encb[] = {31,38};
const int pwm[] = {12,8};
const int in1[] = {35,37};
const int in2[] = {34,36};
// Globals
long prevT = 0;
int posPrev[] = {0,0};
volatile int posi[] = {0,0};
volatile int veloci[] = {0,0};
volatile int prevTi[] = {0,0};

float vFilt[] = {0,0};
float vPrev[] = {0,0};
//{x,y}
float s0[] = {0,0};

int cont = 0;
int cont2 = 0;

float trajectoire[3][3] = {
  {3,4},
  {7,0},
  {13,0},
  };


float s_atual[] = {0,0};
float s_proximo[] = {0,0};

float s1[] = {3,4};
float s2[] = {7,0};
float s3[] = {7,0};

// PID class instances
SimplePID pid[NMOTORS];
MeGyro gyro;
SimplePID pidAngle;


//COMMUNICATION ARDUINO _ RASPBERRY
uint8_t code = 0;
int comm_established = 0;
int32_t x0;
int32_t x1;
int32_t y0;
int32_t y1;

uint8_t buffer[16];
uint8_t codeBuffer[1];
const int dataSize = 4;
int32_t data[dataSize] = {x0, y0, x1, y1};

bool stopCommand = false;

float delta_x;
float delta_y;
double targetAngle = 0;
int32_t targetAngleInt;
int doubleSize = 4;


void updateGyro() {
  gyro.update();
}


void setup() {
  Serial.begin(115200);
  gyro.begin();
  //Wire.setClock(400000); //improve gyro accuracy  

  //interrupt to update gyro
  //Timer1.initialize(5000); //every 5ms
  //Timer1.attachInterrupt(updateGyro);

  for(int k = 0; k < NMOTORS; k++){
    pinMode(enca[k],INPUT);
    pinMode(encb[k],INPUT);
    pinMode(pwm[k],OUTPUT);
    pinMode(in1[k],OUTPUT);
    pinMode(in2[k],OUTPUT);

    pid[k].setParams(5,0,48,191);
  }
  pinMode(LED_BUILTIN, OUTPUT);

  pidAngle.setParams(3.5, 0, 0.75, 255);
  
  //attachInterrupt(digitalPinToInterrupt(enca[0]),readEncoder<0>,RISING);
  //attachInterrupt(digitalPinToInterrupt(enca[1]),readEncoder<1>,RISING);
  
}
  


void loop() {
  
  //blink(1,150);
  updateGyro(); //continually updating gyro

  //nothing received this iteration but communication already established
  if (Serial.available() == 0 && comm_established) {
    code = 0;
  } 
  //nothing received since the beginning
  else if (Serial.available() == 0 && !comm_established) {
    //we do nothing before we haven't received a next point
    code = 0;
    return;
  } 
  //data received from the Raspberry
  else {
    Serial.readBytes(codeBuffer, 1);
    code = (uint8_t)codeBuffer[0];
    emptySerial(); //get rid of extra input that might rest    
  }

  
  if (code == 2) {
      //marvelmind doesn't work, motors have to be stopped
      blink(10,50);
      acknowledge();
      comm_established = 1;
      stopCommand = true;   
  } 
  else if (code == 1) {
      blink(10,50);
      
      acknowledge();

      blink(10,50);

      comm_established = 1;
      
      while (Serial.available() < (4 * sizeof(int32_t))) {
        blink(1,15);
      }

      Serial.readBytes(buffer, 16);
      emptySerial();
      
      for(int i = 0; i<4; i++) {
        int firstIx = 4*i;
        data[i] = (((int32_t)buffer[firstIx+3] << 24) + ((int32_t)buffer[firstIx+2] << 16)\
                 + ((int32_t)buffer[firstIx+1] << 8) + ((int32_t)buffer[firstIx]));
      }
      x0 = data[0];
      y0 = data[1];
      x1 = data[2];
      y1 = data[3];

      stopCommand = false;

      
      delta_x = (x1 - x0);
      delta_y = (y1 - y0);

      
      targetAngle = atan2(delta_y,delta_x);
      targetAngle = (targetAngle * 360) / (2*PI); //converting rads to degrees

      targetAngleInt = (int32_t)targetAngle;
      /*Serial.print(targetAngle);
      Serial.print(" ");
      Serial.println(targetAngleInt);*/
      
      //debug code for checking the integers constructed
      emptySerial();

      for(int j = 0; j < dataSize; j++) {
        char * intptr = (char*)&data[j];
        for(int k = 0; k < sizeof(int32_t); k++) {
          //send each byte as a char seperately
          Serial.write((uint8_t*)(intptr+k), 1);
        }
      }
      
      char * intptr = (char*)&targetAngleInt;
      for(int j = 0; j < sizeof(int32_t); j++) {
        Serial.write((uint8_t*)(intptr+j), 1);        
      }
      float currentAngle = angleWrap(gyro.getAngleZ());
      int32_t currentAngleInt = (int32_t)currentAngle;
      
      intptr = (char*)&currentAngleInt;
      for(int j = 0; j < sizeof(int32_t); j++) {
        Serial.write((uint8_t*)(intptr+j), 1);        
      }
      
      
  }


  //float targetAngle = 45;
  /*Serial.print(" AngleTarget:");
  Serial.println(targetAngle );


  Serial.print(" AngleActuel:");*/
  //Serial.println(gyro.getAngleZ() );
  //Serial.println(targetAngle);
  
  /*x0 = 0;
  y0 = 0;
  x1 = 1;
  y1 = -2;

  
  delta_x = (x1 - x0);
  delta_y = (y1 - y0);

  
  targetAngle = atan2(delta_y,delta_x);
  targetAngle = (targetAngle * 360) / (2*PI); //converting rads to degrees

  targetAngleInt = (int32_t)targetAngle;
  Serial.print(targetAngle);
  Serial.print(" ");
  Serial.print(gyro.getAngleZ());
  Serial.print(" ");
  Serial.println(targetAngleInt);*/

  // time difference
  long currT = micros();
  float deltaT = ((float) (currT - prevT))/( 1.0e6 );
  prevT = currT;


  double pwr = pidAngle.evalu(angleWrap(gyro.getAngleZ()), targetAngle, deltaT);
  //Serial.print(" PWR:");
  //Serial.println(pwr );

  if (stopCommand == true) {
    moteur(0,pwm[0],in1[0],in2[0]);
    moteur(0,pwm[1],in1[1],in2[1]);
    return;
  }
  moteur(30 - pwr,pwm[0],in1[0],in2[0]);
  moteur(30 + pwr,pwm[1],in1[1],in2[1]);
  
  //Serial.println("end of loop");

}
 


void moteur(int valeur, int pwm, int in1, int in2)
{
  if(valeur>0)
   {
    digitalWrite(in2,0);
    digitalWrite(in1,1);
   } else {
    digitalWrite(in1,0);
    digitalWrite(in2,1);
   }
  if (valeur == 0) {
    analogWrite(pwm,constrain( 0 ,0,0));
    return;
  }
  analogWrite(pwm,constrain( abs(valeur) ,0,50));
  
}



double angleWrap(double angle){
  while(angle > 180){
    angle -= 360; 
  }
  //while(angle < 180){
  //  angle += 360; 
  //}

  return angle;
}

template <int j>
void readEncoder(){
  int b = digitalRead(encb[j]);
  int increment = 0;
  if(b > 0){
    increment = 1;
    posi[j]++;
  }
  else{
    increment = -1;
    posi[j]--;
  }
  long currT = micros();
  float deltaT = ((float) (currT - prevTi[j]))/1.0e6;
  veloci[j] = increment/deltaT;
  //velocity_i = increment/deltaT;
  prevTi[j] = currT;
}
