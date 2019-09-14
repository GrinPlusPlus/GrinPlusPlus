# Grin++ Keychain

Grin & Grin++ follow the BIP32 protocol described [here](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki "BIP32"), with the exception that "IamVoldermort" is used as the seed.
To match Grin, keys for Grin++ will be derived using `m/account/chain/address` for now, where chain is always 0, but the option may be added later to follow [BIP44](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki "BIP44").



