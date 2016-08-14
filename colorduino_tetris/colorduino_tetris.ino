/*
  RGB Tetris using I2C on Colorduino
   
  based on 
  -arduino firmware by michael vogt <michu@neophob.com>
  -blinkm firmware by thingM
  -"daft punk" firmware by Scott C / ThreeFN
  -Adapted from Tetris.ino code by 
    Jae Yeong Bae
    UBC ECE
    jocker.tistory.com

  Wirings

  I2C connections between Arduino UNO and the Colorduino RGB LED Matrix Driver Modules are SCL, SDA , DTR, VCC, GND which are connected to A5, A4, Reset, +5V and GND respectively.        
  8 ohms 0.5W speaker is connected via a 100 ohms resistor to Arduino Digital Pin 9 and Gnd
  4 pushbutton switches are connected for game control:
  Pin 4 - Rotate
  Pin 5 - Right
  Pin 6 - Left
  Pin 7 - Down
  
  Please visit instructable at http://www.instructables.com/id/Arduino-based-Bi-color-LED-Matrix-Tetris-Game/ for more detail

  libraries to patch:
  Wire: 
   	utility/twi.h: #define TWI_FREQ 400000L (was 100000L)
                     #define TWI_BUFFER_LENGTH 70 (was 32)
   	wire.h: #define BUFFER_LENGTH 70 (was 32)
 
  This DEMO is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This DEMO is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* ============================= LED Matrix Display ============================= */
#include <Wire.h>

#define START_OF_DATA 0x10       //data markers
#define END_OF_DATA 0x20         //data markers
#define DEST_I2C_ADDR 5          //set destination I2C address (must match firmware in Colorduino module)
#define DEST_I2C_ADDR2 6

#define SCREENSIZEX 8            //num of LEDs accross
#define SCREENSIZEY 16            //num of LEDs down
byte display_byte[3][64];        //display array - 64 bytes x 3 colours 


/* ============================= Colors ============================ */
typedef struct {
  int r;
  int g;
  int b;
} color;

color cyan = { 0, 255, 255 };
color blue = {0, 0, 255};
color orange = {255, 165, 0};
color yellow = {255, 255, 0};
color green = {50, 205, 50};
color purple = {160, 32, 240};
color red = {255, 0, 0};
color selected;
  
  
/* ============================= Audio ============================= */
int speakerOut = 9;

#define mC 1911
#define mC1 1804
#define mD 1703
#define mEb 1607
#define mE 1517
#define mF 1432
#define mF1 1352
#define mG 1276
#define mAb 1204
#define mA 1136
#define mBb 1073
#define mB 1012
#define mc 955
#define mc1 902
#define md 851
#define meb 803
#define me 758
#define mf 716
#define mf1 676
#define mg 638
#define mab 602
#define ma 568
#define mbb 536
#define mb 506

#define mp 0  //pause


// MELODY and TIMING  =======================================
//  melody[] is an array of notes, accompanied by beats[],
//  which sets each note's relative length (higher #, longer note)
int melody[] = {mg};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int beats[]  = {8};

//int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.
int MAX_COUNT = sizeof(melody) / sizeof(int); // Melody length, for looping.

// Set overall tempo
long tempo = 20000;
// Set length of pause between notes
int pause = 1000;
// Loop variable to increase Rest length
int rest_count = 100; //<-BLETCHEROUS HACK; See NOTES

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration  = 0;


/* ========================== Tetris Game ========================= */
long delays = 0;
short delay_ = 300;
long bdelay = 0;
short buttondelay = 150;
short btdowndelay = 30;
short btsidedelay = 80;
unsigned char blocktype;
unsigned char blockrotation;

boolean block[SCREENSIZEX][SCREENSIZEY+2];
boolean pile[SCREENSIZEX][SCREENSIZEY];
int disp[SCREENSIZEX][SCREENSIZEY];

boolean gameoverFlag = false;

unsigned long startTime;

int buttonRotate = 4; // Rotate
int buttonRight = 5;  // Right
int buttonLeft = 6;   // Left
int buttonDown = 7;   // Down


void setup() 
{
  pinMode(speakerOut, OUTPUT); 

  TriggerSound();
  
  Wire.begin(1); // join i2c bus (address optional for master) 

  int seed = 
  (analogRead(0)+1)*
  (analogRead(1)+1)*
  (analogRead(2)+1)*
  (analogRead(3)+1);
  randomSeed(seed);
  random(10,9610806);
  seed = seed *random(3336,15679912)+analogRead(random(4)) ;
  randomSeed(seed);  
  random(10,98046);

  pinMode(buttonRotate, INPUT_PULLUP); // Rotate
  pinMode(buttonRight, INPUT_PULLUP);  // Right
  pinMode(buttonLeft, INPUT_PULLUP);   // Left
  pinMode(buttonDown, INPUT_PULLUP);   // Down 
  
  newBlock();
  updateLED();
}


