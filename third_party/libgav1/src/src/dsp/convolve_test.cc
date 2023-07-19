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

#include "src/dsp/convolve.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <tuple>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/third_party/libvpx/md5_helper.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

// The convolve function will access at most (block_height + 7) rows/columns
// from the beginning.
constexpr int kMaxBlockWidth = kMaxSuperBlockSizeInPixels + kSubPixelTaps;
constexpr int kMaxBlockHeight = kMaxSuperBlockSizeInPixels + kSubPixelTaps;

// Test all the filters in |kSubPixelFilters|. There are 6 different filters but
// filters [4] and [5] are only reached through GetFilterIndex().
constexpr int kMinimumViableRuns = 4 * 16;

struct ConvolveTestParam {
  enum BlockSize {
    kBlockSize2x2,
    kBlockSize2x4,
    kBlockSize4x2,
    kBlockSize4x4,
    kBlockSize4x8,
    kBlockSize8x2,
    kBlockSize8x4,
    kBlockSize8x8,
    kBlockSize8x16,
    kBlockSize16x8,
    kBlockSize16x16,
    kBlockSize16x32,
    kBlockSize32x16,
    kBlockSize32x32,
    kBlockSize32x64,
    kBlockSize64x32,
    kBlockSize64x64,
    kBlockSize64x128,
    kBlockSize128x64,
    kBlockSize128x128,
    kNumBlockSizes
  };

  static constexpr int kBlockWidth[kNumBlockSizes] = {
      2, 2, 4, 4, 4, 8, 8, 8, 8, 16, 16, 16, 32, 32, 32, 64, 64, 64, 128, 128};
  static constexpr int kBlockHeight[kNumBlockSizes] = {
      2, 4, 2, 4, 8, 2, 4, 8, 16, 8, 16, 32, 16, 32, 64, 32, 64, 128, 64, 128};

  explicit ConvolveTestParam(BlockSize block_size)
      : block_size(block_size),
        width(kBlockWidth[block_size]),
        height(kBlockHeight[block_size]) {}

  BlockSize block_size;
  int width;
  int height;
};

#if !LIBGAV1_CXX17
constexpr int ConvolveTestParam::kBlockWidth[kNumBlockSizes];   // static.
constexpr int ConvolveTestParam::kBlockHeight[kNumBlockSizes];  // static.
#endif

