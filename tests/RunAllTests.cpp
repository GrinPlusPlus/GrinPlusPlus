#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "Preparing to run BlockChain tests\n";
	system("pause");
	system("BlockChain_Tests.exe");

	std::cout << "Preparing to run Consensus tests\n";
	system("pause");
	system("Consensus_Tests.exe");

	std::cout << "Preparing to run core tests\n";
	system("pause");
	system("Core_Tests.exe");

	std::cout << "Preparing to run crypto tests\n";
	system("pause");
	system("Crypto_Tests.exe");

	std::cout << "Preparing to run Network tests\n";
	system("pause");
	system("Net_Tests.exe");

	std::cout << "Preparing to run PMMR tests\n";
	system("pause");
	system("PMMR_TESTS.exe");

	std::cout << "Preparing to run Wallet tests\n";
	system("pause");
	system("Wallet_Tests.exe");

	std::cout << "Preparing to run GrinNode tests\n";
	system("pause");
	system("GrinNode_Tests.exe");

	system("pause");

	return 0;
}