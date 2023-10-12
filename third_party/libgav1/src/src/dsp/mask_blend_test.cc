// Copyright 2020 The libgav1 Authors
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

#include "src/dsp/mask_blend.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <type_traits>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/cpu.h"
#include "src/utils/memory.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kNumSpeedTests = 50000;
// mask_blend is applied to compound prediction values when is_inter_intra is
// false. This implies a range far exceeding that of pixel values. The ranges
// include kCompoundOffset in 10bpp and 12bpp.
// see: src/dsp/convolve.cc & src/dsp/warp.cc.
constexpr int kCompoundPredictionRange[3][2] = {
    // 8bpp
    {-5132, 9212},
    // 10bpp
    {3988, 61532},
    // 12bpp
    {3974, 61559},
};

const char* GetDigest8bpp(int id) {
  static const char* const kDigest[] = {
      "4b70d5ef5ac7554b4b2660a4abe14a41", "64adb36f07e4a2c4ea4f05cfd715ff58",
      "2cd162cebf99724a3fc22d501bd8c8e4", "c490478208374a43765900ef7115c264",
      "b98f222eb70ef8589da2d6c839ca22b8", "54752ca05f67b5af571bc311aa4e3de3",
      "5ae48814dd285bfca4f5ee8e339dca99", "383f3f4f47563f065d1b6068e5931a24",
      "344b2dab7accd8bd0a255bee16207336", "0b2f6f755d1547eea7e0172f8133ea01",
      "310dc6364fdacba186c01f0e8ac4fcb7", "c2ee4673078d34971319c77ca77b23d1",
      "b0c9f08b73d9e5c16eaf5abdbca1fdc0", "eaad805999d949fa1e1bbbb63b4b7827",
      "6eb2a80d212df89403efb50db7a81b08", "c30730aa799dba78a2ebd3f729af82c7",
      "4346c2860b23f0072b6b288f14c1df36", "1cdace53543063e129a125c4084ca5d7",
      "1ae5328e0c0f4f2bec640d1af03b2978", "3860e040fbee0c5f68f0b4af769209b3",
      "e9480ded15d9c38ee19bf5fa816dd296", "4e17c222b64f428df29938a8120ca256",
      "2a943bc6de9b29c8bcae189ad3bec276", "b5a6bc02c76fa61040678fb2c6c112d2",
      "2c11bb9bd29c5577194edb77cfd1c614", "31ed1832810ae385f4ad8f57795dde1e",
      "eb87d647839c33984dfb25bac0e7cdb3", "f652ec2b1478e35acb19cf28042ee849",
      "0cfb18ac0cb94af1447bcac32ac20c36", "e152bbbf5ee4b40b7b41ec1f2e901aaa",
      "f17f78fd485f7beafa8126c1cda801d7", "9f9fbee0cc9d99435efd3dff644be273",
      "9b498843d66440c1e68dc7ab04f57d42", "2f2b0beceb31b79ccb9179991629e4b8",
      "e06a6ebb6791529bb23fe5b0a9914220", "2b3d1ff19812a17c17b1be1f1727815e",
      "d0bbdecec414950ed63a8a35c2bae397", "8e53906c6513058d7f17013fe0d32bf1",
      "be0690efd31f0bf3c2adcd27ca011ed5", "c2b26243c5f147fdeadf52735aa68fb5",
      "94bb83e774d9189c5ee04fb361855e19", "dad6441e723791a91f31a56b2136fd33",
      "10ccac76a2debb842a0685a527b6a659", "346fb0a4914b64dda3ca0f521412b999",
      "d7e400b855502bbb4f2b8294e207bb96", "3487503f2d73ec52f25b5e8d06c81da4",
      "3f49c096acfcf46d44ce18b48debca7c", "8ed6a745a2b5457ac7f3ac145ce57e72",
      "21f9dda5ef934a5ee6274b22cc22f93b", "507b60611afeb373384d9b7606f7ea46",
      "ac766fadcdb85a47ad14a6846b9e5c36", "fde149bc2162e02bbc5fa85cc41641a5",
      "f5f094b5742d0a920ba734b017452d24", "c90d06b0c76a0983bd1428df2a1b64b3",
      "3649e6a6ed9f69e3f78e0b75160fb82a", "1d44b7649497e651216db50d325e3073",
      "948fa112e90e3ca4d15f3d2f2acfab9a", "9bb54c0f7d07c0b44c44ba09379a04ff",
      "228261ab6f098f489a8968cff1e1f7ae", "5e128db7462164f7327d1d8feeb2e4c7",
      "9e8b97f6d9d482d5770b138bd1077747", "81563d505a4e8dd779a089abf2a28b77",
      "b7157451de7cfa161dff1afd7f9b8622", "6a25cc0a4aaf8a315d1158dbb0ec2966",
      "303867ee010ba51da485ee10149c6f9b", "63b64b7527d2476e9ae5139b8166e8c9",
      "cfa93c2aeeb27a1190a445a6fee61e15", "804bcff8709665eed6830e24346101be",
      "829947ed3e90776cda4ae82918461497", "1df10a1cb80c1a81f521e7e0f80b4f99",
      "3c9593e42ac574f3555bb8511d438a54", "eecef71492c0626685815e646f728f79",
      "0c43d59f456ddca2449e016ae4e34be7", "207d4ac2579f1271fc9eca8d743917b3",
      "3c472bb0b1c891ffda19077ebb659e48", "a4ae7a0d25113bc0238fa27409f9c0dd",
      "e8ad037ca81f46774bb01d20f46671ce", "b22741e4fe0e4062e40a2decec102ffd",
      "c72f9e7bc0170163cb94da0faa0d3ffb", "accaf5d475d155cbd3a8c113f90718bc",
      "2fd31e72444ea258380c16881580de81", "8a6a2a253f6f5b0ff75ba39488e6b082",
      "c5e8159c0f3ebb7536e84ab3dadac1b3", "ef7ec20b46c7dcf16591835642bd68ef",
      "0c3425399dc64870d726c2837666a55e", "0365029ffbfc4cedf3bf2d757ea5b9df",
      "836aa403254af2e04d4b7a7c4db8bfc5", "7f2f3f9c91677b233795169f9a88b2b2",
      "9fc8bbe787244dac638c367b9c611d13", "f66ef45fae8e163ab0f0f393531dad26",
      "beb984e88b6f9b96ae6efe5da23ad16b", "1083b829ea766b1d4eb0bb96e9fb3bff",
      "be8abad1da69e4d238a45fc02a0061cf",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigest10bpp(int id) {
  static const char* const kDigest[] = {
      "1af3cbd1616941b59e6a3f6a417b6312", "1d8b3f4b9d5d2f4ff5be8e81b7243121",
      "e767350f150a84ac5a06dc348e815d62", "53a3a76bf2bcd5761cd15fc739a4f4e1",
      "7597f69dc19a584280be0d67911db6a6", "e1221c172843dc6c1b345bcd370771cc",
      "1a640c71ff9bb45505d89761f19efa8f", "e192f64322e0edb250b52f63aaa4de97",
      "2ccbe012ca167114b14c3ba70befa960", "0f68632d7e5faddb4554ca430d1df822",
      "8caa0061a26e142b783951d5abd7bf5d", "b01eeed3ec549e4a593100d9c5ba587a",
      "1cce6acdbd8ca8d2546ba937584730bf", "022913e87a3c1a86aaefe2c2d4f89882",
      "48f8ab636ba15a06731d869b603cbe58", "ba1616c990d224c20de123c3ccf19952",
      "346a797b7cb4de10759e329f8b49e077", "d4929154275255f2d786d6fc42c7c5d3",
      "18a6af6f36ca1ea4ab6f5a76505de040", "0c43e68414bfc02f9b20e796506f643b",
      "9f483f543f6b1d58e23abf9337ed6fe6", "e114860c2538b63f1be4a23560420cdc",
      "da8680798f96572c46155c7838b452c3", "20b47a27617297231843c0f2ed7b559b",
      "16fa4a4f33a32e28c79da83dca63fd41", "76e2c1d3c323777a3c478e11e1ba6bf2",
      "dccdfd52a71855cc4da18af52bda4c03", "121befbd6c246e85a34225241b8bcaf1",
      "5780757555fd87ca1ff3f1b498a1d6e9", "6b0be2256285694b1edc0201608e1326",
      "b7ef338c58d17f69426b5a99170c7295", "b92b84b5b3d01afac02fb9c092e84b06",
      "e6ef7fea8b183f871c4306c4f49370c5", "c1bf95c05774d8471504e57a3efa66e4",
      "bbacdbdafc625a139361ec22fe2cf003", "5fbbb2d6ca8fc6d07ca8d4105fda4a01",
      "c1cbb295d9f00aa865d91a95e96f99b2", "1490e4f2c874a76ecc2bbf35dce446c3",
      "c3bd73daaeec39895a8b64812773c93c", "6d385068ef3afbd821183d36851f709b",
      "a34c52ef7f2fd04d1cd420238641ef48", "45d10029358c6835cf968a30605659ea",
      "a72c1bb18cf9312c5713ce0de370743d", "df7368db2a7515a1c06a4c9dd9e32ebf",
      "52782632271caccfa9a35ed7533e2052", "6f0ef9b62d2b9956a6464694b7a86b79",
      "814dbc176f7201725a1cfd1cf668b4b9", "065ffbee984f4b9343c8acb0eb04fcbe",
      "0915d76ce458d5164e3c90c1ce150795", "bf2b431d9bfa7a9925ea6f6509267ae9",
      "d3df8c0c940a01b7bf3c3afb80b6dcd4", "15ab86216c9856a8427a51fe599258a3",
      "2cb078484472c88e26b7401c9f11cf51", "7c5f68cc098c8adabc9e26f9cd549151",
      "a8e47da1fcc91c2bc74d030892621576", "71af422ba2d86a401f8278591c0ef540",
      "964c902bb4698ce82f4aa0a1edc80cd6", "78271c37d62af86576dab72ed59746b3",
      "7247c3a7534a41137027e7d3f255f5ef", "8e529ab964f5f9d0f7c3ced98239cfc8",
      "2481ed50bff6b36a3cac6dca2aca5ae5", "78a1ff18bf217d45f5170675dee26948",
      "00fc534119c13aa7af4b818cad9218a2", "67501a83c93f2f9debfa86955bdffde5",
      "2a512ef738e33a4d8476f72654deffb4", "f4eef28078bbc12de9cfb5bc2fef6238",
      "b7ac3a35205a978bed587356155bae0e", "51ea101f09c4de2f754b61ab5aff1526",
      "2bd689d7ec964ee8c8f6f0682f93f5ca", "eecac8dbdaa73b8b3c2234892c444147",
      "cb7086f44ef70ef919086a3d200d8c13", "0abe35e3c796c2de1e550426b2b19441",
      "0eb140561e1ea3843464a5247d8ecb18", "d908f7317f00daacbe3dd43495db64ad",
      "d4d677c4b347de0a13ccab7bc16b8e6e", "26523c2c2df7f31896a3ae5aa24d5ada",
      "0ebb9f816684769816b2ae0b1f94e3a4", "fd938d0577e3687b0a810e199f69f0bb",
      "eb8fb832e72030e2aa214936ae0effe4", "56631887763f7daf6e1e73783e5ff656",
      "590a25cc722c2aa4d885eede5ef09f20", "80944a218ed9b9b0374cde72914449eb",
      "d9cbc2f1e0e56cdd6722310932db1981", "a88eb213b7a6767bbe639cda120a4ab6",
      "9972ecbadfdf3ed0b3fedf435c5a804f", "01fdf7e22405a1b17a8d275b7451094f",
      "6a7824e10406fade0d032e886bbc76b6", "76fefadd793ec3928e915d92782bc7e1",
      "0fbd6b076752c9f5c926ca5c1df892ac", "aac9457239f07ad633fcd45c1465af2a",
      "56823ef9a8e21c9c7441cc9ed870d648", "52f4c7a0b7177175302652cbc482f442",
      "f4a4f4d7c8b93c0486cf3cbaa26fbc19",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigest12bpp(int id) {
  static const char* const kDigest[] = {
      "79a505b3877177197c94f0faeb0c9ec6", "cd22657d242f30c88bb83eae9efbbcce",
      "c4c60a60976d119df3832ff6956e0181", "796bd78bf2346e8dfd61cecbf508ea0e",
      "79e06cc6f880daf6cdb59b9b3a8efe1c", "f0643108e6b57bd566bc0d47b2dc64a1",
      "8272a471e538ca469eaf5c997309589c", "3094741b63a29925da83dc1dc187654a",
      "d0141df80f2335ed6051397cb2a5bc61", "33d9fd317b74f4572afbe004f991ca83",
      "ea2413cd11bf1da93de9285381b471df", "c4f78ae2b994a3a999cb3f5dac2bb498",
      "44804ec226453bc5f688506b56ad2a8a", "9de9c12a5f3bb8d4af13da8807dfe53f",
      "c190dac15c08f2e591b222e1d75b60c2", "c46889b58b44d242e24b91ef531e9176",
      "b6697e1256b60b3426a8980c7c6f9a80", "1e0eb156152fbb74b0cff41bdbdf98b5",
      "98ab6c0abc45fd44565f84e66dc71133", "f2f2126fac1b7c0c7b7ff511c6f3c91e",
      "0cc720e878cfa35f9b72762d08adb1bf", "6efee9ce87e098122dd05525f4c74a2f",
      "187270514a93bd7065d2cfdb02146959", "947be7f2921b5a192d4296b2060a215c",
      "42f02b046eda2a94133032184fdaa26d", "487e94b20867e7021dd1f10d477c3acf",
      "9f9eac4394d8821f5c14857a28c5549b", "75d781b60c1f4aa44ceb6bc65f597a52",
      "779f9ac3c01a86812964ccc38da2711a", "16dc8824efbd7a47808ccdbf8e37df56",
      "e72899a8ddf6cc816e1917c25739a512", "96a4bcaedae79b55399d931fecd64312",
      "5c5e8f4a4f0153315133e4e86a02c3a6", "d1c339b6f6cc0eabdd6674028e1f4260",
      "4ef5868adaf6712d033dce9e51837c0b", "ed90a4ddfc463dddfe71314bc3415b4e",
      "2312299492a47246269d6d37e67c8c0c", "56baf1c4453c5cf5ce3d6857cff4aa8f",
      "d534ce3430377b355c3f59695cfb188b", "f40248f1a6fac4299c9645350138f598",
      "f2e3cbbd066d9d28304667d82312d950", "e8a7784eb367b72b96486bec856b873c",
      "02941ae2cf8272b353268a30cf9c2ee0", "8f6273a5fa62b9a4225ebdbf2ce44e27",
      "85bb0aaba73fe8c89dcee6b5c55d5cfc", "c28c63a4e46ee2a98dd2b58379971c8c",
      "4af35738c29d27ca9930a488bacdffe6", "34a419cc3e6ab21cf099d244169d253e",
      "7c5b8d19ac8a81b37011fabac10143d0", "e582811e05def83270d8f65060fe8966",
      "24662536326615a3c325409e780f65bf", "717a7f7e99d329a74391477ef3c6d738",
      "e0f38a3dba4c6e060b6ca12a18d75fc2", "fbd0cba6a27eb06e74c5ed376187e05c",
      "14dfb487c4a7e989629a195810b814ee", "3cf6d595317ec46e08f6eaa0f0e99b43",
      "b3cb98c418ea854e433b612fc532bac5", "262206cee670c082361497e51cbd0f43",
      "84c11b103a9b0a61f07493dcd269e6fd", "bd9bd9994057371252398bf52c7586f0",
      "72e5537ba5f04fe17b7a371bd12ca0e2", "5986a20b406ceed273f9e41bc0c4c775",
      "d5eb9ea00ce19079b49562ba4a8cb574", "3205e6f3c532a63f8d5d939fa46bc444",
      "cfb21ac467f21954903948d4e6c9a2a1", "bd9fd6aab18bbba8096746f9ed35a640",
      "d42ec4f13f042014c5b4af5f03d19034", "8a7fdee2b57ac641e03365625850f5d6",
      "d18638521275b3aa9dd463d067d6a390", "a7a71c433d85576198b52608c99cab47",
      "96e2a2443bf8cfe32d7590c5011c7523", "6fbe7cd83208937229c11a8e3be5e1e9",
      "ecf66dac310e332a108be639171b5cf3", "327b1656c61d795c30a914f52e3d7629",
      "157d26190bde1a6f34680708bff5d02e", "d927bba0073263a7914a4076a5edfe29",
      "b88930ec68e5e49da8204ef21635cea2", "58e174ed0036b1ac1f5a9bdd44860222",
      "415055dfa80c6fe7c12e4d16cac22168", "9058939bfb5998d6ecd71d87a52be893",
      "847894efa35f1528732ec3584f62f86f", "8aa9b33c0d9695690cb4088c32f31214",
      "11e28ab9a3192a2bc9ffd3fd0a466a13", "f246009c5efafd9310fa8e365d23cab4",
      "2381fcd9ee0ffceba5509879d9f5709d", "1cf1dc7c7c6ecf1f3381455c99e2239e",
      "e74601883b53791045f50bbcbbbcc803", "22926eecefa94f9f39b9bb9dbb183e5b",
      "128c24f5a5342aebb21bdaa87907daf7", "11c39f844a2e51cc4c80ffe1afa58e70",
      "2c0548cff2145031e304d8f97abfd751", "66e1a3daf84029341b999b18bf86e5b3",
      "0f790f210d5366bbad7eb352b4909dd9",
  };
  assert(id >= 0);
  assert(id < sizeof(kDigest) / sizeof(kDigest[0]));
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

struct MaskBlendTestParam {
  MaskBlendTestParam(BlockSize block_size, int subsampling_x, int subsampling_y,
                     bool is_inter_intra, bool is_wedge_inter_intra)
      : block_size(block_size),
        width(kBlockWidthPixels[block_size]),
        height(kBlockHeightPixels[block_size]),
        subsampling_x(subsampling_x),
        subsampling_y(subsampling_y),
        is_inter_intra(is_inter_intra),
        is_wedge_inter_intra(is_wedge_inter_intra) {}
  BlockSize block_size;
  int width;
  int height;
  int subsampling_x;
  int subsampling_y;
  bool is_inter_intra;
  bool is_wedge_inter_intra;
};

std::ostream& operator<<(std::ostream& os, const MaskBlendTestParam& param) {
  return os << ToString(param.block_size)
            << ", subsampling(x/y): " << param.subsampling_x << "/"
            << param.subsampling_y
            << ", is_inter_intra: " << param.is_inter_intra
            << ", is_wedge_inter_intra: " << param.is_wedge_inter_intra;
}

template <int bitdepth, typename Pixel>
class MaskBlendTest : public testing::TestWithParam<MaskBlendTestParam>,
                      public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  MaskBlendTest() = default;
  ~MaskBlendTest() override = default;

  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    MaskBlendInit_C();
    const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp, nullptr);
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    const absl::string_view test_case = test_info->test_suite_name();
    if (absl::StartsWith(test_case, "C/")) {
    } else if (absl::StartsWith(test_case, "NEON/")) {
      MaskBlendInit_NEON();
    } else if (absl::StartsWith(test_case, "SSE41/")) {
      if ((GetCpuInfo() & kSSE4_1) != 0) {
        MaskBlendInit_SSE4_1();
      }
    } else {
      FAIL() << "Unrecognized architecture prefix in test case name: "
             << test_case;
    }
    func_ = (param_.is_inter_intra && !param_.is_wedge_inter_intra)
                ? dsp->mask_blend[0][param_.is_inter_intra]
                : dsp->mask_blend[param_.subsampling_x + param_.subsampling_y]
                                 [param_.is_inter_intra];
    func_8bpp_ = dsp->inter_intra_mask_blend_8bpp[param_.is_wedge_inter_intra
                                                      ? param_.subsampling_x +
                                                            param_.subsampling_y
                                                      : 0];
  }

 protected:
  int GetDigestIdOffset() const {
    // id is for retrieving the corresponding digest from the lookup table given
    // the set of input parameters. id can be figured out by the block size and
    // an offset (id_offset).
    // For example, in kMaskBlendTestParam, this set of parameters
    // (8, 8, 0, 0, false, false) corresponds to the first entry in the
    // digest lookup table, where id == 0.
    // (8, 8, 1, 0, false, false) corresponds to id == 17.
    // (8, 8, 1, 1, false, false) corresponds to id == 34.
    // (8, 8, 0, 0, true, false) corresponds to id == 51.
    // Id_offset denotes offset for different modes (is_inter_intra,
    // is_wedge_inter_intra).
    // ...
    if (!param_.is_inter_intra && !param_.is_wedge_inter_intra) {
      return param_.subsampling_x * 17 + param_.subsampling_y * 17;
    }
    if (param_.is_inter_intra && !param_.is_wedge_inter_intra) {
      return 51 + param_.subsampling_x * 7 + param_.subsampling_y * 7;
    }
    if (param_.is_inter_intra && param_.is_wedge_inter_intra) {
      return 72 + param_.subsampling_x * 7 + param_.subsampling_y * 7;
    }
    return 0;
  }

  int GetDigestId() const {
    // Only 8x8 and larger blocks are tested.
    int block_size_adjustment =
        static_cast<int>(param_.block_size > kBlock16x4);
    if (param_.is_inter_intra || param_.is_wedge_inter_intra) {
      // 4:1/1:4 blocks are invalid for these modes.
      block_size_adjustment += static_cast<int>(param_.block_size > kBlock8x32);
      block_size_adjustment +=
          static_cast<int>(param_.block_size > kBlock16x64);
      block_size_adjustment += static_cast<int>(param_.block_size > kBlock32x8);
      block_size_adjustment +=
          static_cast<int>(param_.block_size > kBlock64x16);
    }
    return GetDigestIdOffset() + param_.block_size - kBlock8x8 -
           block_size_adjustment;
  }

  void Test(const char* digest, int num_runs);

 private:
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  static constexpr int kStride = kMaxSuperBlockSizeInPixels;
  static constexpr int kDestStride = kMaxSuperBlockSizeInPixels * sizeof(Pixel);
  const MaskBlendTestParam param_ = GetParam();
  alignas(kMaxAlignment) PredType
      source1_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels] = {};
  uint8_t source1_8bpp_[kMaxSuperBlockSizeInPixels *
                        kMaxSuperBlockSizeInPixels] = {};
  alignas(kMaxAlignment) PredType
      source2_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels] = {};
  uint8_t source2_8bpp_[kMaxSuperBlockSizeInPixels *
                        kMaxSuperBlockSizeInPixels] = {};
  uint8_t source2_8bpp_cache_[kMaxSuperBlockSizeInPixels *
                              kMaxSuperBlockSizeInPixels] = {};
  uint8_t mask_[kMaxSuperBlockSizeInPixels * kMaxSuperBlockSizeInPixels];
  uint8_t dest_[sizeof(Pixel) * kMaxSuperBlockSizeInPixels *
                kMaxSuperBlockSizeInPixels] = {};
  dsp::MaskBlendFunc func_;
  dsp::InterIntraMaskBlendFunc8bpp func_8bpp_;
};

