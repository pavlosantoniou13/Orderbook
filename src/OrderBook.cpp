#include "OrderBook.h"
#include <numeric>
#include <algorithm>

bool OrderBook::canMatch(Side side, Price price) const 
{
    if (side == Side::Buy)
    {
        if (asks_.empty()) return false;
        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else 
    {
        if (bids_.empty()) return false;
        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

Trades OrderBook::MatchOrders() 
{
    Trades trades;
    trades.reserve(orders_.size());
    
    while (!bids_.empty() && !asks_.empty()) 
    {
        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice) break;
        
        while (!bids.empty() && !asks.empty())
        {
            auto& bid = bids.front();
            auto& ask = asks.front();

            Quantity quantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());
            bid->Fill(quantity);
            ask->Fill(quantity);

            if (bid->isFilled())
            {
                bids.pop_front();
                orders_.erase(bid->getOrderId());
            }

            if (ask->isFilled())
            {
                asks.pop_front();
                orders_.erase(ask->getOrderId());
            }

            if (bids.empty()) bids_.erase(bidPrice);
            if (asks.empty()) asks_.erase(askPrice);

            trades.push_back(Trade{
                TradeInfo{ bid->getOrderId(), bid->getPrice(), quantity },
                TradeInfo{ ask->getOrderId(), ask->getPrice(), quantity }
            });
        }

        if (!bids_.empty() && !bids_.begin()->second.empty())
        {
            auto& order = bids_.begin()->second.front();
            if (order->getOrderType() == OrderType::FillAndKill)
                cancelOrder(order->getOrderId());
        }
        
        if (!asks_.empty() && !asks_.begin()->second.empty())
        {
            auto& order = asks_.begin()->second.front();
            if (order->getOrderType() == OrderType::FillAndKill)
                cancelOrder(order->getOrderId());
        }
    }
    return trades;
}

Trades OrderBook::addOrder(OrderPointer order)
{
    if (orders_.contains(order->getOrderId()))
        return { };

    if (order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice()))
        return { };

    OrderPointers::iterator iterator;

    if (order->getSide() == Side::Buy)
    {
        auto& orders = bids_[order->getPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }
    else
    {
        auto& orders = asks_[order->getPrice()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({ order->getOrderId(), OrderEntry{ order, iterator } });
    return MatchOrders();
}

void OrderBook::cancelOrder(OrderId orderId)
{
    if (!orders_.contains(orderId)) return;

    const auto& [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->getSide() == Side::Sell)
    {
        auto price = order->getPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty()) asks_.erase(price);
    }
    else
    {
        auto price = order->getPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty()) bids_.erase(price);
    }
}

Trades OrderBook::MatchOrders(OrderModify order)
{
    if (!orders_.contains(order.getOrderId())) return {};
    const auto& [existingOrder, _] = orders_.at(order.getOrderId());
    cancelOrder(order.getOrderId());
    return addOrder(order.ToOrderPointer(existingOrder->getOrderType()));
}

OrderBookLevelInfos OrderBook::getOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto createLevelInfos = [](Price price, const OrderPointers& orders)
    {
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), static_cast<Quantity>(0),
            [](Quantity runingSum, const OrderPointer& order)
            { return runingSum + order->getRemainingQuantity(); }) };
    };

    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(createLevelInfos(price, orders));

    for (const auto& [price, orders] : asks_)
        askInfos.push_back(createLevelInfos(price, orders));
    
    return OrderBookLevelInfos{ bidInfos, askInfos }; 
}