const char* GetConvolveDigest8bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 16] = {
      "ae5977a4ceffbac0cde72a04a43a9d57", "6cf5f791fe0d8dcd3526be3c6b814035",
      "d905dfcad930aded7718587c05b48aaf", "6baf153feff04cc5b7e87c0bb60a905d",
      "871ed5a69ca31e6444faa720895949bf", "c9cf1deba08dac5972b3b0a43eff8f98",
      "68e2f90eaa0ab5da7e6f5776993f7eea", "f1f8282fb33c30eb68c0c315b7a4bc01",
      "9412064b0eebf8123f23d74147d04dff", "cc08936effe309ab9a4fa1bf7e28e24e",
      "36cbef36fa21b98df03536c918bf752a", "9d0da6321cf5311ea0bdd41271763030",
      "55a10165ee8a660d7dddacf7de558cdd", "ac7fc9f9ea7213743fae5a023faaaf08",
      "077e1b7b355c7ab3ca40230ee8efd8ea", "7a3e8de2a1caae206cf3e51a86dfd15a",
      "1ddf9020f18fa7883355cf8c0881186a", "2377dd167ef2707978bed6f10ffd4e76",
      "f918e0e4422967c6a7e47298135c7ae9", "b2264e129636368b5496760b39e64b7a",
      "1168251e6261e2ff1fa69a93226dbd76", "4821befdf63f8c6da6440afeb57f320f",
      "c30fc44d83821141e84cc4793e127301", "a8293b933d9f2e5d7f922ea40111d643",
      "354a54861a94e8b027afd9931e61f997", "b384e9e3d81f9f4f9024028fbe451d8b",
      "eeeb8589c1b31cbb565154736ca939ec", "f49dab626ddd977ed171f79295c24935",
      "78d2f27e0d4708cb16856d7d40dc16fb", "9d2393ea156a1c2083f5b4207793064b",
      "a9c62745b95c66fa497a524886af57e2", "2c614ec4463386ec075a0f1dbb587933",
      "7a8856480d752153370240b066b90f6a", "beaef1dbffadc701fccb7c18a03e3a41",
      "72b1e700c949d06eaf62d664dafdb5b6", "684f5c3a25a080edaf79add6e9137a8e",
      "3be970f49e4288988818b087201d54da", "d2b9dba2968894a414756bb510ac389a",
      "9a3215eb97aedbbddd76c7440837d040", "4e317feac6da46addf0e8b9d8d54304b",
      "d2f5ca2b7958c332a3fb771f66da01f0", "7aec92c3b65e456b64ae285c12b03b0d",
      "f72a99ad63f6a88c23724e898b705d21", "07a1f07f114c4a38ba08d2f44e1e1132",
      "26b9de95edb45b31ac5aa19825831c7a", "4e4677a0623d44237eb8d6a622cdc526",
      "c1b836a6ce023663b90db0e320389414", "5befcf222152ebc8d779fcc10b95320a",
      "62adf407fc27d8682ced4dd7b55af14e", "35be0786a072bf2f1286989261bf6580",
      "90562fc42dc5d879ae74c4909c1dec30", "a1427352f9e413975a0949e2b300c657",
      "bcbc418bc2beb243e463851cd95335a9", "cb8fedcbecee3947358dc61f95e56530",
      "0d0154a7d573685285a83a4cf201ac57", "b14bd8068f108905682b83cc15778065",
      "c96c867d998473197dde9b587be14e3a", "f596c63c7b14cada0174e17124c83942",
      "eb2822ad8204ed4ecbf0f30fcb210498", "538ce869ffd23b6963e61badfab7712b",
      "6bbcc075f8b768a02cdc9149f150326d", "4ae70d9db2ec36885394db7d59bdd4f7",
      "5fee162fe52c11c823db4d5ede370654", "9365186c59ef66d9def40f437022ad93",
      "0f95fb0276c9c7910937fbdf75f2811d", "356d4003477283e157c8d2b5a79d913c",
      "b355dab2dbb6f5869018563eece22862", "cf6ff8c43d8059cea6090a23ab66a0ef",
      "a336f8b7bcf188840ca65c0d0e66518a", "de953f03895923359c6a719e6a537b89",
      "8463ade9347ed602663e2cec5c4c3fe6", "392de11ffcd5c2ecf3db3480ee135340",
      "bddd31e3e852712e6244b616622af83d", "30a36245c40d978fc8976b442a8600c3",
      "93aa662b988b8502e5ea95659eafde59", "70440ba9ee7f9d16d297dbb49e54a56e",
      "1eb2be4c05b50e427e29c72fa566bff5", "52c0980bae63e8459e82eee7d8af2334",
      "75e57104d6058cd2bce1d3d8142d273d", "b4c735269ade44419169adbd852d5ddc",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "a7305087fae23de53d21a6909009ff69",
      "8dcce009395264379c1a51239f4bb22c", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "8dcce009395264379c1a51239f4bb22c", "d90a69e7bae8aa46ed0e1e5f911d7a07",
      "6ab4dc87be03be1dcc5d956ca819d938", "6ab4dc87be03be1dcc5d956ca819d938",
      "8f2afdb2f03cd04ffacd421b958caaa0", "710ccecc103033088d898a2b924551fb",
      "710ccecc103033088d898a2b924551fb", "a4093e3e5902dd659407ce6471635a4e",
      "375d7f5358d7a088a498b8b3aaecc0d5", "375d7f5358d7a088a498b8b3aaecc0d5",
      "08867ea5cc38c705ec52af821bc4736a", "2afb540e8063f58d1b03896486c5e89b",
      "2afb540e8063f58d1b03896486c5e89b", "6ce47b11d2e60c5d183c84ce9f2e46cc",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "a5a1ac658d7ce4a846a32b9fcfaa3475",
      "2370f4e4a83edf91b7f504bbe4b00e90", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "ae5464066a049622a7a264cdf9394b55", "45368b6db3d1fee739a64b0bc823ea9c",
      "8dff0f28192d9f8c0bf7fb5405719dd8", "632738ef3ff3021cff45045c41978849",
      "f7ec43384037e8d6c618e0df826ec029", "a6bc648197781a2dc99c487e66464320",
      "1112ebd509007154c72c5a485b220b62", "9714c4ce636b6fb0ad05cba246d48c76",
      "2c93dde8884f09fb5bb5ad6d95cde86d", "a49e6160b5d1b56bc2046963101cd606",
      "7f084953976111e9f65b57876e7552b1", "0846ec82555b66197c5c45b08240fbcc",
      "ca7471c126ccd22189e874f0a6e41960", "0802b6318fbd0969a33de8fdfcd07f10",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "3b1ceebf0579fcbbfd6136938c595b91",
      "ecafabcad1045f15d31ce2f3b13132f2", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "5f211eba020e256a5781b203c5aa1d2e", "3b04497634364dd2cd3f2482b5d4b32f",
      "a8ac7b5dc65ffb758b0643508a0e744e", "561ed8be43c221a561f8885a0d74c7ef",
      "8159619fc234598c8c75154d80021fd4", "8f43645dce92cf7594aa4822aa53b17d",
      "b6ccddb7dfa4eddc87b4eff08b5a3195", "b4e605327b28db573d88844a1a09db8d",
      "15b00a15d1cc6cc96ca85d00b167e4dd", "7bf911888c11a9fefd604b8b9c82e9a1",
      "bfb69b4d7d4aed73cfa75a0f55b66440", "034d1d62581bd0d840c4cf1e28227931",
      "8cba849640e9e2859d509bc81ca94acd", "bc79acf2a0fe419194cdb4529bc7dcc8",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "3bfad931bce82335219e0e29c15f2b21",
      "68a701313d2247d2b32636ebc1f2a008", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "05afe1f40d37a45a97a5e0aadd5066fb", "9e1f0e0bddb58d15d0925eeaede9b84c",
      "03313cdaa593a1a7b4869010dcc7b241", "88a50d2b4107ee5b5074b2520183f8ac",
      "ac50ea9f7306da95a5092709442989cf", "739b17591437edffd36799237b962658",
      "b8a7eb7dd9c216e240517edfc6489397", "75b755f199dbf4a0e5ebbb86c2bd871d",
      "31b0017ba1110e3d70b020901bc15564", "0a1aa8f5ecfd11ddba080af0051c576a",
      "536181ee90de883cc383787aec089221", "29f82b0f3e4113944bd28aacd9b8489a",
      "ee3e76371240d1f1ff811cea6a7d4f63", "17a20dbbf09feae557d40aa5818fbe76",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "6baf153feff04cc5b7e87c0bb60a905d",
      "871ed5a69ca31e6444faa720895949bf", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "68e2f90eaa0ab5da7e6f5776993f7eea", "f1f8282fb33c30eb68c0c315b7a4bc01",
      "9412064b0eebf8123f23d74147d04dff", "cc08936effe309ab9a4fa1bf7e28e24e",
      "36cbef36fa21b98df03536c918bf752a", "9d0da6321cf5311ea0bdd41271763030",
      "55a10165ee8a660d7dddacf7de558cdd", "ac7fc9f9ea7213743fae5a023faaaf08",
      "077e1b7b355c7ab3ca40230ee8efd8ea", "7a3e8de2a1caae206cf3e51a86dfd15a",
      "1ddf9020f18fa7883355cf8c0881186a", "2377dd167ef2707978bed6f10ffd4e76",
      "f918e0e4422967c6a7e47298135c7ae9", "b2264e129636368b5496760b39e64b7a",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "4cfad2c437084a93ea76913e21c2dd89",
      "d372f0c17bce98855d6d59fbee814c3d", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "d99ffd2579eb781c30bc0df7b76ad61e", "4e139e57cbb049a0f4ef816adc48d026",
      "be53b2507048e7ff50226d15c0b28865", "b73f3c1a10405de89d1f9e812ff73b5a",
      "c7d51b1f2df49ab83962257e8a5934e5", "159e443d79cc59b11ca4a80aa7aa09be",
      "6ef14b14882e1465b0482b0e0b16d8ce", "22a8d287b425c870f40c64a50f91ce54",
      "f1d96db5a2e0a2160df38bd96d28d19b", "637d1e5221422dfe9a6dbcfd7f62ebdd",
      "f275af4f1f350ffaaf650310cb5dddec", "f81c4d6b001a14584528880fa6988a87",
      "a5a2f9c2e7759d8a3dec1bc4b56be587", "2317c57ab69a36eb3bf278cf8a8795a3",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "1a0bdfc96a3b9fd904e658f238ab1076",
      "56d16e54afe205e97527902770e71c71", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "1f7b5b8282ff3cf4d8e8c52d80ef5b4d", "79e9e260a2028c5fe320005c272064b9",
      "2418ebcdf85551b9ae6e3725f04aae6d", "98bdf907ebacacb734c9eef1ee727c6e",
      "4dd5672d53c8f359e8f80badaa843dfc", "a1bef519bbf07138e2eec5a91694de46",
      "df1cb51fe1a937cd7834e973dc5cb814", "317fe65abf81ef3ea07976ef8667baeb",
      "2da29da97806ae0ee300c5e69c35a4aa", "555475f5d1685638169ab904447e4f13",
      "b3e3a6234e8045e6182cf90a09f767b2", "849dfeca59074525dea59681a7f88ab4",
      "39a68af80be11e1682b6f3c4ede33530", "b22d765af176d87e7d3048b4b89b86ad",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "b8a710baa6a9fc784909671d450ecd99",
      "f9e6a56382d8d12da676d6631bb6ef75", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "3bf8e11e18527b16f0d7c0361d74a52d", "b9ff54c6f1e3b41fc7fc0f3fa0e75cf2",
      "06ef1504f31af5f173d3317866ca57cb", "635e8ee11cf04d73598549234ad732a0",
      "fab693410d59ee88aa2895527efc31ac", "3041eb26c23a63a587fbec623919e2d2",
      "c61d99d5daf575664fb7ad64976f4b03", "822f6c4eb5db760468d822b21f48d94d",
      "3f6fcb9fae3666e085b9e29002a802fc", "d9b9fecd195736a6049c528d4cb886b5",
      "fed17fc391e6c3db4aa14ea1d6596c87", "d0d3482d981989e117cbb32fc4550267",
      "39561688bf6680054edbfae6035316ce", "087c5992ca6f829e1ba4ba5332d67947",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetConvolveScaleDigest8bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 2] = {
      "0291a23f2ac4c40b5d8e957e63769904", "1d48447857472d6455af10d5526f6827",
      "409b2278d6d372248f1891ca0dd12760", "9e416606a3f82fe5bb3f7182e4f42c2d",
      "e126563f859ddd5c5ffde6f641168fad", "9bad4f1b7e1865f814b6fd5620816ebd",
      "50e5e5a57185477cb2af83490c33b47c", "3d2fb301c61d7fbd0e21ac263f7ac552",
      "5920032c6432c80c6e5e61b684018d13", "07ada64d24339488cdce492e6e0c6b0d",
      "aaf1589aff6d062a87c627ab9ba20e3e", "91adf91bb24d2c4ea3f882bdf7396e33",
      "1d17a932a68bb1f199f709e7725fe44b", "07716c63afda034cb386511ea25a63b5",
      "cca17ef3324c41d189e674a059ef1255", "37d17e70619823a606c0b5f74bf2e33b",
      "ba8ed5474c187c8e8d7f82a6a29ee860", "27663f037973ebe82ec10252a4d91299",
      "24c27e187e8d5a2bbfa0fef9046d3eb0", "9854fdc91a48e3bd4639edcc940e5c09",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "a71907c60a9f1f81972a2859ae54a805",
      "817bc3bf0c77abc4186eac39f2320184", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "4e7182a8b226982e2678abcf5f83325d", "50cef7c6e57544135f102226bb95bed9",
      "225e054dbcfff05b1c8b0792c731449e", "16eb63f03839159f3af0e08be857170f",
      "c8e5d111a2e3f4487330a8bd893cb894", "4fd99eaf9c160442aab35b9bdc5d275b",
      "8b0f61bfb30747d4c9215618ac42557c", "1df78022da202cefb9a8100b114152d9",
      "378466e1eda63dbc03565b78af8e723f", "28ea721411fbf5fc805035be9a384140",
      "4fed5d4163a3bfcc6726a42f20410b0a", "55abfca0c820771bd926e4b94f66a499",
      "6c8b8ef0a78859c768e629e1decc0019", "d0ead286b5ba3841d24dd114efbfef0a",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetConvolveDigest10bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 16] = {
      "b1b6903d60501c7bc11e5285beb26a52", "a7855ed75772d7fa815978a202bbcd9f",
      "bde291a4e8087c085fe8b3632f4d7351", "238980eebc9e63ae3eea2771c7a70f12",
      "0eac13431bd7d8a573318408a72246d5", "d05a237ed7a9ca877256b71555b1b8e4",
      "73438155feb62595e3e406921102d748", "5871e0e88a776840d619670fbf107858",
      "1c6376ce55c9ee9e35d432edb1ffb3b7", "d675e0195c9feca956e637f3f1959f40",
      "b5681673903ade13d69e295f82fdd009", "3c43020105ae93a301404b4cd6238654",
      "dd2c5880a94ed3758bfea0b0e8c78286", "4ebb1a7b25a39d8b9868ec8a1243103f",
      "d34ec07845cd8523651e5f5112984a14", "2ce55308d873f4cd244f16da2b06e06e",
      "a4bb5d5ff4b25f391265b5231049a09a", "c9106e0c820b03bcdde3aa94efc11a3e",
      "7ec2eae9e118506da8b33440b399511a", "78de867c8ee947ed6d29055747f26949",
      "a693b4bd0334a3b98d45e67d3985bb63", "156de3172d9acf3c7f251cd7a18ad461",
      "e545b8a3ff958f8363c7968cbae96732", "7842b2047356c1417d9d88219707f1a1",
      "1a487c658d684314d91bb6d961a94672", "94b3e5bcd6b849b66a4571ec3d23f9be",
      "0635a296be01b7e641de98ee27c33cd2", "82dc120bf8c2043bc5eee81007309ebf",
      "58c826cad3c14cdf26a649265758c58b", "f166254037c0dfb140f54cd7b08bddfe",
      "74ab206f14ac5f62653cd3dd71a7916d", "5621caef7cc1d6522903290ccc5c2cb8",
      "78ec6cf42cce4b1feb65e076c78ca241", "42188e2dbb4e02cd353552ea147ad03f",
      "f9813870fc27941a7c00a0443d7c2fe7", "20b14a6b5af7aa356963bcaaf23d230d",
      "9c9c41435697f75fa118b6d6464ee7cb", "38816245ed832ba313fefafcbed1e5c8",
      "5d34137cc8ddba75347b0fa1d0a91791", "465dcb046a0449b9dfb3e0b297aa3863",
      "3e787534dff83c22b3033750e448865a", "4c91f676a054d582bcae1ca9adb87a31",
      "eab5894046a99ad0a1a12c91b0f37bd7", "765b4cfbfc1a4988878c412d53bcb597",
      "bc63b29ec78c1efec5543885a45bb822", "91d6bdbc62d4bb80c9b371d9704e3c9e",
      "cecd57396a0033456408f3f3554c6912", "5b37f94ef136c1eb9a6181c19491459c",
      "716ba3a25b454e44b46caa42622c128c", "9076f58c4ab20f2f06d701a6b53b1c4f",
      "d3212ab3922f147c3cf126c3b1aa17f6", "b55fea77f0e14a8bf8b6562b766fe91f",
      "59b578268ff26a1e21c5b4273f73f852", "16761e7c8ba2645718153bed83ae78f6",
      "a9e9805769fe1baf5c7933793ccca0d8", "553a2c24939dff18ec5833c77f556cfb",
      "5c1ec75a160c444fa90abf106fa1140e", "2266840f11ac4c066d941ec473b1a54f",
      "9e194755b2a37b615a517d5f8746dfbb", "bbf86f8174334f0b8d869fd8d58bf92d",
      "fd1da8d197cb385f7917cd296d67afb9", "a984202c527b757337c605443f376915",
      "c347f4a58fd784c5e88c1a23e4ff15d2", "29cbaadbff9adf4a3d49bd9900a9dd0b",
      "c5997b802a6ba1cf5ba1057ddc5baa7e", "4f750f6375524311d260306deb233861",
      "59f33727e5beeb783a057770bec7b4cd", "0654d72f22306b28d9ae42515845240c",
      "6c9d7d9e6ef81d76e775a85c53abe209", "a35f435ccc67717a49251a07e62ae204",
      "c5325015cb0b7c42839ac4aa21803fa0", "f81f31f1585c0f70438c09e829416f20",
      "ab10b22fb8dd8199040745565b28595d", "0d928d6111f86c60ccefc6c6604d5659",
      "4ed1a6200912995d4f571bdb7822aa83", "92e31a45513582f386dc9c22a57bbbbd",
      "6dbf310a9c8d85f76306d6a35545f8af", "80fce29dc82d5857c1ed5ef2aea16835",
      "14f2c5b9d2cd621c178a39f1ec0c38eb", "da54cfb4530841bda29966cfa05f4879",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "7e3fa9c03bc3dfbdeb67f24c5d9a49cd",
      "f3454ca93cbb0c8c09b0695d90a0df3d", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "f3454ca93cbb0c8c09b0695d90a0df3d", "1a77d2af4d2b6cf8737cfbcacacdc4e4",
      "89bec831efea2f88129dedcad06bb3fa", "89bec831efea2f88129dedcad06bb3fa",
      "dead0fe4030085c22e92d16bb110de9d", "306a2f5dfd675df4ed9af44fd5cac8c0",
      "306a2f5dfd675df4ed9af44fd5cac8c0", "9d01c946a12f5ef9d9cebd9816e06014",
      "768f63912e43148c13688d7f23281531", "768f63912e43148c13688d7f23281531",
      "2e7927158e7b8e40e7269fc909fb584b", "123028e18c2bfb334e34adb5a4f67de4",
      "123028e18c2bfb334e34adb5a4f67de4", "2c979c2bddef79a760e72a802f83cc76",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "da1a6ff2be03ec8acde4cb1cd519a6f0",
      "a4ca37cb869a0dbd1c4a2dcc449a8f31", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "1b5d1d4c7be8d5ec00a42a49eecf918f", "98b77e88b0784baaea64c98c8707fe46",
      "8148788044522edc3c497e1017efe2ce", "acf60abeda98bbea161139b915317423",
      "262c96b1f2c4f85c86c0e9c77fedff1e", "f35a3d13516440f9168076d9b07c9e98",
      "13782526fc2726100cb3cf375b3150ed", "13c07441b47b0c1ed80f015ac302d220",
      "02880fde51ac991ad18d8986f4e5145c", "aa25073115bad49432953254e7dce0bc",
      "69e3361b7199e10e75685b90fb0df623", "2f8ab35f6e7030e82ca922a68b29af4a",
      "452f91b01833c57db4e909575a029ff6", "1fabf0655bedb671e4d7287fec8119ba",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "d54206c34785cc3d8a06c2ceac46378c",
      "85a11892ed884e3e74968435f6b16e64", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "16434230d24b9522ae2680e8c37e1b95", "963dea92f3efbb99137d1de9c56728d3",
      "b72fb6a9a073c2fe65013af1842dc9b0", "86fa0c299737eb499cbcdce94abe2d33",
      "6b80af04470b83673d98f46925e678a5", "65baca6167fe5249f7a839ce5b2fd591",
      "e47ded6c0eec1d5baadd02aff172f2b1", "c0950e609f278efb7050d319a9756bb3",
      "9051290279237f9fb1389989b142d2dd", "34cdc1be291c95981c98812c5c343a15",
      "5b64a6911cb7c3d60bb8f961ed9782a2", "7133de9d03a4b07716a12226b5e493e8",
      "3594eff52d5ed875bd9655ddbf106fae", "90d7e13aa2f9a064493ff2b3b5b12109",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "b1f26ee13df2e14a757416ba8a682278",
      "996b6c166f9ed25bd07ea6acdf7597ff", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "34895d4c69a6c3303693e6f431bcd5d8", "c9497b00cb1bc3363dd126ffdddadc8e",
      "1e461869bb2ee9b6069c5e52cf817291", "8d7f1d7ea6a0dcc922ad5d2e77bc74dd",
      "138855d9bf0ccd0c62ac14c7bff4fd37", "64035142864914d05a48ef8e013631d0",
      "205904fa3c644433b46e01c11dd2fe40", "291425aaf8206b20e88db8ebf3cf7e7f",
      "cb6238b8eb6b72980958e6fcceb2f2eb", "626321a6dfac542d0fc70321fac13ff3",
      "1c6fda7501e0f8bdad972f7857cd9354", "4fd485dadcb570e5a0a5addaf9ba84da",
      "d3f140aea9e8eabf4e1e5190e0148288", "e4938219593bbed5ae638a93f2f4a580",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "238980eebc9e63ae3eea2771c7a70f12",
      "0eac13431bd7d8a573318408a72246d5", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "73438155feb62595e3e406921102d748", "5871e0e88a776840d619670fbf107858",
      "1c6376ce55c9ee9e35d432edb1ffb3b7", "d675e0195c9feca956e637f3f1959f40",
      "b5681673903ade13d69e295f82fdd009", "3c43020105ae93a301404b4cd6238654",
      "dd2c5880a94ed3758bfea0b0e8c78286", "4ebb1a7b25a39d8b9868ec8a1243103f",
      "d34ec07845cd8523651e5f5112984a14", "2ce55308d873f4cd244f16da2b06e06e",
      "a4bb5d5ff4b25f391265b5231049a09a", "c9106e0c820b03bcdde3aa94efc11a3e",
      "7ec2eae9e118506da8b33440b399511a", "78de867c8ee947ed6d29055747f26949",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "e552466a4e7ff187251b8914b084d404",
      "981b7c44b6f7b7ac2acf0cc4096e6bf4", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "a4c75372af36162831cb872e24e1088c", "497271227a70a72f9ad25b415d41563f",
      "c48bd7e11ec44ba7b2bc8b6a04592439", "0960a9af91250e9faa1eaac32227bf6f",
      "746c2e0f96ae2246d534d67102be068c", "d6f6db079da9b8909a153c07cc9d0e63",
      "7c8928a0d769f4264d195f39cb68a772", "db645c96fc8be04015e0eb538afec9ae",
      "946af3a8f5362def5f4e27cb0fd4e754", "7ad78dfe7bbedf696dd58d9ad01bcfba",
      "f0fd9c09d454e4ce918faa97e9ac10be", "af6ae5c0eb28417bd251184baf2eaba7",
      "866f8df540dd3b58ab1339314d139cbd", "72803589b453a29501540aeddc23e6f4",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "aba5d5ef5e96fe418e65d20e506ea834",
      "d70bf16e2a31e90b7b3cdeaef1494cf9", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "6df80bb7f264f4f285d09a4d61533fae", "c8831118d1004a7cca015a4fca140018",
      "b7f82c140369067c105c7967c75b6f9e", "130f47aae365aabfec4360fa5b5ff554",
      "92483ed631de21b685ffe6ccadbbec8f", "cbb6ab31547df6b91cfb48630fdffb48",
      "1eea5e8a24d6aa11778eb3e5e5e9c9f2", "9e193b6b28ce798c44c744efde19eee9",
      "885c384d90aaa34acd8303958033c252", "8110ed10e7234851dff3c7e4a51108a2",
      "6fb9383302eb7e7a13387464d2634e03", "864d51fcc737bc73a3f588b67515039a",
      "2ecb7890f00234bcb28c1d969f489012", "c4793d431dbf2d88826bb440bf027512",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "972aeba65e8a6d20dd0f95279be2aa75",
      "34165457282e2af2e9b3f5840e4dec5d", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "b8c5582b9bbb789c45471f93be83b41f", "257bf5467db570974d7cf2356bacf116",
      "5255dded79f56b0078543b5a1814a668", "ef745100f5f34c8ff841b2b0b57eb33f",
      "edae8ed67286ca6a31573a541b3deb6f", "01adcd8bf15fbf70df47fbf3a953aa14",
      "ba539808a8501609ce052a1562a62b25", "ac8e6391200cec2abdebb00744a2ba82",
      "54b17120f7d71ddb4d70590ecd231cc1", "f6e36446a97611a4db4425df926974b2",
      "a82f4080699300b659bbe1b5c4463147", "ecedb178f7cad3dc1b921eca67f9efb6",
      "0609ca0ff3ca90069e8b48829b4b0891", "839e86c681e97359f7819c766000dd1c",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetConvolveScaleDigest10bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 2] = {
      "27e21eb31687f9fbd0a66865fa8d7c8a", "9bff726c8e1d0998451a3b9cf2b3d8c8",
      "661d74cfef36f12ed8d9b4c3ccb7fe0d", "5fc365fd1fcc9599dd97a885ba0c2eec",
      "acdba2c82a6268e3c0ae8fc32be1b41f", "a5db60bbeaf56ab030ed21c42d553cf3",
      "1228bb633f9fd63fdb998b775ca79e98", "07812c97f9f43a2a8ae07329dc488699",
      "903525fb782119c4dfaf61b98a310c9f", "f38b51cef38b929e317861ccbc73ecd8",
      "b78b05138e1d5fbf089144c42ce03058", "f2e227664cbf2d821b242a34fcbc9835",
      "cb992dac70591e7d3663588ae13b9adc", "f2292d33657d939fa85ea5bacdfe39a3",
      "7049dc742d6d8ad6f5d4309968ff281c", "e4beebde1ac335a4d92e4af94653a2ce",
      "cc77875f98f54b9b26b5f7d9fcbc828d", "fb623f7b9e1ffcf2ae361599728a5589",
      "c33847e47a7eda214734084640818df9", "ab3e1aec3d720c0c89c46a8d5b161b44",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "efe4de861dcf0f7458b6208cae7e3584",
      "814751c55fa84f0fed94ff15fc30fc24", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "31a63fe47297102937acbe7a328588b7", "b804a0a24633243f7da48d7a5f51c0bf",
      "cb492672b005fc378cccc8c03003cd4a", "1d18732bcf2ea487e84579489cc59a22",
      "457c4b3ec38a8d6c210584ade1a9fae2", "a3afdd468e6a5238a3dbd2cc21c11c9e",
      "6ff8a16f21d6e8a9741dacf0734ae563", "3ffa29ef7e54e51f6849c9a3d3c79d03",
      "af89899b083cf269ac1bd988aeb15b15", "3365d8411c11081fb228436238b9a671",
      "3ba56d30f5f81d7098f356635a58b9af", "b3013776900c6520bd30f868e8c963b6",
      "81febaa7342692483040f500ba2e5e2b", "4a51ff1d9a4a68687d590b41aa7835a3",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetConvolveDigest12bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 16] = {
      "e25031afae184cc4d186cde7e3d51e33", "6fb55dec2506dae6c229469cdf2e7d83",
      "9df34d27f5bd040d1ed1455b151cd1ff", "7f6829458f00edb88f78851dd1a08739",
      "a8bbe9b6b9eaf6f681d91c981b994949", "21f74980b36cb246426f4bc3fe7c08c3",
      "403c2ccced3b5b141a4c7559c0cd841b", "1c3c4c6cd8a3e79cd95d6038531b47e5",
      "f18d6950d36619086ac0055bab528cb1", "37d9c5babddf24fe8cb061297297b769",
      "c111000d4021381f3d18ea0e24a1b5f5", "4e1e4f0a592ff028e35f8187627d6e68",
      "0ca9ad4614d32da05746f8712a46d861", "8a122ab194e2fdb7089b29be50af8c86",
      "3c21326e22a80982d1b0ffc09be4beae", "f6c8d1fe2c1fb19604c49c6a49bd26a8",
      "d3eda9d7aa80e4ea1f18bf565b143e57", "fe21bd1cb8e90466dc727f2223ea7aed",
      "01efe3df83c715325aaddd4d4ce130ad", "ecaa751360121d3746b72932415fb998",
      "291e67095399651dc5c8a033390f255f", "66b26828e434faf37ddc57d3e0abb6db",
      "e9cd69e9ba70864e3d0b175ac0a177d6", "64e4db895a843cb05384f5997b1ba978",
      "f305161c82de999d2c93eac65f609cfe", "4762b2bd27983ad916ec0a930c0eca6b",
      "1631495ffae43a927267ebd476015ef1", "b0f22de7b10057e07af71f9bce4615ce",
      "6fa29dc4be1a46d246a41d66a3d35cb4", "734601c2185bdf30ba9ded8b07003a05",
      "524e4553d92c69e7e4ed934f7b806c6b", "3709c8950bc5fcc4a2b3ec68fc78bf7e",
      "69c274d9f8e0fd6790495e9695251f1f", "ee30cc1232c27494ef53edd383568f25",
      "e525dbeb0b4341952a92270dcfc51730", "b3685c9e783d3402497bbd49d28c7dd7",
      "d1c9f02dc818e6b974794dfb7749aac8", "bdb9e4961f9aa8c25568d3394e968518",
      "f5f74555adcad85f3ebd3cb85dc7b770", "737e2a0be806dbd701014f2078be7898",
      "20a18294e3a9422193aa0a219fd80ede", "7106648ecb9ae24a54d1dbabf2a9e318",
      "20f39cbd6b5ed87a6ae4f818932325c0", "a99666e3157e32a07c87b01e52091a76",
      "123e4d533d478c3089c975323c85396b", "d2a8021f7683a0cdf2658418fa90a6fc",
      "1437e192a3349db8702d5b90eb88dbc1", "fe097fc4aeed7cda0b0f405124efb19d",
      "1988227c51fa589db1307fd890bb5972", "537e25a6c30b240dc1e3bddd1c3a0a03",
      "aebe5cffaae448db5a08987a3375a428", "7127ae9bdc63df4459590dc02ca95403",
      "7ad281903a210f2b1f39f7c40c8df272", "d4b97ba21f7b400ba9f9cd8bb1a576df",
      "0884a824203aaf72c78f73fdaf2b23a2", "63d60388605c92daee55d517de622a9e",
      "171ec49a779de1efa69510eefbd09bba", "541cf051579c5a10b9debd3bfdcb7f32",
      "91c14451ad93ed88e96b5d639ce408de", "3b0313ec0e043d19744bf88c90e875a1",
      "6adcb3cee91fe3a83b36deb11c5ad6dd", "c6d4bfad24616a88222681992a99d782",
      "515dc0f2a41730d5c434e4f3c81b02c3", "1c69abdee3b9608a6094034badc2bec0",
      "1785a0f321d7dd90aa8846961737a767", "dd12c5b8c341f2423d0d5db4f285d199",
      "5741fb69aae1ca8a0fbe4f1478df88ef", "a4390ceb4e4e9f5cf6a47a9b11a97015",
      "6778eb25df902092b440c3402e7f0f80", "5ad9d6b36f8898bb55e901c1c0c523da",
      "73969b6c03bb5a7345a8b968b542668e", "f48192947e66d70f116193a4186d0186",
      "53f60d0e89d7d994ec6d6131fb7e75ae", "c75f6f8813839ae3cf192baa29039265",
      "9ff0852ebbad56663250f86ac3a3bf9b", "668938580a770ea7ace8bbf7d349e89f",
      "5b06bb0a15ac465a250d9b209f05289f", "a2128f5c8692fed7e7c1c7af22ce9f72",
      "f80f1d7a58869ec794258c0f7df14620", "ed1e03a35924c92ed2fc9808dc3f06f3",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "35ef89c35d2e8e46feb856c554c21c9f",
      "b98ce33a1bf4fab840b7ef261b30dbc4", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "b98ce33a1bf4fab840b7ef261b30dbc4", "42263fb359c4fdf1c7cdb4980b3e97f2",
      "1e7071b7db3144188bdcf5d199fe5355", "1e7071b7db3144188bdcf5d199fe5355",
      "30d367304a87bd25f0ad2ff8e4b5eb41", "4abe6dbb3198219015838dbedf07297a",
      "4abe6dbb3198219015838dbedf07297a", "acec349a95b5bba98bb830372fa15e73",
      "a73ad8661256ce2fdf5110425eb260b2", "a73ad8661256ce2fdf5110425eb260b2",
      "8ff2f049d3f972867f14775188fe589b", "87f5f9a07aea75c325e6d7ff6c96c7c2",
      "87f5f9a07aea75c325e6d7ff6c96c7c2", "325fcde7d415d7aa4929a3ea013fb9cc",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "05aa29814d5ce35389dbcf20368850da",
      "fbb89f907a040e70953e3364dbe1feda", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "44ac511baf45032078cc0b45e41dba79", "efb98974adc58d88e122293436bb9184",
      "7eee18c1a16bcb4e7ef7b27f68ba884f", "b0904c9b118dd9a1f9f034c0ff82d1c1",
      "54436deb5183dd9669dd4f5feadb3850", "4db1c310b7d9a8bd3e2b5d20fa820e3b",
      "c40abc6b2d67527f48a287cd7e157428", "48ec3fcf509805f484c8e0948c3469be",
      "cb7d4a76fa7de52ed2fe889785327b38", "f57983346815fa41e969c195c1c03774",
      "7dba59b0de2c877666ded6bdaefdcc30", "4837f8ba2f67f17f28a38c5e2a434c73",
      "09e06fe9dc7ef7818f2a96895235afd4", "002976970ec62b360f956b9c091782d4",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "78673b1084367e97b8dd83990adc5219",
      "06b5d4a30b9efb6c1d95ef7957f49e76", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "ce460146922cd53970b510d460aa4062", "6fd051938b8efcec9ece042f1edc177a",
      "f5ff0dcfe3c1a56e3856549d8ded416b", "b69bc2cfc17c6b4313264db96831f0d1",
      "38a5e65bd71934becfb376eb3b9bc513", "32c1163aa4ca6b6c69d950aba7b06d48",
      "0c22a6c014c6347983de4ca863f3b53f", "a80c5ee9eb2dfb9a0d56e92eb3f85d91",
      "a9719722a150a81175427bc161b95d7a", "8237befd456131a488cc5b8b63f4aca5",
      "51616abcd0beea53a78ffce106b974fc", "6c47b22270f01d27b404da07e1be1202",
      "356268298d3887edaabd0169a912c94e", "d2b00216e106cb8c5450e2eff1f8481a",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "c2de3a582c79aee811076211c497d2bc",
      "d1b6d9c73da41def26dd4f85fbd1bde8", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "d8374eb7825081b89f74b05c66bccd63", "d5f7d68c10b5eaf0fba6f93ee26266e6",
      "94d19cb65f29db65e6656b588f431ade", "5126e95f0249024a6f6d426714bd5b20",
      "d7d3654b9c2dabe13239875984770acd", "6491afd5d651aab80aa179b579b74341",
      "037a5de0de89983808f8e8f6dc39110f", "5980073b7685c5c9b2ec027e06be2cbc",
      "0abb9d035aca426b62ca0f3fab063bab", "fe002a902bb4ec24dfe3ea0fe381a472",
      "1ac15726df1aa2cd8855162a91893379", "0758c3ac16467605d73c725a697c3dc1",
      "97d894d85f6ccfa4ff81e0e8fdf03da1", "c3c7b362f063a18244ea542a42d2873c",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "7f6829458f00edb88f78851dd1a08739",
      "a8bbe9b6b9eaf6f681d91c981b994949", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "403c2ccced3b5b141a4c7559c0cd841b", "1c3c4c6cd8a3e79cd95d6038531b47e5",
      "f18d6950d36619086ac0055bab528cb1", "37d9c5babddf24fe8cb061297297b769",
      "c111000d4021381f3d18ea0e24a1b5f5", "4e1e4f0a592ff028e35f8187627d6e68",
      "0ca9ad4614d32da05746f8712a46d861", "8a122ab194e2fdb7089b29be50af8c86",
      "3c21326e22a80982d1b0ffc09be4beae", "f6c8d1fe2c1fb19604c49c6a49bd26a8",
      "d3eda9d7aa80e4ea1f18bf565b143e57", "fe21bd1cb8e90466dc727f2223ea7aed",
      "01efe3df83c715325aaddd4d4ce130ad", "ecaa751360121d3746b72932415fb998",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "67b2ea94cc4d0b32db3ae3c29eee4d46",
      "bcfec99ad75988fa1efc1733204f17f2", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "79c222c5796e50119f1921e7bc534a25", "ae3f7c189458f044e9c52399d37a55e2",
      "fd6dde45bd2937331428de9ef4f8e869", "b384d065423f3d271b85781d76a73218",
      "466ea0680c06f59e8b3bb293608731fb", "360541ba94f42d115fe687a97a457ffb",
      "e5a0794d37af40c40a4d2c6d3f7d2aa2", "4eed285651a75614bd60adebbe2e185c",
      "bbdbf93942282d7b9c4f07591a1764a6", "1288a9ec3e6f79213b6745e6e7568c44",
      "4ff1310bfd656d69ed5c108a91a9b01a", "3380806b5f67eb3ebce42f8e7c05b256",
      "09c4bdf0f30aca6812fb55a5ac06b1bd", "722c86ba6bf21f40742ee33b4edc17c4",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "f5303c96d1630f9840eaaba058cd818b",
      "c20cd45782b2f52c05e4189912047570", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "d6360f96fe15a3ee1e903b0a53dcaaeb", "4b18995cdf2f5d18410d3a732c5932b1",
      "6f62bf7395c3dfccc1565ba8424f20e8", "c9987ed30491cd28bbc711dd57228247",
      "8e277ec837cbecf529ae2eb0578fddc1", "c0c132386f23c5f0fba055a12fb79547",
      "6b5617ab78dd0916690dfa358298b7b3", "394abedca37f60d1a5148a4c975305ed",
      "bb88881e0e4cf2d88c2d2b38b5833f20", "bef10806be8d58ea8e97870a813b075e",
      "b4b017d1f792bea69d3b773db7c80c7c", "0660bc63041213a8a4d74724a3bc4291",
      "5050c8c5388a561691fd414b00c041df", "9ed40c68de6a8008a902d7224f8b620f",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "ec10ce4a674424478a401847f744251d",
      "bdd897eafc8ef2651a7bba5e523a6ac2", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "2745de4a6b29abb85ee513e22ad362c3", "8aaad384b7cd349b4b968e657ec15523",
      "fb6c0723432bcd2246d51a90f5fb5826", "f8104ed5921ebd48c6eed16150ffe028",
      "85c2e236b3e32bf731601237cf0594cd", "8bd6eefff9640766cdf64ab082cb1485",
      "78b5cd9dde6c6a5900f3040c57172091", "aaa980506bd7bb1d75924a8025698d1a",
      "90050a411d501f7166f6741832b0c342", "d6ec88b2c38e32511f3359e1d5f9d85b",
      "96506b8b39274c8fe687ea39761997f1", "3322ea83995c2762fb60db993b401658",
      "151b6e4ce60392639982fca5a73ac3d3", "d52a1038e135bef233674a843f8c7cb6",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

const char* GetConvolveScaleDigest12bpp(int id) {
  // Entries containing 'XXXXX...' are skipped. See the test for details.
  static const char* const kDigest[ConvolveTestParam::kNumBlockSizes * 2] = {
      "aea59b7a638f27acad2b90fd2b8c9fee", "be87ba981a0af25611a7d5f0970be9b3",
      "7c81f1486cd607376d776bf2c6e81dec", "f683ba2a9b353bea35f26c1ed730f3c5",
      "11e2d70daff1726093cb4fcae33ce0d6", "567575eac0dea2f379019b2d4bafe444",
      "216479ed580d6e0d7c1d523015394814", "dcabbe5f5709a4b6634d77cc514e863a",
      "4e888207fe917faeea2b44383ac16caf", "d617c5608fae3b01c507c7e88040fee3",
      "eeac5d9b3dc005e76f13dfc7483eae48", "8ff0a82811f77303c4516bb8c761336f",
      "95a7c315aaa208097b6ab006f1d07654", "da63527ee80e6772435cff8321a29a95",
      "404457f72e7113d1f3797a39319fd3fe", "43cbccfe2663ec11c157319acfe629a5",
      "1dc5b8dee4542f3d7fcf6b0fa325dfde", "16d4506674f2fcedfcd1e006eb097141",
      "4fcf329ddb405cd6bbb0a6fb87e29eb3", "de77a781957653ea1750f79995605cdc",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", "436f6fdc008d94a94bc6f516f98f402f",
      "b436bd9036f08ba7e50cfc536911dbbd", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
      "720a01018856bd83f4d89a9024b14728", "b7e01a3f161007712ce342f59b2c51f2",
      "922420ebe5dec4f19c259ebdf8a3259a", "979aaba579556207a7bbcc939123c1b2",
      "89a30898cbaa4d64f9072173e8365864", "0586ff961f2e4228f4e38299fb25ae07",
      "adb27a03f8b1b50fe2a52b5ca8d4fc28", "4f91cd92aab2e09f4b123251a8d2f219",
      "620fba0fff163d96a1cd663d1af4a4c5", "bf7a0fa65b1a90ba34c834558fa2c86e",
      "c21f7d7d16d047a27b871a7bf8476e2d", "a94b17c81f3ce2b47081bd8dd762a2e5",
      "9078d20f59bc24862af3856acb8c0357", "ee510ce6b3d22de9e4bd7920a26fd69a",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct ConvolveTypeParam {
  ConvolveTypeParam(bool is_intra_block_copy, bool is_compound,
                    bool has_vertical_filter, bool has_horizontal_filter)
      : is_intra_block_copy(is_intra_block_copy),
        is_compound(is_compound),
        has_vertical_filter(has_vertical_filter),
        has_horizontal_filter(has_horizontal_filter) {}
  bool is_intra_block_copy;
  bool is_compound;
  bool has_vertical_filter;
  bool has_horizontal_filter;
};

std::ostream& operator<<(std::ostream& os, const ConvolveTestParam& param) {
  return os << "BlockSize" << param.width << "x" << param.height;
}

std::ostream& operator<<(std::ostream& os, const ConvolveTypeParam& param) {
  return os << "is_intra_block_copy: " << param.is_intra_block_copy
            << ", is_compound: " << param.is_compound
            << ", has_(vertical/horizontal)_filter: "
            << param.has_vertical_filter << "/" << param.has_horizontal_filter;
}

//------------------------------------------------------------------------------
template <int bitdepth, typename Pixel>
class ConvolveTest : public testing::TestWithParam<
                         std::tuple<ConvolveTypeParam, ConvolveTestParam>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  ConvolveTest() = default;
  ~ConvolveTest() override = default;

  void SetUp() override {
    ConvolveInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    GetConvolveFunc(dsp, &base_convolve_func_);

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_convolve_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        ConvolveInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        ConvolveInit_AVX2();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      ConvolveInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
      ConvolveInit10bpp_NEON();
#endif
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    GetConvolveFunc(dsp, &cur_convolve_func_);

    // Skip functions that have not been specialized for this particular
    // architecture.
    if (cur_convolve_func_ == base_convolve_func_) {
      cur_convolve_func_ = nullptr;
    }
  }

 protected:
  int GetDigestId() const {
    int id = param_.block_size;
    id += param_.kNumBlockSizes *
          static_cast<int>(type_param_.has_horizontal_filter);
    id += 2 * param_.kNumBlockSizes *
          static_cast<int>(type_param_.has_vertical_filter);
    id += 4 * param_.kNumBlockSizes * static_cast<int>(type_param_.is_compound);
    id += 8 * param_.kNumBlockSizes *
          static_cast<int>(type_param_.is_intra_block_copy);
    return id;
  }

  void GetConvolveFunc(const Dsp* dsp, ConvolveFunc* func);
  void SetInputData(bool use_fixed_values, int value);
  void Check(bool use_fixed_values, const Pixel* src, const Pixel* dest,
             libvpx_test::MD5* md5_digest);
  void Check16Bit(bool use_fixed_values, const uint16_t* src,
                  const uint16_t* dest, libvpx_test::MD5* md5_digest);
  // |num_runs| covers the categories of filters (6) and the number of filters
  // under each category (16).
  void Test(bool use_fixed_values, int value,
            int num_runs = kMinimumViableRuns);

  const ConvolveTypeParam type_param_ = std::get<0>(GetParam());
  const ConvolveTestParam param_ = std::get<1>(GetParam());

 private:
  ConvolveFunc base_convolve_func_;
  ConvolveFunc cur_convolve_func_;
  // Convolve filters are 7-tap, which need 3 pixels
  // (kRestorationHorizontalBorder) padding.
  Pixel source_[kMaxBlockHeight * kMaxBlockWidth] = {};
  uint16_t source_16bit_[kMaxBlockHeight * kMaxBlockWidth] = {};
  uint16_t dest_16bit_[kMaxBlockHeight * kMaxBlockWidth] = {};
  Pixel dest_clipped_[kMaxBlockHeight * kMaxBlockWidth] = {};

  const int source_stride_ = kMaxBlockWidth;
  const int source_height_ = kMaxBlockHeight;
};

template <int bitdepth, typename Pixel>
void ConvolveTest<bitdepth, Pixel>::GetConvolveFunc(const Dsp* const dsp,
                                                    ConvolveFunc* func) {
  *func =
      dsp->convolve[type_param_.is_intra_block_copy][type_param_.is_compound]
                   [type_param_.has_vertical_filter]
                   [type_param_.has_horizontal_filter];
}

template <int bitdepth, typename Pixel>
void ConvolveTest<bitdepth, Pixel>::SetInputData(bool use_fixed_values,
                                                 int value) {
  if (use_fixed_values) {
    std::fill(source_, source_ + source_height_ * source_stride_, value);
  } else {
    const int offset =
        kConvolveBorderLeftTop * source_stride_ + kConvolveBorderLeftTop;
    const int mask = (1 << bitdepth) - 1;
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    const int height = param_.height;
    const int width = param_.width;
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        source_[y * source_stride_ + x + offset] = rnd.Rand16() & mask;
      }
    }
    // Copy border pixels to the left and right borders.
    for (int y = 0; y < height; ++y) {
      Memset(&source_[(y + kConvolveBorderLeftTop) * source_stride_],
             source_[y * source_stride_ + offset], kConvolveBorderLeftTop);
      Memset(&source_[y * source_stride_ + offset + width],
             source_[y * source_stride_ + offset + width - 1],
             kConvolveBorderLeftTop);
    }
    // Copy border pixels to the top and bottom borders.
    for (int y = 0; y < kConvolveBorderLeftTop; ++y) {
      memcpy(&source_[y * source_stride_],
             &source_[kConvolveBorderLeftTop * source_stride_],
             source_stride_ * sizeof(Pixel));
      memcpy(&source_[(y + kConvolveBorderLeftTop + height) * source_stride_],
             &source_[(kConvolveBorderLeftTop + height - 1) * source_stride_],
             source_stride_ * sizeof(Pixel));
    }
  }
}

