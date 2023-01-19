/*************************************************************************
 *
 * Copyright 2021 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include "realm/error_codes.hpp"

namespace realm {

StringData ErrorCodes::error_string(Error code)
{
    switch (code) {
        case ErrorCodes::OK:
            return "OK";
        case ErrorCodes::RuntimeError:
            return "RuntimeError";
        case ErrorCodes::LogicError:
            return "LogicError";
        case ErrorCodes::BrokenPromise:
            return "BrokenPromise";
        case ErrorCodes::OperationAborted:
            return "OperationAborted";

        /// WebSocket error codes
        case ErrorCodes::WebSocket_GoingAway:
            return "WebSocket: Going Away";
        case ErrorCodes::WebSocket_ProtocolError:
            return "WebSocket: Protocol Error";
        case ErrorCodes::WebSocket_UnsupportedData:
            return "WebSocket: Unsupported Data";
        case ErrorCodes::WebSocket_Reserved:
            return "WebSocket: Reserved";
        case ErrorCodes::WebSocket_NoStatusReceived:
            return "WebSocket: No Status Received";
        case ErrorCodes::WebSocket_AbnormalClosure:
            return "WebSocket: Abnormal Closure";
        case ErrorCodes::WebSocket_InvalidPayloadData:
            return "WebSocket: Invalid Payload Data";
        case ErrorCodes::WebSocket_PolicyViolation:
            return "WebSocket: Policy Violation";
        case ErrorCodes::WebSocket_MessageTooBig:
            return "WebSocket: Message Too Big";
        case ErrorCodes::WebSocket_InavalidExtension:
            return "WebSocket: Invalid Extension";
        case ErrorCodes::WebSocket_InternalServerError:
            return "WebSocket: Internal Server Error";
        case ErrorCodes::WebSocket_TLSHandshakeFailed:
            return "WebSocket: TLS Handshake Failed";

        case ErrorCodes::UnknownError:
            [[fallthrough]];
        default:
            return "UnknownError";
    }
}

} // namespace realm
