// Copyright 2018 The Outline Authors
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

#include <fwpmtypes.h>
#include <fwpmu.h>
#include <iostream>

#pragma comment(lib, "fwpuclnt.lib")

using namespace ::std;

PCWSTR providerName = L"Outline";

int main(int argc, char** argv) {
  // Connect to the filtering engine. By using a dynamic session, all of our changes are
  // *non-destructive* and will vanish on exit/crash/whatever.
  FWPM_SESSION0 session;
  memset(&session, 0, sizeof(session));
  session.flags = FWPM_SESSION_FLAG_DYNAMIC;

  HANDLE engine = 0;
  DWORD result = FwpmEngineOpen0(NULL, RPC_C_AUTHN_DEFAULT, NULL, &session, &engine);
  if (result != ERROR_SUCCESS) {
    cerr << "could not connect to to filtering engine: " << result << endl;
    return 1;
  }
  cout << "connected to filtering engine" << endl;

  // Create our filters. We create two filters, each with multiple conditions. The first filter
  // blocks all UDP traffic on port 53 while the second allows such traffic *on the TAP device*.
  // This approach is the same as that used in the SDK documentation:
  //   https://docs.microsoft.com/en-us/windows/desktop/fwp/reserving-ports
  FWPM_FILTER_CONDITION0 conditions[3];

  conditions[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
  conditions[0].matchType = FWP_MATCH_EQUAL;
  conditions[0].conditionValue.type = FWP_UINT8;
  conditions[0].conditionValue.uint16 = IPPROTO_UDP;

  conditions[1].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
  conditions[1].matchType = FWP_MATCH_EQUAL;
  conditions[1].conditionValue.type = FWP_UINT16;
  conditions[1].conditionValue.uint16 = 53;

  // TODO: lookup interface index of outline-tap0.
  conditions[2].fieldKey = FWPM_CONDITION_LOCAL_INTERFACE_INDEX;
  conditions[2].matchType = FWP_MATCH_EQUAL;
  conditions[2].conditionValue.type = FWP_UINT32;
  conditions[2].conditionValue.uint32 = 3;

  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.displayData.name = (PWSTR)providerName;
  filter.filterCondition = conditions;
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;

  UINT64 filterId;

  // Blanket UDP port 53 block.
  filter.numFilterConditions = 2;
  filter.action.type = FWP_ACTION_BLOCK;
  result = FwpmFilterAdd0(engine, &filter, NULL, &filterId);
  if (result != ERROR_SUCCESS) {
    cerr << "could not connect block port 53: " << result << endl;
    return 1;
  }
  cout << "port 53 blocked with filter " << filterId << endl;

  // Whitelist UDP port 53 on the TAP device.
  filter.numFilterConditions = 3;
  filter.action.type = FWP_ACTION_PERMIT;
  result = FwpmFilterAdd0(engine, &filter, NULL, &filterId);
  if (result != ERROR_SUCCESS) {
    cerr << "could not connect whitelist port 53 on outline-tap0: " << result << endl;
    return 1;
  }
  cout << "port 53 whitelisted on outline-tap0 with filter " << filterId << endl;

  // TODO: blanket IPv6 block?

  // Wait forever.
  system("pause");
}
