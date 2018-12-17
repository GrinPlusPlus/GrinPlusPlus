#pragma once

enum class EClientMode
{
	// Fully verifies current state (not history).
	// Prunes kernels, rangeproofs, headers, and spent outputs.
	// Does not support mining.
	LIGHT_CLIENT,

	// Fully verifies current state (not history).
	// Prunes spent outputs and rangeproofs.
	// Supports mining.
	FAST_SYNC,

	// Fully verifies entire history.
	// Never prunes any headers, kernels, outputs, or rangeproofs.
	// Supports mining.
	FULL_HISTORY
};