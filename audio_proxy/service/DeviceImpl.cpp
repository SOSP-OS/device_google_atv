// Copyright (C) 2020 The Android Open Source Project
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

#include "DeviceImpl.h"

#include <android-base/logging.h>
#include <utils/RefBase.h>

#include "AidlTypes.h"
#include "BusOutputStream.h"
#include "BusStreamProvider.h"
#include "StreamOutImpl.h"

using namespace ::android::hardware::audio::CPP_VERSION;
using namespace ::android::hardware::audio::common::CPP_VERSION;

using ::android::wp;

namespace audio_proxy {
namespace service {
namespace {
AudioPatchHandle gNextAudioPatchHandle = 1;

AidlAudioConfig toAidlAudioConfig(const AudioConfig& hidl_config) {
  AidlAudioConfig aidlConfig = {
      .format = static_cast<AidlAudioFormat>(hidl_config.format),
      .sampleRateHz = static_cast<int32_t>(hidl_config.sampleRateHz),
      .channelMask =
          static_cast<AidlAudioChannelMask>(hidl_config.channelMask)};

  return aidlConfig;
}
}  // namespace

DeviceImpl::DeviceImpl(BusStreamProvider& busStreamProvider,
                       uint32_t bufferSizeMs, uint32_t latencyMs)
    : mBusStreamProvider(busStreamProvider),
      mBufferSizeMs(bufferSizeMs),
      mLatencyMs(latencyMs) {}

// Methods from ::android::hardware::audio::V5_0::IDevice follow.
Return<Result> DeviceImpl::initCheck() { return Result::OK; }

Return<Result> DeviceImpl::setMasterVolume(float volume) {
  // software mixer will emulate this ability
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getMasterVolume(getMasterVolume_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, 0.f);
  return Void();
}

Return<Result> DeviceImpl::setMicMute(bool mute) {
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getMicMute(getMicMute_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, false);
  return Void();
}

Return<Result> DeviceImpl::setMasterMute(bool mute) {
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getMasterMute(getMasterMute_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, false);
  return Void();
}

Return<void> DeviceImpl::getInputBufferSize(const AudioConfig& config,
                                            getInputBufferSize_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, 0);
  return Void();
}

Return<void> DeviceImpl::openOutputStream(int32_t ioHandle,
                                          const DeviceAddress& device,
                                          const AudioConfig& config,
                                          hidl_bitfield<AudioOutputFlag> flags,
                                          const SourceMetadata& sourceMetadata,
                                          openOutputStream_cb _hidl_cb) {
  std::shared_ptr<BusOutputStream> busOutputStream =
      mBusStreamProvider.openOutputStream(device.busAddress,
                                          toAidlAudioConfig(config),
                                          static_cast<int32_t>(flags));
  DCHECK(busOutputStream);
  auto streamOut = sp<StreamOutImpl>::make(std::move(busOutputStream),
                                           mBufferSizeMs, mLatencyMs);
  mBusStreamProvider.onStreamOutCreated(streamOut);
  _hidl_cb(Result::OK, streamOut, config);
  return Void();
}

Return<void> DeviceImpl::openInputStream(int32_t ioHandle,
                                         const DeviceAddress& device,
                                         const AudioConfig& config,
                                         hidl_bitfield<AudioInputFlag> flags,
                                         const SinkMetadata& sinkMetadata,
                                         openInputStream_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, sp<IStreamIn>(), config);
  return Void();
}

Return<bool> DeviceImpl::supportsAudioPatches() { return true; }

// Create a do-nothing audio patch.
Return<void> DeviceImpl::createAudioPatch(
    const hidl_vec<AudioPortConfig>& sources,
    const hidl_vec<AudioPortConfig>& sinks, createAudioPatch_cb _hidl_cb) {
  AudioPatchHandle handle = gNextAudioPatchHandle++;
  mAudioPatchHandles.insert(handle);
  _hidl_cb(Result::OK, handle);
  return Void();
}

Return<Result> DeviceImpl::releaseAudioPatch(AudioPatchHandle patch) {
  size_t removed = mAudioPatchHandles.erase(patch);
  return removed > 0 ? Result::OK : Result::INVALID_ARGUMENTS;
}

Return<void> DeviceImpl::getAudioPort(const AudioPort& port,
                                      getAudioPort_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, port);
  return Void();
}

Return<Result> DeviceImpl::setAudioPortConfig(const AudioPortConfig& config) {
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getHwAvSync(getHwAvSync_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, 0);
  return Void();
}

Return<Result> DeviceImpl::setScreenState(bool turnedOn) {
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getParameters(const hidl_vec<ParameterValue>& context,
                                       const hidl_vec<hidl_string>& keys,
                                       getParameters_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, hidl_vec<ParameterValue>());
  return Void();
}

Return<Result> DeviceImpl::setParameters(
    const hidl_vec<ParameterValue>& context,
    const hidl_vec<ParameterValue>& parameters) {
  return Result::NOT_SUPPORTED;
}

Return<void> DeviceImpl::getMicrophones(getMicrophones_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, hidl_vec<MicrophoneInfo>());
  return Void();
}

Return<Result> DeviceImpl::setConnectedState(const DeviceAddress& address,
                                             bool connected) {
  return Result::OK;
}

#if MAJOR_VERSION >= 6
Return<void> DeviceImpl::updateAudioPatch(
    AudioPatchHandle previousPatch, const hidl_vec<AudioPortConfig>& sources,
    const hidl_vec<AudioPortConfig>& sinks, updateAudioPatch_cb _hidl_cb) {
  if (mAudioPatchHandles.erase(previousPatch) == 0) {
    _hidl_cb(Result::INVALID_ARGUMENTS, 0);
    return Void();
  }
  AudioPatchHandle newPatch = gNextAudioPatchHandle++;
  mAudioPatchHandles.insert(newPatch);
  _hidl_cb(Result::OK, newPatch);
  return Void();
}

Return<Result> DeviceImpl::close() {
  return mBusStreamProvider.cleanAndCountStreamOuts() == 0
             ? Result::OK
             : Result::INVALID_STATE;
}

Return<Result> DeviceImpl::addDeviceEffect(AudioPortHandle device,
                                           uint64_t effectId) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DeviceImpl::removeDeviceEffect(AudioPortHandle device,
                                              uint64_t effectId) {
  return Result::NOT_SUPPORTED;
}
#endif

}  // namespace service
}  // namespace audio_proxy
