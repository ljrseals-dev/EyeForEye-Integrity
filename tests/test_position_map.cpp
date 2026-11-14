#include "core/PositionMap.hpp"
#include <cmath>
#include <iostream>
int main(){eye::PositionMap m;if(!m.empty())return 1;m.record(0,10);m.record(2,12);m.record(1,11);m.record(std::nan(""),99);if(m.size()!=3||m.lookup(1.5).value_or(0)!=11)return 2;auto b=m.serialize();eye::PositionMap n;if(!n.deserialize(b)||n.lookup(2).value_or(0)!=12)return 3;b[0]^=std::byte{0xff};if(n.deserialize(b))return 4;m.freeze();m.record(3,13);if(m.size()!=3)return 5;m.unfreeze();m.reset();if(!m.empty())return 6;std::cout<<"PositionMap tests passed\n";}
