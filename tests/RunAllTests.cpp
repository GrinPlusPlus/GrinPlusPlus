#include <iostream>

void RunTest(const std::string& test)
{
#if defined(_WIN32)
	std::string str = test + ".exe";
#else
	std::string str = "./" + test;
#endif
	system(str.c_str());
}

int main(int argc, char* argv[])
{
	std::cout << "Preparing to run BlockChain tests\n";
	system("pause");
	RunTest("BlockChain_Tests");

	std::cout << "Preparing to run Consensus tests\n";
	system("pause");
	RunTest("Consensus_Tests");

	std::cout << "Preparing to run core tests\n";
	system("pause");
	RunTest("Core_Tests");

	std::cout << "Preparing to run crypto tests\n";
	system("pause");
	RunTest("Crypto_Tests");

	std::cout << "Preparing to run Network tests\n";
	system("pause");
	RunTest("Net_Tests");

	std::cout << "Preparing to run PMMR tests\n";
	system("pause");
	RunTest("PMMR_TESTS");

	std::cout << "Preparing to run Wallet tests\n";
	system("pause");
	RunTest("Wallet_Tests");

	std::cout << "Preparing to run GrinNode tests\n";
	system("pause");
	RunTest("GrinNode_Tests");

	system("pause");

	return 0;
}