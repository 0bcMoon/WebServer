#include <iostream>
#include "gtest/gtest.h"

TEST(Welcome, Welcome)
{
	std::string expected_out = "Welcome to the web server!\n";
	testing::internal::CaptureStdout();
	std::cout << "Welcome to the web server\n";
	std::string output = testing::internal::GetCapturedStdout();
	EXPECT_EQ(expected_out, output);
}

TEST(config, getsrever)
{
	
}

int main (int argc, char *argv[]) 
{
	
	testing::InitGoogleTest(&argc, argv);
	(void)argc;
	(void)argv;

	std::cout << "Welcome to the web server!\n";
	return RUN_ALL_TESTS();
}