//**********************************************************************************************************************************************************
void loop()
{
  if (delays < millis()) {
    delays = millis() + delay_;
    movedown();
  }   
  
  //button actions
  int button = readBut();  
  
  if (button == 1)
    rotate();
   
  else if (button == 2)
    moveright();
    
  if (button == 3)
    moveleft();
    
  else if (button == 4)
    movedown();
    
  updateDisp();
}


//**********************************************************************************************************************************************************  
boolean moveleft()
{  
  TriggerSound();
  
  if (space_left()) {
    int i;
    int j;
    for (i=0;i<7;i++) {
      for (j=0;j<16;j++) {
        block[i][j]=block[i+1][j];
      }
    }
    
    for (j=0;j<16;j++) {
      block[7][j]=0;
    }    

    updateLED();
    return 1;
  }

  return 0;
}


//**********************************************************************************************************************************************************  
boolean moveright()
{
  TriggerSound();
  
  if (space_right())
  {
    int i;
    int j;
    for (i=7;i>0;i--) {
      for (j=0;j<16;j++) {
        block[i][j]=block[i-1][j];
      }
    }

    for (j=0;j<16;j++) {
      block[0][j]=0;
    }    
    
    updateLED(); 
    return 1;
  }

  return 0;
}


//**********************************************************************************************************************************************************  
int readBut()
{
  if (bdelay > millis()) {
    return 0;
  }
  
  if ((digitalRead(buttonRotate) == LOW)) {
    bdelay = millis() + buttondelay;
    return 1;
  }  
  
  if ((digitalRead(buttonLeft) == LOW)) {
    bdelay = millis() + btsidedelay;    
    return 2;
  }
  
  if ((digitalRead(buttonRight) == LOW)) {
    bdelay = millis() + btsidedelay;
    return 3;
  }  
  
  if ((digitalRead(buttonDown) == LOW)) {
    bdelay = millis() + btdowndelay;    
    return 4;
  }
  
  return 0;
}


//**********************************************************************************************************************************************************  
void updateLED()
{
  int i;
  int j;
  
  for (i=0;i<8;i++) {
    for (j=0;j<16;j++) {
      disp[i][j] = block[i][j] | pile[i][j];
    }
  }
}


