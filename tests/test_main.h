#pragma once

#include <cstdlib>
#include <iostream>

#define CHECK(cond) do { if(!(cond)) { std::cerr << "CHECK failed: " #cond " at " << __FILE__ << ':' << __LINE__ << "\n"; std::exit(1);} } while(0)
