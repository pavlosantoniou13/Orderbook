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

enum class Side 
{
    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint32_t;