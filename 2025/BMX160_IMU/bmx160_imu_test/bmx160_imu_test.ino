/*!
 * @file readAllData.ino
 * @brief Through the example, you can get the sensor data by using getSensorData:
 * @n     get all data of magnetometer, gyroscope, accelerometer.
 * @n     With the rotation of the sensor, data changes are visible.
 * @copyright	Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author [luoyufeng] (yufeng.luo@dfrobot.com)
 * @maintainer [Fary](feng.yang@dfrobot.com)
 * @version  V1.0
 * @date  2021-10-20
 * @url https://github.com/DFRobot/DFRobot_BMX160
 */
#include <BMX160.h>
#include <math.h>

BMX160 bmx160;

void transform2(float *inp, float *rotation, float* out, short g_scale){
  out[0] = rotation[0] * inp[0] + rotation[1] * inp[1] + rotation[2]*inp[2];
  out[1] = rotation[3] * inp[0] + rotation[4] * inp[1] + rotation[5]*inp[2];
  out[2] = rotation[6] * inp[0] + rotation[7] * inp[1] + rotation[8]*inp[2];

  //scale by g_scale
  for(int i=0; i<2; ++i){
    out[i] = g_scale*out[i];
  }
}


void setup(){
  Serial.begin(115200);
  delay(10000);
  Serial.println("FUCK WHY DOESNT THIS WORK");
  
  //init the hardware bmx160  
  if (bmx160.begin() != true){
    Serial.println("init false");
    while(1);
  }
  //bmx160.setLowPower();   //disable the gyroscope and accelerometer sensor
  //bmx160.wakeUp();        //enable the gyroscope and accelerometer sensor
  //bmx160.softReset();     //reset the sensor
  
  /** 
   * enum{eGyroRange_2000DPS,
   *       eGyroRange_1000DPS,
   *       eGyroRange_500DPS,
   *       eGyroRange_250DPS,
   *       eGyroRange_125DPS
   *       }eGyroRange_t;
   **/
  //bmx160.setGyroRange(eGyroRange_500DPS);
  
  /**
   *  enum{eAccelRange_2G,
   *       eAccelRange_4G,
   *       eAccelRange_8G,
   *       eAccelRange_16G
   *       }eAccelRange_t;
   */
  //bmx160.setAccelRange(eAccelRange_4G);
  delay(100);
}

void loop(){
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;

  /* Get a new sensor event */
  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);



  Serial.println("BEFPRE");
  Serial.println("BEFORE");
  Serial.println("BEFORE");

  /* Display the magnetometer results (magn is magnetometer in uTesla) */
  Serial.print("M ");
  Serial.print("X: "); Serial.print(Omagn.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Omagn.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Omagn.z); Serial.print("  ");
  Serial.println("uT");

  /* Display the gyroscope results (gyroscope data is in g) */
  Serial.print("G ");
  Serial.print("X: "); Serial.print(Ogyro.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Ogyro.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Ogyro.z); Serial.print("  ");
  Serial.println("g");
  
  /* Display the accelerometer results (accelerometer data is in m/s^2) */
  Serial.print("A ");
  Serial.print("X: "); Serial.print(Oaccel.x    ); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Oaccel.y    ); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Oaccel.z    ); Serial.print("  ");
  Serial.println("m/s^2");

  Serial.println("");

  delay(2000);
  //Calculate Pitch, roll
  float roll = atan2f(Oaccel.y, Oaccel.z);
  float denom = sqrtf(Oaccel.y * Oaccel.y + Oaccel.z * Oaccel.z);
  float pitch = atan2f(-Oaccel.x, denom);
  Serial.print("Roll: ");
  Serial.println(roll);
  Serial.print("Pitch: ");
  Serial.println(pitch);

  float s_r = sinf(roll);
  float c_r = cosf(roll);
  float s_p = sinf(pitch);
  float c_p = cosf(pitch);


  float RxRy[9];

  //0, 1, 2
  //3, 4, 5
  //6, 7, 8
  RxRy[0] = c_p;
  RxRy[1] = s_p * s_r;
  RxRy[2] = s_p * c_r;
  RxRy[3] = 0.0f;
  RxRy[4] = c_r;
  RxRy[5] = -s_r;
  RxRy[6] = -s_p;
  RxRy[7] = c_p*s_r;
  RxRy[8] = c_p*c_r;

  float declination = 0.214;
  float m_x = RxRy[0] * Omagn.x + RxRy[1] * Omagn.y + RxRy[2] * Omagn.z;
  float m_y = RxRy[3] * Omagn.x + RxRy[4] * Omagn.y + RxRy[5] * Omagn.z;
  float psi = atan2f(m_y,m_x + declination);
  Serial.print("Yaw: ");
  Serial.println(psi);


  /*
  *    float declination = 0.214;
    float m_x = RxRy[0]*magn[0]+RxRy[1]*magn[1]+RxRy[2]*magn[2];
    float m_y = RxRy[3]*magn[0]+RxRy[4]*magn[1]+RxRy[5]*magn[2];
    float psi = atan2f(m_y,m_x)+declination;
  */  

  delay(500);
  //now apply and then reprint

  float a_in[3];
  float a_out[3];
  float g_in[3];
  float g_out[3];
  float m_in[3];
  float m_out[3];

  a_in[0] = Oaccel.x;
  a_in[1] = Oaccel.y;
  a_in[2] = Oaccel.z;

  transform2(a_in, RxRy, a_out, 1.0);

  
  Serial.println("AFTER");
  Serial.println("AFTER");
  Serial.println("AFTER");

    /* Display the magnetometer results (magn is magnetometer in uTesla) */
  Serial.print("M ");
  Serial.print("X: "); Serial.print(Omagn.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Omagn.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Omagn.z); Serial.print("  ");
  Serial.println("uT");

  /* Display the gyroscope results (gyroscope data is in g) */
  Serial.print("G ");
  Serial.print("X: "); Serial.print(Ogyro.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(Ogyro.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(Ogyro.z); Serial.print("  ");
  Serial.println("g");
  
  /* Display the accelerometer results (accelerometer data is in m/s^2) */
  Serial.print("A ");
  Serial.print("X: "); Serial.print(a_out[0]); Serial.print("  ");
  Serial.print("Y: "); Serial.print(a_out[1]); Serial.print("  ");
  Serial.print("Z: "); Serial.print(a_out[2]); Serial.print("  ");
  Serial.println("m/s^2");


}