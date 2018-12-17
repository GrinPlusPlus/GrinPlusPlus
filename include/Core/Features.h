#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

////////////////////////////////////////
// FEATURES
////////////////////////////////////////

enum EOutputFeatures
{
	// No Flags
	DEFAULT_OUTPUT = 0,

	// Output is a coinbase output, must not be spent until maturity
	COINBASE_OUTPUT = 1
};

enum EKernelFeatures
{
	// No flags
	DEFAULT_KERNEL = 0,

	// Kernel matching a coinbase output
	COINBASE_KERNEL = 1
};