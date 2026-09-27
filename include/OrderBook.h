#pragma once
#include <map>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"
#include "OrderBookLevelInfos.h"
#include "Trade.h"

class OrderBook
{
private:
    struct OrderEntry
    {
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    struct LevelData
    {
        Quantity quantity_{ };
        Quantity count_{ };

        enum class Action
        {
            Add,
            Remove,
            Match,
        };
    };

    std::unordered_map<Price, LevelData> data_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;
    mutable std::mutex ordersMutex_;
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
    Trades MatchOrders();

public: 
    OrderBook();
    OrderBook(const OrderBook&) = delete;
    void operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    void operator=(OrderBook&&) = delete;
    ~OrderBook();

    Trades addOrder(OrderPointer order);
    void cancelOrder(OrderId orderId);
    Trades ModifyOrder(OrderModify order); 
    std::size_t Size() const;
    OrderBookLevelInfos getOrderInfos() const;
};