template <int bitdepth, typename Pixel>
void ConvolveTest<bitdepth, Pixel>::Check(bool use_fixed_values,
                                          const Pixel* src, const Pixel* dest,
                                          libvpx_test::MD5* md5_digest) {
  if (use_fixed_values) {
    // For fixed values, input and output are identical.
    const bool success =
        test_utils::CompareBlocks(src, dest, param_.width, param_.height,
                                  kMaxBlockWidth, kMaxBlockWidth, false, false);
    EXPECT_TRUE(success);
  } else {
    // For random input, compare md5.
    const int offset =
        kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
    const size_t size = sizeof(dest_clipped_) - offset * sizeof(Pixel);
    md5_digest->Add(reinterpret_cast<const uint8_t*>(dest), size);
  }
}

template <int bitdepth, typename Pixel>
void ConvolveTest<bitdepth, Pixel>::Check16Bit(bool use_fixed_values,
                                               const uint16_t* src,
                                               const uint16_t* dest,
                                               libvpx_test::MD5* md5_digest) {
  if (use_fixed_values) {
    // For fixed values, input and output are identical.
    const bool success =
        test_utils::CompareBlocks(src, dest, param_.width, param_.height,
                                  kMaxBlockWidth, kMaxBlockWidth, false);
    EXPECT_TRUE(success);
  } else {
    // For random input, compare md5.
    const int offset =
        kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
    const size_t size = sizeof(dest_16bit_) - offset * sizeof(uint16_t);
    md5_digest->Add(reinterpret_cast<const uint8_t*>(dest), size);
  }
}

