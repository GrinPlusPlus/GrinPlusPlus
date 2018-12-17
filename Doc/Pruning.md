# Pruning and Compacting

One of the advantages of the MimbleWimble protocol is that it allows nodes to prune old data like spent outputs and corresponding rangeproofs, while maintaining enough data to fully validate the current state.
Full nodes are only expected to retain the data necessary to sync new nodes that join the network. This data consists of:
	* All Block Headers - TODO: Mention future plans to incorporate FlyClient
	* All Kernels - This is necessary to validate the total number of coins in circulation.
	* Unspent TxOutputs & Output PMMR - 
	* Unspent RangeProofs & RangeProof PMMR - 
	* Full Blocks Since Horizon - The "Horizon" refers to the period for which all full nodes maintain the history, and is currently defined as 1 week long. These are retained to allow for large chain re-organizations.

This document describes the process of pruning and compacting chain and TxHashSet data.

### Chain Compacting

### TxHashSet Pruning & Compacting