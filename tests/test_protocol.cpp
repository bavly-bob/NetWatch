#include "protocol.hpp"
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace netwatch::protocol;

TEST(ProtocolTest, EncodeSize)
{
    const std::string payload = "hello";
    const std::string frame = encode(payload);
    EXPECT_EQ(frame.size(), static_cast<size_t>(4 + payload.size()));
}

TEST(ProtocolTest, EncodeHeaderValue)
{
    const std::string payload = "test message";
    const std::string frame = encode(payload);
    const uint32_t decoded_len = decodeHeader(frame.data());
    EXPECT_EQ(decoded_len, static_cast<uint32_t>(payload.size()));
}

TEST(ProtocolTest, EncodeBody)
{
    const std::string payload = "{\"type\":\"heartbeat\"}";
    const std::string frame = encode(payload);
    const std::string body = frame.substr(4);
    EXPECT_EQ(body, payload);
}

TEST(ProtocolTest, DecodeHeader)
{
    const std::string payload = "some data here";
    const std::string frame = encode(payload);
    const uint32_t len = decodeHeader(frame.data());
    EXPECT_EQ(len, static_cast<uint32_t>(payload.size()));
}

TEST(ProtocolTest, EmptyPayload)
{
    const std::string payload = "";
    const std::string frame = encode(payload);
    EXPECT_EQ(frame.size(), static_cast<size_t>(4));

    const uint32_t len = decodeHeader(frame.data());
    EXPECT_EQ(len, static_cast<uint32_t>(0));
}

TEST(ProtocolTest, LargePayload)
{
    const std::string payload(100000, 'X');
    const std::string frame = encode(payload);

    const uint32_t len = decodeHeader(frame.data());
    EXPECT_EQ(len, static_cast<uint32_t>(payload.size()));

    const std::string body = frame.substr(4);
    EXPECT_EQ(body, payload);
}

TEST(ProtocolTest, ToStringMapping)
{
    EXPECT_EQ(toString(MessageType::Heartbeat), std::string("Heartbeat"));
    EXPECT_EQ(toString(MessageType::SystemStats), std::string("SystemStats"));
    EXPECT_EQ(toString(MessageType::ProcessList), std::string("ProcessList"));
    EXPECT_EQ(toString(MessageType::Unknown), std::string("Unknown"));
}

TEST(ProtocolTest, FromStringMapping)
{
    EXPECT_EQ(fromString("Heartbeat"), MessageType::Heartbeat);
    EXPECT_EQ(fromString("SystemStats"), MessageType::SystemStats);
    EXPECT_EQ(fromString("ProcessList"), MessageType::ProcessList);
    EXPECT_EQ(fromString("garbage"), MessageType::Unknown);
    EXPECT_EQ(fromString(""), MessageType::Unknown);
}

TEST(ProtocolTest, RoundtripTypeConversion)
{
    auto check = [](MessageType t) {
        EXPECT_EQ(fromString(toString(t)), t);
    };

    check(MessageType::Heartbeat);
    check(MessageType::SystemStats);
    check(MessageType::ProcessList);
    check(MessageType::Unknown);
}

TEST(ProtocolTest, MultipleFrames)
{
    const std::vector<std::string> payloads = {
        "first",
        "second message",
        "{\"key\":\"value\",\"num\":42}",
        std::string(500, 'Z')
    };

    for (const auto& payload : payloads)
    {
        const std::string frame = encode(payload);
        const uint32_t len = decodeHeader(frame.data());
        EXPECT_EQ(len, static_cast<uint32_t>(payload.size()));

        const std::string body = frame.substr(4);
        EXPECT_EQ(body, payload);
    }
}