template <int bitdepth, typename Pixel>
void ConvolveTest<bitdepth, Pixel>::Test(
    bool use_fixed_values, int value, int num_runs /*= kMinimumViableRuns*/) {
  // There's no meaning testing fixed input in compound convolve.
  if (type_param_.is_compound && use_fixed_values) return;

  // There should not be any function set for this combination.
  if (type_param_.is_intra_block_copy && type_param_.is_compound) {
    ASSERT_EQ(cur_convolve_func_, nullptr);
    return;
  }

  // Compound and intra block copy functions are only used for blocks 4x4 or
  // greater.
  if (type_param_.is_compound || type_param_.is_intra_block_copy) {
    if (param_.width < 4 || param_.height < 4) {
      GTEST_SKIP();
    }
  }

  // Skip unspecialized functions.
  if (cur_convolve_func_ == nullptr) {
    GTEST_SKIP();
  }

  SetInputData(use_fixed_values, value);
  int subpixel_x = 0;
  int subpixel_y = 0;
  int vertical_index = 0;
  int horizontal_index = 0;
  const int offset =
      kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
  const Pixel* const src = source_ + offset;
  const ptrdiff_t src_stride = source_stride_ * sizeof(Pixel);
  const ptrdiff_t src_stride_16 = source_stride_;
  const ptrdiff_t dst_stride = kMaxBlockWidth * sizeof(Pixel);
  // Pack Compound output since we control the predictor buffer.
  const ptrdiff_t dst_stride_compound = param_.width;

  // Output is always 16 bits regardless of |bitdepth|.
  uint16_t* dst_16 = dest_16bit_ + offset;
  // Output depends on |bitdepth|.
  Pixel* dst_pixel = dest_clipped_ + offset;

  // Collect the first |kMinimumViableRuns| into one md5 buffer.
  libvpx_test::MD5 md5_digest;

  absl::Duration elapsed_time;
  for (int i = 0; i < num_runs; ++i) {
    // Test every filter.
    // Because of masking |subpixel_{x,y}| values roll over every 16 iterations.
    subpixel_x += 1 << 6;
    subpixel_y += 1 << 6;

    const int horizontal_filter_id = (subpixel_x >> 6) & 0xF;
    const int vertical_filter_id = (subpixel_y >> 6) & 0xF;

    // |filter_id| == 0 (copy) must be handled by the appropriate 1D or copy
    // function.
    if (horizontal_filter_id == 0 || vertical_filter_id == 0) {
      continue;
    }

    // For focused speed testing these can be set to the desired filter. Want
    // only 8 tap filters? Set |{vertical,horizontal}_index| to 2.
    vertical_index += static_cast<int>(i % 16 == 0);
    vertical_index %= 4;
    horizontal_index += static_cast<int>(i % 16 == 0);
    horizontal_index %= 4;

    if (type_param_.is_compound) {
      // Output type is uint16_t.
      const absl::Time start = absl::Now();
      cur_convolve_func_(src, src_stride, horizontal_index, vertical_index,
                         horizontal_filter_id, vertical_filter_id, param_.width,
                         param_.height, dst_16, dst_stride_compound);
      elapsed_time += absl::Now() - start;
    } else {
      // Output type is Pixel.
      const absl::Time start = absl::Now();
      cur_convolve_func_(src, src_stride, horizontal_index, vertical_index,
                         horizontal_filter_id, vertical_filter_id, param_.width,
                         param_.height, dst_pixel, dst_stride);
      elapsed_time += absl::Now() - start;
    }

    // Only check the output for the first set. After that it's just repeated
    // runs for speed timing.
    if (i >= kMinimumViableRuns) continue;

    if (type_param_.is_compound) {
      // Need to copy source to a uint16_t buffer for comparison.
      Pixel* src_ptr = source_;
      uint16_t* src_ptr_16 = source_16bit_;
      for (int y = 0; y < kMaxBlockHeight; ++y) {
        for (int x = 0; x < kMaxBlockWidth; ++x) {
          src_ptr_16[x] = src_ptr[x];
        }
        src_ptr += src_stride_16;
        src_ptr_16 += src_stride_16;
      }

      Check16Bit(use_fixed_values, source_16bit_ + offset, dst_16, &md5_digest);
    } else {
      Check(use_fixed_values, src, dst_pixel, &md5_digest);
    }
  }

  if (!use_fixed_values) {
    // md5 sums are only calculated for random input.
    const char* ref_digest = nullptr;
    switch (bitdepth) {
      case 8:
        ref_digest = GetConvolveDigest8bpp(GetDigestId());
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        ref_digest = GetConvolveDigest10bpp(GetDigestId());
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        ref_digest = GetConvolveDigest12bpp(GetDigestId());
        break;
#endif
    }
    ASSERT_NE(ref_digest, nullptr);

    const char* direction;
    if (type_param_.has_vertical_filter && type_param_.has_horizontal_filter) {
      direction = "2D";
    } else if (type_param_.has_vertical_filter) {
      direction = "Vertical";
    } else if (type_param_.has_horizontal_filter) {
      direction = "Horizontal";
    } else {
      direction = "Copy";
    }
    const auto elapsed_time_us =
        static_cast<int>(absl::ToInt64Microseconds(elapsed_time));
    printf("Mode Convolve%s%s%s[%25s]: %5d us MD5: %s\n",
           type_param_.is_compound ? "Compound" : "",
           type_param_.is_intra_block_copy ? "IntraBlockCopy" : "", direction,
           absl::StrFormat("%dx%d", param_.width, param_.height).c_str(),
           elapsed_time_us, md5_digest.Get());
    EXPECT_STREQ(ref_digest, md5_digest.Get());
  }
}

