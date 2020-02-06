// Copyright 2020-present Open Networking Foundation
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

#include "stratum/hal/lib/phal/tai/tai_switch_configurator.h"

#include <string>
#include <utility>
#include <vector>

#include "stratum/glue/gtl/map_util.h"
#include "stratum/hal/lib/common/common.pb.h"
#include "stratum/hal/lib/phal/phal.pb.h"
#include "stratum/hal/lib/phal/tai/tai_optics_datasource.h"

namespace stratum {
namespace hal {
namespace phal {
namespace tai {

// Make an instance of TaiSwitchConfigurator
::util::StatusOr<std::unique_ptr<TaiSwitchConfigurator>>
TaiSwitchConfigurator::Make(TaiInterface* tai_interface) {
  // Make sure we've got a valid Tai Interface
  CHECK_RETURN_IF_FALSE(tai_interface != nullptr);

  return absl::WrapUnique(new TaiSwitchConfigurator(tai_interface));
}

// Generate a default config using the TAI API.
::util::Status TaiSwitchConfigurator::CreateDefaultConfig(
    PhalInitConfig* phal_config) const {
  // TODO(plvision)
  auto optical_card = phal_config->add_optical_cards();
  optical_card->set_slot(1);

  return ::util::OkStatus();
}

// Configure the switch's attribute database with the given
// PhalInitConfig config.
::util::Status TaiSwitchConfigurator::ConfigurePhalDB(
    PhalInitConfig* phal_config, AttributeGroup* root) {
  auto mutable_root = root->AcquireMutable();

  // Add cards
  // TODO(plvision)
  ASSIGN_OR_RETURN(auto card_group,
                   mutable_root->AddRepeatedChildGroup("optical_cards"));

  auto mutable_card_group = card_group->AcquireMutable();
  for (auto& card : *phal_config->mutable_optical_cards()) {
    if (!card.has_cache_policy()) {
      card.set_allocated_cache_policy(
          new CachePolicyConfig(phal_config->cache_policy()));
    }
    RETURN_IF_ERROR(
        AddOpticalCard(card.slot(), mutable_card_group.get(), card));
  }

  return ::util::OkStatus();
}

::util::Status TaiSwitchConfigurator::AddOpticalCard(
    int slot, MutableAttributeGroup* mutable_card,
    const PhalOpticalCardConfig& config) {
  ASSIGN_OR_RETURN(auto cache, CachePolicyFactory::CreateInstance(
                                   config.cache_policy().type(),
                                   config.cache_policy().timed_value()));

  ASSIGN_OR_RETURN(auto datasource,
                   TaiOpticsDataSource::Make(slot, tai_interface_, cache));

  RETURN_IF_ERROR(
      mutable_card->AddAttribute("id", datasource->GetModuleSlot()));
  RETURN_IF_ERROR(
      mutable_card->AddAttribute("tx_power", datasource->GetTxPowerAttr()));
  RETURN_IF_ERROR(
      mutable_card->AddAttribute("vendor_name", datasource->GetModuleVendor()));
  RETURN_IF_ERROR(mutable_card->AddAttribute(
      "hardware_state", datasource->GetModuleHardwareState()));

  return ::util::OkStatus();
}

}  // namespace tai
}  // namespace phal
}  // namespace hal

}  // namespace stratum
