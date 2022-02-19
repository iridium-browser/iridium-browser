---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
page_name: libchromeos
title: libchromeos
---

## Background

## libchromeos is the utility library used by most daemons in Chromium OS to share common functionality specific to Chromium OS. For more generic utility functions, see [libchrome](/chromium-os/packages/libchrome), utility library from the Chromium browser, also used by most daemons in Chromium OS.

platform2 projects use C++11 and libchromeos uses some of the new features to
simplify and optimize code. For this reason, some generic modules more suitable
for libchrome are included in libcrhomeos due to the lack of C++11 support in
the former.

## Content overview

The following is an overview of the current modules of libchromeos. Refer to the
latest version of the header files for up to date documentation.

**any.h**

Implementation of chromeos::Any, a native C++ variant type. Any could contain
any copy-constructible type.

Example:

chromeos::Any any1 = 5;

chromeos::Any any2 = true;

int v1 = any1.Get&lt;int&gt;();

bool v2 = any2.Get&lt;bool&gt;();

**bind_lambda.h**

Bind adapter for C++ functor objects, including lambdas:

base::Callback&lt;int(int, int)&gt; callback_add = base::Bind(\[\](int a, int b)
{ return a + b;});

LOG(INFO) &lt;&lt; "2+2=" &lt;&lt; callback_add.Run(2, 2);

**data_encoding.h**

Utility functions to encode/decode URLs and web-form-like parameters.

std::string encoded = WebParamsEncode({{ '{{' }}"q", "test"}, {"path", "/usr/bin"},
{"#", "%"{{ '}}' }});

EXPECT_EQ("q=test&path=%2Fusr%2Fbin&%23=%25", encoded);

**dbus/\***

A whole lot of useful utility functions and classes to help build native D-Bus
clients and servers. Helper functions to dispatch D-Bus method calls to remote
object as if they were native C++ functions:

auto response = chromeos::dbus_utils::CallMethodAndBlock(object_proxy,
"org.chromium.MyService.MyInterface", "MyMethod", 2, 8.7);

chromeos::ErrorPtr error;

std::string return_value;

if (ExtractMethodCallResults(response, &error, &return_value)) {

// Use the |return_value|.

} else {

// An error occurred. Use |error| to get details.

}

**errors/error.h**

Error class that provides rich error reporting facilities. Each error object
contains an error domain, error code and error message. Errors can be nested to
provide additional error context.

void foo(ErrorPtr\* error) {

Error::AddTo(error, "my_domain", "not_supported", "function not supported");

}

**flag_helper.h**

Command line flag handling class that exposes an API similar to that of gflags,
using base/command_line.h as the actual parser. Flags are defined from within
main(..) using macros, then once initialized can be referenced by FLAGS_name
variables. Unlike with gflags, however, these variables are scoped to within
main(...). All the DEFINE_type macros take three arguments: the flag name, the
default value, and the help text. The types of flags supported are: bool, int32,
int64, uint64, double, and string. The FlagHelper class automatically handles
generation of '--help' output, as well as exiting the program in the case of
incorrect flag parsing, such as passing a string to an int flag.

#include &lt;chromeos/flag_helper.h&gt;

#include &lt;stdio.h&gt;

int main(int argc, char\*\* argv) {

DEFINE_int32(example, 0, "Example int flag");

chromeos::FlagHelper::Init(argc, argv, "Test application.");

printf("You passed in %d to --example command line flag\\n",

FLAGS_example);

return 0;

}

**http/http_utils.h** (and **http/\***)

A bunch of functionality to make http requests (currently built on top of
libcurl).

chromeos::http::HeaderList headers = {{ '{{' }}"X-Chrome-UMA-Log-SHA1", hash{{ '}}' }};

chromeos::ErrorPtr error;

auto response = chromeos::http::PostText(

"<http://server.com/url>",

"text data to post",

chromeos::mime::text::kPlain, headers,
chromeos::http::Transport::CreateDefault(), &error);

if (!response || response-&gt;GetDataAsString() != "OK") {

LOG(ERROR) &lt;&lt; "Failed to send data: " &lt;&lt; error-&gt;GetMessage();

}

**map_utils.h**

Template helper functions for std::map. You can get a list of keys or values out
of a map:

std::map&lt;string, string&gt; map = ....

std::vector&lt;string&gt; keys = chromeos::GetMapKeys(map);

std::vector&lt;string&gt; values = chromeos::GetMapValues(map);

**mime_utils.h**

Utility functions to compose, parse, and manipulate MIME strings. Also contains
lots of predefined constants for common mime types. So you should never have to
hardcode "text/html" in your code.

EXPECT_EQ(mime::text::kPlain,
mime::RemoveParameters("text/plain;charset=utf8"));

**strings/string_utils.h**

Helper string functions in addition to those provided by base/strings. Mostly
for splitting/joining.

std::string input = "abc;def;ghi";

for (const std::string& token : chromeos::string_utils::Split(input, ';')) {

// process each token

}

**url_utils.h**

Functions to split, combine and manipulate URLs

EXPECT_EQ("<http://sample.org/obj/part1/part2>",
url::CombineMultiple("<http://sample.org>", {"obj", "part1", "part2"}));
