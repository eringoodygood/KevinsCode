// Begin Global Variable Playtime
// globals.h file having specific values for 113Sn weak data - 4 July 2017
// B.Sudarsan.
const int NSAMPLES  = 1030;  // should be set in the wavedump config file. 
                              // the code will exit with an error if this value is wrong. The code will tell you what it is though.
const int ChanToAnalyze = 2; // PC channels: 5, 7, 9 , 11, 12, 13, 14, 15
const int Tclk = 4;           // Sampling period in nanoseconds for the 1730 digitizer (172* models=4, 1730=2) Comes out to half the actual sampling period for some reason. OR SO I THINK; this might not be as true as I first thought

// Trapezoidal Filter Parameters
const int Tk = 9;          //  This code is assuming the convention k<l !!!!!!
const int Tl = 36;          //  If l>k, The flat top time m = l-k.
//const int Ttau = 59.7e3; // 1/e Decay Time in nanoseconds. (high gain 60 mV/MeV)
const int Ttau = 42;        // (pulser data) 11e3; 1/e (27 mV/MeV chip)
const int Tdelay = 2;         // flat top delay (only works for Tdelay >= 0 currently)
const int Twidth = 10;       // width of flat top to average for the energy 

// First Derivative Filter Parameters 
const int Dk = 8; //98 57
const int Dt = 10; //100 MUST HAVE: Dt >= Dk + 1 !!!!!!  59

// Second Derivative Filter Parameters
const int DDk = 8;  //55 96
const int DDt = 9;  //57 98
const int DDr = 10;  //59 100

// Trigger/Timing Parameters
const int polarity = -1; // 1 = positive, -1 = negative
const double thresh = 200;
; // Threshold for second derivative trigger
const int trigDelay = 100; // Look for the first zero crossing in the second derivative within
                            // a fixed time window after being armed.

const int trigHO = 100; // Only allow double triggers if separated by a time trigHO

// End of Global Variable playground
