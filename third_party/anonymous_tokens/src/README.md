# Anonymous Tokens (AT)

The implementations follow the two IETF standards drafts:

*   [RSA Blind Signatures](https://datatracker.ietf.org/doc/draft-irtf-cfrg-rsa-blind-signatures/)
*   [RSA Blind Signatures with Public Metadata](https://datatracker.ietf.org/doc/draft-amjad-cfrg-partially-blind-rsa/)

As the standardization process is in progress, we expect the code in this repo to change over time to conform to modifications in the IETF specifications.

## Problem Statement

Anonymous Tokens (AT) are a cryptographic protocol that enables propagating trust in a cryptographically secure manner while
maintaining anonymity. At a high level, trust propagation occurs in a two step manner.

* Trusted Setting (User and Signer): In the first stage, a specific user is in a trusted setting with a party that we denote the signer (also known as the issuer). Here, trust may have been established in a variety of ways (authentication, log-in, etc.). To denote that the user is trusted, the issuer may issue a token to the user.
* Untrusted Setting (User and Verifier): In the second stage, suppose the user is now in an untrusted setting with another party that we denote the verifier. The user is now able to prove that they were trusted by the issuer through the use of the prior received token. By using a cryptographically secure verification process, the verifier can check that the user was once trusted by the issuer.

At a high level, anonymous tokens provide the following privacy guarantees:

*   Unforgeability: Adversarial users are not able to fabricate tokens that will pass the verification step of the verifier. In particular, if a malicious user interacts with the signer `K` times, then the user cannot generate `K+1` tokens that successfully verify.
*   Unlinkability: Adversarial signers are unable to determine the interaction that created tokens. Suppose a signer has interacted with users `K` times then and receives one of the resulting signatures at random. Then, the signer cannot determine the interaction resulting in the
challenge signature with probability better than random guess of `1/K`.

## Dependencies

The Private Set Membership library requires the following dependencies:

*   [Abseil](https://github.com/abseil/abseil-cpp) for C++ common libraries.

*   [Bazel](https://github.com/bazelbuild/bazel) for building the library.

*   [BoringSSL](https://github.com/google/boringssl) for underlying
    cryptographic operations.

*   [Google Test](https://github.com/google/googletest) for unit testing the
    library.

*   [Protocol Buffers](https://github.com/google/protobuf) for data
    serialization.

*   [Tink](https://github.com/google/tink) for cryptographic libraries.

## How to build

In order to run this library, you need to install Bazel, if you don't have
it already.
[Follow the instructions for your platform on the Bazel website. Make sure you
 are installing version 6.1.2.]
(https://docs.bazel.build/versions/master/install.html)

You also need to install Git, if you don't have it already.
[Follow the instructions for your platform on the Git website.](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)

Once you've installed Bazel and Git, open a Terminal and clone the repository into a local folder.

Navigate into the `anonymous-tokens` folder you just created, and build the
library and dependencies using Bazel. Note, the library must be built using C++17.

```bash
cd anonymous-tokens
bazel build ... --cxxopt='-std=c++17'
```

You may also run all tests (recursively) using the following command:

```bash
bazel test ... --cxxopt='-std=c++17'
```

## Disclaimers

This is not an officially supported Google product. The software is provided as-is without any guarantees or warranties, express or implied.