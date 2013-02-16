#include "Carriage.h"
#include "Logger.h"

//http://arduino.cc/forum/index.php/topic,140205.0.html

CarriageDriver::CarriageDriver(unsigned char endSensorPinNr)
{
  endSensorPin = endSensorPinNr;
  quad_A = 2;
  quad_B = 13;
  motorDirectionPin = 4;
  motorSpeedPin = 5;
  
  mask_quad_A = digitalPinToBitMask(quad_A);
  mask_quad_B = digitalPinToBitMask(quad_B);  
  
  pinMode(motorDirectionPin, OUTPUT);   
  pinMode(motorSpeedPin, OUTPUT);   
  pinMode(endSensorPin, INPUT);   
  
  // activate peripheral functions for quad pins
  REG_PIOB_PDR = mask_quad_A;     // activate peripheral function (disables all PIO functionality)
  REG_PIOB_ABSR |= mask_quad_A;   // choose peripheral option B    
  REG_PIOB_PDR = mask_quad_B;     // activate peripheral function (disables all PIO functionality)
  REG_PIOB_ABSR |= mask_quad_B;   // choose peripheral option B 
    
  // activate clock for TC0
  REG_PMC_PCER0 = (1<<27);
  // select XC0 as clock source and set capture mode
  REG_TC0_CMR0 = 5; 
  // activate quadrature encoder and position measure mode, no filters
  REG_TC0_BMR = (1<<9)|(1<<8)|(1<<12);
  // enable the clock (CLKEN=1) and reset the counter (SWTRG=1) 
  // SWTRG = 1 necessary to start the clock!!
  REG_TC0_CCR0 = 5;    
  SetMotorSpeed(0);
}

long CarriageDriver::GetPosition()
{
  long result = REG_TC0_CV0;
  return -result;
}  
  
void CarriageDriver::Calibrate()
{
  for (short i = 0; i < 3; i++)
  {
    Serial.println("Going left...");
    if (!IsAtTheEnd())
    {
      SetMotorSpeed(-200);
      while(!IsAtTheEnd())
      {
        ;
      }
      SetMotorSpeed(0);
    }
    Serial.println("Going right...");  
    SetMotorSpeed(200);
    while(IsAtTheEnd())
    {
      ;
    }
  }
  SetMotorSpeed(0); 
  delay(300);
  ResetPosition();
}

void CarriageDriver::ResetPosition()
{
  REG_TC0_CCR0 = 5;     
}

boolean CarriageDriver::IsAtTheEnd()
{
  return digitalRead(endSensorPin) == LOW;
}

void CarriageDriver::SetMotorSpeed(int newSpeed)
{
  if (newSpeed < 0)
  {
    digitalWrite(motorDirectionPin, HIGH);
  }
  else
  {
    digitalWrite(motorDirectionPin, LOW);
  }  
  
  int ms = abs(newSpeed);
  if (ms > 255)
  {
    ms = 255;
  }
  analogWrite(motorSpeedPin, ms);     
}

void CarriageDriver::SpeedCheck()
{
  Logger lg;
  lg.Clear();
  
  int motorSpeed;
  int prevPos = GetPosition();
  int counter = 0;
  int phase = 0;
  long nxtTime;
  
  while (counter < 500)
  {
    nxtTime = micros() + 1000;
    long curPos = GetPosition();  
    
    switch (phase)
    {
      case 0: // acceleration
        motorSpeed = 250;
        if (curPos >= 7000)
        {
          motorSpeed = -200;
          phase = 1;
        }
        break;
      case 1: // deceleration
        motorSpeed = -200;
        if (curPos <= prevPos)
        {
          motorSpeed = 0;
          phase = 2;
        }
        break;     
      case 2: // idle
        motorSpeed = 0;
        break;             
    }        
    prevPos = curPos;
    SetMotorSpeed(motorSpeed);
    lg.AddToLog(curPos, motorSpeed);
    counter++;
    while (micros() < nxtTime)
    {
      ;
    }
  }
  SetMotorSpeed(0);
  lg.FlushToSerial();
}



