#include <catch.hpp>

#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Wallet/Models/Slatepack/SlatepackMessage.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Crypto/ChaChaPoly.h>
#include <API/Wallet/Owner/Models/CreateWalletCriteria.h>
#include <Common/Secure.h>
#include <Common/GrinStr.h>
#include <API/Wallet/Owner/Models/CreateWalletResponse.h>
#include <Wallet/WalletDB/Models/EncryptedSeed.h>
#include <Wallet/Keychain/Mnemonic.h>
#include <Wallet/WalletDB/WalletStore.h>
#include <Wallet/Keychain/KeyChain.h>


TEST_CASE("SlatepackAddress")
{
    SECTION("Testing Multiple Addresses Generation")
    {
        // seed words = dehydrate opened lilac elapse subtly prying swept ruby liar veteran wife afloat strained camp tugs pager dual tomorrow aimless boxes saucepan invoke utensils vapidly lilac
        SecretKey seed = CBigInteger<32>::FromHex("f9a0e73d3cd533368f75ff63cbd97b2100beffbc339cdfa5c203c1a022d9cf11");
        
        /* 
        *  From RFC-0010 (https://github.com/mimblewimble/grin-rfcs/blob/master/text/0010-online-transacting-via-tor.md):
        * 
        *  Although ed25519 is a different curve than used by the grin protocol, we can still use our HD wallets to 
        *  generate deterministic ed25519 public keys (and therefore Grin addresses). For account m/0, addresses will be 
        *  generated using keychain paths m/0/1/x, for account m/1, addresses will be generated using m/1/1/x, etc. To 
        *  generate addresses for a keychain path, we derive the private key in the usual way, but then blake2b hash the 
        *  derived key to get the ed25519 secret key, which can then be used to calculate the public key and address.
        * 
        */

        KeyChainPath path0 = KeyChainPath::FromString("m/0/1").GetChild(0);
        KeyChainPath path1 = KeyChainPath::FromString("m/0/1/1");
        KeyChainPath path2 = KeyChainPath::FromString("m/0/1/").GetChild(2);
        KeyChainPath path3 = KeyChainPath::FromString("m/0/1/").GetChild(3);
        KeyChainPath path4 = KeyChainPath::FromString("m/0/1/4");

        REQUIRE(path0.Format() == "m/0/1/0");
        REQUIRE(path1.Format() == "m/0/1/1");
        REQUIRE(path2.Format() == "m/0/1/2");
        REQUIRE(path3.Format() == "m/0/1/3");
        REQUIRE(path4.Format() == "m/0/1/4");

        ed25519_keypair_t key_pair_t0 = KeyChain::FromSeed(seed.GetSecure()).DeriveED25519Key(path0);
        SlatepackAddress recipient0(key_pair_t0.public_key);
        REQUIRE(recipient0.GetEdwardsPubKey().Format() == "068131549ec5c3bdcb3d13a855e1b76d179919efd974669292b322f5d59a4ccc");
        REQUIRE(recipient0.ToTorAddress().ToString() == "a2atcve6yxb33sz5couflynxnulzsgpp3f2gneuswmrplvm2jtgmjead");
        REQUIRE(recipient0.ToString() == "grin1q6qnz4y7chpmmjeazw59tcdhd5tejx00m96xdy5jkv30t4v6fnxqv9kwer");

        ed25519_keypair_t key_pair_t1 = KeyChain::FromSeed(seed.GetSecure()).DeriveED25519Key(path1);
        SlatepackAddress recipient1(key_pair_t1.public_key);
        REQUIRE(recipient1.GetEdwardsPubKey().Format() == "b861e80849b78b4d8e04faeb79645aad7abbb2b71ebe242830045e30719b47f2");
        REQUIRE(recipient1.ToTorAddress().ToString() == "xbq6qccjw6fu3dqe7lvxszc2vv5lxmvxd27cikbqarpda4m3i7zedkqd");
        REQUIRE(recipient1.ToString() == "grin1hps7szzfk795mrsylt4hjez644athv4hr6lzg2psq30rquvmgleq8epcsx");

        ed25519_keypair_t key_pair_t2 = KeyChain::FromSeed(seed.GetSecure()).DeriveED25519Key(path2);
        SlatepackAddress recipient2(key_pair_t2.public_key);
        REQUIRE(recipient2.GetEdwardsPubKey().Format() == "90dc6c505d48f5e125632ccfe638fbdf07da083319334d490a92802075e29dc3");
        REQUIRE(recipient2.ToTorAddress().ToString() == "sdogyuc5jd26cjldfth6moh334d5ucbtdezu2sikskaca5pctxbvsrid");
        REQUIRE(recipient2.ToString() == "grin1jrwxc5zafr67zftr9n87vw8mmura5zpnrye56jg2j2qzqa0znhpsq2d477");

        ed25519_keypair_t key_pair_t3 = KeyChain::FromSeed(seed.GetSecure()).DeriveED25519Key(path3);
        SlatepackAddress recipient3(key_pair_t3.public_key);
        REQUIRE(recipient3.GetEdwardsPubKey().Format() == "cf54e41d1eb7f7d45787f008d5723e15671f65a013dafd0cb1603bd620c24577");
        REQUIRE(recipient3.ToTorAddress().ToString() == "z5koihi6w735iv4h6aenk4r6cvtr6znacpnp2dfrma55migciv36koqd");
        REQUIRE(recipient3.ToString() == "grin1ea2wg8g7klmag4u87qyd2u37z4n37edqz0d06r93vqaavgxzg4msznam2f");

        ed25519_keypair_t key_pair_t4 = KeyChain::FromSeed(seed.GetSecure()).DeriveED25519Key(path4);
        SlatepackAddress recipient4(key_pair_t4.public_key);
        REQUIRE(recipient4.GetEdwardsPubKey().Format() == "ce4e426102139765def0dce6cf4e190ed04aeb1651903dc00ceb7dcfae349af0");
        REQUIRE(recipient4.ToTorAddress().ToString() == "zzheeyiccolwlxxq3ttm6tqzb3iev2ywkgid3qam5n647lrutlymi5yd");
        REQUIRE(recipient4.ToString() == "grin1ee8yycgzzwtkthhsmnnv7nsepmgy46ck2xgrmsqvad7ult35ntcq4gmv9g");
    }

    SECTION("Testing Derived Key")
    {
        SecretKey seed = CBigInteger<32>::FromHex("29a5b01c3ecf2dff63e30d8857f12a2bc99e0ab51a610c5c33f5035070d62a0b");
        ed25519_keypair_t edwards_keypair_t = ED25519::CalculateKeypair(seed);

        SlatepackAddress recipient(edwards_keypair_t.public_key);

        REQUIRE(recipient.GetEdwardsPubKey().Format() == "dcb57a361f64ca43b6c82fde6b2c771f0408d5115c007b90b1f249bc4efc5fcc");
        REQUIRE(recipient.ToTorAddress().ToString() == "3s2xunq7mtfehnwif7pgwldxd4carvirlqahxefr6je3ytx4l7glq5yd");
        REQUIRE(recipient.ToString() == "grin1mj6h5dslvn9y8dkg9l0xktrhruzq34g3tsq8hy937fymcnhutlxqkuf6xx");

        SlatepackAddress recipient_parsed = SlatepackAddress::Parse("grin1mj6h5dslvn9y8dkg9l0xktrhruzq34g3tsq8hy937fymcnhutlxqkuf6xx");
        REQUIRE(recipient == recipient_parsed);

        SlatepackAddress sender = SlatepackAddress::Random();
        Slate slate;

        std::string armored = Armor::Pack(sender, slate, { recipient });

        x25519_keypair_t x_keypair = Curve25519::ToX25519(edwards_keypair_t);
        SlatepackMessage message = Armor::Unpack(armored, x_keypair);

        ByteBuffer deserializer(message.m_payload);
        REQUIRE(Slate::Deserialize(deserializer).ToJSON() == slate.ToJSON());

        REQUIRE(SlatepackAddress::Parse("grin1uh6fju32utj3lmht4tpnvkvfje29mcex2edqxu5lutxyx0qpt8nqk05lsy").ToTorAddress().ToString() == "4x2js4rk4lsr73xlvlbtmwmjszkf3yzgkznag4u74lgegpablhtguqyd");
    }
}

