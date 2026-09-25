#include <iostream>
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <deque>
#include <stack>
#include <limits>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>
#include <tuple>
#include <format>

enum class OrderType 
{
    GoodTillCancel,
    FillAndKill
};

enum class Side 
{
    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint32_t;

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
        : bids_{ bids }
        , asks_{ asks }
        { }

        const LevelInfos& getBids() const {return bids_; }
        const LevelInfos& getAsks() const {return asks_; }

    private: 
        LevelInfos bids_;
        LevelInfos asks_;
};

class Order 
{
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_{ orderType }
        , orderId_{ orderId }
        , side_{ side }
        , price_{ price }
        , initialQuantity_{ quantity }
        , remainingQuantity_{ quantity }
    { }
    
    OrderId getOrderId() const { return orderId_; }
    Side getSide() const { return side_; }
    Price getPrice() const { return price_; }
    OrderType getOrderType() const { return orderType_; }
    Quantity getInitialQuantity() const { return initialQuantity_; }
    Quantity getRemainingQuantity() const { return remainingQuantity_; }
    Quantity getFilledQuantity() const { return getInitialQuantity() - getRemainingQuantity(); }
    void Fill(Quantity quantity) {
        if (quantity > getRemainingQuantity())
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", getOrderId()));
        
        remainingQuantity_ -= quantity;
    }

    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify
{
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_{ orderId }
        , price_{ price }
        , side_{ side }
        , quantity_{ quantity }
        { }

        OrderId getOrderId() const { return orderId_; }
        Price getPrice() const { return price_; }
        Side getSide() const { return side_; }
        Quantity getQuantity() const { return quantity_ ;}

        OrderPointer ToOrderPointer(OrderType type) const 
        {
            return std::make_shared<Order>(type, getOrderId(), getSide(), getPrice(), getQuantity());
        }
    private:
        OrderId orderId_;
        Price price_;
        Side side_;
        Quantity quantity_;
};

struct TradeInfo
{
    OrderId orderId;
    Price price_;
    Quantity quantity_;
};

class Trade
{
    public:
        Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
        : bidTrade_{ bidTrade }
        , askTrade_{ askTrade }
        { }

        const TradeInfo& getBidTrade() const { return bidTrade_; }
        const TradeInfo& getAskTrade() const { return askTrade_; }

    private:
        TradeInfo bidTrade_;
        TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;





int main() {

    return 0;
}