void ApplyFilterToSignedInput(const int min_input, const int max_input,
                              const int8_t filter[kSubPixelTaps],
                              int* min_output, int* max_output) {
  int min = 0, max = 0;
  for (int i = 0; i < kSubPixelTaps; ++i) {
    const int tap = filter[i];
    if (tap > 0) {
      max += max_input * tap;
      min += min_input * tap;
    } else {
      min += max_input * tap;
      max += min_input * tap;
    }
  }
  *min_output = min;
  *max_output = max;
}

void ApplyFilterToUnsignedInput(const int max_input,
                                const int8_t filter[kSubPixelTaps],
                                int* min_output, int* max_output) {
  ApplyFilterToSignedInput(0, max_input, filter, min_output, max_output);
}

// Validate the maximum ranges for different parts of the Convolve process.
template <int bitdepth>
void ShowRange() {
  // Subtract one from the shift bits because the filter is pre-shifted by 1.
  constexpr int horizontal_bits = (bitdepth == kBitdepth12)
                                      ? kInterRoundBitsHorizontal12bpp - 1
                                      : kInterRoundBitsHorizontal - 1;
  constexpr int vertical_bits = (bitdepth == kBitdepth12)
                                    ? kInterRoundBitsVertical12bpp - 1
                                    : kInterRoundBitsVertical - 1;
  constexpr int compound_vertical_bits = kInterRoundBitsCompoundVertical - 1;

  constexpr int compound_offset = (bitdepth == 8) ? 0 : kCompoundOffset;

  constexpr int max_input = (1 << bitdepth) - 1;

  const int8_t* worst_convolve_filter = kHalfSubPixelFilters[2][8];

  // First pass.
  printf("Bitdepth: %2d Input range:            [%8d, %8d]\n", bitdepth, 0,
         max_input);

  int min, max;
  ApplyFilterToUnsignedInput(max_input, worst_convolve_filter, &min, &max);

  if (bitdepth == 8) {
    // 8bpp can use int16_t for sums.
    assert(min > INT16_MIN);
    assert(max < INT16_MAX);
  } else {
    // 10bpp and 12bpp require int32_t.
    assert(min > INT32_MIN);
    assert(max > INT16_MAX && max < INT32_MAX);
  }

  printf("  Horizontal upscaled range:         [%8d, %8d]\n", min, max);

  const int first_pass_min = RightShiftWithRounding(min, horizontal_bits);
  const int first_pass_max = RightShiftWithRounding(max, horizontal_bits);

  // All bitdepths can use int16_t for first pass output.
  assert(first_pass_min > INT16_MIN);
  assert(first_pass_max < INT16_MAX);

  printf("  Horizontal downscaled range:       [%8d, %8d]\n", first_pass_min,
         first_pass_max);

  // Second pass.
  ApplyFilterToSignedInput(first_pass_min, first_pass_max,
                           worst_convolve_filter, &min, &max);

  // All bitdepths require int32_t for second pass sums.
  assert(min < INT16_MIN && min > INT32_MIN);
  assert(max > INT16_MAX && max < INT32_MAX);

  printf("  Vertical upscaled range:           [%8d, %8d]\n", min, max);

  // Second pass non-compound output is clipped to Pixel values.
  const int second_pass_min =
      Clip3(RightShiftWithRounding(min, vertical_bits), 0, max_input);
  const int second_pass_max =
      Clip3(RightShiftWithRounding(max, vertical_bits), 0, max_input);
  printf("  Pixel output range:                [%8d, %8d]\n", second_pass_min,
         second_pass_max);

  // Output is Pixel so matches Pixel values.
  assert(second_pass_min == 0);
  assert(second_pass_max == max_input);

  const int compound_second_pass_min =
      RightShiftWithRounding(min, compound_vertical_bits) + compound_offset;
  const int compound_second_pass_max =
      RightShiftWithRounding(max, compound_vertical_bits) + compound_offset;

  printf("  Compound output range:             [%8d, %8d]\n",
         compound_second_pass_min, compound_second_pass_max);

  if (bitdepth == 8) {
    // 8bpp output is int16_t without an offset.
    assert(compound_second_pass_min > INT16_MIN);
    assert(compound_second_pass_max < INT16_MAX);
  } else {
    // 10bpp and 12bpp use the offset to fit inside uint16_t.
    assert(compound_second_pass_min > 0);
    assert(compound_second_pass_max < UINT16_MAX);
  }

  printf("\n");
}

