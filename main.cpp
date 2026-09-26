#include <iostream>
#include "OrderBook.h"

int main() 
{
    OrderBook orderBook;
    const OrderId orderId = 1;
    
    orderBook.addOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout << "OrderBook Size: " << orderBook.Size() << std::endl; // 1
    
    orderBook.cancelOrder(orderId);
    std::cout << "OrderBook Size: " << orderBook.Size() << std::endl; // 0
    
    return 0;
}