//**********************************************************************************************************************************************************  
void rotate()
{
  TriggerSound();
  
  //skip for square block(3)
  if (blocktype == 3) return;
  
  int xi;
  int yi;
  int i;
  int j;
  //detect left
  for (i=7;i>=0;i--) {
    for (j=0;j<16;j++) {
      if (block[i][j]) {
        xi = i;
      }
    }
  }
  
  //detect up
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        yi = i;
      }
    }
  }  
    
  if (blocktype == 0) {
    if (blockrotation == 0) {
      if (!space_left()) {
        if (space_right3()) {
          if (!moveright())
            return;
          xi++;
        }
        else return;
      }     
      else if (!space_right()) {
        if (space_left3()) {
          if (!moveleft())
            return;
          if (!moveleft())
            return;          
          xi--;
          xi--;        
        }
        else
          return;
      }
      else if (!space_right2()) {
        if (space_left2()) {
          if (!moveleft())
            return;          
          xi--;      
        }
        else
          return;
      }   
      
      block[xi][yi]=0;
      block[xi][yi+2]=0;
      block[xi][yi+3]=0;      
      
      block[xi-1][yi+1]=1;
      block[xi+1][yi+1]=1;
      block[xi+2][yi+1]=1;      

      blockrotation = 1;
    }
    else {
      block[xi][yi]=0;
      block[xi+2][yi]=0;
      block[xi+3][yi]=0;
      
      block[xi+1][yi-1]=1;
      block[xi+1][yi+1]=1;
      block[xi+1][yi+2]=1;

      blockrotation = 0;
    }    
  }
  
  //offset to mid
  xi ++;  
  yi ++;  
  
  if (blocktype == 1) {
    if (blockrotation == 0) {
      block[xi-1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 2;
      block[xi+1][yi-1] = 2;
      block[xi][yi+1] = 2;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1) {
      if (!space_left()) {
        if (!moveright())
          return;
        xi++;
      }        
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi] = 2;
      block[xi+1][yi] = 2;
      block[xi+1][yi+1] = 2;      
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2) {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;      
      
      block[xi][yi-1] = 2;
      block[xi][yi+1] = 2;
      block[xi-1][yi+1] = 2;      
      
      blockrotation = 3;            
    }
    else {
      if (!space_right()) {
        if (!moveleft())
          return;
        xi--;
      }
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi+1] = 0;        

      block[xi-1][yi-1] = 2;
      block[xi-1][yi] = 2;
      block[xi+1][yi] = 2;
      
      blockrotation = 0;          
    }  
  }



  if (blocktype == 2) {
    if (blockrotation == 0) {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 3;
      block[xi+1][yi+1] = 3;
      block[xi][yi+1] = 3;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1) {
      if (!space_left()) {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi+1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi] = 3;
      block[xi+1][yi] = 3;
      block[xi-1][yi+1] = 3;      
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2) {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi-1][yi+1] = 0;      
      
      block[xi][yi-1] = 3;
      block[xi][yi+1] = 3;
      block[xi-1][yi-1] = 3;      
      
      blockrotation = 3;            
    }
    else {
      if (!space_right()) {
        if (!moveleft())
          return;
        xi--;
      }      
      block[xi][yi-1] = 0;
      block[xi][yi+1] = 0;
      block[xi-1][yi-1] = 0;        

      block[xi+1][yi-1] = 3;
      block[xi-1][yi] = 3;
      block[xi+1][yi] = 3;
      
      blockrotation = 0;          
    }  
  }
  
  if (blocktype == 4) {
    if (blockrotation == 0) {
      block[xi+1][yi-1] = 0;
      block[xi-1][yi] = 0;

      block[xi+1][yi] = 5;
      block[xi+1][yi+1] = 5;      
      
      blockrotation = 1;
    }
    else {
      if (!space_left()) {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi+1][yi] = 0;
      block[xi+1][yi+1] = 0;      
      
      block[xi-1][yi] = 5;
      block[xi+1][yi-1] = 5;
      
      blockrotation = 0;          
    }  
  }  

  if (blocktype == 5) {
    if (blockrotation == 0) {
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;

      block[xi][yi-1] = 6;
      block[xi+1][yi] = 6;
      block[xi][yi+1] = 6;      
      
      blockrotation = 1;
    }
    else if (blockrotation == 1) {
      if (!space_left()) {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi][yi-1] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;
      
      block[xi-1][yi] = 6;
      block[xi+1][yi] = 6;
      block[xi][yi+1] = 6;
      
      blockrotation = 2;      
    }
    else if (blockrotation == 2) {
      yi --;
      
      block[xi-1][yi] = 0;
      block[xi+1][yi] = 0;
      block[xi][yi+1] = 0;     
      
      block[xi][yi-1] = 6;
      block[xi-1][yi] = 6;
      block[xi][yi+1] = 6;      
      
      blockrotation = 3;            
    }
    else {
      if (!space_right()) {
        if (!moveleft())
          return;
        xi--;
      }      
      block[xi][yi-1] = 0;
      block[xi-1][yi] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi][yi-1] = 6;
      block[xi-1][yi] = 6;
      block[xi+1][yi] = 6;
      
      blockrotation = 0;          
    }  
  }
  
  if (blocktype == 6) {
    if (blockrotation == 0) {
      block[xi-1][yi-1] = 0;
      block[xi][yi-1] = 0;

      block[xi+1][yi-1] = 7;
      block[xi][yi+1] = 7;      
      
      blockrotation = 1;
    }
    else {
      if (!space_left()) {
        if (!moveright())
          return;
        xi++;
      }              
      xi--;
      
      block[xi+1][yi-1] = 0;
      block[xi][yi+1] = 0;      
      
      block[xi-1][yi-1] = 7;
      block[xi][yi-1] = 7;
      
      blockrotation = 0;          
    }  
  }  

  //if rotating made block and pile overlap, push rows up
  while (!check_overlap()) {
    for (i=0;i<18;i++) {
      for (j=0;j<8;j++) {
         block[j][i] = block[j][i+1];
      }
    }
    delays = millis() + delay_;
  }
  
  
  updateLED();    
}


//**********************************************************************************************************************************************************  
void movedown()
{ 
  if (space_below()) {
    //move down
    int i;
    for (i=15;i>=0;i--) {
      int j;
      for (j=0;j<8;j++) {
        block[j][i] = block[j][i-1];
      }
    }
    for (i=0;i<7;i++) {
      block[i][0] = 0;
    }
  }
  else
  {
    //merge and new block
    int i;
    int j;
    
    for (i=0;i<8;i++) {
     for(j=0;j<16;j++) {
       if (block[i][j]) {
         pile[i][j]=block[i][j];
         block[i][j]=0;
       }
     }
    }
    newBlock();   
  }
  updateLED();  
}