TEST(ConvolveTest, ShowRange) {
  ShowRange<kBitdepth8>();
  ShowRange<kBitdepth10>();
  ShowRange<kBitdepth12>();
}

using ConvolveTest8bpp = ConvolveTest<8, uint8_t>;

TEST_P(ConvolveTest8bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, 255);
}

TEST_P(ConvolveTest8bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveTest8bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

//------------------------------------------------------------------------------
template <int bitdepth, typename Pixel>
class ConvolveScaleTest
    : public testing::TestWithParam<
          std::tuple<bool /*is_compound*/, ConvolveTestParam>> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  ConvolveScaleTest() = default;
  ~ConvolveScaleTest() override = default;

  void SetUp() override {
    ConvolveInit_C();

    const Dsp* const dsp = GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    base_convolve_scale_func_ = dsp->convolve_scale[is_compound_];

    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
      base_convolve_scale_func_ = nullptr;
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        ConvolveInit_SSE4_1();
      }
    } else if (absl::StartsWith(test_case, "AVX2/")) {
      if ((GetCpuInfo() & kAVX2) != 0) {
        ConvolveInit_AVX2();
      }
    } else if (absl::StartsWith(test_case, "NEON/")) {
      ConvolveInit_NEON();
#if LIBGAV1_MAX_BITDEPTH >= 10
      ConvolveInit10bpp_NEON();
#endif
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }

    cur_convolve_scale_func_ = dsp->convolve_scale[is_compound_];

    // Skip functions that have not been specialized for this particular
    // architecture.
    if (cur_convolve_scale_func_ == base_convolve_scale_func_) {
      cur_convolve_scale_func_ = nullptr;
    }
  }

 protected:
  int GetDigestId() const {
    return param_.block_size +
           param_.kNumBlockSizes * static_cast<int>(is_compound_);
  }

  void SetInputData(bool use_fixed_values, int value);
  void Check(bool use_fixed_values, const Pixel* src, const Pixel* dest,
             libvpx_test::MD5* md5_digest);
  void Check16Bit(bool use_fixed_values, const uint16_t* src,
                  const uint16_t* dest, libvpx_test::MD5* md5_digest);
  // |num_runs| covers the categories of filters (6) and the number of filters
  // under each category (16).
  void Test(bool use_fixed_values, int value,
            int num_runs = kMinimumViableRuns);

  const bool is_compound_ = std::get<0>(GetParam());
  const ConvolveTestParam param_ = std::get<1>(GetParam());

 private:
  ConvolveScaleFunc base_convolve_scale_func_;
  ConvolveScaleFunc cur_convolve_scale_func_;
  // Convolve filters are 7-tap, which need 3 pixels
  // (kRestorationHorizontalBorder) padding.
  // The source can be at most 2 times of max width/height.
  Pixel source_[kMaxBlockHeight * kMaxBlockWidth * 4] = {};
  uint16_t source_16bit_[kMaxBlockHeight * kMaxBlockWidth * 4] = {};
  uint16_t dest_16bit_[kMaxBlockHeight * kMaxBlockWidth] = {};
  Pixel dest_clipped_[kMaxBlockHeight * kMaxBlockWidth] = {};

  const int source_stride_ = kMaxBlockWidth * 2;
  const int source_height_ = kMaxBlockHeight * 2;
};

