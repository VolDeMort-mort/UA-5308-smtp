#include <gtest/gtest.h>
#include "Base64Encoder.hpp"
#include "Base64Decoder.hpp"
#include "Base64StreamEncoder.hpp"
#include "Base64StreamDecoder.hpp"
#include "AttachmentHandler.hpp"

TEST(BasicEncoder, EmptyInput)
{
	std::vector<uint8_t> data{};
	auto encoded_data = Base64Encoder::EncodeBase64(data);

	EXPECT_EQ(encoded_data, "");
}

TEST(BasicEncoder, OneByte)
{
	std::vector<uint8_t> data{'A'};
	auto encoded_data = Base64Encoder::EncodeBase64(data);

	EXPECT_EQ(encoded_data, "QQ==");
}

TEST(BasicEncoder, TwoBytes)
{
	std::vector<uint8_t> data{ 'M', 'a'};
	auto encoded_data = Base64Encoder::EncodeBase64(data);

	EXPECT_EQ(encoded_data, "TWE=");
}

TEST(BasicEncoder, ThreeBytes)
{
	std::vector<uint8_t> data{ 'M', 'a', 'n'};
	auto encoded_data = Base64Encoder::EncodeBase64(data);

	EXPECT_EQ(encoded_data, "TWFu");
}

TEST(BasicEncoder, MoreBytes)
{
	std::vector<uint8_t> data{ 'H', 'e', 'l', 'l', 'o'};
	auto encoded_data = Base64Encoder::EncodeBase64(data);

	EXPECT_EQ(encoded_data, "SGVsbG8=");
}

TEST(BasicEncoder, LineBreak)
{
	std::vector<uint8_t> data(57, 'A');
	auto encoded_data = Base64Encoder::EncodeBase64(data);
	std::string expected;
	for (int i = 0; i < 19; ++i)
	{
		expected += "QUFB";
	}

	EXPECT_EQ(encoded_data, expected);
	EXPECT_EQ(encoded_data.size(), 76);
	EXPECT_EQ(encoded_data.find("\r\n"), std::string::npos);
}

TEST(BasicEncoder, FileSizeCheck)
{
	std::vector<uint8_t> data(15 * 1024 * 1024 + 1);

	EXPECT_THROW(Base64Encoder::EncodeBase64(data), std::runtime_error);
}

TEST(BasicDecoder, SimpleStr)
{
	std::string encoded = "TWFu";
	auto decoded_v = Base64Decoder::DecodeBase64(encoded);
	std::string decoded(decoded_v.begin(), decoded_v.end());

	EXPECT_EQ(decoded, "Man");
}

TEST(BasicDecoder, Padding)
{
	std::string encoded = "TWE=";
	auto decoded_v = Base64Decoder::DecodeBase64(encoded);
	std::string decoded(decoded_v.begin(), decoded_v.end());

	EXPECT_EQ(decoded, "Ma");
}

TEST(BasicDecoder, NotValidSymbol)
{
	std::string encoded = "TWFu$";
	EXPECT_THROW({ auto decoded_v = Base64Decoder::DecodeBase64(encoded);}, std::runtime_error);
}

TEST(BasicDecoder, LengthCheck)
{
	std::string encoded = "TWF";
	EXPECT_THROW({ auto decoded_v = Base64Decoder::DecodeBase64(encoded); }, std::runtime_error);
}

TEST(StreamEncoder, OneCall)
{
	Base64StreamEncoder encoder;
	std::stringstream out;
	const uint8_t data[] = { 'M','a','n' };
	encoder.EncodeStream(data, 3, out);
	encoder.Finalize(out);
	std::string result = out.str();

	EXPECT_EQ(result, "TWFu\r\n");
}

TEST(StreamEncoder, OneByteRecv)
{
	Base64StreamEncoder encoder;
	std::stringstream out;

	const uint8_t b1[] = { 'M' };
	const uint8_t b2[] = { 'a' };
	const uint8_t b3[] = { 'n' };

	encoder.EncodeStream(b1, 1, out);
	encoder.EncodeStream(b2, 1, out);
	encoder.EncodeStream(b3, 1, out);
	encoder.Finalize(out);
	std::string result = out.str();

	EXPECT_EQ(result, "TWFu\r\n");
}

TEST(StreamEncoder, LineBreak)
{
	std::vector<uint8_t> data(57, 'A');

	std::string expected;
	for (int i = 0; i < 19; ++i)
	{
		expected += "QUFB";
	}

	Base64StreamEncoder encoder;
	std::stringstream out;
	encoder.EncodeStream(data.data(), data.size(), out);
	encoder.Finalize(out);
	std::string encoded_data = out.str();

	EXPECT_EQ(encoded_data.substr(0, 76), expected);
	std::size_t pos = encoded_data.find("\r\n");
	EXPECT_EQ(pos, 76);
	EXPECT_EQ(encoded_data.size(), 78);
}

TEST(StreamDecoder, FourSymbols)
{
	Base64StreamDecoder decoder;
	std::stringstream out;
	decoder.DecodeStream("T", 1, out);
	decoder.DecodeStream("W", 1, out);
	decoder.DecodeStream("F", 1, out);
	decoder.DecodeStream("u", 1, out);
	decoder.Finalize(out);
	std::string result = out.str();

	EXPECT_EQ(result, "Man");
}

