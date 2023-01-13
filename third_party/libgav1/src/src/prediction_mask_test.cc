// Copyright 2021 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/prediction_mask.h"

#include <array>
#include <cstdint>
#include <string>

#include "gtest/gtest.h"
#include "src/utils/array_2d.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"
#include "tests/utils.h"

namespace libgav1 {
namespace {

constexpr int kWedgeDirectionTypes = 16;

enum kWedgeDirection : uint8_t {
  kWedgeHorizontal,
  kWedgeVertical,
  kWedgeOblique27,
  kWedgeOblique63,
  kWedgeOblique117,
  kWedgeOblique153,
};

const char* const kExpectedWedgeMask[] = {
    "cea09e4bf4227efef749672283f7369b", "2763ab02b70447b2f9d5ed4796ca33bc",
    "8d83c4315eadda824893c3e79aa866d9", "a733fd7f143c1c6141983c5f816bb3d8",
    "9a205bfca776ccde57a8031350f2f467", "d78b964719f52f302f4454df14e45e35",
    "bdc3972cfeb44d0acebb49b2fcb76072", "c8872571833c165be99ada1c552bfd9b",
    "26d2541e2f8efe48e2f4a1819b3a6896", "783871179337e78e5ef41a66c0c6937c",
    "253d21c612d732fceedcf610c4ff099c", "c868d177dc2a2378ef362fa482f601e8",
    "782d75e143d87cc1aeb5d040c48d3c2d", "718cbecf4db45c7d596eba07bd956601",
    "3b60b9336c2cf699172eb4a3fef18787", "afe72d4bd206f1cb27e3736c3b0068cf",
    "7b830a1a94bad23a1df1b8d9668708d0", "d3f421ff2b81686fd421f7c02622aac1",
    "d9ac14dff8e3c415e85e99c3ce0fbd5b", "da493727a08773a950a0375881d912f2",
    "2f4251fd1b4636a034e22611ea1223b6", "84f84f01900b8a894b19e353605846b0",
    "bbf5dae73300b6a6789710ffc4fc59fd", "c711941a0889fbed9b926c1eb39a5616",
    "2fcf270613df57a57e647f37bf9a19ec", "79ed9c2f828b765edf65027f1f0847f5",
    "e8d3e821f4e7f2f39659071da8f2cc71", "823bb09e2c28f2a81bf8a2d030e8bab6",
    "d598fb4f70ea6b705674497994aecbfa", "3737c39f058c57650be7e720dcd87aa1",
    "eb1d9b1d30485d9870ca9380cbdfad43", "a23d3c24f291080fcd62c0a2a2aea181",
    "968543d91aeae3b1814a5074b6aa9e8c", "6e2444d71a4f3ddfe643e72f9c3cf6c3",
    "3bf78413aa04830849a3d9c7bfa41a84", "ece8306f9859bcfb042b0bda8f6750b6",
    "608b29fcedb7fa054a599945b497c78c", "d69d622016872469dfbde4e589bfd679",
    "38a2307174c27b634323c59da3339dc6", "5e44f0fad99dbe802ffd69c7dc239d56",
    "a0eeaf3755a724fdf6469f43cb060d75", "7bcf8035c5057619ea8660c32802d6a1",
    "6054e1c35fe13b9269ab01d1bc0d8848", "e0ec8f7c66ebabff60f5accd3d707788",
    "0b9fd6e1053a706af5d0cd59dc7e1992", "709648ffab1992d8522b04ca23de577a",
    "c576e378ed264d6cb00adfd3b4e428f1", "f6f3ae5348e7141775a8a6bc2be22f80",
    "9289722adb38fa3b2fb775648f0cc3a8", "b7e02fa00b56aeea8e6098a92eac72e1",
    "db2f6d66ffca8352271f1e3f0116838a", "5858c567b0719daaa364fb0e6d8aa5dc",
    "db2d300f875d2465adabf4c1322cea6f", "05c66b54c4d32e5b64a7e77e751f0c51",
    "f2c2a5a3ce510d21ef2e62eedba85afb", "3959d2191a11e800289e21fd283b2837",
    "cc86023d079a4c5daadce8ad0cdd176f", "e853f3c6814a653a52926488184aae5e",
    "8568b9d7215bb8dfb1b7ce66ef38e055", "42814ac5ed652afb4734465cca9e038c",
    "dba6b7d5e93e6a20dac9a514824ad45c", "be77e0dce733b564e96024ea23c9db43",
    "2aa7bd75a1d8eb1000f0ef9e19aa0d1d", "226d85741e3f35493e971dd13b689ec7",
    "9e5a0cf4416f8afeaa3ddbe686b5b7db", "18389c77b362f6b4b727b99426251159",
    "10c5d899de999bbdf35839be3f2d5ee3", "942ae479a36fb4b4d359bebd78a92f03",
    "f14e4dd174958e16755cd1f456b083e0", "8a036cbd0aaf1bece25a1140109f688b",
    "2e48eade95f9fa0b7dae147e66d83e13", "4387d723350a011e26b0e91bbeb3d7c2",
    "5470f977d859232335945efc8bb49ff1", "6780fd81cf2561300c75c930e715c7a6",
    "9786aca6b1b9abfc3eae51404bc3cbd5", "da65c1440fa370a0237284bf30e56b0b",
    "8e0d5d83ab3c477fd11ef143a832f7bf", "97489c7a47aa69fef091e7e6e4049a8f",
    "28787beac9e69001c2999976742764a3", "67760c48ff5f7bc50cd92727694ba271",
    "57c2b0b7de5de0f40fb739ed095d82a4", "7b2a663ca7da4b73f1adfc7e0ca1eff1",
    "980869e1795efb63ca623ce2f0043fb3", "575497eb213b05bab24017cc6ea4e56a",
    "ca3b31382439f0bdd87b61fa10c7863b", "72c65bf29afb288f4d4ff51816429aa7",
    "1fe8929387be982993cd2309e3eeae7a", "994246e2585179e00f49537713f33796",
    "82ae324ba01002370e918724ce452738", "fb3bcb4811b8251f0cc5ec40859617e7",
    "a2e24b21c1d3661412e00411d719210c", "7adc2b60d7d62df1d07e3e4458a46dc2",
    "e71c1b2f9ccb1af0868c3869dc296506", "3e33e087c7e6f724528abbc658a1b631",
    "19b80d80f6b83eedac4bab6226865ae1", "7d9293641c4ed3b21c14964ec785cfb9",
    "5dd0fb9700f30c25bf7b65367c8f098d", "f96b55ec2d012807c972ef4731acd73d",
    "5fc70808c3fa5b3c511926b434bfba66", "768c3ce37acfcd4e5ba05152e5710bc9",
    "1271a52682566ebfc01d5c239177ffd4", "52d4fc11a7507695b2548e0424be50ab",
    "729e7d421aaaf74daa27b0ce1ca0a305", "92d2ff4a9a679cdf0ff765a2d30bced1",
    "d160ec6f1bd864eb2ac8fabf5af7fedd", "ad323dbcb4a651e96bd5c81bc185385d",
    "937c1b7106a2e6aef0adf2c858b4df18", "0f9ad42d1c48970f8462921ac79849ee",
    "32ed1e1a16ddbf816f81caca7cb56c93", "e91aa6389d8255b7744aaa875ba2ceec",
    "88f9dedf6d565b2f60b511e389cf366a", "d0428fd42ca311cd3680ff4670d4f047",
    "b9c7eeb7c9733f0220587643952602cb", "65adf32a5e03d161a411815179078ba3",
    "4984a4e9a5bdf732c071d5b60029daf4", "b9b65a2a9f04b59766d305221e4cda5a",
    "7b2d372fe33d6db1fcf75820b7523ed5", "9a07593316707f8e59fe09c7647ade15",
    "33e75e0d2aa73e3410095c2f98c27a14", "f9ddb33b16431ff9cf6ae96dd4acc792",
    "2df1a8655b2ef23f642b11b76b20f557", "9faba399ccf555c25a33c336cdd54d94",
    "c94404e263c2dae2e955ead645348c08", "3d16d4be87cd4467c3f7be17287940c8",
    "99d0fdae81d61680c7a5b1df38dc98fc", "a23b402d699a00c5c349b17e77f73552",
    "c6f76c81c4050939a6bd5d30ca00b307", "bc3d035bd6e8f55497bfc6d1f81fc8be",
    "99b10db073e13b49bd90655f7516383b", "ddfd0e434efe076e2706c5669c788566",
    "e1d836f814e6eca80ef530f8676e0599", "ed3e4c64e9fd1006e0016e460970a423",
    "0282542e21fa0dea0bf48ec0a2d25b2d", "7482eb8a7bf1417a61c21d82bc7c95f9",
    "e98e9bb3d5edf7b943d0bbf1eec9bef6", "ad4d313beecf609ff3a7d30da3e54a1d",
    "b98f8db9fa62fb73d26415f6fa31b330", "0591b3c34bf4750f20a74eee165a54bd",
    "3054b56fec6968255f21d40f80f5121c", "59ecf60cbb8408e042816e73446fa79c",
    "8fa8c996209a1ddb8a00c14ca19953f8", "e20d2462bc43a1a1bfbc5efe7a905666",
    "b5065e40d5d103e21daabcf4d5fea805", "b65aba0f8e307ef08951f1abdb7c8f62",
    "5fbec6e57c1c651bd7be69fccb0b39a6", "9dfc362f7212d086418b0def54a7c76c",
    "6644928e9aaac5e5d64f4a2c437c778a", "1bf63c7539ea32489bec222d5bc5305f",
    "755ec607a5edf116d188353a96a025c3", "bdc4cc354c4f57c38d3be3dbc9380e2d",
    "7851752b4ae36793ab6f03cd91e7ba6f", "99b9834ea2f6ea8d9168c5c1ba7fe790",
    "75a155c83b618b28d48f5f343cdfef62", "38821c97e04d2294766699a6846fefaf",
    "14be7f588461273862c9d9b83d2f6f0a", "8c38ce521671f0eee7e6f6349ef4f981",
    "043347de994f2fe68c08e7c06a7f6735", "cda15ea2caccbdd8a7342a6144278578",
    "244d586e88c9d6a9a59059a82c3b8e57", "3712928dd0dd77f027370f22d61366a0",
    "e4f1cd4785fc331ad6e3100da4a934f3", "3181459434921b5b15b64cfd2ee734c4",
    "2d588831e98c7178c5370421a6f2fc60", "135cf6a67fc1b51dbcf9fcddb3ae1237",
    "d701da4e1a890a37bb0e9af4a2f0b048", "02138b5a4882181f248945c3a8262050",
    "7fbd4d06965b1d152d6c037b0302f307", "7917a20573da241868689ed49c0d5972",
    "ffdd4257d91fe00e61de4d2668f1ee07", "72999b6d3bf1ee189e9269a27105991f",
    "1b63d7f25388c9af4adac60d46b7a8ca", "e3ce0977224197ade58aa979f3206d68",
    "73178ffd388b46891fc4a0440686b554", "f1f99faf52cea98c825470c6edd1d973",
    "e6fae5d5682862ec3377b714b6b69825", "a4f96cca8da155204b0cc4258b068d3c",
    "75c7674c2356325dcb14c222266c46f8", "932b23521c9d9d06096879a665a14e28",
    "8ed48a84a99b4a5bf2ec8a7a2c1f1c79", "4f6f0214857a92ad92eca1c33a762424",
    "34865190c3e91200a0609a6e770ebc5c", "e793f1f2e46876b1e417da5d59475fda",
    "e83cd9a228941a152f6878aa939e1290", "d6f5cd74ba386bd98282e1fcb0528dbd",
    "131b55ec66ffe76f9088f7b35d38c0dd", "2d0ae8ee059cbd8c7816e3c862efdf37",
    "65baadd2cb85ffbc6480bf8c1f128d1a", "2b8e8af333c464b4213bbd9185a9b751",
    "951fd5faed77a1ae9bf5ef8f30bd65c3", "41d38d40dfe9da2b9ff2146711bf6ab5",
    "7430bde28aed5a9429db54ea663a5e26", "46576d59a13756c494793ad4b3a663e5",
    "21802d0db30caa44cbdba2ac84cc49b5", "591cad82ae106d9e9670acd5b60e4548",
    "c0484c58c6c009939e7f3ec0c1aa8e2d", "6405c55d0a1830cfdd37950bfd65fd6f",
    "3bd74c067d2ba027fc004e9bf62254db", "6e920e6dbdbe55a97ff2bf3dfb38a3e0",
    "e2ed20f89da293516b14be766a624299", "0a613ee53ec38cad995faa17a24fcb8f",
    "0de937145c030d766c3f9fff09d7e39c", "4a560325b804fcb6643866e971ade8e8",
    "be82c41d3a0f8bd4032c3e5e45b453da", "b27219f02db167bf5a416831b908b031",
    "7cf5437e25d362bc373dd53d8fd78186", "39c801e28cc08150c2016083113d1a03",
    "785a21219d9c42a7c5bd417f365535a3", "008c79298a87837bcb504c4dc39ca628",
    "af24d1d6f4d3ee94f2af52471a64ca1f", "cd82218aae9815c106336aec7ce18833",
    "9f405c66d4ce7533213c4ca82feaf252", "7ceda4ea6ddeccd04dbf6d3237fe956a",
    "ae21b52869b85a64fa4e3a85a2a8bb8d", "a004927cdbf48e0dafcccfb6066cdd0c",
    "949337a963a8a5c0f46cf774b078a7cd", "24f58b8db17d02f66d04d22ca6c5e026",
    "2b1315a2e7c5d5309a7621651e741616", "5b317ef820e6c8e7ea7a7d7022e8349d",
    "debd504650d35d9deca4c2461094949f", "19d0ca33e5b3a0afff1f39f0f42238e0",
    "df1c6c7582bfa5ceb147a8dd253cfa43", "176647077c5e2d985b3807134aac118f",
    "dd2850172602688eaaa768f705c1ba67", "6ba1a3929ae9725fc688b8189b16314f",
    "639189abb754dfa6be3c813ee8342954", "d5d1b8bff370f280fba13827d6bdf0fb",
    "4b0ad4ea387a952724cab42730f712d2", "8c9c1f09946b61315e9a45c7e39f1992",
    "50ef75c2b7a17f972586ce053eb62d24", "d5922dd01d8d02ca00ab9648a3db343f",
    "091f517b18f4438ea9c581b7471f2fc0", "fede855bfb936caaa8fb4a434adac1d3",
    "081b612f810f38c5ff6dc1cd03bf2eb6", "bd10e764eaf7d7e0ec89de96423d0afe",
    "3e64cb1355e05b0a4b0237fae3f33bb2", "7cb92e0ecc0dd06d0a5d248efba48630",
    "ec875f2e155a2e124ef52bf35e9a876c", "15529c83eae41bfa804f2c386f480e90",
    "ee0e59567874155fb54de63fc901ded7", "4ad160b0d0f5166f9cddf7235725406e",
    "176b64b3883c33e2aa251159983ccaa1", "d9cca01946d2a47c0114b1f49e4d688f",
    "73d706a13afa279d9c716b3ba3a2ed68", "dea5a7f010d2f1385fe2b7d1d36aafb0",
    "b5432fbc22d2f96c1230cc33178da09e", "8b0e7399ce98b68de4048411ab649468",
    "3d52c986a5a5852a4620fbb38259a109", "eb61882738fefdd105094d4c104cf8b0",
    "24fbc0d3ee28e937cfa1a3fbbc4e8214", "c69eb0687e477c27ac0d4c5fe54bbe8b",
    "00a4f498f05b2b348252927ecc82c8a3", "c76471a61250be52e8d5933e582b1e19",
    "22ebb8812dd795fdc14f20a7f9f89844", "f7c7d5c04bc234545726f4b116b623ec",
    "9fc323d6619af0101edfacb4e9c2b647", "902d7888215d6aac1cf41f1fb6a916d8",
    "5817d80a0504a5b08627502aeece4f38", "a1afa4b4065c143bc4857e364cec7f3d",
    "506d5a6ff434411ea893bb2dc021aa25", "31cd3ca39015ccee1e217e1c83fff2a0",
    "eb1ed4ef292c7d8fead1f113c9fd998f", "35f3abf3a056b778e3d7885f8df6c07a",
    "299d71ee557382f5e64f26f1a8e4e156", "12f8c591a4e257bcc26b385424cd8d47",
    "0b273b03d817af587c8fb23de71f346d", "1d7592fe89c661e9f61d215d235aa2ee",
    "331dc544956ee14064ab432c85d52828", "a0a4ccbe1c442717ad40b7d40ed81a40",
    "45009d915bf1d4ab855b5b670d314839", "641dfe93841aaa18888cebb17b8566eb",
    "2b177c880ce0c2b4e891abc1dc23dfc2", "23984491f7d6c206fb8babafc9aacfdb",
    "5841b93edb22c702035e31b26c58a728", "9852506766cb47f48783640d14753089",
    "8a43698d32f63b1e7191482e4b274fc3", "7bdef02623beae507a651ad398422876",
    "b105138645ad27657a08a3a8e8871a7e", "913e40ebbf1b983ca4956b85364b9459",
    "5776f97b4f0cfa435a99d5d90822922d", "a0ae92a24c2b20039d996ee2a7d8b107",
    "a925cc792412e2a7abe89367c9fe28b1", "778183eab5c9e0ee559d828d8347a21c",
    "c4b4777355a4c8e8858faec37ba23eec", "4cdd41c3648e8d05c3e8f58d08385f8b",
    "7c1246737874f984feb1b5827a1f95db", "c75d766ff5af8db39d400962d5aba0b4",
    "964f010f5aa6748461ca5573b013091d", "b003f3eab3b118e5a8a85c1873b3bb55"};

TEST(WedgePredictionMaskTest, GenerateWedgeMask) {
  WedgeMaskArray wedge_masks;
  ASSERT_TRUE(GenerateWedgeMask(&wedge_masks));

  // Check wedge masks.
  int block_size_index = 0;
  int index = 0;
  for (int block_size = kBlock8x8; block_size < kMaxBlockSizes; ++block_size) {
    const int width = kBlockWidthPixels[block_size];
    const int height = kBlockHeightPixels[block_size];
    if (width < 8 || height < 8 || width > 32 || height > 32) continue;

    for (int flip_sign = 0; flip_sign <= 1; ++flip_sign) {
      for (int direction = 0; direction < kWedgeDirectionTypes; ++direction) {
        uint8_t* const block_wedge_mask =
            wedge_masks[block_size_index][flip_sign][direction][0];
        const std::string digest =
            test_utils::GetMd5Sum(block_wedge_mask, width * height);
        EXPECT_STREQ(digest.c_str(), kExpectedWedgeMask[index]);
        index++;
      }
    }
    block_size_index++;
  }
}

}  // namespace
}  // namespace libgav1
