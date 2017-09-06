// Begin Global Variable Playtime

const int NSAMPLES  = 4100;  // should be *approximately* set in the wavedump config file
                              // the code will exit with an error if this value is wrong.
const int ChanToAnalyze = 3; // PC channels: 5, 7, 9 , 11, 12, 13, 14, 15
const int Tclk = 2;           // Sampling period in nanoseconds for the 1730 digitizer

// Trapezoidal Filter Parameters
const int Tk = 900;          //  This code is assuming the convention k<l !!!!!! 
const int Tl = 902;          //  If l>k, The flat top time m = l-k.
//const int Ttau = 59.7e3; // 1/e Decay Time in nanoseconds. (high gain 60 mV/MeV)
const int Ttau = 30e3;        // (pulser data) 11e3; 1/e (27 mV/MeV chip)
const int Tdelay = 50;         // flat top delay (only works for Tdelay >= 0 currently)
const int Twidth = 900;       // width of flat top to average for the energy 

// First Derivative Filter Parameters 
const int Dk = 900; // 
const int Dt = 901; // MUST HAVE: Dt >= Dk + 1 !!!!!!

// Second Derivative Filter Parameters
const int DDk = 850;
const int DDt = 851;
const int DDr = 852;//

// Trigger/Timing Parameters
const int polarity =-1; // 1 = positive, -1 = negative
const double thresh = 1000; // Threshold for second derivative trigger
const int trigDelay = 2000; // Look for the first zero crossing in the second derivative within
                            // a fixed time window

const int trigHO = 2000; // Only allow double triggers if separated by a time trigHO

// End of Global Variable playground
