#include <gtest/gtest.h>
extern "C" {
#include "DataFrame.h"
#include "sensorParser.h"
}

TEST(parseMPPTHexTests, idealVoltageBufferTest)
{
	DataFrame data = {};
	uint8_t buf[20] = ":7D5ED003B123F\n";
	parseMPPTHex(&data, buf, 20);

	ASSERT_EQ(data.mppt.voltage,46.67f);
}

TEST(parseMPPTHexTests, idealPowerLimitBufferTest)
{
	DataFrame data = {};
	uint8_t buf[20] = ":7BCED00C402964900\n";
	parseMPPTHex(&data, buf, 20);

	ASSERT_EQ(data.mppt.power,65535);
}

TEST(parseMPPTHexTests, idealPowerNormalBufferTest)
{
	DataFrame data = {};
	uint8_t buf[20] = ":7BCED00959203007B\n";
	parseMPPTHex(&data, buf, 20);

	ASSERT_EQ(data.mppt.power,2341);
}

TEST(parseMPPTHexTests, noiseCharsBufferTest)
{
	DataFrame data = {};
	uint8_t buf[40] = "asdhuhevb1235:7D5ED003B123F\nasdhwfgafwh";
	parseMPPTHex(&data, buf, 40);

	ASSERT_EQ(data.mppt.voltage,46.67f);
}

TEST(parseMPPTHexTests, multipleColonTest)
{
	DataFrame data = {};
	uint8_t buf[40] = "asdhu:evb123::7D5ED003B123F\nasdhwfgafwh";
	parseMPPTHex(&data, buf, 40);

	ASSERT_EQ(data.mppt.voltage,46.67f);
}

TEST(parseMPPTHexTests, multipleColonMessageSpacingTest)
{
	DataFrame data = {};
	uint8_t buf[40] = "asdhu:evb12adsawdwa\n::7D5ED003B123F\nad";
	parseMPPTHex(&data, buf, 40);

	ASSERT_EQ(data.mppt.voltage,46.67f);
}

TEST(parseMPPTHexTests, badChecksum)
{
	DataFrame data = {};
	uint8_t buf[20] = ":7D5ED003B1246\n";
	parseMPPTHex(&data, buf, 20);

	ASSERT_EQ(data.mppt.voltage,0);
}
