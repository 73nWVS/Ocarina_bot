// Ocarina bot!
// www.nerdzoo.xyz

// Fourier Transorm definitions
#define FS 8000 //Hz
#define samples 100
unsigned int sampling_period;
long microseconds;
int V[samples];
int t[samples];
int Vsum = 0;
int DCBias = 0;
long Tstart = 0;
long Tend = 0;

// Notes
float F1 = 1050; //C 
float F2 = 1150; //D
float F3 = 1350; //F
float F4 = 1530; //G
float F5 = 1760; //A
float F6 = 1960; //B

int MaxVal = 0;
int MaxNote = 0;
float SNR = 5;
int lastnote = 0;

// Motors
int motora[] = {6,4,9,11};
int motorb[] = {5,3,10,12};
int power[] = {7,2,8,13};

//Directions
int rev[4] = {-1,-1,-1,-1};
int fwd[4] = {1,1,1,1};
int rgt[4] = {1,-1,-1,1};
int lft[4] = {-1,1,1,-1};
int str[4] = {1,-1,1,-1};
int stl[4] = {-1,1,-1,1};
int stp[4] = {0,0,0,0};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  sampling_period = round(1.0 / FS * 1000000);
}

void loop() {
  //**********************
  //***SAMPLING SECTION***
  //**********************
  
  //Record samples:
  Tstart = 0;
  Vsum = 0;
  for (int i = 0; i < samples; i++)
  {
    if (i == 0) Tstart = micros();
    microseconds = micros() - Tstart;
    V[i] = analogRead(A0);
    t[i] = microseconds;
    Vsum += V[i];
    
    //Wait to take the next sample according to the sampling frequency
    while (micros() < (microseconds + sampling_period)) {
    }  
  }

  DCBias = Vsum / samples;
  
  int NoteArray[6] = {
    FTF(samples, V, DCBias, t, F1),
    FTF(samples, V, DCBias, t, F2),
    FTF(samples, V, DCBias, t, F3),
    FTF(samples, V, DCBias, t, F4),
    FTF(samples, V, DCBias, t, F5),
    FTF(samples, V, DCBias, t, F6)
  };
  

  int maxVal = 0;
  int maxNote = 0;
  int sumNote = 0;
  float avgNote = 0;

  //Test to see if 1 frequency > 2.5x the average of all the others
  for (int n = 0; n <= 5; n++) {
    if (NoteArray[n] > maxVal) {
      maxVal = NoteArray[n];
      maxNote = n;
    }
    sumNote += NoteArray[n];  
  }
  
  avgNote = (sumNote - maxVal) / 5;
  

  if (maxVal / avgNote > SNR) {
    if (maxNote==0) Move(motora, motorb, power, rev);  //Reverse
    if (maxNote==1) Move(motora, motorb, power, fwd);      //Forward
    if (maxNote==2) Move(motora, motorb, power, rgt);    //Turn Right
    if (maxNote==3) Move(motora, motorb, power, lft);    //Turn Left
    if (maxNote==4) Move(motora, motorb, power, str);    //Strafe Right
    if (maxNote==5) Move(motora, motorb, power, stl);    //Strafe Left
    if (maxNote == lastnote) SNR = ;
    else SNR = 6;
  }
  else {
    Move(motora, motorb, power, stp);
    SNR = 6;
  }
 
  lastnote = maxNote;
  
}

  //**********************
  //***CUSTOM FUNCTIONS***
  //**********************


float FTF(int num_samples, int sample_array[samples], int bias, int time_array[samples], float test_frequency) {
  /* Fourier Transform Function
   *  Inputs:
   *  num_samples: number of data samples
   *  sample_array: recorded analog mic data
   *  bias: DC Bias of mic data signal
   *  time_array: time data of mic signal in microseconds
   *  test_frequency: desired frequency to check for]
   */
  int Signal[num_samples];  
  float Wavelengths[num_samples];  //Array to store the number of test frequency periods that have passed for each mic sample.  Used to build test frequency square wave
  int TruncatedWavelengths[num_samples];  //Used to build test frequency square wave
  int Multiplier[num_samples];  //Test frequency square wave array
  int FTFArray[num_samples];    //Product of test frequency square wave and mic sample data
  long FTFSum[2];
  int FTFFit = 0;
  
  int test_period = 1000000/test_frequency;  // = 1 / test_frequency * 10e6

  for (int n = 0; n <= 1; n++) {  //Used for sin vs cosine multiplier array
    FTFSum[n] = 0;
    for (int j = 0; j < num_samples; j++) {
    Signal[j] = sample_array[j] - bias;  //Normalize sample array data about x axis
    //The following takes the mic sample array times and builds a square wave of the test frequency
    Wavelengths[j] = (time_array[j] - (0.25 * n * test_period)) / (0.5 * test_period);  //Look at how many multiples of half the test frequency period have passed
    TruncatedWavelengths[j] = int(Wavelengths[j]);  //Truncate float to int
    if ( (TruncatedWavelengths[j] % 2) == 0) Multiplier[j] = 1;  //If an even integer multiple of half test periods has passed, multiplier is 1
    else Multiplier[j] = -1;  //Odd integer multiples get -1
    FTFArray[j] = Signal[j] * Multiplier[j];  //Get product of test frequency square wave multiplier array
    FTFSum[n] += FTFArray[j];  // "integrate" FTF array to test correlation of sample signal and test frequency square wave
    }
  }
  FTFFit = abs(FTFSum[0]) + abs(FTFSum[1]);  //Sum two fourier transforms 90 degrees out of phase to account for phase shift
  return FTFFit;  //Returns correlation score of sample signal with test frequency
}

void Move(int in1[4], int in2[4], int enable[4], int command[4]) {
  for (int i = 0; i <= 3; i++) {
    digitalWrite2(in1[i], command[i]);
    digitalWrite2(in2[i], -1*command[i]);
    digitalWrite2(enable[i], command[i]*command[i]);
    Serial.print(command[i]);
    Serial.print(" ");
  }
}

void digitalWrite2(int pin, int value) {
  if (value == 1) digitalWrite(pin, HIGH);
  if (value == 0) digitalWrite(pin, LOW);
}




