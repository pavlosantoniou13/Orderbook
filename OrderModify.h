#pragma once
#include <memory>
#include "OrderTypes.h"
#include "Order.h"

class OrderModify
{
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_{ orderId }, price_{ price }, side_{ side }, quantity_{ quantity }
    { }

    OrderId getOrderId() const { return orderId_; }
    Price getPrice() const { return price_; }
    Side getSide() const { return side_; }
    Quantity getQuantity() const { return quantity_; }

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