template <int bitdepth, typename Pixel>
void MaskBlendTest<bitdepth, Pixel>::Test(const char* const digest,
                                          const int num_runs) {
  if (func_ == nullptr && func_8bpp_ == nullptr) return;
  const int width = param_.width >> param_.subsampling_x;
  const int height = param_.height >> param_.subsampling_y;

  // Add id offset to seed just to add more randomness to input blocks.
  // If we use the same seed for different block sizes, the generated input
  // blocks are repeated. For example, if input size is 8x8, the generated
  // block is exactly the upper left half of the generated 16x16 block.
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed() +
                             GetDigestIdOffset());
  PredType* src_1 = source1_;
  uint8_t* src_1_8bpp = source1_8bpp_;
  PredType* src_2 = source2_;
  uint8_t* src_2_8bpp = source2_8bpp_;
  const ptrdiff_t src_2_stride = param_.is_inter_intra ? kStride : width;
  const ptrdiff_t mask_stride = param_.width;
  uint8_t* mask_row = mask_;
  const int range_mask = (1 << (bitdepth)) - 1;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      src_1[x] = static_cast<PredType>(rnd.Rand16() & range_mask);
      src_2[x] = static_cast<PredType>(rnd.Rand16() & range_mask);
      if (param_.is_inter_intra && bitdepth == 8) {
        src_1_8bpp[x] = src_1[x];
        src_2_8bpp[x] = src_2[x];
      }
      if (!param_.is_inter_intra) {
        // Implies isCompound == true.
        constexpr int bitdepth_index = (bitdepth - 8) >> 1;
        const int min_val = kCompoundPredictionRange[bitdepth_index][0];
        const int max_val = kCompoundPredictionRange[bitdepth_index][1];
        src_1[x] = static_cast<PredType>(rnd(max_val - min_val) + min_val);
        src_2[x] = static_cast<PredType>(rnd(max_val - min_val) + min_val);
      }
    }
    src_1 += width;
    src_1_8bpp += width;
    src_2 += src_2_stride;
    src_2_8bpp += src_2_stride;
  }
  // Mask should be setup regardless of subsampling.
  for (int y = 0; y < param_.height; ++y) {
    for (int x = 0; x < param_.width; ++x) {
      mask_row[x] = rnd.Rand8() & 63;
      mask_row[x] += rnd.Rand8() & 1;  // Range of mask is [0, 64].
    }
    mask_row += mask_stride;
  }

  absl::Duration elapsed_time;
  for (int i = 0; i < num_runs; ++i) {
    const absl::Time start = absl::Now();
    if (param_.is_inter_intra && bitdepth == 8) {
      ASSERT_EQ(func_, nullptr);
      static_assert(sizeof(source2_8bpp_cache_) == sizeof(source2_8bpp_), "");
      // source2_8bpp_ is modified in the call.
      memcpy(source2_8bpp_cache_, source2_8bpp_, sizeof(source2_8bpp_));
      func_8bpp_(source1_8bpp_, source2_8bpp_, src_2_stride, mask_, mask_stride,
                 width, height);
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          dest_[y * kDestStride + x] = source2_8bpp_[y * src_2_stride + x];
        }
      }
      memcpy(source2_8bpp_, source2_8bpp_cache_, sizeof(source2_8bpp_));
    } else {
      if (bitdepth != 8) {
        ASSERT_EQ(func_8bpp_, nullptr);
      }
      ASSERT_NE(func_, nullptr);
      func_(source1_, source2_, src_2_stride, mask_, mask_stride, width, height,
            dest_, kDestStride);
    }
    elapsed_time += absl::Now() - start;
  }

  test_utils::CheckMd5Digest("MaskBlend", ToString(param_.block_size), digest,
                             dest_, sizeof(dest_), elapsed_time);
}

