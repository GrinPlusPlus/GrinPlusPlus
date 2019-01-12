#pragma once

enum class EDandelionStatus
{
	// Tx to be included in the next "stem" run.
	TO_STEM,

	// Tx previously "stemmed" and propagated.
	STEMMED,

	// Tx to be included in the next "fluff" run.
	TO_FLUFF,

	// Tx previously "fluffed" and broadcast.
	FLUFFED
};