//**********************************************************************************************************************************************************  
boolean check_overlap()
{
  int i;
  int j;
  
  for (i=0;i<16;i++) {
    for (j=0;j<7;j++) {
       if (block[j][i]) {
         if (pile[j][i])
           return false;
       }        
    }
  }
  for (i=16;i<18;i++) {
    for (j=0;j<7;j++) {
       if (block[j][i]) {
         return false;
       }        
    }
  }  
  return true;
}


//**********************************************************************************************************************************************************  
void check_gameover()
{
  int i;
  int j;
  int cnt=0;;
  
  for(i=15;i>=0;i--) {
    cnt=0;
    
    for (j=0;j<8;j++) {
      if (pile[j][i])
        cnt ++;
    }    
    
    if (cnt == 8) {
      for (j=0;j<8;j++) {
        pile[j][i]=0;
      }
      updateLED();
      delay(50);
      
      int k;
      for(k=i;k>0;k--) {
        for (j=0;j<8;j++) {
          pile[j][k] = pile[j][k-1];
        }                
      }
      
      for (j=0;j<8;j++) {
        pile[j][0] = 0;
      }        
      updateLED();      
      delay(50);      
      i++;     
    }
  }
  
  for(i=0;i<8;i++) {
    if (pile[i][0])
      gameover();
  }
  return;
}


//**********************************************************************************************************************************************************  
void gameover()
{
  int i;
  int j;

  gameoverFlag = true;
  startTime = millis();       
       
  delay(300);       
            
  while(true)      //To re-play if any buttons depressed again
  {      
    int button = readBut();
    
    if ((button < 5) && (button > 0)) {
      gameoverFlag = false;    
    
      for(i=15;i>=0;i--) {
        for (j=0;j<8;j++) {
          pile[j][i]=0;
        }             
      }
    
      break;
    }  
  }  
}


//**********************************************************************************************************************************************************  
void newBlock()
{
  check_gameover();
  blocktype = random(7);
  
  // 0
  // 0
  // 0
  // 0
  if (blocktype == 0) {
    block[3][0]=1;
    block[3][1]=1;
    block[3][2]=1;
    block[3][3]=1; 
  }

  // 0
  // 0 0 0
  if (blocktype == 1) {
    block[2][0]=2;
    block[2][1]=2;
    block[3][1]=2;
    block[4][1]=2;
  }
  
  //     0
  // 0 0 0
  if (blocktype == 2) {
    block[4][0]=3;
    block[2][1]=3;
    block[3][1]=3;
    block[4][1]=3;
  }
  
  // 0 0
  // 0 0
  if (blocktype == 3) {
    block[3][0]=4;
    block[3][1]=4;
    block[4][0]=4;
    block[4][1]=4;
  }    
  
  //   0 0
  // 0 0
  if (blocktype == 4) {
    block[4][0]=5;
    block[5][0]=5;
    block[3][1]=5;
    block[4][1]=5;
  }    
  
  //   0
  // 0 0 0
  if (blocktype == 5) {
    block[4][0]=6;
    block[3][1]=6;
    block[4][1]=6;
    block[5][1]=6;
  }        

  // 0 0
  //   0 0
  if (blocktype == 6) {
    block[3][0]=7;
    block[4][0]=7;
    block[4][1]=7;
    block[5][1]=7; 
  }    

  blockrotation = 0;
}


//**********************************************************************************************************************************************************  
boolean space_below()
{ 
  int i;
  int j;  
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
       if (block[j][i]) {
         if (i == 15)
           return false;
           
         if (pile[j][i+1])
           return false;
      }
    }
  }
  return true;
}


//**********************************************************************************************************************************************************  
boolean space_left()
{ 
  int i;
  int j;  
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++)  {
      if (block[j][i]) {
        if (j == 0)
          return false;
           
        if (pile[j-1][i])
          return false;
      }        
    }
  }
  return true;
}


//**********************************************************************************************************************************************************  
boolean space_left2()
{ 
  int i;
  int j;
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        if (j == 0 || j == 1)
          return false;
           
        if (pile[j-1][i] | pile[j-2][i])
          return false;
      }
    }
  }
  return true;
}