const MaskBlendTestParam kMaskBlendTestParam[] = {
    // is_inter_intra = false, is_wedge_inter_intra = false.
    // block size range is from 8x8 to 128x128.
    MaskBlendTestParam(kBlock8x8, 0, 0, false, false),
    MaskBlendTestParam(kBlock8x16, 0, 0, false, false),
    MaskBlendTestParam(kBlock8x32, 0, 0, false, false),
    MaskBlendTestParam(kBlock16x8, 0, 0, false, false),
    MaskBlendTestParam(kBlock16x16, 0, 0, false, false),
    MaskBlendTestParam(kBlock16x32, 0, 0, false, false),
    MaskBlendTestParam(kBlock16x64, 0, 0, false, false),
    MaskBlendTestParam(kBlock32x8, 0, 0, false, false),
    MaskBlendTestParam(kBlock32x16, 0, 0, false, false),
    MaskBlendTestParam(kBlock32x32, 0, 0, false, false),
    MaskBlendTestParam(kBlock32x64, 0, 0, false, false),
    MaskBlendTestParam(kBlock64x16, 0, 0, false, false),
    MaskBlendTestParam(kBlock64x32, 0, 0, false, false),
    MaskBlendTestParam(kBlock64x64, 0, 0, false, false),
    MaskBlendTestParam(kBlock64x128, 0, 0, false, false),
    MaskBlendTestParam(kBlock128x64, 0, 0, false, false),
    MaskBlendTestParam(kBlock128x128, 0, 0, false, false),
    MaskBlendTestParam(kBlock8x8, 1, 0, false, false),
    MaskBlendTestParam(kBlock8x16, 1, 0, false, false),
    MaskBlendTestParam(kBlock8x32, 1, 0, false, false),
    MaskBlendTestParam(kBlock16x8, 1, 0, false, false),
    MaskBlendTestParam(kBlock16x16, 1, 0, false, false),
    MaskBlendTestParam(kBlock16x32, 1, 0, false, false),
    MaskBlendTestParam(kBlock16x64, 1, 0, false, false),
    MaskBlendTestParam(kBlock32x8, 1, 0, false, false),
    MaskBlendTestParam(kBlock32x16, 1, 0, false, false),
    MaskBlendTestParam(kBlock32x32, 1, 0, false, false),
    MaskBlendTestParam(kBlock32x64, 1, 0, false, false),
    MaskBlendTestParam(kBlock64x16, 1, 0, false, false),
    MaskBlendTestParam(kBlock64x32, 1, 0, false, false),
    MaskBlendTestParam(kBlock64x64, 1, 0, false, false),
    MaskBlendTestParam(kBlock64x128, 1, 0, false, false),
    MaskBlendTestParam(kBlock128x64, 1, 0, false, false),
    MaskBlendTestParam(kBlock128x128, 1, 0, false, false),
    MaskBlendTestParam(kBlock8x8, 1, 1, false, false),
    MaskBlendTestParam(kBlock8x16, 1, 1, false, false),
    MaskBlendTestParam(kBlock8x32, 1, 1, false, false),
    MaskBlendTestParam(kBlock16x8, 1, 1, false, false),
    MaskBlendTestParam(kBlock16x16, 1, 1, false, false),
    MaskBlendTestParam(kBlock16x32, 1, 1, false, false),
    MaskBlendTestParam(kBlock16x64, 1, 1, false, false),
    MaskBlendTestParam(kBlock32x8, 1, 1, false, false),
    MaskBlendTestParam(kBlock32x16, 1, 1, false, false),
    MaskBlendTestParam(kBlock32x32, 1, 1, false, false),
    MaskBlendTestParam(kBlock32x64, 1, 1, false, false),
    MaskBlendTestParam(kBlock64x16, 1, 1, false, false),
    MaskBlendTestParam(kBlock64x32, 1, 1, false, false),
    MaskBlendTestParam(kBlock64x64, 1, 1, false, false),
    MaskBlendTestParam(kBlock64x128, 1, 1, false, false),
    MaskBlendTestParam(kBlock128x64, 1, 1, false, false),
    MaskBlendTestParam(kBlock128x128, 1, 1, false, false),
    // is_inter_intra = true, is_wedge_inter_intra = false.
    // block size range is from 8x8 to 32x32 (no 4:1/1:4 blocks, Section 5.11.28
    // Read inter intra syntax).
    MaskBlendTestParam(kBlock8x8, 0, 0, true, false),
    MaskBlendTestParam(kBlock8x16, 0, 0, true, false),
    MaskBlendTestParam(kBlock16x8, 0, 0, true, false),
    MaskBlendTestParam(kBlock16x16, 0, 0, true, false),
    MaskBlendTestParam(kBlock16x32, 0, 0, true, false),
    MaskBlendTestParam(kBlock32x16, 0, 0, true, false),
    MaskBlendTestParam(kBlock32x32, 0, 0, true, false),
    MaskBlendTestParam(kBlock8x8, 1, 0, true, false),
    MaskBlendTestParam(kBlock8x16, 1, 0, true, false),
    MaskBlendTestParam(kBlock16x8, 1, 0, true, false),
    MaskBlendTestParam(kBlock16x16, 1, 0, true, false),
    MaskBlendTestParam(kBlock16x32, 1, 0, true, false),
    MaskBlendTestParam(kBlock32x16, 1, 0, true, false),
    MaskBlendTestParam(kBlock32x32, 1, 0, true, false),
    MaskBlendTestParam(kBlock8x8, 1, 1, true, false),
    MaskBlendTestParam(kBlock8x16, 1, 1, true, false),
    MaskBlendTestParam(kBlock16x8, 1, 1, true, false),
    MaskBlendTestParam(kBlock16x16, 1, 1, true, false),
    MaskBlendTestParam(kBlock16x32, 1, 1, true, false),
    MaskBlendTestParam(kBlock32x16, 1, 1, true, false),
    MaskBlendTestParam(kBlock32x32, 1, 1, true, false),
    // is_inter_intra = true, is_wedge_inter_intra = true.
    // block size range is from 8x8 to 32x32 (no 4:1/1:4 blocks, Section 5.11.28
    // Read inter intra syntax).
    MaskBlendTestParam(kBlock8x8, 0, 0, true, true),
    MaskBlendTestParam(kBlock8x16, 0, 0, true, true),
    MaskBlendTestParam(kBlock16x8, 0, 0, true, true),
    MaskBlendTestParam(kBlock16x16, 0, 0, true, true),
    MaskBlendTestParam(kBlock16x32, 0, 0, true, true),
    MaskBlendTestParam(kBlock32x16, 0, 0, true, true),
    MaskBlendTestParam(kBlock32x32, 0, 0, true, true),
    MaskBlendTestParam(kBlock8x8, 1, 0, true, true),
    MaskBlendTestParam(kBlock8x16, 1, 0, true, true),
    MaskBlendTestParam(kBlock16x8, 1, 0, true, true),
    MaskBlendTestParam(kBlock16x16, 1, 0, true, true),
    MaskBlendTestParam(kBlock16x32, 1, 0, true, true),
    MaskBlendTestParam(kBlock32x16, 1, 0, true, true),
    MaskBlendTestParam(kBlock32x32, 1, 0, true, true),
    MaskBlendTestParam(kBlock8x8, 1, 1, true, true),
    MaskBlendTestParam(kBlock8x16, 1, 1, true, true),
    MaskBlendTestParam(kBlock16x8, 1, 1, true, true),
    MaskBlendTestParam(kBlock16x16, 1, 1, true, true),
    MaskBlendTestParam(kBlock16x32, 1, 1, true, true),
    MaskBlendTestParam(kBlock32x16, 1, 1, true, true),
    MaskBlendTestParam(kBlock32x32, 1, 1, true, true),
};

