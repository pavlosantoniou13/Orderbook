#include <gtest/gtest.h>
#include "OrderBook.h"

namespace googletest = ::testing;

enum class ActionType
{
    Add,
    Modify,
    Cancel,
};

struct Information
{
    ActionType type_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;

struct Result
{
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

