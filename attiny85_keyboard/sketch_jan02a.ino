/*
Arduino based KeeloQ-Decoder. 

comment out microview stuff, if you don't have a microview-Display. Decrypted data will be send via serial.

connect Pin2 to OOK-Receiver board.

*/
#include <Wire.h>

#define LED_PIN    13
#define HCS_RECIEVER_PIN  2

#define KeeLoq_NLF    0x3A5C742E
#define bit(x,n)    (((x)>>(n))&1)
#define g5(x,a,b,c,d,e) (bit(x,a)+bit(x,b)*2+bit(x,c)*4+bit(x,d)*8+bit(x,e)*16)

uint64_t k = 0x0123456789ABCDEF; //PUT IN DEVICE-KEY HERE
uint64_t k1;
uint32_t serial;
//uint32_t seed = 0x12345678;
long prevtime;
long lasttime;

uint32_t decrypted;


uint32_t encrypt (const uint32_t data, const uint64_t key)
{
  uint32_t x = data, r;

  for (r = 0; r < 528; r++)
  {
    x = (x>>1)^((bit(x,0)^bit(x,16)^(uint32_t)bit(key,r&63)^bit(KeeLoq_NLF,g5(x,1,9,20,26,31)))<<31);
  }
  return x;
}


uint32_t decrypt (const uint32_t data, const uint64_t key)
{
  uint32_t x = data, r;

  for (r = 0; r < 528; r++)
  {
    x = (x<<1)^bit(x,31)^bit(x,15)^(uint32_t)bit(key,(15-r)&63)^bit(KeeLoq_NLF,g5(x,0,8,19,25,30));
  }
  return x;
}



class HCS301 {
public:
  unsigned BattaryLow : 1;
  unsigned Repeat : 1;
  unsigned BtnNoSound : 1;
  unsigned BtnOpen : 1; 
  unsigned BtnClose : 1; 
  unsigned BtnRing : 1;
  unsigned long SerialNum;
  unsigned long Encript;
  void print();
  void disp();
};

volatile boolean  HCS_Listening = true;   
byte        HCS_preamble_count = 0;
uint32_t      HCS_last_change = 0;
//uint32_t      HCS_start_preamble = 0;
uint8_t       HCS_bit_counter;
uint8_t       HCS_bit_array[66];
#define       HCS_TE    400
#define       HCS_Te2_3 600

HCS301 hcs301;


void setup()
{
  Wire.begin(4);
  Serial.begin(9600);
  pinMode(HCS_RECIEVER_PIN, INPUT);
  attachInterrupt(0, HCS_interrupt, CHANGE); //PIN 2 = INT 0
  Serial.println("Setup OK");
  prevtime = millis();
  lasttime = millis();
}


void loop()
{
  long CurTime = millis();

  if(CurTime - prevtime >= 20000){     
        prevtime = millis();
    }

  if(HCS_Listening == false){
    HCS301 msg;
    memcpy(&msg,&hcs301,sizeof(HCS301));
    HCS_Listening = true;

  k1 = decrypt((0x60000000|hcs301.SerialNum),k); 
  k1 <<= 32; 
  k1 |= decrypt((0x20000000|hcs301.SerialNum),k);

    decrypted = decrypt(hcs301.Encript,k1);    
    hcs301.print();
  }

}

 
void HCS301::print(){
  String btn;
  if(millis() - lasttime >= 2000)
  {
  if (BtnRing == 1) btn += "A";
  if (BtnClose == 1) btn += "B";
  if (BtnOpen == 1) btn += "C";
  if (BtnNoSound == 1) btn += "D";

  Serial.print("Encrypt ");
  Serial.print(Encript,HEX);
  Serial.print(" Serial ");
  Serial.print(SerialNum,HEX);
  Serial.print(" Button: ");
  Serial.print(btn);
  Serial.print(" BatteryLow=");
  Serial.print(BattaryLow);
  Serial.print(" Rep=");
  Serial.println(Repeat);
  Serial.print("decrypt: ");
  Serial.println(decrypted,HEX);

  if(SerialNum == 0x41DBB){
    Wire.beginTransmission(4);
    Serial.println(" REMOTE ALLOWED");
    Serial.print(" REMOTE BUTTON:");    
    if(btn == "A"){
      Serial.println("A");
      Wire.write(1);
      Wire.endTransmission(); 
    } else if(btn == "B")
    {
      Serial.println("B");
      Wire.write(2);     
      Wire.endTransmission(); 
    } else if(btn == "C")
    {
      Serial.println("C");
      Wire.write(3);
      Wire.endTransmission(); 
    } else if(btn == "D")
    {
      Serial.println("D");
      Wire.write(4);
      Wire.endTransmission(); 
    }
    //blk(3);
  } 
    lasttime = millis();  
  }  
}


void blk(int c){
     for(int i = 0; i < c;i++){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
}


void HCS_interrupt(){

  if(HCS_Listening == false){
    return;
  }

  uint32_t cur_timestamp = micros();
  uint8_t  cur_status = digitalRead(HCS_RECIEVER_PIN);
  uint32_t pulse_duration = cur_timestamp - HCS_last_change;
  HCS_last_change     = cur_timestamp;


  if(HCS_preamble_count < 12){
    if(cur_status == HIGH){
      if( ((pulse_duration > 150) && (pulse_duration < 500)) || HCS_preamble_count == 0){
        //if(HCS_preamble_count == 0){
        //  HCS_start_preamble = cur_timestamp;
        //}
      } else {
        HCS_preamble_count = 0;
        goto exit; 

      }
    } else {
      if((pulse_duration > 300) && (pulse_duration < 600)){
        HCS_preamble_count ++;
        if(HCS_preamble_count == 12){
          //HCS_Te = (cur_timestamp - HCS_start_preamble) / 23;
          //HCS_Te2_3 = HCS_Te * 3 / 2;
          HCS_bit_counter = 0;
          goto exit; 
        }
      } else {
        HCS_preamble_count = 0;
        goto exit; 
      }
    }
  }
  

  if(HCS_preamble_count == 12){
    if(cur_status == HIGH){
      if(((pulse_duration > 250) && (pulse_duration < 900)) || HCS_bit_counter == 0){
      } else {
        HCS_preamble_count = 0;
        goto exit; 
      }
    } else {
      digitalWrite(LED_PIN,HIGH);
      if((pulse_duration > 250) && (pulse_duration < 900)){
        HCS_bit_array[65 - HCS_bit_counter] = (pulse_duration > HCS_Te2_3) ? 0 : 1;
        HCS_bit_counter++;  
        if(HCS_bit_counter == 66){
        
          HCS_Listening = false;  
          HCS_preamble_count = 0;

          hcs301.Repeat = HCS_bit_array[0];
          hcs301.BattaryLow = HCS_bit_array[1];
          hcs301.BtnNoSound = HCS_bit_array[2];
          hcs301.BtnOpen = HCS_bit_array[3];
          hcs301.BtnClose = HCS_bit_array[4];
          hcs301.BtnRing = HCS_bit_array[5];

          hcs301.SerialNum = 0;
          for(int i = 6; i < 34;i++){
            hcs301.SerialNum = (hcs301.SerialNum << 1) + HCS_bit_array[i];
          };

          uint32_t Encript = 0;
          for(int i = 34; i < 66;i++){
             Encript = (Encript << 1) + HCS_bit_array[i];
          };
          hcs301.Encript = Encript;
        }
      } else {
        HCS_preamble_count = 0;
        goto exit; 
      }
      digitalWrite(LED_PIN,LOW);
    }
  }
  
  exit:;
}
