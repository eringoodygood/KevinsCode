Basics of using Kevin's Code: 

Calibration and FWHM: 
1. create symbolic link to data in same folder that globals.h and whatever else is in
	a. create symbolic link: ln -s /media/Spare1_G/folderwheredatais/ data
		this is the location of the *.wav or .dat files and data is the name of the link you create 
	b. remove it: rm data  
		where data is the name of the link
2. choose channel to analyze in global variables: this is in globals.h, which is the variable file for the actual code
3. tweak parameters appropriately and see what you get out using root -l readBin.C++
4. once parameters are all good, run in batch mode: root -b readBin.C++
5. files will be spit out into the output folder. open them using root -l wave0.root (or whatever channel number you are interested in
6. draw final data using T->Draw(“energy”)
7. measure peak ch # and FWHM


Fit Syntax in Root: using the fitpanel
How to fit the preamp output: 
1. fit pol0 to baseline, hit Fit
2. hit add, add exponential function
3. hit Set Parameters
4. Enter previous pol0 value, hit Fix, hit ok
5. fit expo portion on panel, hit Fit
6. You win! Also these might be the wrong order but you’ll figure it out



Notes on what all the parameters in globals.h are: 
For when the notes in the file aren’t good enough. 
-NSAMPLES: if you don’t have it right the code won’t run, but it will spit an error with the correct value
-ChanToAnalyze: duh. what channel on the digitizer do you want
-Tclk: sampling period in ns for the digitizer: look at notes in code that I wrote, this is a little weird

Trapezoidal Filter Parameters: look at trapezoid plot to adjust
-Tk: trapezoid rise time
-Tl: helps determine flat top time for trapezoid, which is equal to Tl-Tk
	good value for this is 2 greater than Tk
-Ttau: time constant of exp. decay of preamp pulse. Either get this from the preamp specifications or measure it using fit panel when you are looking at the initial preamp pulses with root -l readBin.C++
-Tdelay: delay of flat top measurement. ideally you want to measure the second half(ish) of the trapezoid’s flat top
-Twidth: this is how much of the flat top you’ll be measuring of the flat top after the delay. in ns

First Derivative Filter Parameters: look at bottom left window 
-Dk: first derivative of tk
-Dt: must be 2 or more greater than Dk
 
Second derivative Filter parameters: look at bottom right window
-DDk: second derivative of tk
-DDt: must be 2 or more greater than DDk
-DDr: must be 2 or more greater than DDr

Trigger/Timing Parameters: These matter! 
-polarity: duh. pick appropriately for your preamp waveforms
-thresh: threshold for second derivative trigger, look at 2nd deriv. window to adjust
-trigDelay: amount of time allowed for 2nd derivative zero crossing (and thus an event) to be found
-trigHO: only allow double triggers after this amount of time after first one
NOTE THAT THESE LAST THREE PARAMETERS are all related, so adjusting more than one at a time is not recommended. Make sure you do a test of which of all these is going to be optimal for you 


For more information, look up recursive digital filtering on google, ask Erin about Kevin’s cool little document on it, take an advanced course on convolution of functions for signal processing, or check out these papers that I found helpful: 

“Digital Synthesis of pulse shapes in real time for high resolution radiation spectroscopy” by Valentin T. Jordan and Glenn F. Knoll in NIMA 345 (1994) 337-345. 

CAEN’s White Paper (WP2081) “Digital Pulse Processing in Nuclear Physics” Rev. 3, August 26, 2011. 



Best of luck to you! And remember, practice makes perfect ;)