TEST_CASE("Slatepack - Recieve")
{
    SecretKey seed = CBigInteger<32>::FromHex("29a5b01c3ecf2dff63e30d8857f12a2bc99e0ab51a610c5c33f5035070d62a0b");
    ed25519_keypair_t edwards_keypair_t = ED25519::CalculateKeypair(seed);
    REQUIRE(edwards_keypair_t.public_key.Format() == "dcb57a361f64ca43b6c82fde6b2c771f0408d5115c007b90b1f249bc4efc5fcc");
    x25519_keypair_t x_keypair = Curve25519::ToX25519(edwards_keypair_t);
    REQUIRE(x_keypair.pubkey.bytes.ToHex() == "86db6c3745d6fe9ffc91f082fba0313042f0406e766375e6c96d25f4daeb4042");

    std::string armored = "BEGINSLATEPACK. B8psw1jVLxmwLzZ qFqb6dAFpPMHxc1 LnFVNZSbPWz8Fpz VYy66B6zNWZrHTY Zbpz27uWRaPvHXi QpnJcQPcmTJrGKH 15efqivHrW1KbQK xX67MqQo3uFo91W yjHzhLM7V2RjpZe LZK4p4ZaXonsQSd JQBfnBwShKsFbgF xhNU8JqjJB4an2Q 6487f5vDFzJHbtm AUpbG8NSiFHVkKS WCFGfh6fnfS9xsA DBDPrsDXdsACXTh GNMywxqYvjhqgT4 A5u7Qv5CwUvcvqr XocutAu4FPo3kcp QPhhTyitSuhtBhX iZ5uvGqdiwLdKjZ awFmcHceCiggWHz L1JiMpRksU9XVff 3Hmhxxp9kaP4mCW 6jFoJdSQkGqxmMv HGXkbszvz17pbnT ivSoYc259i92Y5t 7pnymB2q58q4G3s s7nKcJSYs1zncFp HqWw4CQeeXR5o7s yxivGQU2Mu7CPTZ bLzgWzwJd8CwfQ5 o49Wz71dePL81bD xkPJ8neynqMYZpJ vvioJe2dJvQkE3C cv4h2SMmcJGBAeu cr83H2bwQztTN69 JUARWyZt8DfvnQF tA8VyrESsej9qB3 xWJE2dAWvM6RKnN TKcVr1DmmfFjgEQ 9qGCpuxRKAgEiS2 EpfcSNg2A9YZSjY DBLU5wd6nigjsi2 1Ko4FxJaqpJwa7G DE2eS7cyrRQj53D HCVfnYPw9ARyKCo 2N6Nk33LyVhidzY vKekrL1ZsTCNGCo VRuwzzS52qzfjmF VfqSbB1JrqFNAxB LU6CkW846dMAgW4 HHQjnwhi3oaEzab b4zmWQQEoMWHe76 BTZGtCy4ARue88g RUJJRjNcPYAB7tw zrPTg2iR88DikUL FFKhpFDb52MYVtX. ENDSLATEPACK.";

    SlatepackMessage message1 = Armor::Unpack(armored, x_keypair);
    ByteBuffer buffer1{ message1.m_payload };
    Slate slate1 = Slate::Deserialize(buffer1);
    REQUIRE(slate1.amount == 1000000000);

    /*x25519_public_key_t ephemeral_public_key(CBigInteger<32>::FromHex("1a7f941f7769a3e132374c13e9e492618ac4bd26ece8161cdd168b4a6845e972"));
    SecretKey shared_secret = X25519::ECDH(x_keypair.seckey, ephemeral_public_key);

    REQUIRE(shared_secret.GetBytes().ToHex() == "e55e34d16234491af9f0c681f70c0cd8c69ccc8f19bb5d0c790c16510b81bb2e");

    std::vector<uint8_t> salt;
    salt.insert(salt.end(), ephemeral_public_key.cbegin(), ephemeral_public_key.cend());
    salt.insert(salt.end(), x_keypair.pubkey.cbegin(), x_keypair.pubkey.cend());
    SecretKey enc_key = KDF::HKDF(
        std::make_optional(std::move(salt)),
        "age-encryption.org/v1/X25519",
        shared_secret.GetVec()
    );

    REQUIRE(enc_key.GetBytes().ToHex() == "b32b4dc51dab4778d5dc13174e9b5f1528729508ed764cbef0fec864bfa35cd3");

    std::vector<uint8_t> decrypted = ChaChaPoly::Init(enc_key).Decrypt(CBigInteger<12>::ValueOf(0), HexUtil::FromHex("a525be27a6033d7d76c1e7fd348fe05fb9c34b"));
    std::cout << HexUtil::ConvertToHex(decrypted) << std::endl;
    REQUIRE(HexUtil::ConvertToHex(decrypted) == "010203");

    //SlatepackMessage message = Armor::Unpack(armored, x_keypair);
    SlatepackMessage message = SlatepackMessage::Decrypt(
        HexUtil::FromHex("010001000000000000000000000000022a6167652d656e6372797074696f6e2e6f72672f76310a2d3e20583235353139203745637778617651442b3966612b6b776a7a73366c592f4c78432f6a5635345a2f374f4e31686a2f5558510a63437947644f4a4f5a7463527835366a524c30537a6f63586277546d4a434476556a4c5345786b324f65410a2d3e206a6f696e742d6f696c2d3d385e6720433c494e0a702f796f45546236382b54534274746a686a524c6d2f764f5a567750387251576b764664425835334a2b6a6472634e646d4848376e56494e43396e44316c66460a6e666f6a7a76746e795545735a516b49676e414b6a4e335864696b512b516231626859504e305632733449476a365873762f6b6f7759557a5a544e4357670a2d2d2d207a384563367979487a58344c6145332b766e36336f59302b4b633153756f68486764455737762f52652b340aeee5e2eaa2d92b0d245ac6e100d19007b79c3f4c032ce1078216c7fc49b0c84d32b253a5c5b6c1afc644a37d1c7b1440ad05df2cb49a5d17394be23fdf85f43464088b8c8a617e28db32c354647b514c936ac2516cd5ec69587bdbcb6e21ea5102ade3009024781aa551af62283618335d78b5bfe6a04d05677c4e281c21eec04c466830e275125fc0b8b839a0f6a4120f4a80fb7e30f4051c0734860fdac7912f4be96b538c7e65bf67ddba579b7db9e993091ee4b062c42b5ba699a6d1e66fb5a860bbf79d86b264439c2d64432c245d734255b9ae8125e1312690ea583def7709974bde6935ed66c7bd"),
        //HexUtil::FromHex("01000100000000000000000000000002366167652d656e6372797074696f6e2e6f72672f76310a2d3e20583235353139203269595a773445436b3641794d4856374179592b2f59757879665345764e326e497552692b6354773846590a4430527864536346325652654759614c4a416a73597641475373457351596c70574e7a382b4679526d48630a2d3e206a6f696e742d6f696c2d744b5d3921623d203f6620676b2055722045353e677e0a45443653767267726e57784e6853546b7a4d334a42514b666962634258412f78416d6d385355455666654f73734f6e4a463361692b33763155617242615349370a676a42443774564f5134727054655973327a6c325a4d536b674b744f56547a386d6c67696c3858434164327452536f4955364b4a776254634b2f324c345054750a594a51350a2d2d2d207158314d544137656d474462753279334b6945455347686e6a473168346f6b4b57476149624a62584b2f670ad62574a04b97a7abee10dc52b2c4315e928f884a12f663eaf8818e7dc81c24c849cf6ebde7e0be448f3fb44a099cf2080aa43e141589a7571cfc75f78c92b326040368d578b68e35c6201801e8db487faec448b95974246e8b9379166afbcd9765e6639faeb0bcb01d889fa152d7d3032267b667bef34621e47455216ddd329bd572cc0a68547f8b9f9a1df71f7376d4c66816eff6ebcbe1ccfcb15d5574866f56899ddfd3a5d6dc4bb7e3b51c859cdd054a382314884d34f22d3a40a8d9ea4cbcac3b0ed8a90f0bbfd67c9618b77b7380e9a089fd0e45e9c32b5406a1e211999aa54b"),
        x_keypair
    );
    ByteBuffer buffer{ message.m_payload };
    Slate slate = Slate::Deserialize(buffer);

    REQUIRE(slate.amount == 1000000000);*/
}

