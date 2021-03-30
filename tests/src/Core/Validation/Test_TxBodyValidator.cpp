#include <catch.hpp>

#include <Core/Validation/TxBodyValidator.h>
#include <Core/Exceptions/BadDataException.h>
#include <Crypto/Crypto.h>
#include <TxBuilder.h>

TEST_CASE("TxBodyValidator")
{
    TxBuilder tx_builder(KeyChain::FromRandom());

	std::vector<Test::Input> inputs{
		tx_builder.BuildInput(EOutputFeatures::DEFAULT, KeyChainPath({ 1, 0 }), (uint64_t)10'000'000),
		tx_builder.BuildInput(EOutputFeatures::DEFAULT, KeyChainPath({ 1, 1 }), (uint64_t)15'000'000)
	};
	std::vector<Test::Output> outputs{
		Test::Output{ KeyChainPath({1, 2}), (uint64_t)8'000'000},
		Test::Output{ KeyChainPath({1, 3}), (uint64_t)10'000'000}
	};

	TxBuilder::Criteria tx1_criteria;
	tx1_criteria.inputs = inputs;
	tx1_criteria.outputs = outputs;
	tx1_criteria.include_change = true;
	tx1_criteria.num_kernels = 3;
    Transaction tx1 = tx_builder.BuildTx(tx1_criteria);

	// Valid transaction
	TxBodyValidator().Validate(tx1.GetBody());

	//
	// Test Sorting
	//
	{
		std::vector<TransactionInput> tx1_inputs = tx1.GetInputs();
		std::vector<TransactionOutput> tx1_outputs = tx1.GetOutputs();
		std::vector<TransactionKernel> tx1_kernels = tx1.GetKernels();

		TransactionBody inputs_unsorted = TransactionBody::NoSort(
			std::vector<TransactionInput>{ tx1_inputs[1], tx1_inputs[0] },
			tx1_outputs,
			tx1_kernels
		);
		REQUIRE_THROWS_AS(TxBodyValidator().Validate(inputs_unsorted), BadDataException);

		TransactionBody outputs_unsorted = TransactionBody::NoSort(
			tx1_inputs,
			std::vector<TransactionOutput>{ tx1_outputs[0], tx1_outputs[2], tx1_outputs[1]},
			tx1_kernels
		);
		REQUIRE_THROWS_AS(TxBodyValidator().Validate(outputs_unsorted), BadDataException);

		TransactionBody kernels_unsorted = TransactionBody::NoSort(
			tx1_inputs,
			tx1_outputs,
			std::vector<TransactionKernel>{ tx1_kernels[2], tx1_kernels[0], tx1_kernels[1] }
		);
		REQUIRE_THROWS_AS(TxBodyValidator().Validate(kernels_unsorted), BadDataException);
	}

	//
	// Test Duplicate Inputs
	//
	{
		std::vector<Test::Input> tx2_inputs = inputs;
		tx2_inputs.push_back(inputs[0]);

		TxBuilder::Criteria tx2_criteria;
		tx2_criteria.inputs = tx2_inputs;
		tx2_criteria.outputs = outputs;
		tx2_criteria.include_change = true;
		tx2_criteria.num_kernels = 3;
		Transaction tx2 = tx_builder.BuildTx(tx2_criteria);

		REQUIRE_THROWS_AS(TxBodyValidator().Validate(tx2.GetBody()), BadDataException);
	}

	//
	// Test Duplicate Outputs
	//
	{
		std::vector<Test::Output> tx2_outputs = outputs;
		tx2_outputs.push_back(outputs[0]);

		TxBuilder::Criteria tx2_criteria;
		tx2_criteria.inputs = inputs;
		tx2_criteria.outputs = tx2_outputs;
		tx2_criteria.include_change = true;
		tx2_criteria.num_kernels = 3;
		Transaction tx2 = tx_builder.BuildTx(tx2_criteria);

		REQUIRE_THROWS_AS(TxBodyValidator().Validate(tx2.GetBody()), BadDataException);
	}

	// TODO: Test Duplicate Kernels (They should be supported)

	//
	// Test Cut-Through
	//
	{
		uint64_t amount = 100;
		KeyChain keychain = KeyChain::FromRandom();
		KeyChainPath path({ 0, 1, 2 });
		SecretKey blindingFactor = keychain.DerivePrivateKey(path, amount);
		Commitment commitment = Crypto::CommitBlinded(
			amount,
			BlindingFactor(blindingFactor.GetBytes())
		);

		RangeProof rangeProof = keychain.GenerateRangeProof(
			path,
			amount,
			commitment,
			blindingFactor,
			EBulletproofType::ENHANCED
		);

		std::vector<TransactionInput> tx2_inputs = tx1.GetBody().GetInputs();
		tx2_inputs.push_back(TransactionInput(EOutputFeatures::DEFAULT, commitment));

		std::vector<TransactionOutput> tx2_outputs = tx1.GetBody().GetOutputs();
		tx2_outputs.push_back(TransactionOutput(EOutputFeatures::DEFAULT, commitment, rangeProof));

		std::vector<TransactionKernel> tx2_kernels = tx1.GetBody().GetKernels();

		TransactionBody body(std::move(tx2_inputs), std::move(tx2_outputs), std::move(tx2_kernels));

		REQUIRE_THROWS_AS(TxBodyValidator().Validate(body), BadDataException);
	}

	// TODO: Verify Range proofs
}