// Copyright 2021 Gerasim Troeglazov. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_HAIKU_AUDIO_INPUT_STREAM_HAIKU_H_
#define MEDIA_AUDIO_HAIKU_AUDIO_INPUT_STREAM_HAIKU_H_

#include "media/audio/audio_io.h"
#include "media/audio/agc_audio_stream.h"
#include "media/base/audio_block_fifo.h"
#include "media/base/audio_parameters.h"
#include "media/base/audio_timestamp_helper.h"

#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <Application.h>
#include <MediaAddOn.h>
#include <MediaFile.h>
#include <MediaNode.h>
#include <MediaRecorder.h>
#include <MediaRoster.h>
#include <MediaTrack.h>
#include <TimeSource.h>
#include <NodeInfo.h>

namespace media {

class AudioManagerHaiku;

class HaikuAudioRecorder {
public:
  HaikuAudioRecorder(const char *node_name = NULL);
  ~HaikuAudioRecorder();

  bool SetCallbacks(BMediaRecorder::ProcessFunc recordFunc = NULL,
                    BMediaRecorder::NotifyFunc notifyFunc = NULL,
                    void* cookie = NULL);

  bool Open();
  void Close();
  void Start();
  void Stop();

  base::TimeDelta Latency();
  BMediaRoster *MediaRoster() { return media_roster_; }

  SampleFormat Format();
  ChannelLayout Channels();
  int SampleRate() { return static_cast<int>(record_format_.u.raw_audio.frame_rate); }

  bool IsOpened() { return is_opened_; }
  bool IsRecording() { return is_recording_; }
  bool InitCheck() { return is_inited_; }

private:
  BString node_name_;

  BMediaRoster *media_roster_{0u};
  BMediaRecorder *recorder_{0u};

  media_format record_format_;
  media_node audio_input_node_;
  media_node audio_mixer_node_;
  media_output audio_output_;

  bool is_inited_{false};
  bool is_opened_{false};
  bool is_recording_{false};
};


class AudioInputStreamHaiku : public AgcAudioStream<AudioInputStream> {
 public:
  AudioInputStreamHaiku(AudioManagerHaiku* manager,
                          const AudioParameters& parameters);
  ~AudioInputStreamHaiku() override;

  bool Open() override;
  void Start(AudioInputCallback* callback) override;
  void Stop() override;
  void Close() override;
  double GetMaxVolume() override;
  void SetVolume(double volume) override;
  double GetVolume() override;
  bool SetAutomaticGainControl(bool enabled) override;
  bool GetAutomaticGainControl() override;
  bool IsMuted() override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;

  void ReadCallback(void* data, size_t size, const media_format &format) noexcept;

 private:
  static void _read_callback(void* cookie, bigtime_t, void* data, size_t size, const media_format &format) noexcept {
      return static_cast<AudioInputStreamHaiku*>(cookie)->ReadCallback(data, size, format);
  }

  AudioManagerHaiku* const manager_;
  AudioParameters parameters_;
  HaikuAudioRecorder* recorder_;
  AudioInputCallback* callback_;
  AudioBlockFifo fifo_;

  std::unique_ptr<AudioBus> audio_bus_;

  DISALLOW_COPY_AND_ASSIGN(AudioInputStreamHaiku);
};

}  // namespace media

#endif  // MEDIA_AUDIO_HAIKU_AUDIO_INPUT_STREAM_HAIKU_H_