TEST_CASE("Slatepack - E2E")
{
    ed25519_keypair_t sender_edwards_keypair = ED25519::CalculateKeypair(CSPRNG::GenerateRandom32());
    x25519_keypair_t sender_x_keypair = Curve25519::ToX25519(sender_edwards_keypair);
    SlatepackAddress sender_address(sender_edwards_keypair.public_key);

    ed25519_keypair_t receiver_edwards_keypair = ED25519::CalculateKeypair(CSPRNG::GenerateRandom32());
    x25519_keypair_t receiver_x_keypair = Curve25519::ToX25519(receiver_edwards_keypair);
    SlatepackAddress receiver_address(receiver_edwards_keypair.public_key);

    // Send slate
    Slate sent_slate;
    std::string sent_armored_slatepack = Armor::Pack(sender_address, sent_slate, { receiver_address });
    std::cout << sent_armored_slatepack << std::endl;

    // Receive slate
    SlatepackMessage received_slatepack = Armor::Unpack(sent_armored_slatepack, receiver_x_keypair);
    ByteBuffer received_slatepack_buffer(received_slatepack.m_payload);
    
    REQUIRE(Slate::Deserialize(received_slatepack_buffer) == sent_slate);
    REQUIRE(received_slatepack.m_sender == sender_address);
}