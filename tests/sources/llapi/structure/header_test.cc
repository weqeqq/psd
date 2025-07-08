#include <gtest/gtest.h>
#include <psd/llapi/structure/header.h>
#include <psd/llapi/stream.h>

using namespace PSD::llapi;

class HeaderTest : public ::testing::Test {};

// Test Version enum
TEST_F(HeaderTest, VersionFromStreamValid) {
    std::vector<U8> data = {0x00, 0x01}; // Version::PSD
    Stream stream(data);
    Version version;

    EXPECT_NO_THROW(stream.ReadTo(version));
    EXPECT_EQ(version, Version::PSD);
}

TEST_F(HeaderTest, VersionFromStreamValidPSB) {
    std::vector<U8> data = {0x00, 0x02}; // Version::PSB
    Stream stream(data);
    Version version;

    EXPECT_NO_THROW(stream.ReadTo(version));
    EXPECT_EQ(version, Version::PSB);
}

TEST_F(HeaderTest, VersionFromStreamInvalid) {
    std::vector<U8> data = {0x00, 0x03}; // Invalid version
    Stream stream(data);
    Version version;

    EXPECT_THROW(stream.ReadTo(version), PSD::Error);
}

TEST_F(HeaderTest, VersionToStream) {
    Stream stream;
    Version version = Version::PSD;

    EXPECT_NO_THROW(stream.Write(version));

    stream.SetOffset(0);
    Version read_version;
    stream.ReadTo(read_version);
    EXPECT_EQ(read_version, Version::PSD);
}

// Test Depth enum
TEST_F(HeaderTest, DepthFromStreamValid) {
    std::vector<U8> data = {0x00, 0x08}; // Depth::Eight
    Stream stream(data);
    Depth depth;

    EXPECT_NO_THROW(stream.ReadTo(depth));
    EXPECT_EQ(depth, Depth::Eight);
}

TEST_F(HeaderTest, DepthFromStreamValidAll) {
    std::vector<Depth> valid_depths = {Depth::One, Depth::Eight, Depth::Sixteen, Depth::ThirtyTwo};
    std::vector<U16> depth_values = {1, 8, 16, 32};

    for (size_t i = 0; i < valid_depths.size(); ++i) {
        std::vector<U8> data = {
            static_cast<U8>(depth_values[i] >> 8),
            static_cast<U8>(depth_values[i] & 0xFF)
        };
        Stream stream(data);
        Depth depth;

        EXPECT_NO_THROW(stream.ReadTo(depth));
        EXPECT_EQ(depth, valid_depths[i]);
    }
}

TEST_F(HeaderTest, DepthFromStreamInvalid) {
    std::vector<U8> data = {0x00, 0x04}; // Invalid depth
    Stream stream(data);
    Depth depth;

    EXPECT_THROW(stream.ReadTo(depth), PSD::Error);
}

TEST_F(HeaderTest, DepthToStream) {
    Stream stream;
    Depth depth = Depth::Sixteen;

    EXPECT_NO_THROW(stream.Write(depth));

    stream.SetOffset(0);
    Depth read_depth;
    stream.ReadTo(read_depth);
    EXPECT_EQ(read_depth, Depth::Sixteen);
}

// Test Color enum
TEST_F(HeaderTest, ColorFromStream) {
    std::vector<U8> data = {0x00, 0x03}; // Color::Rgb
    Stream stream(data);
    Color color;

    EXPECT_NO_THROW(stream.ReadTo(color));
    EXPECT_EQ(color, Color::Rgb);
}

TEST_F(HeaderTest, ColorToStream) {
    Stream stream;
    Color color = Color::Cmyk;

    EXPECT_NO_THROW(stream.Write(color));

    stream.SetOffset(0);
    Color read_color;
    stream.ReadTo(read_color);
    EXPECT_EQ(read_color, Color::Cmyk);
}

// Test Header class
TEST_F(HeaderTest, HeaderDefaultConstruction) {
    Header header;

    EXPECT_EQ(header.version, Version::PSD);
    EXPECT_EQ(header.channel_count, 0);
    EXPECT_EQ(header.row_count, 0);
    EXPECT_EQ(header.column_count, 0);
    EXPECT_EQ(header.depth, Depth::Eight);
    EXPECT_EQ(header.color, Color::Rgb);
}

TEST_F(HeaderTest, HeaderEqualityOperator) {
    Header header1;
    Header header2;

    EXPECT_EQ(header1, header2);

    header2.channel_count = 3;
    EXPECT_NE(header1, header2);
}

TEST_F(HeaderTest, HeaderFromStreamValid) {
    std::vector<U8> data = {
        0x38, 0x42, 0x50, 0x53, // Signature "8BPS"
        0x00, 0x01,             // Version::PSD
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6 reserved bytes
        0x00, 0x03,             // channel_count = 3
        0x00, 0x00, 0x01, 0x00, // row_count = 256
        0x00, 0x00, 0x01, 0x00, // column_count = 256
        0x00, 0x08,             // depth = 8
        0x00, 0x03              // color = RGB
    };
    Stream stream(data);
    Header header;

    EXPECT_NO_THROW(stream.ReadTo(header));
    EXPECT_EQ(header.version, Version::PSD);
    EXPECT_EQ(header.channel_count, 3);
    EXPECT_EQ(header.row_count, 256);
    EXPECT_EQ(header.column_count, 256);
    EXPECT_EQ(header.depth, Depth::Eight);
    EXPECT_EQ(header.color, Color::Rgb);
}

TEST_F(HeaderTest, HeaderFromStreamInvalidSignature) {
    std::vector<U8> data = {
        0x38, 0x42, 0x50, 0x54, // Invalid signature
        0x00, 0x01,             // Version::PSD
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6 reserved bytes
        0x00, 0x03,             // channel_count = 3
        0x00, 0x00, 0x01, 0x00, // row_count = 256
        0x00, 0x00, 0x01, 0x00, // column_count = 256
        0x00, 0x08,             // depth = 8
        0x00, 0x03              // color = RGB
    };
    Stream stream(data);
    Header header;

    EXPECT_THROW(stream.ReadTo(header), PSD::Error);
}

TEST_F(HeaderTest, HeaderToStream) {
    Header header;
    header.version = Version::PSB;
    header.channel_count = 4;
    header.row_count = 512;
    header.column_count = 1024;
    header.depth = Depth::Sixteen;
    header.color = Color::Cmyk;

    Stream stream;
    EXPECT_NO_THROW(stream.Write(header));

    stream.SetOffset(0);
    Header read_header;
    stream.ReadTo(read_header);

    EXPECT_EQ(read_header, header);
}

TEST_F(HeaderTest, HeaderRoundTrip) {
    Header original;
    original.version = Version::PSD;
    original.channel_count = 3;
    original.row_count = 1080;
    original.column_count = 1920;
    original.depth = Depth::Eight;
    original.color = Color::Rgb;

    Stream stream;
    stream.Write(original);

    stream.SetOffset(0);
    Header read_header;
    stream.ReadTo(read_header);

    EXPECT_EQ(read_header, original);
}
