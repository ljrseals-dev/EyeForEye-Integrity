#include "core/ScaleEngine.hpp"
#include <cmath>
#include <iostream>
int main(){eye::ScaleEngine s;s.setScale(eye::ScaleType::Major,60);auto near=[](double a,double b){return std::fabs(a-b)<1e-9;};if(!near(s.getPitchRatio(60),1)||!near(s.getPitchRatio(72),2)||!near(s.getPitchRatio(61),1)||!near(s.getPitchRatio(63),std::pow(2.0,2.0/12.0)))return 1;if(std::fabs(s.getCentsDeviation(61)-100)>1e-4)return 2;if(s.snappedMidi(61)!=60||s.snappedMidi(63)!=62||s.snappedMidi(72)!=72)return 3;s.setScale(eye::ScaleType::Major,62);if(!near(s.getPitchRatio(62),1)||!near(s.getPitchRatio(63),1)||s.snappedMidi(63)!=62)return 4;s.setCustomMask(0);if(!near(s.getPitchRatio(63),std::pow(2.0,1.0/12.0))||s.snappedMidi(63)!=63)return 5;std::cout<<"ScaleEngine tests passed\n";return 0;}
