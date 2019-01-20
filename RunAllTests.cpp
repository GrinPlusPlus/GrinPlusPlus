#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "Preparing to run core tests\n";
	system("pause");
	system("Core_Tests.exe");

	std::cout << "Preparing to run crypto tests\n";
	system("pause");
	system("Crypto_Tests.exe");

	std::cout << "Preparing to run PMMR tests\n";
	system("pause");
	system("PMMR_TESTS.exe");

	system("pause");

	return 0;
}