using MaskBlendTest8bpp = MaskBlendTest<8, uint8_t>;

TEST_P(MaskBlendTest8bpp, Blending) { Test(GetDigest8bpp(GetDigestId()), 1); }

TEST_P(MaskBlendTest8bpp, DISABLED_Speed) {
  Test(GetDigest8bpp(GetDigestId()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, MaskBlendTest8bpp,
                         testing::ValuesIn(kMaskBlendTestParam));

#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, MaskBlendTest8bpp,
                         testing::ValuesIn(kMaskBlendTestParam));
#endif

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, MaskBlendTest8bpp,
                         testing::ValuesIn(kMaskBlendTestParam));
#endif

#if LIBGAV1_MAX_BITDEPTH >= 10
using MaskBlendTest10bpp = MaskBlendTest<10, uint16_t>;

TEST_P(MaskBlendTest10bpp, Blending) { Test(GetDigest10bpp(GetDigestId()), 1); }

TEST_P(MaskBlendTest10bpp, DISABLED_Speed) {
  Test(GetDigest10bpp(GetDigestId()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, MaskBlendTest10bpp,
                         testing::ValuesIn(kMaskBlendTestParam));

#if LIBGAV1_ENABLE_SSE4_1
INSTANTIATE_TEST_SUITE_P(SSE41, MaskBlendTest10bpp,
                         testing::ValuesIn(kMaskBlendTestParam));
#endif
#if LIBGAV1_ENABLE_NEON
INSTANTIATE_TEST_SUITE_P(NEON, MaskBlendTest10bpp,
                         testing::ValuesIn(kMaskBlendTestParam));
#endif
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using MaskBlendTest12bpp = MaskBlendTest<12, uint16_t>;

TEST_P(MaskBlendTest12bpp, Blending) { Test(GetDigest12bpp(GetDigestId()), 1); }

TEST_P(MaskBlendTest12bpp, DISABLED_Speed) {
  Test(GetDigest12bpp(GetDigestId()), kNumSpeedTests);
}

INSTANTIATE_TEST_SUITE_P(C, MaskBlendTest12bpp,
                         testing::ValuesIn(kMaskBlendTestParam));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace
}  // namespace dsp
}  // namespace libgav1