template <int bitdepth, typename Pixel>
void ConvolveScaleTest<bitdepth, Pixel>::SetInputData(bool use_fixed_values,
                                                      int value) {
  if (use_fixed_values) {
    std::fill(source_, source_ + source_height_ * source_stride_, value);
  } else {
    const int offset =
        kConvolveBorderLeftTop * source_stride_ + kConvolveBorderLeftTop;
    const int mask = (1 << bitdepth) - 1;
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    const int height = param_.height * 2;
    const int width = param_.width * 2;
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        source_[y * source_stride_ + x + offset] = rnd.Rand16() & mask;
      }
    }
    // Copy border pixels to the left and right borders.
    for (int y = 0; y < height; ++y) {
      Memset(&source_[(y + kConvolveBorderLeftTop) * source_stride_],
             source_[y * source_stride_ + offset], kConvolveBorderLeftTop);
      Memset(&source_[y * source_stride_ + offset + width],
             source_[y * source_stride_ + offset + width - 1],
             kConvolveBorderLeftTop);
    }
    // Copy border pixels to the top and bottom borders.
    for (int y = 0; y < kConvolveBorderLeftTop; ++y) {
      memcpy(&source_[y * source_stride_],
             &source_[kConvolveBorderLeftTop * source_stride_],
             source_stride_ * sizeof(Pixel));
      memcpy(&source_[(y + kConvolveBorderLeftTop + height) * source_stride_],
             &source_[(kConvolveBorderLeftTop + height - 1) * source_stride_],
             source_stride_ * sizeof(Pixel));
    }
  }
}

template <int bitdepth, typename Pixel>
void ConvolveScaleTest<bitdepth, Pixel>::Check(bool use_fixed_values,
                                               const Pixel* src,
                                               const Pixel* dest,
                                               libvpx_test::MD5* md5_digest) {
  if (use_fixed_values) {
    // For fixed values, input and output are identical.
    const bool success =
        test_utils::CompareBlocks(src, dest, param_.width, param_.height,
                                  kMaxBlockWidth, kMaxBlockWidth, false, false);
    EXPECT_TRUE(success);
  } else {
    // For random input, compare md5.
    const int offset =
        kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
    const size_t size = sizeof(dest_clipped_) - offset * sizeof(Pixel);
    md5_digest->Add(reinterpret_cast<const uint8_t*>(dest), size);
  }
}

template <int bitdepth, typename Pixel>
void ConvolveScaleTest<bitdepth, Pixel>::Check16Bit(
    bool use_fixed_values, const uint16_t* src, const uint16_t* dest,
    libvpx_test::MD5* md5_digest) {
  if (use_fixed_values) {
    // For fixed values, input and output are identical.
    const bool success =
        test_utils::CompareBlocks(src, dest, param_.width, param_.height,
                                  kMaxBlockWidth, kMaxBlockWidth, false);
    EXPECT_TRUE(success);
  } else {
    // For random input, compare md5.
    const int offset =
        kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
    const size_t size = sizeof(dest_16bit_) - offset * sizeof(uint16_t);
    md5_digest->Add(reinterpret_cast<const uint8_t*>(dest), size);
  }
}

