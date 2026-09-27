#include <gtest/gtest.h>
#include "OrderBook.h"
#include <filesystem>
#include <fstream>

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

struct InputHandler
{
private:
    std::uint64_t toNumber(const std::string_view& str) const
    {
        std::int64_t value{};
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0) 
            throw std::logic_error("Value is below zero.");
        return static_cast<std::uint64_t>(value);
    }

    bool tryParseResult(const std::string_view& str, Result& result) const
    {
        if (str.at(0) != 'R')
            return false;
        
        auto values = Split(str, ' '); // R 0 0 0
        result.allCount_ = toNumber(values.at(1));
        result.bidCount_ = toNumber(values.at(2));
        result.askCount_ = toNumber(values.at(3));

        return true;
    }

    bool tryParseInformation(const std::string_view& str, Information& info) const
    {
        auto value = str.at(0);
        auto values = Split(str, ' ');
        if (value == 'A')
        {
            info.type_ = ActionType::Add;
            info.side_ = parseSide(values.at(1));
            info.orderType_ = parseOrderType(values.at(2));
            info.price_ = parsePrice(values.at(3));
            info.quantity_ = parseQuantity(values.at(4)) ;
            info.orderId_ = parseOrderId(values.at(5));
        }
        else if (value == 'M')
        {
            info.type_ = ActionType::Modify;
            info.orderId_ = parseOrderId(values.at(1));
            info.side_ = parseSide(values.at(2));
            info.price_ = parsePrice(values.at(3));
            info.quantity_ = parseQuantity(values.at(4)) ;
        }
        else if (value == 'C')
        {
            info.type_ = ActionType::Cancel;
            info.orderId_ = parseOrderId(values.at(1));

        }
        else return false;

        return true;
    }

    std::vector<std::string_view> Split(const std::string_view& str, char delimeter) const
    {
        std::vector<std::string_view> columns{};
        std::size_t startIndex{}, endIndex{};
        while ((endIndex = str.find(delimeter, startIndex)) && endIndex != std::string::npos)
        {
            auto distance = endIndex - startIndex;
            auto column = str.substr(startIndex, distance);
            startIndex = endIndex + 1;
            columns.push_back(column);
        }

        columns.push_back(str.substr(startIndex));
        return columns;
    }

    Side parseSide(const std::string_view& str) const
    {
        if (str == "B")
            return Side::Buy;
        else if (str == "S")
            return Side::Sell;
        else throw std::logic_error("");
    }

    OrderType parseOrderType(const std::string_view& str) const
    {
        if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "GoodForDay")
            return OrderType::GoodForDay;
        else if (str == "FillOrKill")
            return OrderType::FillOrKill;
        else if (str == "Market")
            return OrderType::Market;
        else throw std::logic_error("Unknown OrderType");
    }

    Price parsePrice(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Price");
        
        return toNumber(str);
    }

    Quantity parseQuantity(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity");
        
        return toNumber(str);
    }

    OrderId parseOrderId(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown OrderId");
        
        return toNumber(str);
    }

public:

    std::tuple<Informations, Result> getInformations(const std::filesystem::path& path) const
    {
        Informations infos;
        infos.reserve(1000);

        std::string line;
        std::ifstream file{ path };
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (line.empty())
                continue;

            const bool isResult = line.at(0) == 'R';
            const bool isUpdate = !isResult;

            if (isUpdate)
            {
                Information update;

                auto isValid = tryParseInformation(line, update);
                if (!isValid)
                    throw;

                infos.push_back(update);
            }
            else
            {
                if (!file.eof())
                    throw std::logic_error("Result must be at the end of the file only");

                Result result;

                auto isValid = tryParseResult(line, result);
                if (!isValid)
                    continue;
                
                return { infos, result };
            }
        }

        throw std::logic_error("No result specified");
    }
};

class OrderBookTestsFixture : public googletest::TestWithParam<const char*>
{
    private:
        const static inline std::filesystem::path TestFiles { ORDERBOOK_TEST_FILES_DIR };
    public:
        const static inline std::filesystem::path TestFolderPath { TestFiles };
};

TEST_P(OrderBookTestsFixture, OrderBookTestSuite)
{
    // Arrange
    const auto file = OrderBookTestsFixture::TestFolderPath / GetParam();

    InputHandler handler;
    const auto [updates, result] = handler.getInformations(file);



    auto getOrder = [](const Information& information)
    {
        return std::make_shared<Order>(
            information.orderType_,
            information.orderId_,
            information.side_,
            information.price_,
            information.quantity_
        );
    };

    auto getModifyOrder = [](const Information& information)
    {
        return OrderModify
        {
            information.orderId_,
            information.side_,
            information.price_,
            information.quantity_

        };
    };

    // Act
    OrderBook orderBook;
    for (const auto& update : updates)
    {
        switch (update.type_)
        {
            case ActionType::Add:
            {
                const Trades& trades = orderBook.addOrder(getOrder(update));
            }
            break;
            case ActionType::Modify:
            {
                const Trades& trades = orderBook.ModifyOrder(getModifyOrder(update));
            }
            break;
            case ActionType::Cancel:
            {
                orderBook.cancelOrder(update.orderId_);
            }
            break;
            default:
                throw std::logic_error("Unsupported Update");
        }
    }

    // Assert
    const auto& orderBookInfos = orderBook.getOrderInfos();
    ASSERT_EQ(orderBook.Size(), result.allCount_);
    ASSERT_EQ(orderBookInfos.getBids().size(), result.bidCount_);
    ASSERT_EQ(orderBookInfos.getAsks().size(), result.askCount_);

}

INSTANTIATE_TEST_SUITE_P(Tests, OrderBookTestsFixture, googletest::ValuesIn({
    "Match_GoodTillCancel.txt",
    "Match_FillAndKill.txt",
    "Match_FillOrKill_Hit.txt",
    "Match_FillOrKill_Miss.txt",
    "Cancel_Success.txt",
    "Modify_Side.txt",
    "Match_Market.txt"
}));