//**********************************************************************************************************************************************************  
boolean space_left3()
{ 
  int i;
  int j;
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        if (j == 0 || j == 1 || j == 2 )
          return false;
          
        if (pile[j-1][i] | pile[j-2][i] | pile[j-3][i])
          return false;
      }
    }
  }
  return true;
}


boolean space_right()
{ 
  int i;
  int j;  
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        if (j == 7)
          return false;
           
        if (pile[j+1][i])
          return false;
      }
    }
  }
  return true;
}


//**********************************************************************************************************************************************************  
boolean space_right2()
{ 
  int i;
  int j;
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        if (j == 7 || j == 6)
          return false;
           
        if (pile[j+1][i] | pile[j+2][i])
          return false;
      }
    }
  }
  return true;
}


//**********************************************************************************************************************************************************  
boolean space_right3()
{ 
  int i;
  int j;
  
  for (i=15;i>=0;i--) {
    for (j=0;j<8;j++) {
      if (block[j][i]) {
        if (j == 7||j == 6||j == 5)
          return false;
           
        if (pile[j+1][i] |pile[j+2][i] | pile[j+3][i])
          return false;
      }        
    }
  }
  return true;
}


void updateDisp()
{ 
    clearLines();
    updateLED(); 
    
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (disp[i][j] != 0) { 
          colorSelect(i, j, disp[i][j]);
        }
        else  {
          display(i, j, 0, 0, 0);   
        }
      }
    }
    update_display(DEST_I2C_ADDR); 
      
    for(int i = 0; i < 8; i++) {
      for (int j = 8; j < 16; j++) {
        if(disp[i][j] != 0) {
          colorSelect(i, j, disp[i][j]);
        }
        else
          display(i, j, 0, 0, 0);
      }
    }
    update_display(DEST_I2C_ADDR2);
}


//**********************************************************************************************************************************************************
void colorSelect(int i, int j, int color)
{
    if (color == 1) 
        display(i, j, cyan.r, cyan.g, cyan.b);
  
    else if (color == 2) 
        display(i, j, blue.r, blue.g, blue.b);
    
    else if (color == 3) 
        display(i, j, orange.r, orange.g, orange.b);
    
    else if (color == 4) 
      display(i, j, yellow.r, yellow.g, yellow.b);
    
    else if (color == 5)
      display(i, j, green.r, green.g, green.b);
   
    else if (color == 6)
      display(i, j, purple.r, purple.g, purple.b);
   
    else
      display(i, j, red.r, red.g, red.b);
}


//**********************************************************************************************************************************************************
boolean line = true;
void clearLines()
{
  for (int j = 0; j < 16; j++) {
    for (int i = 0; i < 8 && line; i++) {
      if ((i == 7 && pile[j][i] > 0) || pile[j][i] >0)
          line = true;
      else
         line = false;
    }
    if (line == true) {
       for (int k = 0; k < 8; k++) {
          pile[k][j] = 0;  
       }
    }
  }    
}


//**********************************************************************************************************************************************************
//update display buffer using x,y,r,g,b format
void display(int x, int y, byte r, byte g, byte b) {
  //Need to reset y value for second display
  if (y > 7)
    y -= 8;
    
  byte p = (y*8)+x;   //convert from x,y to pixel number in array
  display_byte[0][p] = r;
  display_byte[1][p] = g;
  display_byte[2][p] = b;
}


//**********************************************************************************************************************************************************
//send display buffer to display 
void update_display(byte addr) {   
  BlinkM_sendBuffer(addr, 0, display_byte[0]);   
  BlinkM_sendBuffer(addr, 1, display_byte[1]);   
  BlinkM_sendBuffer(addr, 2, display_byte[2]);  
}


//**********************************************************************************************************************************************************
//send data via I2C to a client
static byte BlinkM_sendBuffer(byte addr, byte col, byte* disp_data) {
  Wire.beginTransmission(addr);
  Wire.write(START_OF_DATA);
  Wire.write(col);
  Wire.write(disp_data, 64);
  Wire.write(END_OF_DATA);
  return Wire.endTransmission();
}


//**********************************************************************************************************************************************************
void TriggerSound()
{
  // Set up a counter to pull from melody[] and beats[]
  for (int i=0; i<MAX_COUNT; i++) {
    tone_ = melody[i];
    beat = beats[i];

    duration = beat * tempo; // Set up timing

    playTone();
    delayMicroseconds(pause);
  }
}


//**********************************************************************************************************************************************************
// Pulse the speaker to play a tone for a particular duration
void playTone() 
{
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(speakerOut,HIGH);
      delayMicroseconds(tone_ / 2);

      // DOWN
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);

      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
    }                                
  }                                
}
