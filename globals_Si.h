// Begin Global Variable Playtime

const int NSAMPLES  = 128;  // should be set in the wavedump config file. 
                              // the code will exit with an error if this value is wrong. The code will tell you what it is though.
const int ChanToAnalyze = 0; // PC channels: 5, 7, 9 , 11, 12, 13, 14, 15
const int Tclk = 4;           // Sampling period in nanoseconds for the 1730 digitizer (172* models=4, 1730=2) Comes out to half the actual sampling period for some reason. OR SO I THINK; this might not be as true as I first thought

// Trapezoidal Filter Parameters
const int Tk = 10;          //  This code is assuming the convention k<l !!!!!!
const int Tl = 25;          //  If l>k, The flat top time m = l-k.
//const int Ttau = 59.7e3; // 1/e Decay Time in nanoseconds. (high gain 60 mV/MeV)
const int Ttau = 20e3;        // (pulser data) 11e3; 1/e (27 mV/MeV chip)
const int Tdelay = 5;         // flat top delay (only works for Tdelay >= 0 currently)
const int Twidth = 15;       // width of flat top to average for the energy 

// First Derivative Filter Parameters 
const int Dk = 10; //98 57
const int Dt = 11; //100 MUST HAVE: Dt >= Dk + 1 !!!!!!  59

// Second Derivative Filter Parameters
const int DDk = 10;  //55 96
const int DDt = 11;  //57 98
const int DDr = 12;  //59 100

// Trigger/Timing Parameters
const int polarity = -1; // 1 = positive, -1 = negative
const double thresh = 2000;
; // Threshold for second derivative trigger
const int trigDelay = 100; // Look for the first zero crossing in the second derivative within
                            // a fixed time window after being armed.

const int trigHO = 100; // Only allow double triggers if separated by a time trigHO

// End of Global Variable playground
