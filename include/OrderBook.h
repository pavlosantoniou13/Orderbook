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
    mutable std::mutex orderdMutex_;
    std::thread ordersPruneThread_;
    std::condition_variable shutdownConditionVariable_;
    std::atomic<bool> shutdown_{ false };

    void PruneGoodForDayOrders();

    void cancelOrders(OrderIds orderIds);
    void cancelOrderInternal(OrderId orderId);

    void onOrderCancelled(OrderPointer order);
    void onOrderAdded(OrderPointer order);
    void onOrderMatched(Price price, Quantity quantity, bool isFullyFilled);
    void updateLevelData(Price price, Quantity quantity, LevelData::Action action);

    bool canFullyFill(Side side, Price price, Quantity quantity) const;

    bool canMatch(Side side, Price price) const;
    bool canFullyFill(Side side, Price price, Quantity quantity) const;
    Trades MatchOrders();

public: 
    Trades addOrder(OrderPointer order);
    void cancelOrder(OrderId orderId);
    Trades MatchOrders(OrderModify order);
    std::size_t Size() const { return orders_.size(); }
    OrderBookLevelInfos getOrderInfos() const;
};