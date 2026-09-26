#pragma once
#include <vector>
#include "OrderTypes.h"

struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderBookLevelInfos
{
public:
    OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
        : bids_{ bids }, asks_{ asks }
    { }

    const LevelInfos& getBids() const { return bids_; }
    const LevelInfos& getAsks() const { return asks_; }

private: 
    LevelInfos bids_;
    LevelInfos asks_;
};