template <int bitdepth, typename Pixel>
void ConvolveScaleTest<bitdepth, Pixel>::Test(
    bool use_fixed_values, int value, int num_runs /*= kMinimumViableRuns*/) {
  // There's no meaning testing fixed input in compound convolve.
  if (is_compound_ && use_fixed_values) return;

  // The compound function is only used for blocks 4x4 or greater.
  if (is_compound_) {
    if (param_.width < 4 || param_.height < 4) {
      GTEST_SKIP();
    }
  }

  // Skip unspecialized functions.
  if (cur_convolve_scale_func_ == nullptr) {
    GTEST_SKIP();
  }

  SetInputData(use_fixed_values, value);
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed() +
                             GetDigestId());
  // [1,2048] for |step_[xy]|. This covers a scaling range of 1/1024 to 2x.
  const int step_x = (rnd.Rand16() & ((1 << 11) - 1)) + 1;
  const int step_y = (rnd.Rand16() & ((1 << 11) - 1)) + 1;
  int subpixel_x = 0;
  int subpixel_y = 0;
  int vertical_index = 0;
  int horizontal_index = 0;
  const int offset =
      kConvolveBorderLeftTop * kMaxBlockWidth + kConvolveBorderLeftTop;
  const int offset_scale =
      kConvolveBorderLeftTop * source_stride_ + kConvolveBorderLeftTop;
  const Pixel* const src_scale = source_ + offset_scale;
  const ptrdiff_t src_stride = source_stride_ * sizeof(Pixel);
  const ptrdiff_t dst_stride = kMaxBlockWidth * sizeof(Pixel);
  // Pack Compound output since we control the predictor buffer.
  const ptrdiff_t dst_stride_compound = param_.width;

  // Output is always 16 bits regardless of |bitdepth|.
  uint16_t* dst_16 = dest_16bit_ + offset;
  // Output depends on |bitdepth|.
  Pixel* dst_pixel = dest_clipped_ + offset;

  // Collect the first |kMinimumViableRuns| into one md5 buffer.
  libvpx_test::MD5 md5_digest;

  absl::Duration elapsed_time;
  for (int i = 0; i < num_runs; ++i) {
    // Test every filter.
    // Because of masking |subpixel_{x,y}| values roll over every 16 iterations.
    subpixel_x += 1 << 6;
    subpixel_y += 1 << 6;

    const int horizontal_filter_id = (subpixel_x >> 6) & 0xF;
    const int vertical_filter_id = (subpixel_y >> 6) & 0xF;

    // |filter_id| == 0 (copy) must be handled by the appropriate 1D or copy
    // function.
    if (horizontal_filter_id == 0 || vertical_filter_id == 0) {
      continue;
    }

    // For focused speed testing these can be set to the desired filter. Want
    // only 8 tap filters? Set |{vertical,horizontal}_index| to 2.
    vertical_index += static_cast<int>(i % 16 == 0);
    vertical_index %= 4;
    horizontal_index += static_cast<int>(i % 16 == 0);
    horizontal_index %= 4;

    // Output type is uint16_t.
    const absl::Time start = absl::Now();
    if (is_compound_) {
      cur_convolve_scale_func_(
          source_, src_stride, horizontal_index, vertical_index, 0, 0, step_x,
          step_y, param_.width, param_.height, dst_16, dst_stride_compound);
    } else {
      cur_convolve_scale_func_(
          source_, src_stride, horizontal_index, vertical_index, 0, 0, step_x,
          step_y, param_.width, param_.height, dst_pixel, dst_stride);
    }
    elapsed_time += absl::Now() - start;

    // Only check the output for the first set. After that it's just repeated
    // runs for speed timing.
    if (i >= kMinimumViableRuns) continue;

    // Convolve function does not clip the output. The clipping is applied
    // later, but libaom clips the output. So we apply clipping to match
    // libaom in tests.
    if (is_compound_) {
      const int single_round_offset = (1 << bitdepth) + (1 << (bitdepth - 1));
      Pixel* dest_row = dest_clipped_;
      for (int y = 0; y < kMaxBlockHeight; ++y) {
        for (int x = 0; x < kMaxBlockWidth; ++x) {
          dest_row[x] = static_cast<Pixel>(Clip3(
              dest_16bit_[y * dst_stride_compound + x] - single_round_offset, 0,
              (1 << bitdepth) - 1));
        }
        dest_row += kMaxBlockWidth;
      }
    }

    if (is_compound_) {
      Check16Bit(use_fixed_values, source_16bit_ + offset_scale, dst_16,
                 &md5_digest);
    } else {
      Check(use_fixed_values, src_scale, dst_pixel, &md5_digest);
    }
  }

  if (!use_fixed_values) {
    // md5 sums are only calculated for random input.
    const char* ref_digest = nullptr;
    switch (bitdepth) {
      case 8:
        ref_digest = GetConvolveScaleDigest8bpp(GetDigestId());
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        ref_digest = GetConvolveScaleDigest10bpp(GetDigestId());
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        ref_digest = GetConvolveScaleDigest12bpp(GetDigestId());
        break;
#endif
    }
    ASSERT_NE(ref_digest, nullptr);

    const auto elapsed_time_us =
        static_cast<int>(absl::ToInt64Microseconds(elapsed_time));
    printf("Mode Convolve%sScale2D[%25s]: %5d us MD5: %s\n",
           is_compound_ ? "Compound" : "",
           absl::StrFormat("%dx%d", param_.width, param_.height).c_str(),
           elapsed_time_us, md5_digest.Get());
    EXPECT_STREQ(ref_digest, md5_digest.Get());
  }
}

using ConvolveScaleTest8bpp = ConvolveScaleTest<8, uint8_t>;

TEST_P(ConvolveScaleTest8bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, 255);
}

TEST_P(ConvolveScaleTest8bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveScaleTest8bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

//------------------------------------------------------------------------------
const ConvolveTestParam kConvolveParam[] = {
    ConvolveTestParam(ConvolveTestParam::kBlockSize2x2),
    ConvolveTestParam(ConvolveTestParam::kBlockSize2x4),
    ConvolveTestParam(ConvolveTestParam::kBlockSize4x2),
    ConvolveTestParam(ConvolveTestParam::kBlockSize4x4),
    ConvolveTestParam(ConvolveTestParam::kBlockSize4x8),
    ConvolveTestParam(ConvolveTestParam::kBlockSize8x2),
    ConvolveTestParam(ConvolveTestParam::kBlockSize8x4),
    ConvolveTestParam(ConvolveTestParam::kBlockSize8x8),
    ConvolveTestParam(ConvolveTestParam::kBlockSize8x16),
    ConvolveTestParam(ConvolveTestParam::kBlockSize16x8),
    ConvolveTestParam(ConvolveTestParam::kBlockSize16x16),
    ConvolveTestParam(ConvolveTestParam::kBlockSize16x32),
    ConvolveTestParam(ConvolveTestParam::kBlockSize32x16),
    ConvolveTestParam(ConvolveTestParam::kBlockSize32x32),
    ConvolveTestParam(ConvolveTestParam::kBlockSize32x64),
    ConvolveTestParam(ConvolveTestParam::kBlockSize64x32),
    ConvolveTestParam(ConvolveTestParam::kBlockSize64x64),
    ConvolveTestParam(ConvolveTestParam::kBlockSize64x128),
    ConvolveTestParam(ConvolveTestParam::kBlockSize128x64),
    ConvolveTestParam(ConvolveTestParam::kBlockSize128x128),
};

const ConvolveTypeParam kConvolveTypeParam[] = {
    ConvolveTypeParam(false, false, false, false),
    ConvolveTypeParam(false, false, false, true),
    ConvolveTypeParam(false, false, true, false),
    ConvolveTypeParam(false, false, true, true),
    ConvolveTypeParam(false, true, false, false),
    ConvolveTypeParam(false, true, false, true),
    ConvolveTypeParam(false, true, true, false),
    ConvolveTypeParam(false, true, true, true),
    ConvolveTypeParam(true, false, false, false),
    ConvolveTypeParam(true, false, false, true),
    ConvolveTypeParam(true, false, true, false),
    ConvolveTypeParam(true, false, true, true),
    // This is left to ensure no function exists for |intra_block_copy| when
    // |is_compound| is true; all combinations aren't necessary.
    ConvolveTypeParam(true, true, false, false),
};

INSTANTIATE_TEST_SUITE_P(C, ConvolveTest8bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(C, ConvolveScaleTest8bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ConvolveTest8bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(NEON, ConvolveScaleTest8bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));
#endif  // LIBGAV1_ENABLE_NEON

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, ConvolveTest8bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(SSE41, ConvolveScaleTest8bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));
#endif  // LIBGAV1_ENABLE_SSE4_1

#if LIBGAV1_ENABLE_AVX2
INSTANTIATE_TEST_SUITE_P(AVX2, ConvolveTest8bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(AVX2, ConvolveScaleTest8bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));
#endif  // LIBGAV1_ENABLE_AVX2

#if LIBGAV1_MAX_BITDEPTH >= 10
using ConvolveTest10bpp = ConvolveTest<10, uint16_t>;

TEST_P(ConvolveTest10bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, (1 << 10) - 1);
}

TEST_P(ConvolveTest10bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveTest10bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

using ConvolveScaleTest10bpp = ConvolveScaleTest<10, uint16_t>;

TEST_P(ConvolveScaleTest10bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, (1 << 10) - 1);
}

TEST_P(ConvolveScaleTest10bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveScaleTest10bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

INSTANTIATE_TEST_SUITE_P(C, ConvolveTest10bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(C, ConvolveScaleTest10bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, ConvolveTest10bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(NEON, ConvolveScaleTest10bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));
#endif  // LIBGAV1_ENABLE_NEON

#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using ConvolveTest12bpp = ConvolveTest<12, uint16_t>;

TEST_P(ConvolveTest12bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, (1 << 12) - 1);
}

TEST_P(ConvolveTest12bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveTest12bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

using ConvolveScaleTest12bpp = ConvolveScaleTest<12, uint16_t>;

TEST_P(ConvolveScaleTest12bpp, FixedValues) {
  Test(true, 0);
  Test(true, 1);
  Test(true, 128);
  Test(true, (1 << 12) - 1);
}

TEST_P(ConvolveScaleTest12bpp, RandomValues) { Test(false, 0); }

TEST_P(ConvolveScaleTest12bpp, DISABLED_Speed) {
  const int num_runs = static_cast<int>(1.0e7 / (param_.width * param_.height));
  Test(false, 0, num_runs);
}

INSTANTIATE_TEST_SUITE_P(C, ConvolveTest12bpp,
                         testing::Combine(testing::ValuesIn(kConvolveTypeParam),
                                          testing::ValuesIn(kConvolveParam)));
INSTANTIATE_TEST_SUITE_P(C, ConvolveScaleTest12bpp,
                         testing::Combine(testing::Bool(),
                                          testing::ValuesIn(kConvolveParam)));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
