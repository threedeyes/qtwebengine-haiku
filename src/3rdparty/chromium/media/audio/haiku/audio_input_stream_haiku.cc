// Copyright 2021 Gerasim Troeglazov. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/haiku/audio_input_stream_haiku.h"

#include "base/logging.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/haiku/audio_manager_haiku.h"

namespace media {
	
const int kNumberOfBlocksBufferInFifo = 2;

HaikuAudioRecorder::HaikuAudioRecorder(const char *node_name)
    : node_name_(node_name) {
  if (node_name == NULL) {
      node_name_.SetTo("QtWebEngine");
      app_info appInfo;
      if (be_app->GetAppInfo(&appInfo) == B_OK) {
          BPath path(&appInfo.ref);
          node_name_.SetTo(path.Leaf());
      }
  }
  status_t error;
  media_roster_ = BMediaRoster::Roster(&error);
  if (error == B_OK)
      is_inited_ = true;
}

HaikuAudioRecorder::~HaikuAudioRecorder() {
  Close();
}

bool HaikuAudioRecorder::SetCallbacks(BMediaRecorder::ProcessFunc record_func,
                                      BMediaRecorder::NotifyFunc notify_func, void* cookie) {
  if (recorder_->SetHooks(record_func, notify_func, cookie) < B_OK) {
      recorder_->SetHooks(NULL, NULL, NULL);
      return false;
  }
  return true;
}

bool HaikuAudioRecorder::Open() {
  if (!is_inited_ || is_opened_)
      return false;

  status_t error;

  error = media_roster_->GetAudioInput(&audio_input_node_);
  if (error < B_OK)
      return false;

  error = media_roster_->GetAudioMixer(&audio_mixer_node_);
  if (error < B_OK)
      return false;

  recorder_ = new BMediaRecorder(node_name_, B_MEDIA_RAW_AUDIO);
  if (recorder_->InitCheck() < B_OK) {
      delete recorder_;
      recorder_ = nullptr;
      return false;
  }

  media_format output_format;
  output_format.type = B_MEDIA_RAW_AUDIO;
  output_format.u.raw_audio = media_raw_audio_format::wildcard;
  recorder_->SetAcceptedFormat(output_format);

  const int maxInputCount = 64;
  dormant_node_info dni[maxInputCount];

  int32 real_count = maxInputCount;

  error = media_roster_->GetDormantNodes(dni, &real_count, 0, &output_format,
                                         0, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
  if (real_count > maxInputCount)
      real_count = maxInputCount;
  char selected_name[B_MEDIA_NAME_LENGTH] = "Default input";

  for (int i = 0; i < real_count; i++) {
      media_node_id ni[12];
      int32 ni_count = 12;
      error = media_roster_->GetInstancesFor(dni[i].addon, dni[i].flavor_id, ni, &ni_count);
      if (error == B_OK) {
           for (int j = 0; j < ni_count; j++) {
               if (ni[j] == audio_input_node_.node) {
                   strcpy(selected_name, dni[i].name);
                   break;
               }
           }
      }
  }
  
  if (!recorder_->IsConnected()) {
      int32 count = 0;
      error = media_roster_->GetFreeOutputsFor(audio_input_node_,
                                               &audio_output_, 1, &count, B_MEDIA_RAW_AUDIO);
      if (error < B_OK || count < 1) {
          delete recorder_;
          recorder_ = nullptr;
          return false;
      }

      record_format_.u.raw_audio = audio_output_.format.u.raw_audio;
  } else {
      record_format_.u.raw_audio = recorder_->AcceptedFormat().u.raw_audio;
  }

  record_format_.type = B_MEDIA_RAW_AUDIO;

  is_opened_ = true;

  return true;
}

void HaikuAudioRecorder::Close() {
  if (!is_opened_)
      return;
  is_opened_ = false;

  if (is_recording_)
      Stop();

  if (recorder_)
      delete recorder_;

  recorder_ = nullptr;
}

void HaikuAudioRecorder::Start() {
  if (recorder_ == NULL || !is_inited_ || is_recording_)
      return;

  if (!recorder_->IsConnected()) {
      if (recorder_->Connect(audio_input_node_, &audio_output_, &record_format_) < B_OK) {
           recorder_->SetHooks(NULL, NULL, NULL);
           return;
      }
  }

  is_recording_ = true;
  recorder_->Start();
}

void HaikuAudioRecorder::Stop() {
  is_recording_ = false;

  if (recorder_->IsConnected())
      recorder_->Disconnect();

  recorder_->Stop();
}

base::TimeDelta HaikuAudioRecorder::Latency() {
  bigtime_t latency;
  if (media_roster_->GetLatencyFor(audio_input_node_, &latency) == B_OK && is_recording_)
      return base::TimeDelta::FromMicroseconds(latency);
  return base::TimeDelta::FromMicroseconds(0);
}

SampleFormat HaikuAudioRecorder::Format() {
  SampleFormat sample_format = kUnknownSampleFormat;

  switch(record_format_.u.raw_audio.format) {
      case media_raw_audio_format::B_AUDIO_UCHAR:
          sample_format = kSampleFormatU8;
          break;
      case media_raw_audio_format::B_AUDIO_SHORT:
          sample_format = kSampleFormatS16;
          break;
      case media_raw_audio_format::B_AUDIO_INT:
          sample_format = kSampleFormatS32;
          break;
      case media_raw_audio_format::B_AUDIO_FLOAT:
          sample_format = kSampleFormatF32;
          break;
  }
  return sample_format;
}

ChannelLayout HaikuAudioRecorder::Channels() {
  ChannelLayout layout = CHANNEL_LAYOUT_UNSUPPORTED;
  
  switch(record_format_.u.raw_audio.channel_count) {
      case 1:
          layout = CHANNEL_LAYOUT_MONO;
          break;
      case 2:
          layout = CHANNEL_LAYOUT_STEREO;
          break;
  }

  return layout;
}


AudioInputStreamHaiku::AudioInputStreamHaiku(
    AudioManagerHaiku* manager,
    const AudioParameters& parameters)
    : manager_(manager),
      parameters_(parameters),
      recorder_(new HaikuAudioRecorder()),
      callback_(nullptr),
      fifo_(parameters.channels(),
            parameters.frames_per_buffer(),
            kNumberOfBlocksBufferInFifo),
      audio_bus_(media::AudioBus::Create(parameters)) {
}

AudioInputStreamHaiku::~AudioInputStreamHaiku() {
	delete recorder_;
}

bool AudioInputStreamHaiku::Open() {
  return recorder_->Open();
}

void AudioInputStreamHaiku::Start(AudioInputCallback* callback) {
  callback_ = callback;
  if (!recorder_->IsOpened() || recorder_->IsRecording())
      return;

  if (!recorder_->SetCallbacks(_read_callback, NULL, this)) {
      recorder_->Close();
      return;
  }

  StartAgc();
  recorder_->Start();
}

void AudioInputStreamHaiku::Stop() {
  StopAgc();
  recorder_->Stop();
  fifo_.Clear();
}

void AudioInputStreamHaiku::Close() {
  recorder_->Close();
  manager_->ReleaseInputStream(this);
}

double AudioInputStreamHaiku::GetMaxVolume() {
  return 1.0;
}

void AudioInputStreamHaiku::SetVolume(double volume) {
  UpdateAgcVolume();
}

double AudioInputStreamHaiku::GetVolume() {
  return 1.0;
}

bool AudioInputStreamHaiku::SetAutomaticGainControl(bool enabled) {
  return false;
}

bool AudioInputStreamHaiku::GetAutomaticGainControl() {
  return false;
}

bool AudioInputStreamHaiku::IsMuted() {
  return false;
}

void AudioInputStreamHaiku::SetOutputDeviceForAec(
    const std::string& output_device_id) {
}

void AudioInputStreamHaiku::ReadCallback(void* buffer, size_t size,
                                         const media_format &format) noexcept
{
  double normalized_volume = 0.0;
  GetAgcVolume(&normalized_volume);

  SampleFormat sample_format = recorder_->Format();

  base::TimeTicks capture_time =
      base::TimeTicks::Now() - (recorder_->Latency() +
      AudioTimestampHelper::FramesToTime(fifo_.GetAvailableFrames(),
      parameters_.sample_rate()));

  const int number_of_frames = size / parameters_.GetBytesPerFrame(sample_format);

  if (number_of_frames > fifo_.GetUnfilledFrames()) {
      const int increase_blocks_of_buffer =
          static_cast<int>((number_of_frames - fifo_.GetUnfilledFrames()) /
                           parameters_.frames_per_buffer()) + 1;
      fifo_.IncreaseCapacity(increase_blocks_of_buffer);
  }

  fifo_.Push(buffer, number_of_frames, SampleFormatToBytesPerChannel(sample_format));

  while (fifo_.available_blocks()) {
    const AudioBus* audio_bus = fifo_.Consume();

    callback_->OnData(audio_bus, capture_time, normalized_volume);

    capture_time += AudioTimestampHelper::FramesToTime(audio_bus->frames(),
                                                       parameters_.sample_rate());

    if (fifo_.available_blocks())
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(5));
  }
}

}  // namespace media
