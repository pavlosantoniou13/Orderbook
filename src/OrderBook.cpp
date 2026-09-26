#include "OrderBook.h"
#include <numeric>
#include <algorithm>
#include <chrono>
#include <ctime>

void OrderBook::PruneGoodForDayOrders()
{
    using namespace std::chrono;
    const auto end = hours(16);

    while (true)
    {
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);
        std::tm now_parts;
        localtime_s(&now_parts, &now_c);

        if (now_parts.tm_hour >= end.count())
            now_parts.tm_mday += 1;

        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        auto next = system_clock::from_time_t(mktime(&now_parts));
        auto till = next - now + milliseconds(100);
        {
            std::unique_lock ordersLock{ ordersMutex_ };

            if (shutdown_.load(std::memory_order_acquire) || 
                shutdownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::no_timeout)
                return;
        }

        OrderIds orderIds;

        {
            std::scoped_lock ordersLock{ ordersMutex_ };

            for (const auto& [_, entry] : orders_)
            {
                const auto& [order, _] = entry;
                
                if (order->getOrderType() != OrderType::GoodForDay)
                    continue;

                orderIds.push_back(order->getOrderId());
            }
        }

        cancelOrders(orderIds);
    }
}

void OrderBook::onOrderCancelled(OrderPointer order)
{
    updateLevelData(order->getPrice(), order->getRemainingQuantity(), LevelData::Action::Remove);
}


void OrderBook::cancelOrders(OrderIds orderIds)
{
    std::scoped_lock ordersLock { ordersMutex_ };

    for (const auto& orderId : orderIds)
        cancelOrderInternal(orderId);
}

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

bool OrderBook::canFullyFill(Side side, Price price, Quantity quantity) const
{
	if (!canMatch(side, price))
		return false;

	std::optional<Price> threshold;

	if (side == Side::Buy)
	{
		const auto [askPrice, _] = *asks_.begin();
		threshold = askPrice;
	}
	else
	{
		const auto [bidPrice, _] = *bids_.begin();
		threshold = bidPrice;
	}

	for (const auto& [levelPrice, levelData] : data_)
	{
		if (threshold.has_value() &&
			(side == Side::Buy && threshold.value() > levelPrice) ||
			(side == Side::Sell && threshold.value() < levelPrice))
			continue;

		if ((side == Side::Buy && levelPrice > price) ||
			(side == Side::Sell && levelPrice < price))
			continue;

		if (quantity <= levelData.quantity_)
			return true;

		quantity -= levelData.quantity_;
	}

	return false;
}

Trades OrderBook::addOrder(OrderPointer order)
{
    if (orders_.contains(order->getOrderId()))
        return { };

    if (order->getOrderType() == OrderType::Market)
        {
            if (order->getSide() == Side::Buy && !asks_.empty())
            {
                const auto& [worstAsk, _] = *asks_.rbegin();
                order->toGoodTillCancel(worstAsk);
            }
            else if (order->getSide() == Side::Sell && !bids_.empty())
            {
                const auto& [worstBid, _] = *bids_.rbegin();
                order->toGoodTillCancel(worstBid);
            }
            else
                return { };

        }

    if (order->getOrderType() == OrderType::FillAndKill && !canMatch(order->getSide(), order->getPrice()))
        return { };
    
    if (order->getOrderType() == OrderType::FillOrKill && !canFullyFill(order->getSide(), order->getPrice(), order->getInitialQuantity()))
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

    onOrderAdded(order);

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