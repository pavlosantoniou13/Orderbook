#pragma once
#include <cstdint>

enum class OrderType 
{
    GoodTillCancel,
    FillAndKill,
    FillOrKill,
    GoodForDay,
    Market

};