TEST(StreamDecoder, WordWrapping)
{
	Base64StreamDecoder decoder;
	std::stringstream out;
	std::string encoded = "SGVs\r\nbG8=";

	decoder.DecodeStream(static_cast<const char*>(encoded.data()), encoded.size(), out);
	decoder.Finalize(out);
	std::string result = out.str();

	EXPECT_EQ(result, "Hello");
}

TEST(StreamDecoder, NotFullBuffer)
{
	Base64StreamDecoder decoder;
	std::stringstream out;

	EXPECT_THROW({ decoder.DecodeStream("TWF", 3, out); decoder.Finalize(out); }, std::runtime_error);
}

TEST(AttachmentHandler, EncodeFile)
{
	std::string testFileName = "test_file.pptx";
	std::ofstream testFile(testFileName, std::ios::binary);

	std::vector<uint8_t> testContent(1024);
	for (size_t i = 0; i < testContent.size(); ++i)
	{
		testContent[i] = static_cast<uint8_t>(i % 256);
	}
	testFile.write(reinterpret_cast<const char*>(testContent.data()), testContent.size());
	testFile.close();

	std::string base64 = AttachmentHandler::EncodeFile(testFileName);

	EXPECT_FALSE(base64.empty());

	for (char c : base64)
	{
		if (c == '\r' || c == '\n') continue;
		EXPECT_TRUE((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' ||c == '/' || c == '=');
	}

	std::remove(testFileName.c_str());
}

TEST(AttachmentHandler, EncodeDecodeFile)
{
	std::string testFileName = "test_file.pptx";
	std::ofstream testFile(testFileName, std::ios::binary);

	std::vector<uint8_t> testContent(1024);
	for (size_t i = 0; i < testContent.size(); ++i)
	{
		testContent[i] = static_cast<uint8_t>(i % 256);
	}
	testFile.write(reinterpret_cast<const char*>(testContent.data()), testContent.size());
	testFile.close();

	std::string base64 = AttachmentHandler::EncodeFile(testFileName);

	std::string outputPath = "decoded.pptx";
	std::ofstream out(outputPath, std::ios::binary);
	Base64StreamDecoder decoder;
	decoder.DecodeStream(base64.c_str(), base64.size(), out);
	decoder.Finalize(out);
	out.close();

	std::ifstream decoded_file(outputPath, std::ios::binary);
	std::vector<uint8_t> decoded_bytes((std::istreambuf_iterator<char>(decoded_file)), std::istreambuf_iterator<char>());
	decoded_file.close();

	EXPECT_EQ(testContent, decoded_bytes);

	std::remove(testFileName.c_str());
	std::remove(outputPath.c_str());
}

TEST(AttachmentHandler, DecodeAllAttachments)
{
	std::string mime;
	std::string boundary = "TEST_BOUNDARY";
	mime += "--" + boundary + "\r\n";
	mime += "Content-Type: text/plain\r\n";
	mime += "Content-Transfer-Encoding: 7bit\r\n\r\n";
	mime += "This is a sample text part.\r\n";
	mime += "--" + boundary + "\r\n";
	mime += "Content-Type: text/plain; name=\"file1.txt\"\r\n";
	mime += "Content-Transfer-Encoding: base64\r\n";
	mime += "Content-Disposition: attachment; filename=\"file2.txt\"\r\n\r\n";
	mime += "SGVsbG8gV29ybGQh\r\n";
	mime += "--" + boundary + "\r\n";

	mime += "Content-Type: text/plain; name=\"file3.txt\"\r\n";
	mime += "Content-Transfer-Encoding: base64\r\n";
	mime += "Content-Disposition: attachment; filename=\"file4.txt\"\r\n\r\n";
	mime += "VGhpcyBpcyBmaWxlIDI=\r\n";

	mime += "--" + boundary + "\r\n";
	mime += "Content-Type: text/plain; name=\"file3.txt\"\r\n";
	mime += "Content-Transfer-Encoding: base64\r\n";
	mime += "Content-Disposition: attachment; filename=\"file5.txt\"\r\n\r\n";
	mime += "QW5vdGhlciBmaWxlIGNvbnRlbnQ=\r\n";

	mime += "--" + boundary + "--\r\n";

	std::vector<std::string> decoded_file = AttachmentHandler::DecodeAllAttachments(mime, "TEST_BOUNDARY");
	EXPECT_EQ(decoded_file.size(), 3);
}

TEST(AttachmentHandler, DecodeWithoutBase64Header)
{
	std::string mime;
	std::string boundary = "TEST_BOUNDARY";
	mime += "--" + boundary + "\r\n";
	mime += "Content-Type: text/plain; name=\"file1.txt\"\r\n";
	mime += "Content-Disposition: attachment; filename=\"file.txt\"\r\n\r\n";
	mime += "This is plain text without base64\r\n";
	mime += "--" + boundary + "--\r\n";

	bool result = AttachmentHandler::DecodeFile(mime, boundary);
	EXPECT_TRUE(result);

	std::ifstream f("file.txt");
	EXPECT_FALSE(f.is_open());
}