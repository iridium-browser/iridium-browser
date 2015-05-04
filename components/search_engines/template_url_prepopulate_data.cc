// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/template_url_prepopulate_data.h"

#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "build/build_config.h"
#include "components/country_codes/country_codes.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"

namespace TemplateURLPrepopulateData {

// Helpers --------------------------------------------------------------------

namespace {
// NOTE: You should probably not change the data in this file without changing
// |kCurrentDataVersion| in prepopulated_engines.json. See comments in
// GetDataVersion() below!

// Put the engines within each country in order with most interesting/important
// first.  The default will be the first engine.

// Default (for countries with no better engine set)
const PrepopulatedEngine* const engines_default[] = {
    &duckduckgo, &qwant,
    &google,
    &bing,
    &yahoo,
};

// Note, the below entries are sorted by country code, not the name in comment.
// Engine selection by country ------------------------------------------------
// clang-format off
// United Arab Emirates
const PrepopulatedEngine* const engines_AE[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Albania
const PrepopulatedEngine* const engines_AL[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Argentina
const PrepopulatedEngine* const engines_AR[] = {
    &google,
    &bing,
    &yahoo_ar,
    &duckduckgo, &qwant,
    &ecosia,
};

// Austria
const PrepopulatedEngine* const engines_AT[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo_at,
    &ecosia,
};

// Australia
const PrepopulatedEngine* const engines_AU[] = {
    &google,
    &bing,
    &yahoo_au,
    &duckduckgo, &qwant,
    &ecosia,
};

// Bosnia and Herzegovina
const PrepopulatedEngine* const engines_BA[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Belgium
const PrepopulatedEngine* const engines_BE[] = {
    &google,
    &bing,
    &yahoo,
    &ecosia,
    &duckduckgo, &qwant,
};

// Bulgaria
const PrepopulatedEngine* const engines_BG[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Bahrain
const PrepopulatedEngine* const engines_BH[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Burundi
const PrepopulatedEngine* const engines_BI[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Brunei
const PrepopulatedEngine* const engines_BN[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Bolivia
const PrepopulatedEngine* const engines_BO[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Brazil
const PrepopulatedEngine* const engines_BR[] = {
    &google,
    &bing,
    &yahoo_br,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Belarus
const PrepopulatedEngine* const engines_BY[] = {
    &google,
    &yandex_by,
    &mail_ru,
    &bing,
    &duckduckgo, &qwant,
};

// Belize
const PrepopulatedEngine* const engines_BZ[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &naver,
};

// Canada
const PrepopulatedEngine* const engines_CA[] = {
    &google,
    &bing,
    &yahoo_ca,
    &duckduckgo, &qwant,
    &ecosia,
};

// Switzerland
const PrepopulatedEngine* const engines_CH[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &ecosia,
    &yahoo_ch,
};

// Chile
const PrepopulatedEngine* const engines_CL[] = {
    &google,
    &bing,
    &yahoo_cl,
    &duckduckgo, &qwant,
    &ecosia,
};

// China
const PrepopulatedEngine* const engines_CN[] = {
    &baidu,
    &bing,
    &sogou,
    &so_360,
    &google,
    &duckduckgo, &qwant,
};

// Colombia
const PrepopulatedEngine* const engines_CO[] = {
    &google,
    &bing,
    &yahoo_co,
    &duckduckgo, &qwant,
    &ecosia,
};

// Costa Rica
const PrepopulatedEngine* const engines_CR[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Czech Republic
const PrepopulatedEngine* const engines_CZ[] = {
    &google,
    &seznam_cz,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
};

// Germany
const PrepopulatedEngine* const engines_DE[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo_de,
    &ecosia,
};

// Denmark
const PrepopulatedEngine* const engines_DK[] = {
    &google,
    &bing,
    &yahoo_dk,
    &duckduckgo, &qwant,
    &ecosia,
};

// Dominican Republic
const PrepopulatedEngine* const engines_DO[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Algeria
const PrepopulatedEngine* const engines_DZ[] = {
    &google,
    &bing,
    &yahoo_fr,
    &yandex_com,
    &duckduckgo, &qwant,
};

// Ecuador
const PrepopulatedEngine* const engines_EC[] = {
    &google,
    &bing,
    &yahoo_es,
    &ecosia,
    &duckduckgo, &qwant,
};

// Estonia
const PrepopulatedEngine* const engines_EE[] = {
    &google,
    &bing,
    &yandex_ru,
    &duckduckgo, &qwant,
    &mail_ru,
};

// Egypt
const PrepopulatedEngine* const engines_EG[] = {
    &google,
    &bing,
    &yahoo,
    &yandex_com,
    &duckduckgo, &qwant,
};

// Spain
const PrepopulatedEngine* const engines_ES[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &ecosia,
};

// Finland
const PrepopulatedEngine* const engines_FI[] = {
    &google,
    &bing,
    &yahoo_fi,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Faroe Islands
const PrepopulatedEngine* const engines_FO[] = {
    &google,
    &bing,
    &yahoo_uk,
    &duckduckgo, &qwant,
    &ecosia,
};

// France
const PrepopulatedEngine* const engines_FR[] = {
    &google,
    &bing,
    &yahoo_fr,
    &qwant,
    &ecosia,
    &duckduckgo,
};

// United Kingdom
const PrepopulatedEngine* const engines_GB[] = {
    &google,
    &bing,
    &yahoo_uk,
    &duckduckgo, &qwant,
    &ecosia,
};

// Greece
const PrepopulatedEngine* const engines_GR[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Guatemala
const PrepopulatedEngine* const engines_GT[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Hong Kong
const PrepopulatedEngine* const engines_HK[] = {
    &google,
    &bing,
    &yahoo_hk,
    &baidu,
    &so_360,
    &duckduckgo, &qwant,
};

// Honduras
const PrepopulatedEngine* const engines_HN[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Croatia
const PrepopulatedEngine* const engines_HR[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &ecosia,
};

// Hungary
const PrepopulatedEngine* const engines_HU[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Indonesia
const PrepopulatedEngine* const engines_ID[] = {
    &google,
    &bing,
    &yahoo_id,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Ireland
const PrepopulatedEngine* const engines_IE[] = {
    &google,
    &bing,
    &yahoo_uk,
    &duckduckgo, &qwant,
    &ecosia,
};

// Israel
const PrepopulatedEngine* const engines_IL[] = {
    &google,
    &bing,
    &yandex_ru,
    &yahoo,
    &duckduckgo, &qwant,
};

// India
const PrepopulatedEngine* const engines_IN[] = {
    &google,
    &bing,
    &yahoo_in,
    &duckduckgo, &qwant,
    &info_com,
};

// Iraq
const PrepopulatedEngine* const engines_IQ[] = {
    &google,
    &bing,
    &yahoo,
    &yandex_tr,
    &petal_search,
    &duckduckgo, &qwant,
};

// Iran
const PrepopulatedEngine* const engines_IR[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo,
    &ask,
};

// Iceland
const PrepopulatedEngine* const engines_IS[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &ecosia,
};

// Italy
const PrepopulatedEngine* const engines_IT[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Jamaica
const PrepopulatedEngine* const engines_JM[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Jordan
const PrepopulatedEngine* const engines_JO[] = {
    &google,
    &bing,
    &yahoo,
    &petal_search,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Japan
const PrepopulatedEngine* const engines_JP[] = {
    &google,
    &bing,
    &yahoo_jp,
    &duckduckgo, &qwant,
    &baidu,
};

// Kenya
const PrepopulatedEngine* const engines_KE[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// South Korea
const PrepopulatedEngine* const engines_KR[] = {
    &google,
    &naver,
    &bing,
    &daum,
    &yahoo,
    &duckduckgo, &qwant,
};

// Kuwait
const PrepopulatedEngine* const engines_KW[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_tr,
};

// Kazakhstan
const PrepopulatedEngine* const engines_KZ[] = {
    &google,
    &yandex_kz,
    &mail_ru,
    &bing,
    &duckduckgo, &qwant,
};

// Lebanon
const PrepopulatedEngine* const engines_LB[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Liechtenstein
const PrepopulatedEngine* const engines_LI[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &ecosia,
};

// Lithuania
const PrepopulatedEngine* const engines_LT[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
    &duckduckgo,
    &yahoo,
};

// Luxembourg
const PrepopulatedEngine* const engines_LU[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &ecosia,
};

// Latvia
const PrepopulatedEngine* const engines_LV[] = {
    &google,
    &bing,
    &yandex_ru,
    &yahoo,
    &duckduckgo, &qwant,
};

// Libya
const PrepopulatedEngine* const engines_LY[] = {
    &google,
    &bing,
    &yahoo,
    &yandex_tr,
    &duckduckgo, &qwant,
};

// Morocco
const PrepopulatedEngine* const engines_MA[] = {
    &google,
    &bing,
    &yahoo_fr,
    &yandex_com,
    &duckduckgo, &qwant,
};

// Monaco
const PrepopulatedEngine* const engines_MC[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo_fr,
    &qwant,
};

// Moldova
const PrepopulatedEngine* const engines_MD[] = {
    &google,
    &yandex_ru,
    &bing,
    &mail_ru,
    &duckduckgo, &qwant,
};

// Montenegro
const PrepopulatedEngine* const engines_ME[] = {
    &google,
    &bing,
    &yandex_ru,
    &yahoo,
    &duckduckgo, &qwant,
};

// Macedonia
const PrepopulatedEngine* const engines_MK[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Mexico
const PrepopulatedEngine* const engines_MX[] = {
    &google,
    &bing,
    &yahoo_mx,
    &duckduckgo, &qwant,
    &ecosia,
};

// Malaysia
const PrepopulatedEngine* const engines_MY[] = {
    &google,
    &bing,
    &yahoo_my,
    &duckduckgo, &qwant,
    &ecosia,
};

// Nicaragua
const PrepopulatedEngine* const engines_NI[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Netherlands
const PrepopulatedEngine* const engines_NL[] = {
    &google,
    &bing,
    &yahoo_nl,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Norway
const PrepopulatedEngine* const engines_NO[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// New Zealand
const PrepopulatedEngine* const engines_NZ[] = {
    &google,
    &bing,
    &yahoo_nz,
    &duckduckgo, &qwant,
    &ecosia,
};

// Oman
const PrepopulatedEngine* const engines_OM[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &petal_search,
};

// Panama
const PrepopulatedEngine* const engines_PA[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &ecosia,
};

// Peru
const PrepopulatedEngine* const engines_PE[] = {
    &google,
    &bing,
    &yahoo_pe,
    &ecosia,
    &duckduckgo, &qwant,
};

// Philippines
const PrepopulatedEngine* const engines_PH[] = {
    &google,
    &bing,
    &yahoo_ph,
    &ecosia,
    &duckduckgo, &qwant,
};

// Pakistan
const PrepopulatedEngine* const engines_PK[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Poland
const PrepopulatedEngine* const engines_PL[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Puerto Rico
const PrepopulatedEngine* const engines_PR[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Portugal
const PrepopulatedEngine* const engines_PT[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Paraguay
const PrepopulatedEngine* const engines_PY[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &ecosia,
};

// Qatar
const PrepopulatedEngine* const engines_QA[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_tr,
};

// Romania
const PrepopulatedEngine* const engines_RO[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_tr,
};

// Serbia
const PrepopulatedEngine* const engines_RS[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Russia
const PrepopulatedEngine* const engines_RU[] = {
    &yandex_ru,
    &google,
    &duckduckgo, &qwant,
    &bing,
    &mail_ru,
};

// Rwanda
const PrepopulatedEngine* const engines_RW[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Saudi Arabia
const PrepopulatedEngine* const engines_SA[] = {
    &google,
    &bing,
    &yahoo,
    &yandex_com,
    &duckduckgo, &qwant,
};

// Sweden
const PrepopulatedEngine* const engines_SE[] = {
    &google,
    &bing,
    &yahoo_se,
    &duckduckgo, &qwant,
    &yandex_ru,
};

// Singapore
const PrepopulatedEngine* const engines_SG[] = {
    &google,
    &bing,
    &yahoo_sg,
    &baidu,
    &duckduckgo, &qwant,
};

// Slovenia
const PrepopulatedEngine* const engines_SI[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &yandex_com,
};

// Slovakia
const PrepopulatedEngine* const engines_SK[] = {
    &google,
    &bing,
    &duckduckgo, &qwant,
    &yahoo,
    &yandex_ru,
};

// El Salvador
const PrepopulatedEngine* const engines_SV[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &ecosia,
};

// Syria
const PrepopulatedEngine* const engines_SY[] = {
    &google,
    &bing,
    &yandex_ru,
    &yahoo,
    &duckduckgo, &qwant,
};

// Thailand
const PrepopulatedEngine* const engines_TH[] = {
    &google,
    &bing,
    &yahoo_th,
    &naver,
    &duckduckgo, &qwant,
};

// Tunisia
const PrepopulatedEngine* const engines_TN[] = {
    &google,
    &bing,
    &yahoo_fr,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Turkey
const PrepopulatedEngine* const engines_TR[] = {
    &google,
    &yandex_tr,
    &yahoo_tr,
    &bing,
    &duckduckgo, &qwant,
};

// Trinidad and Tobago
const PrepopulatedEngine* const engines_TT[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Taiwan
const PrepopulatedEngine* const engines_TW[] = {
    &google,
    &yahoo_tw,
    &bing,
    &baidu,
    &duckduckgo, &qwant,
};

// Tanzania
const PrepopulatedEngine* const engines_TZ[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Ukraine
const PrepopulatedEngine* const engines_UA[] = {
    &google,
    &yandex_ru,
    &bing,
    &duckduckgo, &qwant,
    &mail_ru,
};

// United States
const PrepopulatedEngine* const engines_US[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Uruguay
const PrepopulatedEngine* const engines_UY[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &ecosia,
};

// Venezuela
const PrepopulatedEngine* const engines_VE[] = {
    &google,
    &bing,
    &yahoo_es,
    &duckduckgo, &qwant,
    &yandex_com,
};

// Vietnam
const PrepopulatedEngine* const engines_VN[] = {
    &google,
    &coccoc,
    &bing,
    &yahoo,
    &baidu,
    &duckduckgo, &qwant,
};

// Yemen
const PrepopulatedEngine* const engines_YE[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &yandex_com,
};

// South Africa
const PrepopulatedEngine* const engines_ZA[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// Zimbabwe
const PrepopulatedEngine* const engines_ZW[] = {
    &google,
    &bing,
    &yahoo,
    &duckduckgo, &qwant,
    &ecosia,
};

// clang-format on
// ----------------------------------------------------------------------------

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulationSetFromCountryID(
    int country_id) {
  const PrepopulatedEngine* const* engines;
  size_t num_engines;
  // If you add a new country make sure to update the unit test for coverage.
  switch (country_id) {
#define UNHANDLED_COUNTRY(code1, code2) \
  case country_codes::CountryCharsToCountryID((#code1)[0], (#code2)[0]):
#define END_UNHANDLED_COUNTRIES(code1, code2)      \
  engines = engines_##code1##code2;                \
  num_engines = std::size(engines_##code1##code2); \
  break;
#define DECLARE_COUNTRY(code1, code2)\
    UNHANDLED_COUNTRY(code1, code2)\
    END_UNHANDLED_COUNTRIES(code1, code2)

    // Countries with their own, dedicated engine set.
    DECLARE_COUNTRY(A, E)  // United Arab Emirates
    DECLARE_COUNTRY(A, L)  // Albania
    DECLARE_COUNTRY(A, R)  // Argentina
    DECLARE_COUNTRY(A, T)  // Austria
    DECLARE_COUNTRY(A, U)  // Australia
    DECLARE_COUNTRY(B, A)  // Bosnia and Herzegovina
    DECLARE_COUNTRY(B, E)  // Belgium
    DECLARE_COUNTRY(B, G)  // Bulgaria
    DECLARE_COUNTRY(B, H)  // Bahrain
    DECLARE_COUNTRY(B, I)  // Burundi
    DECLARE_COUNTRY(B, N)  // Brunei
    DECLARE_COUNTRY(B, O)  // Bolivia
    DECLARE_COUNTRY(B, R)  // Brazil
    DECLARE_COUNTRY(B, Y)  // Belarus
    DECLARE_COUNTRY(B, Z)  // Belize
    DECLARE_COUNTRY(C, A)  // Canada
    DECLARE_COUNTRY(C, H)  // Switzerland
    DECLARE_COUNTRY(C, L)  // Chile
    DECLARE_COUNTRY(C, N)  // China
    DECLARE_COUNTRY(C, O)  // Colombia
    DECLARE_COUNTRY(C, R)  // Costa Rica
    DECLARE_COUNTRY(C, Z)  // Czech Republic
    DECLARE_COUNTRY(D, E)  // Germany
    DECLARE_COUNTRY(D, K)  // Denmark
    DECLARE_COUNTRY(D, O)  // Dominican Republic
    DECLARE_COUNTRY(D, Z)  // Algeria
    DECLARE_COUNTRY(E, C)  // Ecuador
    DECLARE_COUNTRY(E, E)  // Estonia
    DECLARE_COUNTRY(E, G)  // Egypt
    DECLARE_COUNTRY(E, S)  // Spain
    DECLARE_COUNTRY(F, I)  // Finland
    DECLARE_COUNTRY(F, O)  // Faroe Islands
    DECLARE_COUNTRY(F, R)  // France
    DECLARE_COUNTRY(G, B)  // United Kingdom
    DECLARE_COUNTRY(G, R)  // Greece
    DECLARE_COUNTRY(G, T)  // Guatemala
    DECLARE_COUNTRY(H, K)  // Hong Kong
    DECLARE_COUNTRY(H, N)  // Honduras
    DECLARE_COUNTRY(H, R)  // Croatia
    DECLARE_COUNTRY(H, U)  // Hungary
    DECLARE_COUNTRY(I, D)  // Indonesia
    DECLARE_COUNTRY(I, E)  // Ireland
    DECLARE_COUNTRY(I, L)  // Israel
    DECLARE_COUNTRY(I, N)  // India
    DECLARE_COUNTRY(I, Q)  // Iraq
    DECLARE_COUNTRY(I, R)  // Iran
    DECLARE_COUNTRY(I, S)  // Iceland
    DECLARE_COUNTRY(I, T)  // Italy
    DECLARE_COUNTRY(J, M)  // Jamaica
    DECLARE_COUNTRY(J, O)  // Jordan
    DECLARE_COUNTRY(J, P)  // Japan
    DECLARE_COUNTRY(K, E)  // Kenya
    DECLARE_COUNTRY(K, R)  // South Korea
    DECLARE_COUNTRY(K, W)  // Kuwait
    DECLARE_COUNTRY(K, Z)  // Kazakhstan
    DECLARE_COUNTRY(L, B)  // Lebanon
    DECLARE_COUNTRY(L, I)  // Liechtenstein
    DECLARE_COUNTRY(L, T)  // Lithuania
    DECLARE_COUNTRY(L, U)  // Luxembourg
    DECLARE_COUNTRY(L, V)  // Latvia
    DECLARE_COUNTRY(L, Y)  // Libya
    DECLARE_COUNTRY(M, A)  // Morocco
    DECLARE_COUNTRY(M, C)  // Monaco
    DECLARE_COUNTRY(M, D)  // Moldova
    DECLARE_COUNTRY(M, E)  // Montenegro
    DECLARE_COUNTRY(M, K)  // Macedonia
    DECLARE_COUNTRY(M, X)  // Mexico
    DECLARE_COUNTRY(M, Y)  // Malaysia
    DECLARE_COUNTRY(N, I)  // Nicaragua
    DECLARE_COUNTRY(N, L)  // Netherlands
    DECLARE_COUNTRY(N, O)  // Norway
    DECLARE_COUNTRY(N, Z)  // New Zealand
    DECLARE_COUNTRY(O, M)  // Oman
    DECLARE_COUNTRY(P, A)  // Panama
    DECLARE_COUNTRY(P, E)  // Peru
    DECLARE_COUNTRY(P, H)  // Philippines
    DECLARE_COUNTRY(P, K)  // Pakistan
    DECLARE_COUNTRY(P, L)  // Poland
    DECLARE_COUNTRY(P, R)  // Puerto Rico
    DECLARE_COUNTRY(P, T)  // Portugal
    DECLARE_COUNTRY(P, Y)  // Paraguay
    DECLARE_COUNTRY(Q, A)  // Qatar
    DECLARE_COUNTRY(R, O)  // Romania
    DECLARE_COUNTRY(R, S)  // Serbia
    DECLARE_COUNTRY(R, U)  // Russia
    DECLARE_COUNTRY(R, W)  // Rwanda
    DECLARE_COUNTRY(S, A)  // Saudi Arabia
    DECLARE_COUNTRY(S, E)  // Sweden
    DECLARE_COUNTRY(S, G)  // Singapore
    DECLARE_COUNTRY(S, I)  // Slovenia
    DECLARE_COUNTRY(S, K)  // Slovakia
    DECLARE_COUNTRY(S, V)  // El Salvador
    DECLARE_COUNTRY(S, Y)  // Syria
    DECLARE_COUNTRY(T, H)  // Thailand
    DECLARE_COUNTRY(T, N)  // Tunisia
    DECLARE_COUNTRY(T, R)  // Turkey
    DECLARE_COUNTRY(T, T)  // Trinidad and Tobago
    DECLARE_COUNTRY(T, W)  // Taiwan
    DECLARE_COUNTRY(T, Z)  // Tanzania
    DECLARE_COUNTRY(U, A)  // Ukraine
    DECLARE_COUNTRY(U, S)  // United States
    DECLARE_COUNTRY(U, Y)  // Uruguay
    DECLARE_COUNTRY(V, E)  // Venezuela
    DECLARE_COUNTRY(V, N)  // Vietnam
    DECLARE_COUNTRY(Y, E)  // Yemen
    DECLARE_COUNTRY(Z, A)  // South Africa
    DECLARE_COUNTRY(Z, W)  // Zimbabwe

    // Countries using the "Australia" engine set.
    UNHANDLED_COUNTRY(C, C)  // Cocos Islands
    UNHANDLED_COUNTRY(C, X)  // Christmas Island
    UNHANDLED_COUNTRY(H, M)  // Heard Island and McDonald Islands
    UNHANDLED_COUNTRY(N, F)  // Norfolk Island
    END_UNHANDLED_COUNTRIES(A, U)

    // Countries using the "China" engine set.
    UNHANDLED_COUNTRY(M, O)  // Macao
    END_UNHANDLED_COUNTRIES(C, N)

    // Countries using the "Denmark" engine set.
    UNHANDLED_COUNTRY(G, L)  // Greenland
    END_UNHANDLED_COUNTRIES(D, K)

    // Countries using the "Spain" engine set.
    UNHANDLED_COUNTRY(A, D)  // Andorra
    END_UNHANDLED_COUNTRIES(E, S)

    // Countries using the "Finland" engine set.
    UNHANDLED_COUNTRY(A, X)  // Aland Islands
    END_UNHANDLED_COUNTRIES(F, I)

    // Countries using the "France" engine set.
    UNHANDLED_COUNTRY(B, F)  // Burkina Faso
    UNHANDLED_COUNTRY(B, J)  // Benin
    UNHANDLED_COUNTRY(C, D)  // Congo - Kinshasa
    UNHANDLED_COUNTRY(C, F)  // Central African Republic
    UNHANDLED_COUNTRY(C, G)  // Congo - Brazzaville
    UNHANDLED_COUNTRY(C, I)  // Ivory Coast
    UNHANDLED_COUNTRY(C, M)  // Cameroon
    UNHANDLED_COUNTRY(D, J)  // Djibouti
    UNHANDLED_COUNTRY(G, A)  // Gabon
    UNHANDLED_COUNTRY(G, F)  // French Guiana
    UNHANDLED_COUNTRY(G, N)  // Guinea
    UNHANDLED_COUNTRY(G, P)  // Guadeloupe
    UNHANDLED_COUNTRY(H, T)  // Haiti
#if BUILDFLAG(IS_WIN)
    UNHANDLED_COUNTRY(I, P)  // Clipperton Island ('IP' is an WinXP-ism; ISO
                             //                    includes it with France)
#endif
    UNHANDLED_COUNTRY(M, L)  // Mali
    UNHANDLED_COUNTRY(M, Q)  // Martinique
    UNHANDLED_COUNTRY(N, C)  // New Caledonia
    UNHANDLED_COUNTRY(N, E)  // Niger
    UNHANDLED_COUNTRY(P, F)  // French Polynesia
    UNHANDLED_COUNTRY(P, M)  // Saint Pierre and Miquelon
    UNHANDLED_COUNTRY(R, E)  // Reunion
    UNHANDLED_COUNTRY(S, N)  // Senegal
    UNHANDLED_COUNTRY(T, D)  // Chad
    UNHANDLED_COUNTRY(T, F)  // French Southern Territories
    UNHANDLED_COUNTRY(T, G)  // Togo
    UNHANDLED_COUNTRY(W, F)  // Wallis and Futuna
    UNHANDLED_COUNTRY(Y, T)  // Mayotte
    END_UNHANDLED_COUNTRIES(F, R)

    // Countries using the "Greece" engine set.
    UNHANDLED_COUNTRY(C, Y)  // Cyprus
    END_UNHANDLED_COUNTRIES(G, R)

    // Countries using the "Italy" engine set.
    UNHANDLED_COUNTRY(S, M)  // San Marino
    UNHANDLED_COUNTRY(V, A)  // Vatican
    END_UNHANDLED_COUNTRIES(I, T)

    // Countries using the "Morocco" engine set.
    UNHANDLED_COUNTRY(E, H)  // Western Sahara
    END_UNHANDLED_COUNTRIES(M, A)

    // Countries using the "Netherlands" engine set.
    UNHANDLED_COUNTRY(A, N)  // Netherlands Antilles
    UNHANDLED_COUNTRY(A, W)  // Aruba
    END_UNHANDLED_COUNTRIES(N, L)

    // Countries using the "Norway" engine set.
    UNHANDLED_COUNTRY(B, V)  // Bouvet Island
    UNHANDLED_COUNTRY(S, J)  // Svalbard and Jan Mayen
    END_UNHANDLED_COUNTRIES(N, O)

    // Countries using the "New Zealand" engine set.
    UNHANDLED_COUNTRY(C, K)  // Cook Islands
    UNHANDLED_COUNTRY(N, U)  // Niue
    UNHANDLED_COUNTRY(T, K)  // Tokelau
    END_UNHANDLED_COUNTRIES(N, Z)

    // Countries using the "Portugal" engine set.
    UNHANDLED_COUNTRY(C, V)  // Cape Verde
    UNHANDLED_COUNTRY(G, W)  // Guinea-Bissau
    UNHANDLED_COUNTRY(M, Z)  // Mozambique
    UNHANDLED_COUNTRY(S, T)  // Sao Tome and Principe
    UNHANDLED_COUNTRY(T, L)  // Timor-Leste
    END_UNHANDLED_COUNTRIES(P, T)

    // Countries using the "Russia" engine set.
    UNHANDLED_COUNTRY(A, M)  // Armenia
    UNHANDLED_COUNTRY(A, Z)  // Azerbaijan
    UNHANDLED_COUNTRY(K, G)  // Kyrgyzstan
    UNHANDLED_COUNTRY(T, J)  // Tajikistan
    UNHANDLED_COUNTRY(T, M)  // Turkmenistan
    UNHANDLED_COUNTRY(U, Z)  // Uzbekistan
    END_UNHANDLED_COUNTRIES(R, U)

    // Countries using the "Saudi Arabia" engine set.
    UNHANDLED_COUNTRY(M, R)  // Mauritania
    UNHANDLED_COUNTRY(P, S)  // Palestinian Territory
    UNHANDLED_COUNTRY(S, D)  // Sudan
    END_UNHANDLED_COUNTRIES(S, A)

    // Countries using the "United Kingdom" engine set.
    UNHANDLED_COUNTRY(B, M)  // Bermuda
    UNHANDLED_COUNTRY(F, K)  // Falkland Islands
    UNHANDLED_COUNTRY(G, G)  // Guernsey
    UNHANDLED_COUNTRY(G, I)  // Gibraltar
    UNHANDLED_COUNTRY(G, S)  // South Georgia and the South Sandwich
                             //   Islands
    UNHANDLED_COUNTRY(I, M)  // Isle of Man
    UNHANDLED_COUNTRY(I, O)  // British Indian Ocean Territory
    UNHANDLED_COUNTRY(J, E)  // Jersey
    UNHANDLED_COUNTRY(K, Y)  // Cayman Islands
    UNHANDLED_COUNTRY(M, S)  // Montserrat
    UNHANDLED_COUNTRY(M, T)  // Malta
    UNHANDLED_COUNTRY(P, N)  // Pitcairn Islands
    UNHANDLED_COUNTRY(S, H)  // Saint Helena, Ascension Island, and Tristan da
                             //   Cunha
    UNHANDLED_COUNTRY(T, C)  // Turks and Caicos Islands
    UNHANDLED_COUNTRY(V, G)  // British Virgin Islands
    END_UNHANDLED_COUNTRIES(G, B)

    // Countries using the "United States" engine set.
    UNHANDLED_COUNTRY(A, S)  // American Samoa
    UNHANDLED_COUNTRY(G, U)  // Guam
    UNHANDLED_COUNTRY(M, P)  // Northern Mariana Islands
    UNHANDLED_COUNTRY(U, M)  // U.S. Minor Outlying Islands
    UNHANDLED_COUNTRY(V, I)  // U.S. Virgin Islands
    END_UNHANDLED_COUNTRIES(U, S)

    // Countries using the "default" engine set.
    UNHANDLED_COUNTRY(A, F)  // Afghanistan
    UNHANDLED_COUNTRY(A, G)  // Antigua and Barbuda
    UNHANDLED_COUNTRY(A, I)  // Anguilla
    UNHANDLED_COUNTRY(A, O)  // Angola
    UNHANDLED_COUNTRY(A, Q)  // Antarctica
    UNHANDLED_COUNTRY(B, B)  // Barbados
    UNHANDLED_COUNTRY(B, D)  // Bangladesh
    UNHANDLED_COUNTRY(B, S)  // Bahamas
    UNHANDLED_COUNTRY(B, T)  // Bhutan
    UNHANDLED_COUNTRY(B, W)  // Botswana
    UNHANDLED_COUNTRY(C, U)  // Cuba
    UNHANDLED_COUNTRY(D, M)  // Dominica
    UNHANDLED_COUNTRY(E, R)  // Eritrea
    UNHANDLED_COUNTRY(E, T)  // Ethiopia
    UNHANDLED_COUNTRY(F, J)  // Fiji
    UNHANDLED_COUNTRY(F, M)  // Micronesia
    UNHANDLED_COUNTRY(G, D)  // Grenada
    UNHANDLED_COUNTRY(G, E)  // Georgia
    UNHANDLED_COUNTRY(G, H)  // Ghana
    UNHANDLED_COUNTRY(G, M)  // Gambia
    UNHANDLED_COUNTRY(G, Q)  // Equatorial Guinea
    UNHANDLED_COUNTRY(G, Y)  // Guyana
    UNHANDLED_COUNTRY(K, H)  // Cambodia
    UNHANDLED_COUNTRY(K, I)  // Kiribati
    UNHANDLED_COUNTRY(K, M)  // Comoros
    UNHANDLED_COUNTRY(K, N)  // Saint Kitts and Nevis
    UNHANDLED_COUNTRY(K, P)  // North Korea
    UNHANDLED_COUNTRY(L, A)  // Laos
    UNHANDLED_COUNTRY(L, C)  // Saint Lucia
    UNHANDLED_COUNTRY(L, K)  // Sri Lanka
    UNHANDLED_COUNTRY(L, R)  // Liberia
    UNHANDLED_COUNTRY(L, S)  // Lesotho
    UNHANDLED_COUNTRY(M, G)  // Madagascar
    UNHANDLED_COUNTRY(M, H)  // Marshall Islands
    UNHANDLED_COUNTRY(M, M)  // Myanmar
    UNHANDLED_COUNTRY(M, N)  // Mongolia
    UNHANDLED_COUNTRY(M, U)  // Mauritius
    UNHANDLED_COUNTRY(M, V)  // Maldives
    UNHANDLED_COUNTRY(M, W)  // Malawi
    UNHANDLED_COUNTRY(N, A)  // Namibia
    UNHANDLED_COUNTRY(N, G)  // Nigeria
    UNHANDLED_COUNTRY(N, P)  // Nepal
    UNHANDLED_COUNTRY(N, R)  // Nauru
    UNHANDLED_COUNTRY(P, G)  // Papua New Guinea
    UNHANDLED_COUNTRY(P, W)  // Palau
    UNHANDLED_COUNTRY(S, B)  // Solomon Islands
    UNHANDLED_COUNTRY(S, C)  // Seychelles
    UNHANDLED_COUNTRY(S, L)  // Sierra Leone
    UNHANDLED_COUNTRY(S, O)  // Somalia
    UNHANDLED_COUNTRY(S, R)  // Suriname
    UNHANDLED_COUNTRY(S, Z)  // Swaziland
    UNHANDLED_COUNTRY(T, O)  // Tonga
    UNHANDLED_COUNTRY(T, V)  // Tuvalu
    UNHANDLED_COUNTRY(U, G)  // Uganda
    UNHANDLED_COUNTRY(V, C)  // Saint Vincent and the Grenadines
    UNHANDLED_COUNTRY(V, U)  // Vanuatu
    UNHANDLED_COUNTRY(W, S)  // Samoa
    UNHANDLED_COUNTRY(Z, M)  // Zambia
    case country_codes::kCountryIDUnknown:
    default:                // Unhandled location
    END_UNHANDLED_COUNTRIES(def, ault)
  }

  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  for (size_t i = 0; i < num_engines; ++i)
    t_urls.push_back(TemplateURLDataFromPrepopulatedEngine(*engines[i]));
  return t_urls;
}

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedTemplateURLData(
    PrefService* prefs) {
  std::vector<std::unique_ptr<TemplateURLData>> t_urls;
  if (!prefs)
    return t_urls;

  const base::Value::List& list =
      prefs->GetList(prefs::kSearchProviderOverrides);

  for (const base::Value& engine : list) {
    if (engine.is_dict()) {
    auto t_url = TemplateURLDataFromOverrideDictionary(engine.GetDict());
    if (t_url) {
      t_urls.push_back(std::move(t_url));
    }
    }
  }
  return t_urls;
}

}  // namespace

// Global functions -----------------------------------------------------------

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  country_codes::RegisterProfilePrefs(registry);
  registry->RegisterListPref(prefs::kSearchProviderOverrides);
  registry->RegisterIntegerPref(prefs::kSearchProviderOverridesVersion, -1);
}

int GetDataVersion(PrefService* prefs) {
  // Allow tests to override the local version.
  return (prefs && prefs->HasPrefPath(prefs::kSearchProviderOverridesVersion)) ?
      prefs->GetInteger(prefs::kSearchProviderOverridesVersion) :
      kCurrentDataVersion;
}

std::vector<std::unique_ptr<TemplateURLData>> GetPrepopulatedEngines(
    PrefService* prefs,
    size_t* default_search_provider_index) {
  // If there is a set of search engines in the preferences file, it overrides
  // the built-in set.
  std::vector<std::unique_ptr<TemplateURLData>> t_urls =
      GetPrepopulatedTemplateURLData(prefs);
  if (t_urls.empty()) {
    t_urls = GetPrepopulationSetFromCountryID(
        country_codes::GetCountryIDFromPrefs(prefs));
  }
  if (default_search_provider_index) {
    const auto itr =
        base::ranges::find(t_urls, qwant.id, &TemplateURLData::prepopulate_id);
    *default_search_provider_index =
        itr == t_urls.end() ? 0 : std::distance(t_urls.begin(), itr);
  }
  return t_urls;
}

std::unique_ptr<TemplateURLData> GetPrepopulatedEngine(PrefService* prefs,
                                                       int prepopulated_id) {
  auto engines =
      TemplateURLPrepopulateData::GetPrepopulatedEngines(prefs, nullptr);
  for (auto& engine : engines) {
    if (engine->prepopulate_id == prepopulated_id)
      return std::move(engine);
  }
  return nullptr;
}

#if BUILDFLAG(IS_ANDROID)

std::vector<std::unique_ptr<TemplateURLData>> GetLocalPrepopulatedEngines(
    const std::string& locale) {
  int country_id = country_codes::CountryStringToCountryID(locale);
  if (country_id == country_codes::kCountryIDUnknown) {
    LOG(ERROR) << "Unknown country code specified: " << locale;
    return std::vector<std::unique_ptr<TemplateURLData>>();
  }

  return GetPrepopulationSetFromCountryID(country_id);
}

#endif

std::vector<const PrepopulatedEngine*> GetAllPrepopulatedEngines() {
  return std::vector<const PrepopulatedEngine*>(
      &kAllEngines[0], &kAllEngines[0] + kAllEnginesLength);
}

void ClearPrepopulatedEnginesInPrefs(PrefService* prefs) {
  if (!prefs)
    return;

  prefs->ClearPref(prefs::kSearchProviderOverrides);
  prefs->ClearPref(prefs::kSearchProviderOverridesVersion);
}

std::unique_ptr<TemplateURLData> GetPrepopulatedDefaultSearch(
    PrefService* prefs) {
  size_t default_search_index;
  // This could be more efficient.  We load all URLs but keep only the default.
  std::vector<std::unique_ptr<TemplateURLData>> loaded_urls =
      GetPrepopulatedEngines(prefs, &default_search_index);

  return (default_search_index < loaded_urls.size())
             ? std::move(loaded_urls[default_search_index])
             : nullptr;
}

}  // namespace TemplateURLPrepopulateData
