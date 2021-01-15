#pragma once

#include <easy/profiler.h>

#define SOL_PFUN(...) EASY_BLOCK(__PRETTY_FUNCTION__, __VA_ARGS__)
#define SOL_PBLOCK(...) EASY_BLOCK(__VA_ARGS__)
