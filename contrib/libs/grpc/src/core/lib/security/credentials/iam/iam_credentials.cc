/*
 *
 * Copyright 2016 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpc/support/port_platform.h>

#include "src/core/lib/security/credentials/iam/iam_credentials.h"

#include "y_absl/strings/str_format.h"

#include <grpc/support/alloc.h>
#include <grpc/support/log.h>
#include <grpc/support/string_util.h>
#include <grpc/support/sync.h>

#include "src/core/lib/gprpp/ref_counted_ptr.h"
#include "src/core/lib/promise/promise.h"
#include "src/core/lib/surface/api_trace.h"

grpc_core::ArenaPromise<y_absl::StatusOr<grpc_core::ClientInitialMetadata>>
grpc_google_iam_credentials::GetRequestMetadata(
    grpc_core::ClientInitialMetadata initial_metadata,
    const grpc_call_credentials::GetRequestMetadataArgs*) {
  if (token_.has_value()) {
    initial_metadata->Append(
        GRPC_IAM_AUTHORIZATION_TOKEN_METADATA_KEY, token_->Ref(),
        [](y_absl::string_view, const grpc_core::Slice&) { abort(); });
  }
  initial_metadata->Append(
      GRPC_IAM_AUTHORITY_SELECTOR_METADATA_KEY, authority_selector_.Ref(),
      [](y_absl::string_view, const grpc_core::Slice&) { abort(); });
  return grpc_core::Immediate(std::move(initial_metadata));
}

grpc_google_iam_credentials::grpc_google_iam_credentials(
    const char* token, const char* authority_selector)
    : grpc_call_credentials(GRPC_CALL_CREDENTIALS_TYPE_IAM),
      token_(token == nullptr ? y_absl::optional<grpc_core::Slice>()
                              : grpc_core::Slice::FromCopiedString(token)),
      authority_selector_(
          grpc_core::Slice::FromCopiedString(authority_selector)),
      debug_string_(y_absl::StrFormat(
          "GoogleIAMCredentials{Token:%s,AuthoritySelector:%s}",
          token != nullptr ? "present" : "absent", authority_selector)) {}

grpc_call_credentials* grpc_google_iam_credentials_create(
    const char* token, const char* authority_selector, void* reserved) {
  grpc_core::ExecCtx exec_ctx;
  GRPC_API_TRACE(
      "grpc_iam_credentials_create(token=%s, authority_selector=%s, "
      "reserved=%p)",
      3, (token, authority_selector, reserved));
  GPR_ASSERT(reserved == nullptr);
  GPR_ASSERT(token != nullptr);
  GPR_ASSERT(authority_selector != nullptr);
  return grpc_core::MakeRefCounted<grpc_google_iam_credentials>(
             token, authority_selector)
      .release();
}
