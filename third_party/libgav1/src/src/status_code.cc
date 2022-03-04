// Copyright 2019 The libgav1 Authors
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

#include "src/gav1/status_code.h"

extern "C" {

const char* Libgav1GetErrorString(Libgav1StatusCode status) {
  switch (status) {
    case kLibgav1StatusOk:
      return "Success.";
    case kLibgav1StatusUnknownError:
      return "Unknown error.";
    case kLibgav1StatusInvalidArgument:
      return "Invalid function argument.";
    case kLibgav1StatusOutOfMemory:
      return "Memory allocation failure.";
    case kLibgav1StatusResourceExhausted:
      return "Ran out of a resource (other than memory).";
    case kLibgav1StatusNotInitialized:
      return "The object is not initialized.";
    case kLibgav1StatusAlready:
      return "An operation that can only be performed once has already been "
             "performed.";
    case kLibgav1StatusUnimplemented:
      return "Not implemented.";
    case kLibgav1StatusInternalError:
      return "Internal error in libgav1.";
    case kLibgav1StatusBitstreamError:
      return "The bitstream is not encoded correctly or violates a bitstream "
             "conformance requirement.";
    case kLibgav1StatusTryAgain:
      return "The operation is not allowed at the moment. Try again later.";
    case kLibgav1StatusNothingToDequeue:
      return "There are no enqueued frames, so there is nothing to dequeue. "
             "Try enqueuing a frame before trying to dequeue again.";
    // This switch statement does not have a default case. This way the compiler
    // will warn if we neglect to update this function after adding a new value
    // to the Libgav1StatusCode enum type.
    case kLibgav1StatusReservedForFutureExpansionUseDefaultInSwitchInstead_:
      break;
  }
  return "Unrecognized status code.";
}

}  // extern "C"
