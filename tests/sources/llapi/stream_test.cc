
#include <psd/llapi/stream.h>

#include <gtest/gtest.h>
#include <vector>
#include <filesystem>
#include <fstream>

class StreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        test_file_path_ = std::filesystem::temp_directory_path() / "stream_test.dat";
        std::ofstream file(test_file_path_, std::ios::binary);
        // Write some test data
        std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        file.write(reinterpret_cast<const char*>(test_data.data()), test_data.size());
        file.close();
    }

    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(test_file_path_)) {
            std::filesystem::remove(test_file_path_);
        }
    }

    std::filesystem::path test_file_path_;
};

TEST_F(StreamTest, DefaultConstructor) {
    PSD::llapi::Stream stream;
    // Should not throw and should be in valid state
    EXPECT_NO_THROW(stream.SetOffset(0));
}

TEST_F(StreamTest, FileConstructor) {
    EXPECT_NO_THROW(PSD::llapi::Stream stream(test_file_path_));
}

TEST_F(StreamTest, ReadU8) {
    PSD::llapi::Stream stream(test_file_path_);
    auto value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x01);
}

TEST_F(StreamTest, ReadU16) {
    PSD::llapi::Stream stream(test_file_path_);
    auto value = stream.Read<PSD::llapi::U16>();
    // Assuming big-endian format (PSD standard)
    EXPECT_EQ(value, 0x0102);
}

TEST_F(StreamTest, ReadU32) {
    PSD::llapi::Stream stream(test_file_path_);
    auto value = stream.Read<PSD::llapi::U32>();
    EXPECT_EQ(value, 0x01020304);
}

TEST_F(StreamTest, ReadU64) {
    PSD::llapi::Stream stream(test_file_path_);
    auto value = stream.Read<PSD::llapi::U64>();
    EXPECT_EQ(value, 0x0102030405060708ULL);
}

TEST_F(StreamTest, ReadSignedTypes) {
    PSD::llapi::Stream stream(test_file_path_);
    auto i8_value = stream.Read<PSD::llapi::I8>();
    EXPECT_EQ(i8_value, 0x01);

    stream.SetOffset(0);
    auto i16_value = stream.Read<PSD::llapi::I16>();
    EXPECT_EQ(i16_value, 0x0102);

    stream.SetOffset(0);
    auto i32_value = stream.Read<PSD::llapi::I32>();
    EXPECT_EQ(i32_value, 0x01020304);

    stream.SetOffset(0);
    auto i64_value = stream.Read<PSD::llapi::I64>();
    EXPECT_EQ(i64_value, 0x0102030405060708LL);
}

TEST_F(StreamTest, WriteU8) {
    PSD::llapi::Stream stream;
    stream.Write<PSD::llapi::U8>(0x42);
    stream.SetOffset(0);
    auto value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x42);
}

TEST_F(StreamTest, WriteU16) {
    PSD::llapi::Stream stream;
    stream.Write<PSD::llapi::U16>(0x1234);
    stream.SetOffset(0);
    auto value = stream.Read<PSD::llapi::U16>();
    EXPECT_EQ(value, 0x1234);
}

TEST_F(StreamTest, WriteU32) {
    PSD::llapi::Stream stream;
    stream.Write<PSD::llapi::U32>(0x12345678);
    stream.SetOffset(0);
    auto value = stream.Read<PSD::llapi::U32>();
    EXPECT_EQ(value, 0x12345678);
}

TEST_F(StreamTest, WriteU64) {
    PSD::llapi::Stream stream;
    stream.Write<PSD::llapi::U64>(0x123456789ABCDEF0ULL);
    stream.SetOffset(0);
    auto value = stream.Read<PSD::llapi::U64>();
    EXPECT_EQ(value, 0x123456789ABCDEF0ULL);
}

TEST_F(StreamTest, ReadIterator) {
    PSD::llapi::Stream stream(test_file_path_);
    std::vector<PSD::llapi::U8> buffer(4);
    stream.Read(buffer.begin(), buffer.end());

    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_EQ(buffer[2], 0x03);
    EXPECT_EQ(buffer[3], 0x04);
}

TEST_F(StreamTest, WriteIterator) {
    PSD::llapi::Stream stream;
    std::vector<PSD::llapi::U8> data = {0x10, 0x20, 0x30, 0x40};
    stream.Write(data.begin(), data.end());

    stream.SetOffset(0);
    std::vector<PSD::llapi::U8> buffer(4);
    stream.Read(buffer.begin(), buffer.end());

    EXPECT_EQ(buffer, data);
}

TEST_F(StreamTest, OffsetOperators) {
    PSD::llapi::Stream stream(test_file_path_);

    stream += 2;
    auto value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x03);

    stream -= 1;
    value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x03);

    ++stream;
    value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x05);

    --stream;
    --stream;
    value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x04);
}

TEST_F(StreamTest, SetOffset) {
    PSD::llapi::Stream stream(test_file_path_);

    stream.SetOffset(3);
    auto value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x04);

    stream.SetOffset(0);
    value = stream.Read<PSD::llapi::U8>();
    EXPECT_EQ(value, 0x01);
}

TEST_F(StreamTest, ReadOverflow) {
    PSD::llapi::Stream stream(test_file_path_);

    // Read all data
    stream.Read<PSD::llapi::U64>();

    // Try to read more - should throw
    EXPECT_THROW(stream.Read<PSD::llapi::U8>(), PSD::Error);
}

TEST_F(StreamTest, ReadFloatingPoint) {
    PSD::llapi::Stream stream;

    float test_float = 3.14159f;
    stream.Write(test_float);

    stream.SetOffset(0);
    float read_float = stream.Read<float>();
    EXPECT_FLOAT_EQ(read_float, test_float);

    stream.SetOffset(0);
    double test_double = 2.71828;
    stream.Write(test_double);

    stream.SetOffset(0);
    double read_double = stream.Read<double>();
    EXPECT_DOUBLE_EQ(read_double, test_double);
}

TEST_F(StreamTest, LargeDataWrite) {
    PSD::llapi::Stream stream;
    std::vector<PSD::llapi::U32> large_data(1000);

    // Fill with test pattern
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<PSD::llapi::U32>(i);
    }

    stream.Write(large_data.begin(), large_data.end());

    stream.SetOffset(0);
    std::vector<PSD::llapi::U32> read_data(1000);
    stream.Read(read_data.begin(), read_data.end());

    EXPECT_EQ(read_data, large_data);
}
