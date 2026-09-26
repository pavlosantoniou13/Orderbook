#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include "OrderTypes.h"
#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "OrderBookLevelInfos.h"

class OrderBook
{
private:
    struct OrderEntry
    {
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool canMatch(Side side, Price price) const;
    Trades MatchOrders();

public: 
    Trades addOrder(OrderPointer order);
    void cancelOrder(OrderId orderId);
    Trades MatchOrders(OrderModify order);
    std::size_t Size() const { return orders_.size(); }
    OrderBookLevelInfos getOrderInfos() const;
};