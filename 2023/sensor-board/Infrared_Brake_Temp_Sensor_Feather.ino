void setup() {
  Serial.begin(9600);
  delay(1000);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
}

void loop() {
  int data[31];
  int count = 0;
  delay(10);
  digitalWrite(4, HIGH);
  delay(1);
  while(count < 31){
      pinMode(4, OUTPUT);
      digitalWrite(4, LOW);
      delayMicroseconds(1);
      digitalWrite(4,HIGH);
      delayMicroseconds(1);
      pinMode(4, INPUT);
      delayMicroseconds(30);
      data[count] = digitalRead(4);
      count++;
  }

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
    
  int tobj[17] = {};
  int tamb[14] = {};
  long tobjConv = 0;
  long tambConv = 0;

for(int i = 0; i < 17; i++){ // read Tobj
      tobj[i] = data[i];
}

for(int i = 17; i < 32; i++){ // read Tamb
      tamb[i] = data[i];
}

for(int i = 0; i < 17; i++){ //
  tobjConv += tobj[i] * pow(2,16 - i);
}

for(int i = 0; i < 14; i++){ //
  tambConv += tamb[i] * pow(2,13 - i);
}

Serial.print(tobjConv - 64500);
Serial.print(" , ");
Serial.println(tambConv);
}
