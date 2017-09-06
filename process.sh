#!/bin/bash

echo 'test'

cp globals0.h globals.h
root -b -q readBin.C++

cp globals1.h globals.h
root -b -q readBin.C++

cp globals2.h globals.h
root -b -q readBin.C++

mv /home/daq/Desktop/WaveAnalysis/output/wave*.root /home/daq/Desktop/BDNScintTest/Data/output/  

