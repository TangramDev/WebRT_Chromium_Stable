// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gpu/gpu_mode_manager.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/gpu_data_manager.h"

// begin Add by TangramTeam
#include "third_party/webruntime/UniverseForChromium.h"
extern CommonUniverse::CWebRTImpl* g_pWebRTImpl;  // 20200108
// end Add by TangramTeam

using base::UserMetricsAction;

namespace {

bool GetPreviousGpuModePref() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  return service->GetBoolean(prefs::kHardwareAccelerationModePrevious);
}

void SetPreviousGpuModePref(bool enabled) {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  service->SetBoolean(prefs::kHardwareAccelerationModePrevious, enabled);
}

}  // namespace

// static
void GpuModeManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(
      prefs::kHardwareAccelerationModeEnabled, true);
  registry->RegisterBooleanPref(
      prefs::kHardwareAccelerationModePrevious, true);
}

GpuModeManager::GpuModeManager()
    : initial_gpu_mode_pref_(true) {
  if (g_browser_process->local_state()) {  // Skip for unit tests
    pref_registrar_.Init(g_browser_process->local_state());
    // Do nothing when the pref changes. It takes effect after
    // chrome restarts.
    pref_registrar_.Add(prefs::kHardwareAccelerationModeEnabled,
                        base::DoNothingAs<void()>());

    initial_gpu_mode_pref_ = IsGpuModePrefEnabled();
    bool previous_gpu_mode_pref = GetPreviousGpuModePref();
    SetPreviousGpuModePref(initial_gpu_mode_pref_);

    UMA_HISTOGRAM_BOOLEAN("GPU.HardwareAccelerationModeEnabled",
                          initial_gpu_mode_pref_);
    if (previous_gpu_mode_pref && !initial_gpu_mode_pref_)
      base::RecordAction(UserMetricsAction("GpuAccelerationDisabled"));
    if (!previous_gpu_mode_pref && initial_gpu_mode_pref_)
      base::RecordAction(UserMetricsAction("GpuAccelerationEnabled"));

    // Begin Add by TangramTeam
    bool bEnableHardwareAcceleration = true;
    if (g_pWebRTImpl)
      bEnableHardwareAcceleration =
          g_pWebRTImpl->m_bEnableHardwareAcceleration;

    if (!initial_gpu_mode_pref_ || !bEnableHardwareAcceleration) {
      content::GpuDataManager* gpu_data_manager =
          content::GpuDataManager::GetInstance();
      DCHECK(gpu_data_manager);
      gpu_data_manager->DisableHardwareAcceleration();
      g_pWebRTImpl->m_bEnableHardwareAcceleration = false;
    } else {
      g_pWebRTImpl->m_bEnableHardwareAcceleration = true;
    }
    // End Add by TangramTeam
  }
}

GpuModeManager::~GpuModeManager() {
}

bool GpuModeManager::initial_gpu_mode_pref() const {
  return initial_gpu_mode_pref_;
}

// static
bool GpuModeManager::IsGpuModePrefEnabled() {
  PrefService* service = g_browser_process->local_state();
  DCHECK(service);
  return service->GetBoolean(
      prefs::kHardwareAccelerationModeEnabled);
}

