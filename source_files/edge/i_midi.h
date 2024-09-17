/*
 * BW_Midi_Sequencer - MIDI Sequencer for C++
 *
 * Copyright (c) 2015-2022 Vitaly Novichkov <admin@wohlnet.ru>
 * Copyright (c) 2024 The EDGE Team.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * MUS2MIDI: MUS to MIDI Library
 *
 * Copyright (C) 2014  Bret Curtis
 * Copyright (C) WildMIDI Developers  2015-2016
 * ADLMIDI Library API: Copyright (c) 2015-2023 Vitaly Novichkov
 * <admin@wohlnet.ru>
 * Copyright (c) 2024 The EDGE Team.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <iterator>
#include <list>
#include <set>
#include <string>
#include <vector>

#include "epi.h"
#include "epi_file.h"

/*! Raw MIDI event hook */
typedef void (*RawEventHook)(void *userdata, uint8_t type, uint8_t subtype, uint8_t channel, const uint8_t *data,
                             size_t len);
/*! PCM render */
typedef void (*PcmRender)(void *userdata, uint8_t *stream, size_t length);
/*! Library internal debug messages */
typedef void (*DebugMessageHook)(void *userdata, const char *fmt, ...);
/*! Loop Start event hook */
typedef void (*LoopStartHook)(void *userdata);
/*! Loop Start event hook */
typedef void (*LoopEndHook)(void *userdata);
typedef void (*SongStartHook)(void *userdata);

/*! Note-On MIDI event */
typedef void (*RtNoteOn)(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity);
/*! Note-Off MIDI event */
typedef void (*RtNoteOff)(void *userdata, uint8_t channel, uint8_t note);
/*! Note-Off MIDI event with a velocity */
typedef void (*RtNoteOffVel)(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity);
/*! Note aftertouch MIDI event */
typedef void (*RtNoteAfterTouch)(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal);
/*! Channel aftertouch MIDI event */
typedef void (*RtChannelAfterTouch)(void *userdata, uint8_t channel, uint8_t atVal);
/*! Controller change MIDI event */
typedef void (*RtControllerChange)(void *userdata, uint8_t channel, uint8_t type, uint8_t value);
/*! Patch change MIDI event */
typedef void (*RtPatchChange)(void *userdata, uint8_t channel, uint8_t patch);
/*! Pitch bend MIDI event */
typedef void (*RtPitchBend)(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb);
/*! System Exclusive MIDI event */
typedef void (*RtSysEx)(void *userdata, const uint8_t *msg, size_t size);
/*! Meta event hook */
typedef void (*MetaEventHook)(void *userdata, uint8_t type, const uint8_t *data, size_t len);
/*! Device Switch MIDI event */
typedef void (*RtDeviceSwitch)(void *userdata, size_t track, const char *data, size_t length);
/*! Get the channels offset for current MIDI device */
typedef size_t (*RtCurrentDevice)(void *userdata, size_t track);

/**
     \brief Real-Time MIDI interface between Sequencer and the Synthesizer
    */
struct MidiRealTimeInterface
{
    /*! MIDI event hook which catches all MIDI events */
    RawEventHook onEvent;
    /*! User data which will be passed through On-Event hook */
    void *onEvent_userdata;

    /*! PCM render hook which catches passing of loop start point */
    PcmRender onPcmRender;
    /*! User data which will be passed through On-PCM-render hook */
    void *onPcmRender_userdata;

    /*! Sample rate */
    uint32_t pcmSampleRate;

    /*! Size of one sample in bytes */
    uint32_t pcmFrameSize;

    /*! Debug message hook */
    DebugMessageHook onDebugMessage;
    /*! User data which will be passed through Debug Message hook */
    void *onDebugMessage_userdata;

    /*! Loop start hook which catches passing of loop start point */
    LoopStartHook onloopStart;
    /*! User data which will be passed through On-LoopStart hook */
    void *onloopStart_userdata;

    /*! Loop start hook which catches passing of loop start point */
    LoopEndHook onloopEnd;
    /*! User data which will be passed through On-LoopStart hook */
    void *onloopEnd_userdata;

    /*! Song start hook which is calling when starting playing song at begin
     */
    SongStartHook onSongStart;
    /*! User data which will be passed through On-SongStart hook */
    void *onSongStart_userdata;

    /*! MIDI Run Time event calls user data */
    void *rtUserData;

    /***************************************************
     * Standard MIDI events. All of them are required! *
     ***************************************************/

    /*! Note-On MIDI event hook */
    RtNoteOn rt_noteOn;
    /*! Note-Off MIDI event hook */
    RtNoteOff rt_noteOff;

    /*! Note-Off MIDI event hook with a velocity */
    RtNoteOffVel rt_noteOffVel;

    /*! Note aftertouch MIDI event hook */
    RtNoteAfterTouch rt_noteAfterTouch;

    /*! Channel aftertouch MIDI event hook */
    RtChannelAfterTouch rt_channelAfterTouch;

    /*! Controller change MIDI event hook */
    RtControllerChange rt_controllerChange;

    /*! Patch change MIDI event hook */
    RtPatchChange rt_patchChange;

    /*! Pitch bend MIDI event hook */
    RtPitchBend rt_pitchBend;

    /*! System Exclusive MIDI event hook */
    RtSysEx rt_systemExclusive;

    /*******************
     * Optional events *
     *******************/

    /*! Meta event hook which catches all meta events */
    MetaEventHook rt_metaEvent;

    /*! Device Switch MIDI event hook */
    RtDeviceSwitch rt_deviceSwitch;

    /*! Get the channels offset for current MIDI device hook. Returms
     * multiple to 16 value. */
    RtCurrentDevice rt_currentDevice;
};

// MidiFraction is a stripped down version of
// Bisqwit's Fraction class with the following
// copyright:
/*
 * Fraction number handling.
 * Copyright (C) 1992,2001 Bisqwit (http://iki.fi/bisqwit/)
 *
 * The license of this file is in Public Domain:
 * https://bisqwit.iki.fi/src/index.html
 *
 * "... and orphan source code files are copyrighted public domain."
 */

class MidiFraction
{
    uint64_t num1_, num2_;
    void     Optim();

  public:
    MidiFraction() : num1_(0), num2_(1)
    {
    }
    MidiFraction(uint64_t value) : num1_(value), num2_(1)
    {
    }
    MidiFraction(uint64_t n, uint64_t d) : num1_(n), num2_(d)
    {
    }
    inline double Value() const
    {
        return Nom() / (double)Denom();
    }
    MidiFraction &operator*=(const MidiFraction &b)
    {
        num1_ *= b.Nom();
        num2_ *= b.Denom();
        Optim();
        return *this;
    }
    MidiFraction operator*(const MidiFraction &b) const
    {
        MidiFraction tmp(*this);
        tmp *= b;
        return tmp;
    }
    const uint64_t &Nom() const
    {
        return num1_;
    }
    const uint64_t &Denom() const
    {
        return num2_;
    }
};

void MidiFraction::Optim()
{
    /* Euclidean algorithm */
    uint64_t n1, n2, nn1, nn2;

    nn1 = num1_;
    nn2 = num2_;

    if (nn1 < nn2)
        n1 = num1_, n2 = num2_;
    else
        n1 = num2_, n2 = num1_;

    if (!num1_)
    {
        num2_ = 1;
        return;
    }
    for (;;)
    {
        uint64_t tmp = n2 % n1;
        if (!tmp)
            break;
        n2 = n1;
        n1 = tmp;
    }
    num1_ /= n1;
    num2_ /= n1;
}

MidiFraction operator*(const uint64_t bla, const MidiFraction &b)
{
    return MidiFraction(bla) * b;
}

class MidiSequencer
{
    /**
     * @brief MIDI Event utility container
     */
    struct MidiEvent
    {
        /**
         * @brief Main MIDI event types
         */
        enum Types
        {
            //! Unknown event
            kUnknown = 0x00,
            //! Note-Off event
            kNoteOff = 0x08,           // size == 2
                                       //! Note-On event
            kNoteOn = 0x09,            // size == 2
                                       //! Note After-Touch event
            kNoteTouch = 0x0A,         // size == 2
                                       //! Controller change event
            kControlChange = 0x0B,     // size == 2
                                       //! Patch change event
            kPatchChange = 0x0C,       // size == 1
                                       //! Channel After-Touch event
            kChannelAftertouch = 0x0D, // size == 1
                                       //! Pitch-bend change event
            kPitchWheel = 0x0E,        // size == 2

            //! System Exclusive message, type 1
            kSysex = 0xF0,                     // size == len
                                               //! Sys Com Song Position Pntr [LSB, MSB]
            kSysComSongPositionPointer = 0xF2, // size == 2
                                               //! Sys Com Song Select(Song #) [0-127]
            kSysComSongSelect = 0xF3,          // size == 1
                                               //! System Exclusive message, type 2
            kSysex2 = 0xF7,                    // size == len
                                               //! Special event
            kSpecial = 0xFF
        };
        /**
         * @brief Special MIDI event sub-types
         */
        enum SubTypes
        {
            //! Sequension number
            kSequensionNumber = 0x00,   // size == 2
                                        //! Text label
            kText = 0x01,               // size == len
                                        //! Copyright notice
            kCopyright = 0x02,          // size == len
                                        //! Sequence track title
            kSequenceTrackTitle = 0x03, // size == len
                                        //! Instrument title
            kInstrumentTitle = 0x04,    // size == len
                                        //! Lyrics text fragment
            kLyrics = 0x05,             // size == len
                                        //! MIDI Marker
            kMarker = 0x06,             // size == len
                                        //! Cue Point
            kCuePoint = 0x07,           // size == len
                                        //! [Non-Standard] Device Switch
            kDeviceSwitch = 0x09,       // size == len <CUSTOM>
                                        //! MIDI Channel prefix
            kMidiChannelPrefix = 0x20,  // size == 1

            //! End of Track event
            kEndTrack = 0x2F,      // size == 0
                                   //! Tempo change event
            kTempoChange = 0x51,   // size == 3
                                   //! SMPTE offset
            kSmpteOffset = 0x54,   // size == 5
                                   //! Time signature
            kTimeSignature = 0x55, // size == 4
                                   //! Key signature
            kKeySignature = 0x59,  // size == 2
                                   //! Sequencer specs
            kSequencerSpec = 0x7F, // size == len

            /* Non-standard, internal ADLMIDI usage only */
            //! [Non-Standard] Loop Start point
            kLoopStart = 0xE1, // size == 0 <CUSTOM>
                               //! [Non-Standard] Loop End point
            kLoopEnd = 0xE2,   // size == 0 <CUSTOM>
                               //! [Non-Standard] Raw OPL data

            //! [Non-Standard] Loop Start point with support of multi-loops
            kLoopStackBegin = 0xE4,  // size == 1 <CUSTOM>
                                     //! [Non-Standard] Loop End point with
                                     //! support of multi-loops
            kLoopStackEnd = 0xE5,    // size == 0 <CUSTOM>
                                     //! [Non-Standard] Loop End point with
                                     //! support of multi-loops
            kLoopStackBreak = 0xE6,  // size == 0 <CUSTOM>
                                     //! [Non-Standard] Callback Trigger
            kCallbackTrigger = 0xE7, // size == 1 <CUSTOM>

            // Built-in hooks
            kSongBeginHook = 0x101
        };
        //! Main type of event
        uint_fast16_t type = kUnknown;
        //! Sub-type of the event
        uint_fast16_t sub_type = kUnknown;
        //! Targeted MIDI channel
        uint_fast16_t channel = 0;
        //! Is valid event
        uint_fast16_t is_valid = 1;
        //! Reserved 5 bytes padding
        uint_fast16_t padding[4];
        //! Absolute tick position (Used for the tempo calculation only)
        uint64_t absolute_tick_position = 0;
        //! Raw data of this event
        std::vector<uint8_t> data;
    };

    /**
     * @brief A track position event contains a chain of MIDI events until next
     * delay value
     *
     * Created with purpose to sort events by type in the same position
     * (for example, to keep controllers always first than note on events or
     * lower than note-off events)
     */
    class MidiTrackRow
    {
      public:
        MidiTrackRow();
        //! Clear MIDI row data
        void Clear();
        //! Absolute time position in seconds
        double time_;
        //! Delay to next event in ticks
        uint64_t delay_;
        //! Absolute position in ticks
        uint64_t absolute_position_;
        //! Delay to next event in seconds
        double time_delay_;
        //! List of MIDI events in the current row
        std::vector<MidiEvent> events_;
        /**
         * @brief Sort events in this position
         * @param note_states Buffer of currently pressed/released note keys in
         * the track
         */
        void SortEvents(bool *note_states = nullptr);
    };

    /**
     * @brief Tempo change point entry. Used in the MIDI data building function
     * only.
     */
    struct TempoChangePoint
    {
        uint64_t     absolute_position;
        MidiFraction tempo;
    };

    /**
     * @brief Song position context
     */
    struct Position
    {
        //! Was track began playing
        bool began = false;
        //! Reserved
        char padding[7] = {0, 0, 0, 0, 0, 0, 0};
        //! Waiting time before next event in seconds
        double wait = 0.0;
        //! Absolute time position on the track in seconds
        double absolute_time_position = 0.0;
        //! Track information
        struct TrackInfo
        {
            //! Delay to next event in a track
            uint64_t delay = 0;
            //! Last handled event type
            int32_t lastHandledEvent = 0;
            //! Reserved
            char padding2[4];
            //! MIDI Events queue position iterator
            std::list<MidiTrackRow>::iterator pos;
        };
        std::vector<TrackInfo> track;
    };

    //! MIDI Output interface context
    const MidiRealTimeInterface *midi_output_interface_;

    /**
     * @brief Prepare internal events storage for track data building
     * @param track_count Count of tracks
     */
    void BuildSMFSetupReset(size_t track_count);

    /**
     * @brief Build MIDI track data from the raw track data storage
     * @return true if everything successfully processed, or false on any error
     */
    bool BuildSMFTrackData(const std::vector<std::vector<uint8_t>> &track_data);

    /**
     * @brief Build the time line from off loaded events
     * @param tempos Pre-collected list of tempo events
     * @param loop_start_ticks Global loop start tick (give zero if no global
     * loop presented)
     * @param loop_end_ticks Global loop end tick (give zero if no global loop
     * presented)
     */
    void BuildTimeLine(const std::vector<MidiEvent> &tempos, uint64_t loop_start_ticks = 0,
                       uint64_t loop_end_ticks = 0);

    /**
     * @brief Parse one event from raw MIDI track stream
     * @param [_inout] ptr pointer to pointer to current position on the raw
     * data track
     * @param [_in] end address to end of raw track data, needed to validate
     * position and size
     * @param [_inout] status status of the track processing
     * @return Parsed MIDI event entry
     */
    MidiEvent ParseEvent(const uint8_t **ptr, const uint8_t *end, int &status);

    /**
     * @brief Process MIDI events on the current tick moment
     * @param is_seek is a seeking process
     * @return returns false on reaching end of the song
     */
    bool ProcessEvents(bool is_seek = false);

    /**
     * @brief Handle one event from the chain
     * @param tk MIDI track
     * @param evt MIDI event entry
     * @param status Recent event type, -1 returned when end of track event was
     * handled.
     */
    void handleEvent(size_t tk, const MidiEvent &evt, int32_t &status);

  public:
    /**
     * @brief MIDI marker entry
     */
    struct MidiMarkerEntry
    {
        //! Label
        std::string label;
        //! Position time in seconds
        double position_time;
        //! Position time in MIDI ticks
        uint64_t position_ticks;
    };

    /**
     * @brief Format of loop points implemented by CC events
     */
    enum LoopFormat
    {
        kLoopDefault,
        kLoopRpgMaker = 1,
        kLoopEMidi,
        kLoopHmi
    };

  private:
    //! SMF format identifier.
    unsigned midi_smf_format_;
    //! Loop points format
    LoopFormat midi_loop_format_;

    //! Current position
    Position midi_current_position_;
    //! Track begin position
    Position midi_track_begin_position_;
    //! Loop start point
    Position midi_loop_begin_position_;

    //! Is looping enabled or not
    bool midi_loop_enabled_;
    //! Don't process loop: trigger hooks only if they are set
    bool midi_loop_hooks_only_;

    //! Full song length in seconds
    double midi_full_song_time_length_;
    //! Delay after song playd before rejecting the output stream requests
    double midi_post_song_wait_delay_;

    //! Global loop start time
    double midi_loop_start_time_;
    //! Global loop end time
    double midi_loop_end_time_;

    //! Pre-processed track data storage
    std::vector<std::list<MidiTrackRow>> midi_track_data_;

    //! Title of music
    std::string midi_music_title_;
    //! Copyright notice of music
    std::string midi_music_copyright_;
    //! List of track titles
    std::vector<std::string> midi_music_track_titles_;
    //! List of MIDI markers
    std::vector<MidiMarkerEntry> midi_music_markers_;

    //! Time of one tick
    MidiFraction midi_individual_tick_delta_;
    //! Current tempo
    MidiFraction midi_tempo_;

    //! Tempo multiplier factor
    double midi_tempo_multiplier_;
    //! Is song at end
    bool midi_at_end_;

    //! Set the number of loops limit. Lesser than 0 - loop infinite
    int midi_loop_count_;

    //! The number of track of multi-track file (for exmaple, XMI) to load
    int midi_load_track_number_;

    //! The XMI-specific list of raw songs, converted into SMF format
    std::vector<std::vector<uint8_t>> midi_raw_songs_data_;

    /**
     * @brief Loop stack entry
     */
    struct LoopStackEntry
    {
        //! is infinite loop
        bool infinity = false;
        //! Count of loops left to break. <0 - infinite loop
        int loops = 0;
        //! Start position snapshot to return back
        Position start_position;
        //! Loop start tick
        uint64_t start = 0;
        //! Loop end tick
        uint64_t end = 0;
    };

    class LoopState
    {
      public:
        //! Loop start has reached
        bool caught_start_;
        //! Loop end has reached, reset on handling
        bool caught_end_;

        //! Loop start has reached
        bool caught_stack_start_;
        //! Loop next has reached, reset on handling
        bool caught_stack_end_;
        //! Loop break has reached, reset on handling
        bool caught_stack_break_;
        //! Skip next stack loop start event handling
        bool skip_stack_start_;

        //! Are loop points invalid?
        bool invalid_loop_; /*Loop points are invalid (loopStart after loopEnd
                             or loopStart and loopEnd are on same place)*/

        //! Is look got temporarily broken because of post-end seek?
        bool temporary_broken_;

        //! How much times the loop should start repeat? For example, if you
        //! want to loop song twice, set value 1
        int loops_count_;

        //! how many loops left until finish the song
        int loops_left_;

        //! Stack of nested loops
        std::vector<LoopStackEntry> stack_;
        //! Current level on the loop stack (<0 - out of loop, 0++ - the index
        //! in the loop stack)
        int stack_level_;

        /**
         * @brief Reset loop state to initial
         */
        void Reset()
        {
            caught_start_       = false;
            caught_end_         = false;
            caught_stack_start_ = false;
            caught_stack_end_   = false;
            caught_stack_break_ = false;
            skip_stack_start_   = false;
            loops_left_         = loops_count_;
        }

        void FullReset()
        {
            loops_count_ = -1;
            Reset();
            invalid_loop_     = false;
            temporary_broken_ = false;
            stack_.clear();
            stack_level_ = -1;
        }

        bool IsStackEnd()
        {
            if (caught_stack_end_ && (stack_level_ >= 0) && (stack_level_ < (int)(stack_.size())))
            {
                const LoopStackEntry &e = stack_[(size_t)(stack_level_)];
                if (e.infinity || (!e.infinity && e.loops > 0))
                    return true;
            }
            return false;
        }

        void StackUp(int count = 1)
        {
            stack_level_ += count;
        }

        void StackDown(int count = 1)
        {
            stack_level_ -= count;
        }

        LoopStackEntry &GetCurrentStack()
        {
            if ((stack_level_ >= 0) && (stack_level_ < (int)(stack_.size())))
                return stack_[(size_t)(stack_level_)];
            if (stack_.empty())
            {
                LoopStackEntry d;
                d.loops    = 0;
                d.infinity = 0;
                d.start    = 0;
                d.end      = 0;
                stack_.push_back(d);
            }
            return stack_[0];
        }
    };

    LoopState midi_loop_;

    //! Whether the nth track has playback disabled
    std::vector<bool> midi_track_disabled_;
    //! Index of solo track, or max for disabled
    size_t midi_track_solo_;
    //! MIDI channel disable (exception for extra port-prefix-based channels)
    bool m_channelDisable[16];

    /**
     * @brief Handler of callback trigger events
     * @param userdata Pointer to user data (usually, context of something)
     * @param trigger Value of the event which triggered this callback.
     * @param track Identifier of the track which triggered this callback.
     */
    typedef void (*TriggerHandler)(void *userdata, unsigned trigger, size_t track);

    //! Handler of callback trigger events
    TriggerHandler midi_trigger_handler_;
    //! User data of callback trigger events
    void *midi_trigger_userdata_;

    //! File parsing errors string (adding into midi_error_string_ on aborting
    //! of the process)
    std::string midi_parsing_errors_string_;
    //! Common error string
    std::string midi_error_string_;

    class SequencerTime
    {
      public:
        //! Time buffer
        double time_rest_;
        //! Sample rate
        uint32_t sample_rate_;
        //! Size of one frame in bytes
        uint32_t frame_size_;
        //! Minimum possible delay, granuality
        double minimum_delay_;
        //! Last delay
        double delay_;

        void Init()
        {
            sample_rate_ = 44100;
            frame_size_  = 2;
            Reset();
        }

        void Reset()
        {
            time_rest_     = 0.0;
            minimum_delay_ = 1.0 / (double)(sample_rate_);
            delay_         = 0.0;
        }
    };

    SequencerTime midi_time_;

  public:
    MidiSequencer();
    virtual ~MidiSequencer();

    /**
     * @brief Sets the RT interface
     * @param intrf Pre-Initialized interface structure (pointer will be taken)
     */
    void SetInterface(const MidiRealTimeInterface *intrf);

    /**
     * @brief Runs ticking in a sync with audio streaming. Use this together
     * with onPcmRender hook to easily play MIDI.
     * @param stream pointer to the output PCM stream
     * @param length length of the buffer in bytes
     * @return Count of recorded data in bytes
     */
    int PlayStream(uint8_t *stream, size_t length);

    /**
     * @brief Returns the number of tracks
     * @return Track count
     */
    size_t GetTrackCount() const;

    /**
     * @brief Sets whether a track is playing
     * @param track Track identifier
     * @param enable Whether to enable track playback
     * @return true on success, false if there was no such track
     */
    bool SetTrackEnabled(size_t track, bool enable);

    /**
     * @brief Disable/enable a channel is sounding
     * @param channel Channel number from 0 to 15
     * @param enable Enable the channel playback
     * @return true on success, false if there was no such channel
     */
    bool SetChannelEnabled(size_t channel, bool enable);

    /**
     * @brief Enables or disables solo on a track
     * @param track Identifier of solo track, or max to disable
     */
    void SetSoloTrack(size_t track);

    /**
     * @brief Set the song number of a multi-song file (such as XMI)
     * @param trackNumber Identifier of the song to load (or -1 to mix all songs
     * as one song)
     */
    void SetSongNum(int track);

    /**
     * @brief Retrive the number of songs in a currently opened file
     * @return Number of songs in the file. If 1 or less, means, the file has
     * only one song inside.
     */
    int GetSongsCount();

    /**
     * @brief Defines a handler for callback trigger events
     * @param handler Handler to invoke from the sequencer when triggered, or
     * nullptr.
     * @param userdata Instance of the library
     */
    void SetTriggerHandler(TriggerHandler handler, void *userdata);

    /**
     * @brief Get string that describes reason of error
     * @return Error string
     */
    const std::string &GetErrorString();

    /**
     * @brief Check is loop enabled
     * @return true if loop enabled
     */
    bool GetLoopEnabled();

    /**
     * @brief Switch loop on/off
     * @param enabled Enable loop
     */
    void SetLoopEnabled(bool enabled);

    /**
     * @brief Get the number of loops set
     * @return number of loops or -1 if loop infinite
     */
    int GetLoopsCount();

    /**
     * @brief How many times song should loop
     * @param loops count or -1 to loop infinite
     */
    void SetLoopsCount(int loops);

    /**
     * @brief Switch loop hooks-only mode on/off
     * @param enabled Don't loop: trigger hooks only without loop
     */
    void SetLoopHooksOnly(bool enabled);

    /**
     * @brief Get music title
     * @return music title string
     */
    const std::string &GetMusicTitle();

    /**
     * @brief Get music copyright notice
     * @return music copyright notice string
     */
    const std::string &GetMusicCopyright();

    /**
     * @brief Get list of track titles
     * @return array of track title strings
     */
    const std::vector<std::string> &GetTrackTitles();

    /**
     * @brief Get list of MIDI markers
     * @return Array of MIDI marker structures
     */
    const std::vector<MidiMarkerEntry> &GetMarkers();

    /**
     * @brief Is position of song at end
     * @return true if end of song was reached
     */
    bool PositionAtEnd();

    /**
     * @brief Load MIDI file from a memory block
     * @param data Pointer to memory block with MIDI data
     * @param size Size of source memory block
     * @return true if file successfully opened, false on any error
     */
    bool LoadMidi(const uint8_t *data, size_t size);

    /**
     * @brief Load MIDI file by using FileAndMemReader interface
     * @param mfr mem_file_c with opened source file
     * @return true if file successfully opened, false on any error
     */
    bool LoadMidi(epi::MemFile *mfr);

    /**
     * @brief Periodic tick handler.
     * @param s seconds since last call
     * @param granularity don't expect intervals smaller than this, in seconds
     * @return desired number of seconds until next call
     */
    double Tick(double s, double granularity);

    /**
     * @brief Change current position to specified time position in seconds
     * @param granularity don't expect intervals smaller than this, in seconds
     * @param seconds Absolute time position in seconds
     * @return desired number of seconds until next call of Tick()
     */
    double Seek(double seconds, const double granularity);

    /**
     * @brief Gives current time position in seconds
     * @return Current time position in seconds
     */
    double Tell();

    /**
     * @brief Gives time length of current song in seconds
     * @return Time length of current song in seconds
     */
    double TimeLength();

    /**
     * @brief Gives loop start time position in seconds
     * @return Loop start time position in seconds or -1 if song has no loop
     * points
     */
    double GetLoopStart();

    /**
     * @brief Gives loop end time position in seconds
     * @return Loop end time position in seconds or -1 if song has no loop
     * points
     */
    double GetLoopEnd();

    /**
     * @brief Return to begin of current song
     */
    void Rewind();

    /**
     * @brief Get current tempor multiplier value
     * @return
     */
    double GetTempoMultiplier();

    /**
     * @brief Set tempo multiplier
     * @param tempo Tempo multiplier: 1.0 - original tempo. >1 - faster, <1 -
     * slower
     */
    void SetTempo(double tempo);

  private:
    /**
     * @brief Load file as Standard MIDI file
     * @param mfr mem_file_c with opened source file
     * @return true on successful load
     */
    bool ParseSMF(epi::MemFile *mfr);

    /**
     * @brief Load file as DMX MUS file (Doom)
     * @param mfr mem_file_c with opened source file
     * @return true on successful load
     */
    bool ParseMUS(epi::MemFile *mfr);
};

static constexpr uint8_t kMusFrequency = 140;
static constexpr int     kMusTempo     = 0x00068A1B; /* MPQN: 60000000 / 140BPM (140Hz) = 428571 */
                                                     /*  0x000D1436 -> MPQN: 60000000 /  70BPM  (70Hz) = 857142 */
static constexpr uint16_t kMusDivision = 0x0101;     /* 257 for 140Hz files with a 140MPQN */
                                                     /*  0x0088 -> 136 for  70Hz files with a 140MPQN */
                                                     /*  0x010B -> 267 for  70hz files with a 70MPQN  */
                                                     /*  0x01F9 -> 505 for 140hz files with a 70MPQN  */

/* New
 * QLS: MPQN/1000000 = 0.428571
 * TDPS: QLS/PPQN = 0.428571/136 = 0.003151257
 * PPQN: 136
 *
 * QLS: MPQN/1000000 = 0.428571
 * TDPS: QLS/PPQN = 0.428571/257 = 0.001667591
 * PPQN: 257
 *
 * QLS: MPQN/1000000 = 0.857142
 * TDPS: QLS/PPQN = 0.857142/267 = 0.00321027
 * PPQN: 267
 *
 * QLS: MPQN/1000000 = 0.857142
 * TDPS: QLS/PPQN = 0.857142/505 = 0.001697311
 * PPQN: 505
 *
 * Old
 * QLS: MPQN/1000000 = 1.745673
 * TDPS: QLS/PPQN = 1.745673 / 89 = 0.019614303 (seconds per tick)
 * PPQN: (TDPS = QLS/PPQN) (0.019614303 = 1.745673/PPQN) (0.019614303*PPQN
 * = 1.745673) (PPQN = 89.000001682)
 *
 */

enum MusEvent
{
    kMusEventKeyOff           = 0,
    kMusEventKeyOn            = 1,
    kMusEventPitchWheel       = 2,
    kMusEventChannelMode      = 3,
    kMusEventControllerChange = 4,
    kMusEventEnd              = 6,
};

static constexpr uint8_t kMusMidiMaxChannels = 16;

static constexpr char kMusHeader[] = {'M', 'U', 'S', 0x1A};

static constexpr uint8_t kMusToMidiMap[] = {
    /* MIDI  Number  Description */
    0,    /* 0    program change */
    0,    /* 1    bank selection */
    0x01, /* 2    Modulation pot (frequency vibrato depth) */
    0x07, /* 3    Volume: 0-silent, ~100-normal, 127-loud */
    0x0A, /* 4    Pan (balance) pot: 0-left, 64-center (default), 127-right */
    0x0B, /* 5    Expression pot */
    0x5B, /* 6    Reverb depth */
    0x5D, /* 7    Chorus depth */
    0x40, /* 8    Sustain pedal */
    0x43, /* 9    Soft pedal */
    0x78, /* 10   All sounds off */
    0x7B, /* 11   All notes off */
    0x7E, /* 12   Mono (use numchannels + 1) */
    0x7F, /* 13   Poly */
    0x79, /* 14   reset all controllers */
};

struct MusHeader
{
    char     ID[4];        /* identifier: "MUS" 0x1A */
    uint16_t scoreLen;
    uint16_t scoreStart;
    uint16_t channels;     /* count of primary channels */
    uint16_t sec_channels; /* count of secondary channels */
    uint16_t instrCnt;
};

struct MidiHeaderChunk
{
    char    name[4];
    int32_t length;
    int16_t format;   /* make 0 */
    int16_t ntracks;  /* make 1 */
    int16_t division; /* 0xe250 ?? */
};

struct MidiTrackChunk
{
    char    name[4];
    int32_t length;
};

struct MusConversionContext
{
    uint8_t *src, *src_ptr;
    uint32_t srcsize;
    uint32_t datastart;
    uint8_t *dst, *dst_ptr;
    uint32_t dstsize, dstrem;
};

static constexpr uint16_t kMusDestinationChunkSize = 8192;
static void               MusToMidiResizeDestination(struct MusConversionContext *ctx)
{
    uint32_t pos = (uint32_t)(ctx->dst_ptr - ctx->dst);
    ctx->dst     = (uint8_t *)realloc(ctx->dst, ctx->dstsize + kMusDestinationChunkSize);
    ctx->dstsize += kMusDestinationChunkSize;
    ctx->dstrem += kMusDestinationChunkSize;
    ctx->dst_ptr = ctx->dst + pos;
}

static void MusToMidiWrite1(struct MusConversionContext *ctx, uint32_t val)
{
    if (ctx->dstrem < 1)
        MusToMidiResizeDestination(ctx);
    *ctx->dst_ptr++ = val & 0xff;
    ctx->dstrem--;
}

static void MusToMidiWrite2(struct MusConversionContext *ctx, uint32_t val)
{
    if (ctx->dstrem < 2)
        MusToMidiResizeDestination(ctx);
    *ctx->dst_ptr++ = (val >> 8) & 0xff;
    *ctx->dst_ptr++ = val & 0xff;
    ctx->dstrem -= 2;
}

static void MusToMidiWrite4(struct MusConversionContext *ctx, uint32_t val)
{
    if (ctx->dstrem < 4)
        MusToMidiResizeDestination(ctx);
    *ctx->dst_ptr++ = (uint8_t)((val >> 24) & 0xff);
    *ctx->dst_ptr++ = (uint8_t)((val >> 16) & 0xff);
    *ctx->dst_ptr++ = (uint8_t)((val >> 8) & 0xff);
    *ctx->dst_ptr++ = (uint8_t)((val & 0xff));
    ctx->dstrem -= 4;
}

static void MusToMidiSeekDestination(struct MusConversionContext *ctx, uint32_t pos)
{
    ctx->dst_ptr = ctx->dst + pos;
    while (ctx->dstsize < pos)
        MusToMidiResizeDestination(ctx);
    ctx->dstrem = ctx->dstsize - pos;
}

static void MusToMidiSkipDestination(struct MusConversionContext *ctx, int32_t pos)
{
    size_t newpos;
    ctx->dst_ptr += pos;
    newpos = ctx->dst_ptr - ctx->dst;
    while (ctx->dstsize < newpos)
        MusToMidiResizeDestination(ctx);
    ctx->dstrem = (uint32_t)(ctx->dstsize - newpos);
}

static uint32_t MusToMidiGetDestinationPosition(struct MusConversionContext *ctx)
{
    return (uint32_t)(ctx->dst_ptr - ctx->dst);
}

/* writes a variable length integer to a buffer, and returns bytes written */
static int32_t MusToMidiWriteVariableLength(int32_t value, uint8_t *out)
{
    int32_t buffer, count = 0;

    buffer = value & 0x7f;
    while ((value >>= 7) > 0)
    {
        buffer <<= 8;
        buffer += 0x80;
        buffer += (value & 0x7f);
    }

    while (1)
    {
        ++count;
        *out = (uint8_t)buffer;
        ++out;
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
    return (count);
}

#define EDGE_MUS_READ_SHORT(b) ((b)[0] | ((b)[1] << 8))
#define EDGE_MUS_READ_INT(b)   ((b)[0] | ((b)[1] << 8) | ((b)[2] << 16) | ((b)[3] << 24))

static int ConvertMusToMidi(uint8_t *in, uint32_t insize, uint8_t **out, uint32_t *outsize, uint16_t frequency)
{
    struct MusConversionContext ctx;
    MusHeader                   header;
    uint8_t                    *cur, *end;
    uint32_t                    track_size_pos, begin_track_pos, current_pos;
    int32_t                     delta_time; /* Delta time for midi event */
    int                         temp, ret = -1;
    int                         channel_volume[kMusMidiMaxChannels];
    int                         channelMap[kMusMidiMaxChannels], currentChannel;

    if (insize < sizeof(MusHeader))
    {
        /*_WM_GLOBAL_ERROR(__FUNCTION__, __LINE__, WM_ERR_CORUPT, "(too short)",
         * 0);*/
        return (-1);
    }

    if (!frequency)
        frequency = kMusFrequency;

    /* read the MUS header and set our location */
    memcpy(header.ID, in, 4);
    header.scoreLen     = EDGE_MUS_READ_SHORT(&in[4]);
    header.scoreStart   = EDGE_MUS_READ_SHORT(&in[6]);
    header.channels     = EDGE_MUS_READ_SHORT(&in[8]);
    header.sec_channels = EDGE_MUS_READ_SHORT(&in[10]);
    header.instrCnt     = EDGE_MUS_READ_SHORT(&in[12]);

    if (memcmp(header.ID, kMusHeader, 4))
    {
        /*_WM_GLOBAL_ERROR(__FUNCTION__, __LINE__, WM_ERR_NOT_MUS, nullptr,
         * 0);*/
        return (-1);
    }
    if (insize < (uint32_t)header.scoreLen + (uint32_t)header.scoreStart)
    {
        /*_WM_GLOBAL_ERROR(__FUNCTION__, __LINE__, WM_ERR_CORUPT, "(too short)",
         * 0);*/
        return (-1);
    }
    /* channel #15 should be excluded in the numchannels field: */
    if (header.channels > kMusMidiMaxChannels - 1)
    {
        /*_WM_GLOBAL_ERROR(__FUNCTION__, __LINE__, WM_ERR_INVALID, nullptr,
         * 0);*/
        return (-1);
    }

    memset(&ctx, 0, sizeof(struct MusConversionContext));
    ctx.src = ctx.src_ptr = in;
    ctx.srcsize           = insize;

    ctx.dst     = (uint8_t *)calloc(kMusDestinationChunkSize, sizeof(uint8_t));
    ctx.dst_ptr = ctx.dst;
    ctx.dstsize = kMusDestinationChunkSize;
    ctx.dstrem  = kMusDestinationChunkSize;

    /* Map channel 15 to 9 (percussions) */
    for (temp = 0; temp < kMusMidiMaxChannels; ++temp)
    {
        channelMap[temp]     = -1;
        channel_volume[temp] = 0x40;
    }
    channelMap[15] = 9;

    /* Header is 14 bytes long and add the rest as well */
    MusToMidiWrite1(&ctx, 'M');
    MusToMidiWrite1(&ctx, 'T');
    MusToMidiWrite1(&ctx, 'h');
    MusToMidiWrite1(&ctx, 'd');
    MusToMidiWrite4(&ctx, 6);            /* length of header */
    MusToMidiWrite2(&ctx, 0);            /* MIDI type (always 0) */
    MusToMidiWrite2(&ctx, 1);            /* MUS files only have 1 track */
    MusToMidiWrite2(&ctx, kMusDivision); /* division */

    /* Write out track header and track length position for later */
    begin_track_pos = MusToMidiGetDestinationPosition(&ctx);
    MusToMidiWrite1(&ctx, 'M');
    MusToMidiWrite1(&ctx, 'T');
    MusToMidiWrite1(&ctx, 'r');
    MusToMidiWrite1(&ctx, 'k');
    track_size_pos = MusToMidiGetDestinationPosition(&ctx);
    MusToMidiSkipDestination(&ctx, 4);

    /* write tempo: microseconds per quarter note */
    MusToMidiWrite1(&ctx, 0x00);   /* delta time */
    MusToMidiWrite1(&ctx, 0xff);   /* sys command */
    MusToMidiWrite2(&ctx, 0x5103); /* command - set tempo */
    MusToMidiWrite1(&ctx, kMusTempo & 0x000000ff);
    MusToMidiWrite1(&ctx, (kMusTempo & 0x0000ff00) >> 8);
    MusToMidiWrite1(&ctx, (kMusTempo & 0x00ff0000) >> 16);

    /* Percussions channel starts out at volume 100 */
    MusToMidiWrite1(&ctx, 0x00);
    MusToMidiWrite1(&ctx, 0xB9);
    MusToMidiWrite1(&ctx, 0x07);
    MusToMidiWrite1(&ctx, 100);

    /* get current position in source, and end of position */
    cur = in + header.scoreStart;
    end = cur + header.scoreLen;

    currentChannel = 0;
    delta_time     = 0;

    /* main loop */
    while (cur < end)
    {
        /*printf("LOOP DEBUG: %d\r\n",iterator++);*/
        uint8_t  channel;
        uint8_t  event;
        uint8_t  temp_buffer[32]; /* temp buffer for current iterator */
        uint8_t *out_local = temp_buffer;
        uint8_t  status, bit1, bit2, bitc = 2;

        /* read in current bit */
        event   = *cur++;
        channel = (event & 15); /* current channel */

        /* write variable length delta time */
        out_local += MusToMidiWriteVariableLength(delta_time, out_local);

        /* set all channels to 127 (max) volume */
        if (channelMap[channel] < 0)
        {
            *out_local++        = 0xB0 + currentChannel;
            *out_local++        = 0x07;
            *out_local++        = 100;
            *out_local++        = 0x00;
            channelMap[channel] = currentChannel++;
            if (currentChannel == 9)
                ++currentChannel;
        }
        status = channelMap[channel];

        /* handle events */
        switch ((event & 122) >> 4)
        {
        case kMusEventKeyOff:
            status |= 0x80;
            bit1 = *cur++;
            bit2 = 0x40;
            break;
        case kMusEventKeyOn:
            status |= 0x90;
            bit1 = *cur & 127;
            if (*cur++ & 128) /* volume bit? */
                channel_volume[channelMap[channel]] = *cur++;
            bit2 = channel_volume[channelMap[channel]];
            break;
        case kMusEventPitchWheel:
            status |= 0xE0;
            bit1 = (*cur & 1) >> 6;
            bit2 = (*cur++ >> 1) & 127;
            break;
        case kMusEventChannelMode:
            status |= 0xB0;
            if (*cur >= sizeof(kMusToMidiMap) / sizeof(kMusToMidiMap[0]))
            {
                /*_WM_ERROR_NEW("%s:%i: can't map %u to midi",
                              __FUNCTION__, __LINE__, *cur);*/
                goto _end;
            }
            bit1 = kMusToMidiMap[*cur++];
            bit2 = (*cur++ == 12) ? header.channels + 1 : 0x00;
            break;
        case kMusEventControllerChange:
            if (*cur == 0)
            {
                cur++;
                status |= 0xC0;
                bit1 = *cur++;
                bit2 = 0; /* silence bogus warnings */
                bitc = 1;
            }
            else
            {
                status |= 0xB0;
                if (*cur >= sizeof(kMusToMidiMap) / sizeof(kMusToMidiMap[0]))
                {
                    /*_WM_ERROR_NEW("%s:%i: can't map %u to midi",
                                  __FUNCTION__, __LINE__, *cur);*/
                    goto _end;
                }
                bit1 = kMusToMidiMap[*cur++];
                bit2 = *cur++;
            }
            break;
        case kMusEventEnd: /* End */
            status = 0xff;
            bit1   = 0x2f;
            bit2   = 0x00;
            if (cur != end)
            { /* should we error here or report-only? */
                /*_WM_DEBUG_MSG("%s:%i: MUS buffer off by %ld bytes",
                              __FUNCTION__, __LINE__, (long)(cur - end));*/
            }
            break;
        case 5:  /* Unknown */
        case 7:  /* Unknown */
        default: /* shouldn't happen */
            /*_WM_ERROR_NEW("%s:%i: unrecognized event (%u)",
                          __FUNCTION__, __LINE__, event);*/
            goto _end;
        }

        /* write it out */
        *out_local++ = status;
        *out_local++ = bit1;
        if (bitc == 2)
            *out_local++ = bit2;

        /* write out our temp buffer */
        if (out_local != temp_buffer)
        {
            if (ctx.dstrem < sizeof(temp_buffer))
                MusToMidiResizeDestination(&ctx);

            memcpy(ctx.dst_ptr, temp_buffer, out_local - temp_buffer);
            ctx.dst_ptr += out_local - temp_buffer;
            ctx.dstrem -= (uint32_t)(out_local - temp_buffer);
        }

        if (event & 128)
        {
            delta_time = 0;
            do
            {
                delta_time = (int32_t)((delta_time * 128 + (*cur & 127)) * (140.0 / (double)frequency));
            } while ((*cur++ & 128));
        }
        else
        {
            delta_time = 0;
        }
    }

    /* write out track length */
    current_pos = MusToMidiGetDestinationPosition(&ctx);
    MusToMidiSeekDestination(&ctx, track_size_pos);
    MusToMidiWrite4(&ctx, current_pos - begin_track_pos - sizeof(MidiTrackChunk));
    MusToMidiSeekDestination(&ctx, current_pos); /* reseek to end position */

    *out     = ctx.dst;
    *outsize = ctx.dstsize - ctx.dstrem;
    ret      = 0;

_end: /* cleanup */
    if (ret < 0)
    {
        free(ctx.dst);
        *out     = nullptr;
        *outsize = 0;
    }

    return (ret);
}

/**
 * @brief Utility function to read Big-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t ReadIntBigEndian(const void *buffer, size_t nbytes)
{
    uint64_t       result = 0;
    const uint8_t *data   = (const uint8_t *)(buffer);

    for (size_t n = 0; n < nbytes; ++n)
        result = (result << 8) + data[n];

    return result;
}

/**
 * @brief Utility function to read Little-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted unsigned integer
 */
static inline uint64_t ReadIntLittleEndian(const void *buffer, size_t nbytes)
{
    uint64_t       result = 0;
    const uint8_t *data   = (const uint8_t *)(buffer);

    for (size_t n = 0; n < nbytes; ++n)
        result = result + (uint64_t)(data[n] << (n * 8));

    return result;
}

/**
 * @brief Secure Standard MIDI Variable-Length numeric value parser with
 * anti-out-of-range protection
 * @param [_inout] ptr Pointer to memory block that contains begin of
 * variable-length value, will be iterated forward
 * @param [_in end Pointer to end of memory block where variable-length value is
 * stored (after end of track)
 * @param [_out] ok Reference to boolean which takes result of variable-length
 * value parsing
 * @return Unsigned integer that conains parsed variable-length value
 */
static inline uint64_t ReadVariableLengthValue(const uint8_t **ptr, const uint8_t *end, bool &ok)
{
    uint64_t result = 0;
    ok              = false;

    for (;;)
    {
        if (*ptr >= end)
            return 2;
        unsigned char byte = *((*ptr)++);
        result             = (result << 7) + (byte & 0x7F);
        if (!(byte & 0x80))
            break;
    }

    ok = true;
    return result;
}

MidiSequencer::MidiTrackRow::MidiTrackRow() : time_(0.0), delay_(0), absolute_position_(0), time_delay_(0.0)
{
}

void MidiSequencer::MidiTrackRow::Clear()
{
    time_              = 0.0;
    delay_             = 0;
    absolute_position_ = 0;
    time_delay_        = 0.0;
    events_.clear();
}

void MidiSequencer::MidiTrackRow::SortEvents(bool *note_states)
{
    typedef std::vector<MidiEvent> EvtArr;
    EvtArr                         sysEx;
    EvtArr                         metas;
    EvtArr                         noteOffs;
    EvtArr                         controllers;
    EvtArr                         anyOther;

    for (size_t i = 0; i < events_.size(); i++)
    {
        if (events_[i].type == MidiEvent::kNoteOff)
        {
            if (noteOffs.capacity() == 0)
                noteOffs.reserve(events_.size());
            noteOffs.push_back(events_[i]);
        }
        else if (events_[i].type == MidiEvent::kSysex || events_[i].type == MidiEvent::kSysex2)
        {
            if (sysEx.capacity() == 0)
                sysEx.reserve(events_.size());
            sysEx.push_back(events_[i]);
        }
        else if ((events_[i].type == MidiEvent::kControlChange) || (events_[i].type == MidiEvent::kPatchChange) ||
                 (events_[i].type == MidiEvent::kPitchWheel) || (events_[i].type == MidiEvent::kChannelAftertouch))
        {
            if (controllers.capacity() == 0)
                controllers.reserve(events_.size());
            controllers.push_back(events_[i]);
        }
        else if ((events_[i].type == MidiEvent::kSpecial) &&
                 ((events_[i].sub_type == MidiEvent::kMarker) || (events_[i].sub_type == MidiEvent::kDeviceSwitch) ||
                  (events_[i].sub_type == MidiEvent::kSongBeginHook) ||
                  (events_[i].sub_type == MidiEvent::kLoopStart) || (events_[i].sub_type == MidiEvent::kLoopEnd) ||
                  (events_[i].sub_type == MidiEvent::kLoopStackBegin) ||
                  (events_[i].sub_type == MidiEvent::kLoopStackEnd) ||
                  (events_[i].sub_type == MidiEvent::kLoopStackBreak)))
        {
            if (metas.capacity() == 0)
                metas.reserve(events_.size());
            metas.push_back(events_[i]);
        }
        else
        {
            if (anyOther.capacity() == 0)
                anyOther.reserve(events_.size());
            anyOther.push_back(events_[i]);
        }
    }

    /*
     * If Note-Off and it's Note-On is on the same row - move this damned note
     * off down!
     */
    if (note_states)
    {
        std::set<size_t> markAsOn;
        for (size_t i = 0; i < anyOther.size(); i++)
        {
            const MidiEvent e = anyOther[i];
            if (e.type == MidiEvent::kNoteOn)
            {
                const size_t note_i = (size_t)(e.channel * 255) + (e.data[0] & 0x7F);
                // Check, was previously note is on or off
                bool wasOn = note_states[note_i];
                markAsOn.insert(note_i);
                // Detect zero-length notes are following previously pressed
                // note
                int noteOffsOnSameNote = 0;
                for (EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end();)
                {
                    // If note was off, and note-off on same row with note-on -
                    // move it down!
                    if (((*j).channel == e.channel) && ((*j).data[0] == e.data[0]))
                    {
                        // If note is already off OR more than one note-off on
                        // same row and same note
                        if (!wasOn || (noteOffsOnSameNote != 0))
                        {
                            anyOther.push_back(*j);
                            j = noteOffs.erase(j);
                            markAsOn.erase(note_i);
                            continue;
                        }
                        else
                        {
                            // When same row has many note-offs on same row
                            // that means a zero-length note follows previous
                            // note it must be shuted down
                            noteOffsOnSameNote++;
                        }
                    }
                    j++;
                }
            }
        }

        // Mark other notes as released
        for (EvtArr::iterator j = noteOffs.begin(); j != noteOffs.end(); j++)
        {
            size_t note_i       = (size_t)(j->channel * 255) + (j->data[0] & 0x7F);
            note_states[note_i] = false;
        }

        for (std::set<size_t>::iterator j = markAsOn.begin(); j != markAsOn.end(); j++)
            note_states[*j] = true;
    }
    /***********************************************************************************/

    events_.clear();
    if (!sysEx.empty())
        events_.insert(events_.end(), sysEx.begin(), sysEx.end());
    if (!noteOffs.empty())
        events_.insert(events_.end(), noteOffs.begin(), noteOffs.end());
    if (!metas.empty())
        events_.insert(events_.end(), metas.begin(), metas.end());
    if (!controllers.empty())
        events_.insert(events_.end(), controllers.begin(), controllers.end());
    if (!anyOther.empty())
        events_.insert(events_.end(), anyOther.begin(), anyOther.end());
}

MidiSequencer::MidiSequencer()
    : midi_output_interface_(nullptr), midi_smf_format_(0), midi_loop_format_(kLoopDefault),
      midi_loop_enabled_(false), midi_loop_hooks_only_(false), midi_full_song_time_length_(0.0),
      midi_post_song_wait_delay_(1.0), midi_loop_start_time_(-1.0), midi_loop_end_time_(-1.0),
      midi_tempo_multiplier_(1.0), midi_at_end_(false), midi_loop_count_(-1), midi_load_track_number_(0),
      midi_track_solo_(~(size_t)(0)), midi_trigger_handler_(nullptr), midi_trigger_userdata_(nullptr)
{
    midi_loop_.Reset();
    midi_loop_.invalid_loop_ = false;
    midi_time_.Init();
}

MidiSequencer::~MidiSequencer()
{
}

void MidiSequencer::SetInterface(const MidiRealTimeInterface *intrf)
{
    // Interface must NOT be nullptr
    EPI_ASSERT(intrf);

    // Note ON hook is REQUIRED
    EPI_ASSERT(intrf->rt_noteOn);
    // Note OFF hook is REQUIRED
    EPI_ASSERT(intrf->rt_noteOff || intrf->rt_noteOffVel);
    // Note Aftertouch hook is REQUIRED
    EPI_ASSERT(intrf->rt_noteAfterTouch);
    // Channel Aftertouch hook is REQUIRED
    EPI_ASSERT(intrf->rt_channelAfterTouch);
    // Controller change hook is REQUIRED
    EPI_ASSERT(intrf->rt_controllerChange);
    // Patch change hook is REQUIRED
    EPI_ASSERT(intrf->rt_patchChange);
    // Pitch bend hook is REQUIRED
    EPI_ASSERT(intrf->rt_pitchBend);
    // System Exclusive hook is REQUIRED
    EPI_ASSERT(intrf->rt_systemExclusive);

    if (intrf->pcmSampleRate != 0 && intrf->pcmFrameSize != 0)
    {
        midi_time_.sample_rate_ = intrf->pcmSampleRate;
        midi_time_.frame_size_  = intrf->pcmFrameSize;
        midi_time_.Reset();
    }

    midi_output_interface_ = intrf;
}

int MidiSequencer::PlayStream(uint8_t *stream, size_t length)
{
    int      count      = 0;
    size_t   samples    = (size_t)(length / (size_t)(midi_time_.frame_size_));
    size_t   left       = samples;
    size_t   periodSize = 0;
    uint8_t *stream_pos = stream;

    EPI_ASSERT(midi_output_interface_->onPcmRender);

    while (left > 0)
    {
        const double leftDelay = left / double(midi_time_.sample_rate_);
        const double maxDelay  = midi_time_.time_rest_ < leftDelay ? midi_time_.time_rest_ : leftDelay;
        if ((PositionAtEnd()) && (midi_time_.delay_ <= 0.0))
            break; // Stop to fetch samples at reaching the song end with
                   // disabled loop

        midi_time_.time_rest_ -= maxDelay;
        periodSize = (size_t)((double)(midi_time_.sample_rate_) * maxDelay);

        if (stream)
        {
            size_t generateSize = periodSize > left ? (size_t)(left) : (size_t)(periodSize);
            midi_output_interface_->onPcmRender(midi_output_interface_->onPcmRender_userdata, stream_pos,
                                                generateSize * midi_time_.frame_size_);
            stream_pos += generateSize * midi_time_.frame_size_;
            count += generateSize;
            left -= generateSize;
            EPI_ASSERT(left <= samples);
        }

        if (midi_time_.time_rest_ <= 0.0)
        {
            midi_time_.delay_ = Tick(midi_time_.delay_, midi_time_.minimum_delay_);
            midi_time_.time_rest_ += midi_time_.delay_;
        }
    }

    return count * (int)(midi_time_.frame_size_);
}

size_t MidiSequencer::GetTrackCount() const
{
    return midi_track_data_.size();
}

bool MidiSequencer::SetTrackEnabled(size_t track, bool enable)
{
    size_t track_count = midi_track_data_.size();
    if (track >= track_count)
        return false;
    midi_track_disabled_[track] = !enable;
    return true;
}

bool MidiSequencer::SetChannelEnabled(size_t channel, bool enable)
{
    if (channel >= 16)
        return false;

    if (!enable && m_channelDisable[channel] != !enable)
    {
        uint8_t ch = (uint8_t)(channel);

        // Releae all pedals
        midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, ch, 64, 0);
        midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, ch, 66, 0);

        // Release all notes on the channel now
        for (int i = 0; i < 127; ++i)
        {
            if (midi_output_interface_->rt_noteOff)
                midi_output_interface_->rt_noteOff(midi_output_interface_->rtUserData, ch, i);
            if (midi_output_interface_->rt_noteOffVel)
                midi_output_interface_->rt_noteOffVel(midi_output_interface_->rtUserData, ch, i, 0);
        }
    }

    m_channelDisable[channel] = !enable;
    return true;
}

void MidiSequencer::SetSoloTrack(size_t track)
{
    midi_track_solo_ = track;
}

void MidiSequencer::SetSongNum(int track)
{
    midi_load_track_number_ = track;
}

int MidiSequencer::GetSongsCount()
{
    return (int)midi_raw_songs_data_.size();
}

void MidiSequencer::SetTriggerHandler(TriggerHandler handler, void *userdata)
{
    midi_trigger_handler_  = handler;
    midi_trigger_userdata_ = userdata;
}

const std::string &MidiSequencer::GetErrorString()
{
    return midi_error_string_;
}

bool MidiSequencer::GetLoopEnabled()
{
    return midi_loop_enabled_;
}

void MidiSequencer::SetLoopEnabled(bool enabled)
{
    midi_loop_enabled_ = enabled;
}

int MidiSequencer::GetLoopsCount()
{
    return midi_loop_count_ >= 0 ? (midi_loop_count_ + 1) : midi_loop_count_;
}

void MidiSequencer::SetLoopsCount(int loops)
{
    if (loops >= 1)
        loops -= 1; // Internally, loops count has the 0 base
    midi_loop_count_ = loops;
}

void MidiSequencer::SetLoopHooksOnly(bool enabled)
{
    midi_loop_hooks_only_ = enabled;
}

const std::string &MidiSequencer::GetMusicTitle()
{
    return midi_music_title_;
}

const std::string &MidiSequencer::GetMusicCopyright()
{
    return midi_music_copyright_;
}

const std::vector<std::string> &MidiSequencer::GetTrackTitles()
{
    return midi_music_track_titles_;
}

const std::vector<MidiSequencer::MidiMarkerEntry> &MidiSequencer::GetMarkers()
{
    return midi_music_markers_;
}

bool MidiSequencer::PositionAtEnd()
{
    return midi_at_end_;
}

double MidiSequencer::GetTempoMultiplier()
{
    return midi_tempo_multiplier_;
}

void MidiSequencer::BuildSMFSetupReset(size_t track_count)
{
    midi_full_song_time_length_ = 0.0;
    midi_loop_start_time_       = -1.0;
    midi_loop_end_time_         = -1.0;
    midi_loop_format_           = kLoopDefault;
    midi_track_disabled_.clear();
    memset(m_channelDisable, 0, sizeof(m_channelDisable));
    midi_track_solo_ = ~(size_t)0;
    midi_music_title_.clear();
    midi_music_copyright_.clear();
    midi_music_track_titles_.clear();
    midi_music_markers_.clear();
    midi_track_data_.clear();
    midi_track_data_.resize(track_count, std::list<MidiTrackRow>());
    midi_track_disabled_.resize(track_count);

    midi_loop_.Reset();
    midi_loop_.invalid_loop_ = false;
    midi_time_.Reset();

    midi_current_position_.began                  = false;
    midi_current_position_.absolute_time_position = 0.0;
    midi_current_position_.wait                   = 0.0;
    midi_current_position_.track.clear();
    midi_current_position_.track.resize(track_count);
}

bool MidiSequencer::BuildSMFTrackData(const std::vector<std::vector<uint8_t>> &track_data)
{
    const size_t track_count = track_data.size();
    BuildSMFSetupReset(track_count);

    bool gotGlobalLoopStart = false, gotGlobalLoopEnd = false, gotStackLoopStart = false, gotLoopEventInThisRow = false;

    //! Tick position of loop start tag
    uint64_t loop_start_ticks = 0;
    //! Tick position of loop end tag
    uint64_t loop_end_ticks = 0;
    //! Full length of song in ticks
    uint64_t ticksSongLength = 0;
    //! Cache for error message strign
    char error[150];

    //! Caches note on/off states.
    bool note_states[16 * 255];
    /* This is required to carefully detect zero-length notes           *
     * and avoid a move of "note-off" event over "note-on" while sort.  *
     * Otherwise, after sort those notes will play infinite sound       */

    //! Tempo change events list
    std::vector<MidiEvent> temposList;

    /*
     * TODO: Make this be safer for memory in case of broken input data
     * which may cause going away of available track data (and then give a
     * crash!)
     *
     * POST: Check this more carefully for possible vulnuabilities are can crash
     * this
     */
    for (size_t tk = 0; tk < track_count; ++tk)
    {
        uint64_t       abs_position = 0;
        int            status       = 0;
        MidiEvent      event;
        bool           ok       = false;
        const uint8_t *end      = track_data[tk].data() + track_data[tk].size();
        const uint8_t *trackPtr = track_data[tk].data();
        memset(note_states, 0, sizeof(note_states));

        // Time delay that follows the first event in the track
        {
            MidiTrackRow evtPos;
            evtPos.delay_ = ReadVariableLengthValue(&trackPtr, end, ok);
            if (!ok)
            {
                int len = snprintf(error, 150,
                                   "buildTrackData: Can't read variable-length "
                                   "value at begin of track %d.\n",
                                   (int)tk);
                if ((len > 0) && (len < 150))
                    midi_parsing_errors_string_ += std::string(error, (size_t)len);
                return false;
            }

            // HACK: Begin every track with "Reset all controllers" event to
            // avoid controllers state break came from end of song
            if (tk == 0)
            {
                MidiEvent resetEvent;
                resetEvent.type     = MidiEvent::kSpecial;
                resetEvent.sub_type = MidiEvent::kSongBeginHook;
                evtPos.events_.push_back(resetEvent);
            }

            evtPos.absolute_position_ = abs_position;
            abs_position += evtPos.delay_;
            midi_track_data_[tk].push_back(evtPos);
        }

        MidiTrackRow evtPos;
        do
        {
            event = ParseEvent(&trackPtr, end, status);
            if (!event.is_valid)
            {
                int len = snprintf(error, 150, "buildTrackData: Fail to parse event in the track %d.\n", (int)tk);
                if ((len > 0) && (len < 150))
                    midi_parsing_errors_string_ += std::string(error, (size_t)len);
                return false;
            }

            evtPos.events_.push_back(event);
            if (event.type == MidiEvent::kSpecial)
            {
                if (event.sub_type == MidiEvent::kTempoChange)
                {
                    event.absolute_tick_position = abs_position;
                    temposList.push_back(event);
                }
                else if (!midi_loop_.invalid_loop_ && (event.sub_type == MidiEvent::kLoopStart))
                {
                    /*
                     * loopStart is invalid when:
                     * - starts together with loopEnd
                     * - appears more than one time in same MIDI file
                     */
                    if (gotGlobalLoopStart || gotLoopEventInThisRow)
                        midi_loop_.invalid_loop_ = true;
                    else
                    {
                        gotGlobalLoopStart = true;
                        loop_start_ticks   = abs_position;
                    }
                    // In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if (!midi_loop_.invalid_loop_ && (event.sub_type == MidiEvent::kLoopEnd))
                {
                    /*
                     * loopEnd is invalid when:
                     * - starts before loopStart
                     * - starts together with loopStart
                     * - appars more than one time in same MIDI file
                     */
                    if (gotGlobalLoopEnd || gotLoopEventInThisRow)
                    {
                        midi_loop_.invalid_loop_ = true;
                        if (midi_output_interface_->onDebugMessage)
                        {
                            midi_output_interface_->onDebugMessage(
                                midi_output_interface_->onDebugMessage_userdata, "== Invalid loop detected! %s %s ==",
                                (gotGlobalLoopEnd ? "[Caught more than 1 loopEnd!]" : ""),
                                (gotLoopEventInThisRow ? "[loopEnd in same row as loopStart!]" : ""));
                        }
                    }
                    else
                    {
                        gotGlobalLoopEnd = true;
                        loop_end_ticks   = abs_position;
                    }
                    // In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if (!midi_loop_.invalid_loop_ && (event.sub_type == MidiEvent::kLoopStackBegin))
                {
                    if (!gotStackLoopStart)
                    {
                        if (!gotGlobalLoopStart)
                            loop_start_ticks = abs_position;
                        gotStackLoopStart = true;
                    }

                    midi_loop_.StackUp();
                    if (midi_loop_.stack_level_ >= (int)(midi_loop_.stack_.size()))
                    {
                        LoopStackEntry e;
                        e.loops    = event.data[0];
                        e.infinity = (event.data[0] == 0);
                        e.start    = abs_position;
                        e.end      = abs_position;
                        midi_loop_.stack_.push_back(e);
                    }
                }
                else if (!midi_loop_.invalid_loop_ && ((event.sub_type == MidiEvent::kLoopStackEnd) ||
                                                       (event.sub_type == MidiEvent::kLoopStackBreak)))
                {
                    if (midi_loop_.stack_level_ <= -1)
                    {
                        midi_loop_.invalid_loop_ = true; // Caught loop end without of loop start!
                        if (midi_output_interface_->onDebugMessage)
                        {
                            midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                                   "== Invalid loop detected! [Caught loop end "
                                                                   "without of loop start] ==");
                        }
                    }
                    else
                    {
                        if (loop_end_ticks < abs_position)
                            loop_end_ticks = abs_position;
                        midi_loop_.GetCurrentStack().end = abs_position;
                        midi_loop_.StackDown();
                    }
                }
            }

            if (event.sub_type != MidiEvent::kEndTrack) // Don't try to read delta after
                                                        // EndOfTrack event!
            {
                evtPos.delay_ = ReadVariableLengthValue(&trackPtr, end, ok);
                if (!ok)
                {
                    /* End of track has been reached! However, there is no EOT
                     * event presented */
                    event.type     = MidiEvent::kSpecial;
                    event.sub_type = MidiEvent::kEndTrack;
                }
            }

            if ((evtPos.delay_ > 0) || (event.sub_type == MidiEvent::kEndTrack))
            {
                evtPos.absolute_position_ = abs_position;
                abs_position += evtPos.delay_;
                evtPos.SortEvents(note_states);
                midi_track_data_[tk].push_back(evtPos);
                evtPos.Clear();
                gotLoopEventInThisRow = false;
            }
        } while ((trackPtr <= end) && (event.sub_type != MidiEvent::kEndTrack));

        if (ticksSongLength < abs_position)
            ticksSongLength = abs_position;
        // Set the chain of events begin
        if (midi_track_data_[tk].size() > 0)
            midi_current_position_.track[tk].pos = midi_track_data_[tk].begin();
    }

    if (gotGlobalLoopStart && !gotGlobalLoopEnd)
    {
        gotGlobalLoopEnd = true;
        loop_end_ticks   = ticksSongLength;
    }

    // loopStart must be located before loopEnd!
    if (loop_start_ticks >= loop_end_ticks)
    {
        midi_loop_.invalid_loop_ = true;
        if (midi_output_interface_->onDebugMessage && (gotGlobalLoopStart || gotGlobalLoopEnd))
        {
            midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                   "== Invalid loop detected! [loopEnd is "
                                                   "going before loopStart] ==");
        }
    }

    BuildTimeLine(temposList, loop_start_ticks, loop_end_ticks);

    return true;
}

void MidiSequencer::BuildTimeLine(const std::vector<MidiEvent> &tempos, uint64_t loop_start_ticks,
                                  uint64_t loop_end_ticks)
{
    const size_t track_count = midi_track_data_.size();
    /********************************************************************************/
    // Calculate time basing on collected tempo events
    /********************************************************************************/
    for (size_t tk = 0; tk < track_count; ++tk)
    {
        MidiFraction             currentTempo       = midi_tempo_;
        double                   time               = 0.0;
        size_t                   tempo_change_index = 0;
        std::list<MidiTrackRow> &track              = midi_track_data_[tk];
        if (track.empty())
            continue;                                // Empty track is useless!

        MidiTrackRow *posPrev = &(*(track.begin())); // First element
        for (std::list<MidiTrackRow>::iterator it = track.begin(); it != track.end(); it++)
        {
            MidiTrackRow &pos = *it;
            if ((posPrev != &pos) && // Skip first event
                (!tempos.empty()) && // Only when in-track tempo events are
                                     // available
                (tempo_change_index < tempos.size()))
            {
                // If tempo event is going between of current and previous event
                if (tempos[tempo_change_index].absolute_tick_position <= pos.absolute_position_)
                {
                    // Stop points: begin point and tempo change points are
                    // before end point
                    std::vector<TempoChangePoint> points;
                    MidiFraction                  t;
                    TempoChangePoint              firstPoint = {posPrev->absolute_position_, currentTempo};
                    points.push_back(firstPoint);

                    // Collect tempo change points between previous and current
                    // events
                    do
                    {
                        TempoChangePoint tempoMarker;
                        const MidiEvent &tempoPoint   = tempos[tempo_change_index];
                        tempoMarker.absolute_position = tempoPoint.absolute_tick_position;
                        tempoMarker.tempo =
                            midi_individual_tick_delta_ *
                            MidiFraction(ReadIntBigEndian(tempoPoint.data.data(), tempoPoint.data.size()));
                        points.push_back(tempoMarker);
                        tempo_change_index++;
                    } while ((tempo_change_index < tempos.size()) &&
                             (tempos[tempo_change_index].absolute_tick_position <= pos.absolute_position_));

                    // Re-calculate time delay of previous event
                    time -= posPrev->time_delay_;
                    posPrev->time_delay_ = 0.0;

                    for (size_t i = 0, j = 1; j < points.size(); i++, j++)
                    {
                        /* If one or more tempo events are appears between of
                         * two events, calculate delays between each tempo
                         * point, begin and end */
                        uint64_t midDelay = 0;
                        // Delay between points
                        midDelay = points[j].absolute_position - points[i].absolute_position;
                        // Time delay between points
                        t = midDelay * currentTempo;
                        posPrev->time_delay_ += t.Value();

                        // Apply next tempo
                        currentTempo = points[j].tempo;
                    }
                    // Then calculate time between last tempo change point and
                    // end point
                    TempoChangePoint tailTempo = points.back();
                    uint64_t         postDelay = pos.absolute_position_ - tailTempo.absolute_position;
                    t                          = postDelay * currentTempo;
                    posPrev->time_delay_ += t.Value();

                    // Store Common time delay
                    posPrev->time_ = time;
                    time += posPrev->time_delay_;
                }
            }

            MidiFraction t  = pos.delay_ * currentTempo;
            pos.time_delay_ = t.Value();
            pos.time_       = time;
            time += pos.time_delay_;

            // Capture markers after time value calculation
            for (size_t i = 0; i < pos.events_.size(); i++)
            {
                MidiEvent &e = pos.events_[i];
                if ((e.type == MidiEvent::kSpecial) && (e.sub_type == MidiEvent::kMarker))
                {
                    MidiMarkerEntry marker;
                    marker.label          = std::string((char *)e.data.data(), e.data.size());
                    marker.position_ticks = pos.absolute_position_;
                    marker.position_time  = pos.time_;
                    midi_music_markers_.push_back(marker);
                }
            }

            // Capture loop points time positions
            if (!midi_loop_.invalid_loop_)
            {
                // Set loop points times
                if (loop_start_ticks == pos.absolute_position_)
                    midi_loop_start_time_ = pos.time_;
                else if (loop_end_ticks == pos.absolute_position_)
                    midi_loop_end_time_ = pos.time_;
            }
            posPrev = &pos;
        }

        if (time > midi_full_song_time_length_)
            midi_full_song_time_length_ = time;
    }

    midi_full_song_time_length_ += midi_post_song_wait_delay_;
    // Set begin of the music
    midi_track_begin_position_ = midi_current_position_;
    // Initial loop position will begin at begin of track until passing of the
    // loop point
    midi_loop_begin_position_ = midi_current_position_;
    // Set lowest level of the loop stack
    midi_loop_.stack_level_ = -1;

    // Set the count of loops
    midi_loop_.loops_count_ = midi_loop_count_;
    midi_loop_.loops_left_  = midi_loop_count_;

    /********************************************************************************/
    // Find and set proper loop points
    /********************************************************************************/
    if (!midi_loop_.invalid_loop_ && !midi_current_position_.track.empty())
    {
        unsigned     caughLoopStart = 0;
        bool         scanDone       = false;
        const size_t ctrack_count   = midi_current_position_.track.size();
        Position     rowPosition(midi_current_position_);

        while (!scanDone)
        {
            const Position rowBeginPosition(rowPosition);

            for (size_t tk = 0; tk < ctrack_count; ++tk)
            {
                Position::TrackInfo &track = rowPosition.track[tk];
                if ((track.lastHandledEvent >= 0) && (track.delay <= 0))
                {
                    // Check is an end of track has been reached
                    if (track.pos == midi_track_data_[tk].end())
                    {
                        track.lastHandledEvent = -1;
                        continue;
                    }

                    for (size_t i = 0; i < track.pos->events_.size(); i++)
                    {
                        const MidiEvent &evt = track.pos->events_[i];
                        if (evt.type == MidiEvent::kSpecial && evt.sub_type == MidiEvent::kLoopStart)
                        {
                            caughLoopStart++;
                            scanDone = true;
                            break;
                        }
                    }

                    if (track.lastHandledEvent >= 0)
                    {
                        track.delay += track.pos->delay_;
                        track.pos++;
                    }
                }
            }

            // Find a shortest delay from all track
            uint64_t shortestDelay         = 0;
            bool     shortestDelayNotFound = true;

            for (size_t tk = 0; tk < ctrack_count; ++tk)
            {
                Position::TrackInfo &track = rowPosition.track[tk];
                if ((track.lastHandledEvent >= 0) && (shortestDelayNotFound || track.delay < shortestDelay))
                {
                    shortestDelay         = track.delay;
                    shortestDelayNotFound = false;
                }
            }

            // Schedule the next playevent to be processed after that delay
            for (size_t tk = 0; tk < ctrack_count; ++tk)
                rowPosition.track[tk].delay -= shortestDelay;

            if (caughLoopStart > 0)
            {
                midi_loop_begin_position_                        = rowBeginPosition;
                midi_loop_begin_position_.absolute_time_position = midi_loop_start_time_;
                scanDone                                         = true;
            }

            if (shortestDelayNotFound)
                break;
        }
    }
}

bool MidiSequencer::ProcessEvents(bool is_seek)
{
    if (midi_current_position_.track.size() == 0)
        midi_at_end_ = true; // No MIDI track data to play
    if (midi_at_end_)
        return false;        // No more events in the queue

    midi_loop_.caught_end_     = false;
    const size_t   track_count = midi_current_position_.track.size();
    const Position rowBeginPosition(midi_current_position_);
    bool           doLoopJump             = false;
    unsigned       caughLoopStart         = 0;
    unsigned       caughLoopStackStart    = 0;
    unsigned       caughLoopStackEnds     = 0;
    double         caughLoopStackEndsTime = 0.0;
    unsigned       caughLoopStackBreaks   = 0;

    for (size_t tk = 0; tk < track_count; ++tk)
    {
        Position::TrackInfo &track = midi_current_position_.track[tk];
        if ((track.lastHandledEvent >= 0) && (track.delay <= 0))
        {
            // Check is an end of track has been reached
            if (track.pos == midi_track_data_[tk].end())
            {
                track.lastHandledEvent = -1;
                break;
            }

            // Handle event
            for (size_t i = 0; i < track.pos->events_.size(); i++)
            {
                const MidiEvent &evt = track.pos->events_[i];

                if (is_seek && (evt.type == MidiEvent::kNoteOn))
                    continue;
                handleEvent(tk, evt, track.lastHandledEvent);

                if (midi_loop_.caught_start_)
                {
                    if (midi_output_interface_->onloopStart) // Loop Start hook
                        midi_output_interface_->onloopStart(midi_output_interface_->onloopStart_userdata);

                    caughLoopStart++;
                    midi_loop_.caught_start_ = false;
                }

                if (midi_loop_.caught_stack_start_)
                {
                    if (midi_output_interface_->onloopStart &&
                        (midi_loop_start_time_ >= track.pos->time_)) // Loop Start hook
                        midi_output_interface_->onloopStart(midi_output_interface_->onloopStart_userdata);

                    caughLoopStackStart++;
                    midi_loop_.caught_stack_start_ = false;
                }

                if (midi_loop_.caught_stack_break_)
                {
                    caughLoopStackBreaks++;
                    midi_loop_.caught_stack_break_ = false;
                }

                if (midi_loop_.caught_end_ || midi_loop_.IsStackEnd())
                {
                    if (midi_loop_.caught_stack_end_)
                    {
                        midi_loop_.caught_stack_end_ = false;
                        caughLoopStackEnds++;
                        caughLoopStackEndsTime = track.pos->time_;
                    }
                    doLoopJump = true;
                    break; // Stop event handling on catching loopEnd event!
                }
            }

            // Read next event time (unless the track just ended)
            if (track.lastHandledEvent >= 0)
            {
                track.delay += track.pos->delay_;
                track.pos++;
            }

            if (doLoopJump)
                break;
        }
    }

    // Find a shortest delay from all track
    uint64_t shortestDelay         = 0;
    bool     shortestDelayNotFound = true;

    for (size_t tk = 0; tk < track_count; ++tk)
    {
        Position::TrackInfo &track = midi_current_position_.track[tk];
        if ((track.lastHandledEvent >= 0) && (shortestDelayNotFound || track.delay < shortestDelay))
        {
            shortestDelay         = track.delay;
            shortestDelayNotFound = false;
        }
    }

    // Schedule the next playevent to be processed after that delay
    for (size_t tk = 0; tk < track_count; ++tk)
        midi_current_position_.track[tk].delay -= shortestDelay;

    MidiFraction t = shortestDelay * midi_tempo_;

    midi_current_position_.wait += t.Value();

    if (caughLoopStart > 0 && midi_loop_begin_position_.absolute_time_position <= 0.0)
        midi_loop_begin_position_ = rowBeginPosition;

    if (caughLoopStackStart > 0)
    {
        while (caughLoopStackStart > 0)
        {
            midi_loop_.StackUp();
            LoopStackEntry &s = midi_loop_.GetCurrentStack();
            s.start_position  = rowBeginPosition;
            caughLoopStackStart--;
        }
        return true;
    }

    if (caughLoopStackBreaks > 0)
    {
        while (caughLoopStackBreaks > 0)
        {
            LoopStackEntry &s = midi_loop_.GetCurrentStack();
            s.loops           = 0;
            s.infinity        = false;
            // Quit the loop
            midi_loop_.StackDown();
            caughLoopStackBreaks--;
        }
    }

    if (caughLoopStackEnds > 0)
    {
        while (caughLoopStackEnds > 0)
        {
            LoopStackEntry &s = midi_loop_.GetCurrentStack();
            if (s.infinity)
            {
                if (midi_output_interface_->onloopEnd &&
                    (midi_loop_end_time_ >= caughLoopStackEndsTime))               // Loop End hook
                {
                    midi_output_interface_->onloopEnd(midi_output_interface_->onloopEnd_userdata);
                    if (midi_loop_hooks_only_)                                     // Stop song on reaching loop
                                                                                   // end
                    {
                        midi_at_end_ = true;                                       // Don't handle events anymore
                        midi_current_position_.wait += midi_post_song_wait_delay_; // One second delay
                                                                                   // until stop playing
                    }
                }

                midi_current_position_       = s.start_position;
                midi_loop_.skip_stack_start_ = true;

                for (uint8_t i = 0; i < 16; i++)
                    midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, i, 123, 0);

                return true;
            }
            else if (s.loops >= 0)
            {
                s.loops--;
                if (s.loops > 0)
                {
                    midi_current_position_       = s.start_position;
                    midi_loop_.skip_stack_start_ = true;

                    for (uint8_t i = 0; i < 16; i++)
                        midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, i, 123, 0);

                    return true;
                }
                else
                {
                    // Quit the loop
                    midi_loop_.StackDown();
                }
            }
            else
            {
                // Quit the loop
                midi_loop_.StackDown();
            }
            caughLoopStackEnds--;
        }

        return true;
    }

    if (shortestDelayNotFound || midi_loop_.caught_end_)
    {
        if (midi_output_interface_->onloopEnd) // Loop End hook
            midi_output_interface_->onloopEnd(midi_output_interface_->onloopEnd_userdata);

        for (uint8_t i = 0; i < 16; i++)
            midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, i, 123, 0);

        // Loop if song end or loop end point has reached
        midi_loop_.caught_end_ = false;
        shortestDelay          = 0;

        if (!midi_loop_enabled_ ||
            (shortestDelayNotFound && midi_loop_.loops_count_ >= 0 && midi_loop_.loops_left_ < 1) ||
            midi_loop_hooks_only_)
        {
            midi_at_end_ = true;                                       // Don't handle events anymore
            midi_current_position_.wait += midi_post_song_wait_delay_; // One second delay until stop
                                                                       // playing
            return true;                                               // We have caugh end here!
        }

        if (midi_loop_.temporary_broken_)
        {
            midi_current_position_       = midi_track_begin_position_;
            midi_loop_.temporary_broken_ = false;
        }
        else if (midi_loop_.loops_count_ < 0 || midi_loop_.loops_left_ >= 1)
        {
            midi_current_position_ = midi_loop_begin_position_;
            if (midi_loop_.loops_count_ >= 1)
                midi_loop_.loops_left_--;
        }
    }

    return true; // Has events in queue
}

MidiSequencer::MidiEvent MidiSequencer::ParseEvent(const uint8_t **pptr, const uint8_t *end, int &status)
{
    const uint8_t          *&ptr = *pptr;
    MidiSequencer::MidiEvent evt;

    if (ptr + 1 > end)
    {
        // When track doesn't ends on the middle of event data, it's must be
        // fine
        evt.type     = MidiEvent::kSpecial;
        evt.sub_type = MidiEvent::kEndTrack;
        return evt;
    }

    unsigned char byte = *(ptr++);
    bool          ok   = false;

    if (byte == MidiEvent::kSysex || byte == MidiEvent::kSysex2) // Ignore SysEx
    {
        uint64_t length = ReadVariableLengthValue(pptr, end, ok);
        if (!ok || (ptr + length > end))
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read SysEx event - Unexpected end of track "
                                           "data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.type = MidiEvent::kSysex;
        evt.data.clear();
        evt.data.push_back(byte);
        std::copy(ptr, ptr + length, std::back_inserter(evt.data));
        ptr += (size_t)length;
        return evt;
    }

    if (byte == MidiEvent::kSpecial)
    {
        // Special event FF
        uint8_t  evtype = *(ptr++);
        uint64_t length = ReadVariableLengthValue(pptr, end, ok);
        if (!ok || (ptr + length > end))
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read Special event - Unexpected end of "
                                           "track data.\n";
            evt.is_valid = 0;
            return evt;
        }
        std::string data(length ? (const char *)ptr : nullptr, (size_t)length);
        ptr += (size_t)length;

        evt.type     = byte;
        evt.sub_type = evtype;
        evt.data.insert(evt.data.begin(), data.begin(), data.end());

        /* TODO: Store those meta-strings separately and give ability to read
         * them by external functions (to display song title and copyright in
         * the player) */
        if (evt.sub_type == MidiEvent::kCopyright)
        {
            if (midi_music_copyright_.empty())
            {
                midi_music_copyright_ = std::string((const char *)evt.data.data(), evt.data.size());
                midi_music_copyright_.push_back('\0'); /* ending fix for UTF16 strings */
                if (midi_output_interface_->onDebugMessage)
                    midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                           "Music copyright: %s", midi_music_copyright_.c_str());
            }
            else if (midi_output_interface_->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                       "Extra copyright event: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::kSequenceTrackTitle)
        {
            if (midi_music_title_.empty())
            {
                midi_music_title_ = std::string((const char *)evt.data.data(), evt.data.size());
                midi_music_title_.push_back('\0'); /* ending fix for UTF16 strings */
                if (midi_output_interface_->onDebugMessage)
                    midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                           "Music title: %s", midi_music_title_.c_str());
            }
            else
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                midi_music_track_titles_.push_back(str);
                if (midi_output_interface_->onDebugMessage)
                    midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                           "Track title: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::kInstrumentTitle)
        {
            if (midi_output_interface_->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                       "Instrument: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::kMarker)
        {
            // To lower
            for (size_t i = 0; i < data.size(); i++)
            {
                if (data[i] <= 'Z' && data[i] >= 'A')
                    data[i] = (char)(data[i] - ('Z' - 'z'));
            }

            if (data == "loopstart")
            {
                // Return a custom Loop Start event instead of Marker
                evt.sub_type = MidiEvent::kLoopStart;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if (data == "loopend")
            {
                // Return a custom Loop End event instead of Marker
                evt.sub_type = MidiEvent::kLoopEnd;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if (data.substr(0, 10) == "loopstart=")
            {
                evt.type      = MidiEvent::kSpecial;
                evt.sub_type  = MidiEvent::kLoopStackBegin;
                uint8_t loops = (uint8_t)(atoi(data.substr(10).c_str()));
                evt.data.clear();
                evt.data.push_back(loops);

                if (midi_output_interface_->onDebugMessage)
                {
                    midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                           "Stack Marker Loop Start at %d to %d level with %d "
                                                           "loops",
                                                           midi_loop_.stack_level_, midi_loop_.stack_level_ + 1, loops);
                }
                return evt;
            }

            if (data.substr(0, 8) == "loopend=")
            {
                evt.type     = MidiEvent::kSpecial;
                evt.sub_type = MidiEvent::kLoopStackEnd;
                evt.data.clear();

                if (midi_output_interface_->onDebugMessage)
                {
                    midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                           "Stack Marker Loop %s at %d to %d level",
                                                           (evt.sub_type == MidiEvent::kLoopStackEnd ? "End" : "Break"),
                                                           midi_loop_.stack_level_, midi_loop_.stack_level_ - 1);
                }
                return evt;
            }
        }

        if (evtype == MidiEvent::kEndTrack)
            status = -1; // Finalize track

        return evt;
    }

    // Any normal event (80..EF)
    if (byte < 0x80)
    {
        byte = (uint8_t)(status | 0x80);
        ptr--;
    }

    // Sys Com Song Select(Song #) [0-127]
    if (byte == MidiEvent::kSysComSongSelect)
    {
        if (ptr + 1 > end)
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read System Command Song Select event - "
                                           "Unexpected end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        return evt;
    }

    // Sys Com Song Position Pntr [LSB, MSB]
    if (byte == MidiEvent::kSysComSongPositionPointer)
    {
        if (ptr + 2 > end)
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read System Command Position Pointer event "
                                           "- Unexpected end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));
        return evt;
    }

    uint8_t midCh = byte & 0x0F, evType = (byte >> 4) & 0x0F;
    status      = byte;
    evt.channel = midCh;
    evt.type    = evType;

    switch (evType)
    {
    case MidiEvent::kNoteOff: // 2 byte length
    case MidiEvent::kNoteOn:
    case MidiEvent::kNoteTouch:
    case MidiEvent::kControlChange:
    case MidiEvent::kPitchWheel:
        if (ptr + 2 > end)
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read regular 2-byte event - Unexpected "
                                           "end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }

        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));

        if ((evType == MidiEvent::kNoteOn) && (evt.data[1] == 0))
        {
            evt.type = MidiEvent::kNoteOff; // Note ON with zero velocity
                                            // is Note OFF!
        }
        else if (evType == MidiEvent::kControlChange)
        {
            switch (evt.data[0])
            {
            case 110:
                if (midi_loop_format_ == kLoopDefault)
                {
                    // Change event type to custom Loop Start event
                    // and clear data
                    evt.type     = MidiEvent::kSpecial;
                    evt.sub_type = MidiEvent::kLoopStart;
                    evt.data.clear();
                    midi_loop_format_ = kLoopHmi;
                }
                else if (midi_loop_format_ == kLoopHmi)
                {
                    // Repeating of 110'th point is BAD practice,
                    // treat as EMIDI
                    midi_loop_format_ = kLoopEMidi;
                }
                break;

            case 111:
                if (midi_loop_format_ == kLoopHmi)
                {
                    // Change event type to custom Loop End event
                    // and clear data
                    evt.type     = MidiEvent::kSpecial;
                    evt.sub_type = MidiEvent::kLoopEnd;
                    evt.data.clear();
                }
                else if (midi_loop_format_ != kLoopEMidi)
                {
                    // Change event type to custom Loop Start event
                    // and clear data
                    evt.type     = MidiEvent::kSpecial;
                    evt.sub_type = MidiEvent::kLoopStart;
                    evt.data.clear();
                }
                break;

            case 113:
                if (midi_loop_format_ == kLoopEMidi)
                {
                    // EMIDI does using of CC113 with same purpose
                    // as CC7
                    evt.data[0] = 7;
                }
                break;
            }
        }

        return evt;
    case MidiEvent::kPatchChange: // 1 byte length
    case MidiEvent::kChannelAftertouch:
        if (ptr + 1 > end)
        {
            midi_parsing_errors_string_ += "ParseEvent: Can't read regular 1-byte event - Unexpected "
                                           "end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.data.push_back(*(ptr++));
        return evt;
    default:
        break;
    }

    return evt;
}

void MidiSequencer::handleEvent(size_t track, const MidiSequencer::MidiEvent &evt, int32_t &status)
{
    if (track == 0 && midi_smf_format_ < 2 && evt.type == MidiEvent::kSpecial &&
        (evt.sub_type == MidiEvent::kTempoChange || evt.sub_type == MidiEvent::kTimeSignature))
    {
        /* never reject track 0 timing events on SMF format != 2
           note: multi-track XMI convert to format 2 SMF */
    }
    else
    {
        if (midi_track_solo_ != ~(size_t)(0) && track != midi_track_solo_)
            return;
        if (midi_track_disabled_[track])
            return;
    }

    if (midi_output_interface_->onEvent)
    {
        midi_output_interface_->onEvent(midi_output_interface_->onEvent_userdata, evt.type, evt.sub_type, evt.channel,
                                        evt.data.data(), evt.data.size());
    }

    if (evt.type == MidiEvent::kSysex || evt.type == MidiEvent::kSysex2) // Ignore SysEx
    {
        midi_output_interface_->rt_systemExclusive(midi_output_interface_->rtUserData, evt.data.data(),
                                                   evt.data.size());
        return;
    }

    if (evt.type == MidiEvent::kSpecial)
    {
        // Special event FF
        uint_fast16_t evtype = evt.sub_type;
        uint64_t      length = (uint64_t)(evt.data.size());
        const char   *data(length ? (const char *)(evt.data.data()) : "\0\0\0\0\0\0\0\0");

        if (midi_output_interface_->rt_metaEvent) // Meta event hook
            midi_output_interface_->rt_metaEvent(midi_output_interface_->rtUserData, evtype, (const uint8_t *)(data),
                                                 size_t(length));

        if (evtype == MidiEvent::kEndTrack) // End Of Track
        {
            status = -1;
            return;
        }

        if (evtype == MidiEvent::kTempoChange) // Tempo change
        {
            midi_tempo_ =
                midi_individual_tick_delta_ * MidiFraction(ReadIntBigEndian(evt.data.data(), evt.data.size()));
            return;
        }

        if (evtype == MidiEvent::kMarker) // Meta event
        {
            // Do nothing! :-P
            return;
        }

        if (evtype == MidiEvent::kDeviceSwitch)
        {
            if (midi_output_interface_->onDebugMessage)
                midi_output_interface_->onDebugMessage(midi_output_interface_->onDebugMessage_userdata,
                                                       "Switching another device: %s", data);
            if (midi_output_interface_->rt_deviceSwitch)
                midi_output_interface_->rt_deviceSwitch(midi_output_interface_->rtUserData, track, data,
                                                        size_t(length));
            return;
        }

        // Turn on Loop handling when loop is enabled
        if (midi_loop_enabled_ && !midi_loop_.invalid_loop_)
        {
            if (evtype == MidiEvent::kLoopStart) // Special non-spec MIDI
                                                 // loop Start point
            {
                midi_loop_.caught_start_ = true;
                return;
            }

            if (evtype == MidiEvent::kLoopEnd) // Special non-spec MIDI loop End point
            {
                midi_loop_.caught_end_ = true;
                return;
            }

            if (evtype == MidiEvent::kLoopStackBegin)
            {
                if (midi_loop_.skip_stack_start_)
                {
                    midi_loop_.skip_stack_start_ = false;
                    return;
                }

                char   x      = data[0];
                size_t slevel = (size_t)(midi_loop_.stack_level_ + 1);
                while (slevel >= midi_loop_.stack_.size())
                {
                    LoopStackEntry e;
                    e.loops    = x;
                    e.infinity = (x == 0);
                    e.start    = 0;
                    e.end      = 0;
                    midi_loop_.stack_.push_back(e);
                }

                LoopStackEntry &s              = midi_loop_.stack_[slevel];
                s.loops                        = (int)(x);
                s.infinity                     = (x == 0);
                midi_loop_.caught_stack_start_ = true;
                return;
            }

            if (evtype == MidiEvent::kLoopStackEnd)
            {
                midi_loop_.caught_stack_end_ = true;
                return;
            }

            if (evtype == MidiEvent::kLoopStackBreak)
            {
                midi_loop_.caught_stack_break_ = true;
                return;
            }
        }

        if (evtype == MidiEvent::kCallbackTrigger)
        {
            if (midi_trigger_handler_)
                midi_trigger_handler_(midi_trigger_userdata_, (unsigned)(data[0]), track);
            return;
        }

        if (evtype == MidiEvent::kSongBeginHook)
        {
            if (midi_output_interface_->onSongStart)
                midi_output_interface_->onSongStart(midi_output_interface_->onSongStart_userdata);
            return;
        }

        return;
    }

    if (evt.type == MidiEvent::kSysComSongSelect || evt.type == MidiEvent::kSysComSongPositionPointer)
        return;

    size_t midCh = evt.channel;
    if (midi_output_interface_->rt_currentDevice)
        midCh += midi_output_interface_->rt_currentDevice(midi_output_interface_->rtUserData, track);
    status = evt.type;

    switch (evt.type)
    {
    case MidiEvent::kNoteOff: // Note off
    {
        if (midCh < 16 && m_channelDisable[midCh])
            break;            // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        if (midi_output_interface_->rt_noteOff)
            midi_output_interface_->rt_noteOff(midi_output_interface_->rtUserData, (uint8_t)(midCh), note);
        if (midi_output_interface_->rt_noteOffVel)
            midi_output_interface_->rt_noteOffVel(midi_output_interface_->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::kNoteOn: // Note on
    {
        if (midCh < 16 && m_channelDisable[midCh])
            break;           // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        midi_output_interface_->rt_noteOn(midi_output_interface_->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::kNoteTouch: // Note touch
    {
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        midi_output_interface_->rt_noteAfterTouch(midi_output_interface_->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::kControlChange: // Controller change
    {
        uint8_t ctrlno = evt.data[0];
        uint8_t value  = evt.data[1];
        midi_output_interface_->rt_controllerChange(midi_output_interface_->rtUserData, (uint8_t)(midCh), ctrlno,
                                                    value);
        break;
    }

    case MidiEvent::kPatchChange: // Patch change
    {
        midi_output_interface_->rt_patchChange(midi_output_interface_->rtUserData, (uint8_t)(midCh), evt.data[0]);
        break;
    }

    case MidiEvent::kChannelAftertouch: // Channel after-touch
    {
        uint8_t chanat = evt.data[0];
        midi_output_interface_->rt_channelAfterTouch(midi_output_interface_->rtUserData, (uint8_t)(midCh), chanat);
        break;
    }

    case MidiEvent::kPitchWheel: // Wheel/pitch bend
    {
        uint8_t a = evt.data[0];
        uint8_t b = evt.data[1];
        midi_output_interface_->rt_pitchBend(midi_output_interface_->rtUserData, (uint8_t)(midCh), b, a);
        break;
    }

    default:
        break;
    } // switch
}

double MidiSequencer::Tick(double s, double granularity)
{
    EPI_ASSERT(midi_output_interface_); // MIDI output interface must be defined!

    s *= midi_tempo_multiplier_;
    midi_current_position_.wait -= s;
    midi_current_position_.absolute_time_position += s;

    int antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
    while ((midi_current_position_.wait <= granularity * 0.5) && (antiFreezeCounter > 0))
    {
        if (!ProcessEvents())
            break;
        if (midi_current_position_.wait <= 0.0)
            antiFreezeCounter--;
    }

    if (antiFreezeCounter <= 0)
        midi_current_position_.wait += 1.0; /* Add extra 1 second when over 10000 events
                                               with zero delay are been detected */

    if (midi_current_position_.wait < 0.0)  // Avoid negative delay value!
        return 0.0;

    return midi_current_position_.wait;
}

double MidiSequencer::Seek(double seconds, const double granularity)
{
    if (seconds < 0.0)
        return 0.0;                        // Seeking negative position is forbidden! :-P
    const double granualityHalf = granularity * 0.5,
                 s              = seconds; // m_setup.delay_ < m_setup.maxdelay ?
                                           // m_setup.delay_ : m_setup.maxdelay;

    /* Attempt to go away out of song end must rewind position to begin */
    if (seconds > midi_full_song_time_length_)
    {
        this->Rewind();
        return 0.0;
    }

    bool loopFlagState = midi_loop_enabled_;
    // Turn loop pooints off because it causes wrong position rememberin on a
    // quick seek
    midi_loop_enabled_ = false;

    /*
     * Seeking search is similar to regular ticking, except of next things:
     * - We don't processsing arpeggio and vibrato
     * - To keep correctness of the state after seek, begin every search from
     * begin
     * - All sustaining notes must be killed
     * - Ignore Note-On events
     */
    this->Rewind();

    /*
     * Set "loop Start" to false to prevent overwrite of loopStart position with
     * seek destinition position
     *
     * TODO: Detect & set loopStart position on load time to don't break loop
     * while seeking
     */
    midi_loop_.caught_start_ = false;

    midi_loop_.temporary_broken_ = (seconds >= midi_loop_end_time_);

    while ((midi_current_position_.absolute_time_position < seconds) &&
           (midi_current_position_.absolute_time_position < midi_full_song_time_length_))
    {
        midi_current_position_.wait -= s;
        midi_current_position_.absolute_time_position += s;
        int    antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
        double dstWait           = midi_current_position_.wait + granualityHalf;
        while ((midi_current_position_.wait <= granualityHalf) /*&& (antiFreezeCounter > 0)*/)
        {
            // std::fprintf(stderr, "wait = %g...\n", CurrentPosition.wait);
            if (!ProcessEvents(true))
                break;
            // Avoid freeze because of no waiting increasing in more than 10000
            // cycles
            if (midi_current_position_.wait <= dstWait)
                antiFreezeCounter--;
            else
            {
                dstWait           = midi_current_position_.wait + granualityHalf;
                antiFreezeCounter = 10000;
            }
        }
        if (antiFreezeCounter <= 0)
            midi_current_position_.wait += 1.0; /* Add extra 1 second when over 10000 events
                                                   with zero delay are been detected */
    }

    if (midi_current_position_.wait < 0.0)
        midi_current_position_.wait = 0.0;

    if (midi_at_end_)
    {
        this->Rewind();
        midi_loop_enabled_ = loopFlagState;
        return 0.0;
    }

    midi_time_.Reset();
    midi_time_.delay_ = midi_current_position_.wait;

    midi_loop_enabled_ = loopFlagState;
    return midi_current_position_.wait;
}

double MidiSequencer::Tell()
{
    return midi_current_position_.absolute_time_position;
}

double MidiSequencer::TimeLength()
{
    return midi_full_song_time_length_;
}

double MidiSequencer::GetLoopStart()
{
    return midi_loop_start_time_;
}

double MidiSequencer::GetLoopEnd()
{
    return midi_loop_end_time_;
}

void MidiSequencer::Rewind()
{
    midi_current_position_ = midi_track_begin_position_;
    midi_at_end_           = false;

    midi_loop_.loops_count_ = midi_loop_count_;
    midi_loop_.Reset();
    midi_loop_.caught_start_     = true;
    midi_loop_.temporary_broken_ = false;
    midi_time_.Reset();
}

void MidiSequencer::SetTempo(double tempo)
{
    midi_tempo_multiplier_ = tempo;
}

bool MidiSequencer::LoadMidi(const uint8_t *data, size_t size)
{
    epi::MemFile *mfr = new epi::MemFile(data, size);
    return LoadMidi(mfr);
}

template <class T> class BufferGuard
{
    T *m_ptr;

  public:
    BufferGuard() : m_ptr(nullptr)
    {
    }

    ~BufferGuard()
    {
        set();
    }

    void set(T *p = nullptr)
    {
        if (m_ptr)
            free(m_ptr);
        m_ptr = p;
    }
};

bool MidiSequencer::LoadMidi(epi::MemFile *mfr)
{
    size_t fsize = 0;
    (void)(fsize);
    midi_parsing_errors_string_.clear();

    EPI_ASSERT(midi_output_interface_); // MIDI output interface must be defined!

    midi_at_end_ = false;
    midi_loop_.FullReset();
    midi_loop_.caught_start_ = true;

    midi_smf_format_ = 0;

    midi_raw_songs_data_.clear();

    const size_t headerSize            = 4 + 4 + 2 + 2 + 2; // 14
    char         headerBuf[headerSize] = "";

    fsize = mfr->Read(headerBuf, headerSize);
    if (fsize < headerSize)
    {
        midi_error_string_ = "Unexpected end of file at header!\n";
        delete mfr;
        return false;
    }

    if (memcmp(headerBuf, "MThd\0\0\0\6", 8) == 0)
    {
        mfr->Seek(0, epi::File::kSeekpointStart);
        return ParseSMF(mfr);
    }

    if (memcmp(headerBuf, "MUS\x1A", 4) == 0)
    {
        mfr->Seek(0, epi::File::kSeekpointStart);
        return ParseMUS(mfr);
    }

    midi_error_string_ = "Unknown or unsupported file format";
    delete mfr;
    return false;
}

bool MidiSequencer::ParseSMF(epi::MemFile *mfr)
{
    const size_t                      headerSize            = 14; // 4 + 4 + 2 + 2 + 2
    char                              headerBuf[headerSize] = "";
    size_t                            fsize                 = 0;
    size_t                            deltaTicks = 192, TrackCount = 1;
    unsigned                          smfFormat = 0;
    std::vector<std::vector<uint8_t>> rawTrackData;

    fsize = mfr->Read(headerBuf, headerSize);
    if (fsize < headerSize)
    {
        midi_error_string_ = "Unexpected end of file at header!\n";
        delete mfr;
        return false;
    }

    if (memcmp(headerBuf, "MThd\0\0\0\6", 8) != 0)
    {
        midi_error_string_ = "MIDI Loader: Invalid format, MThd signature is not found!\n";
        delete mfr;
        return false;
    }

    smfFormat  = (unsigned)(ReadIntBigEndian(headerBuf + 8, 2));
    TrackCount = (size_t)(ReadIntBigEndian(headerBuf + 10, 2));
    deltaTicks = (size_t)(ReadIntBigEndian(headerBuf + 12, 2));

    if (smfFormat > 2)
        smfFormat = 1;

    rawTrackData.clear();
    rawTrackData.resize(TrackCount, std::vector<uint8_t>());
    midi_individual_tick_delta_ = MidiFraction(1, 1000000l * (uint64_t)(deltaTicks));
    midi_tempo_                 = MidiFraction(1, (uint64_t)(deltaTicks) * 2);

    size_t totalGotten = 0;

    for (size_t tk = 0; tk < TrackCount; ++tk)
    {
        // Read track header
        size_t trackLength;

        fsize = mfr->Read(headerBuf, 8);
        if ((fsize < 8) || (memcmp(headerBuf, "MTrk", 4) != 0))
        {
            midi_error_string_ = "MIDI Loader: Invalid format, MTrk signature is not found!\n";
            delete mfr;
            return false;
        }
        trackLength = (size_t)ReadIntBigEndian(headerBuf + 4, 4);

        // Read track data
        rawTrackData[tk].resize(trackLength);
        fsize = mfr->Read(&rawTrackData[tk][0], trackLength);
        if (fsize < trackLength)
        {
            midi_error_string_ = "MIDI Loader: Unexpected file ending while getting raw track "
                                 "data!\n";
            delete mfr;
            return false;
        }

        totalGotten += fsize;
    }

    for (size_t tk = 0; tk < TrackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if (totalGotten == 0)
    {
        midi_error_string_ = "MIDI Loader: Empty track data";
        delete mfr;
        return false;
    }

    // Build new MIDI events table
    if (!BuildSMFTrackData(rawTrackData))
    {
        midi_error_string_ = "MIDI Loader: MIDI data parsing error has occouped!\n" + midi_parsing_errors_string_;
        delete mfr;
        return false;
    }

    midi_smf_format_        = smfFormat;
    midi_loop_.stack_level_ = -1;

    delete mfr;

    return true;
}

bool MidiSequencer::ParseMUS(epi::MemFile *mfr)
{
    const size_t         headerSize            = 14;
    char                 headerBuf[headerSize] = "";
    size_t               fsize                 = 0;
    BufferGuard<uint8_t> cvt_buf;

    fsize = mfr->Read(headerBuf, headerSize);
    if (fsize < headerSize)
    {
        midi_error_string_ = "Unexpected end of file at header!\n";
        delete mfr;
        return false;
    }

    if (memcmp(headerBuf, "MUS\x1A", 4) != 0)
    {
        midi_error_string_ = "MIDI Loader: Invalid format, MUS\\x1A signature is not found!\n";
        delete mfr;
        return false;
    }

    size_t mus_len = mfr->GetLength();

    mfr->Seek(0, epi::File::kSeekpointStart);
    uint8_t *mus = (uint8_t *)malloc(mus_len);
    if (!mus)
    {
        midi_error_string_ = "Out of memory!";
        delete mfr;
        return false;
    }
    fsize = mfr->Read(mus, mus_len);
    if (fsize < mus_len)
    {
        midi_error_string_ = "Failed to read MUS file data!\n";
        delete mfr;
        return false;
    }

    // Close source stream
    delete mfr;
    mfr = nullptr;

    uint8_t *mid     = nullptr;
    uint32_t mid_len = 0;
    int      m2mret  = ConvertMusToMidi(mus, (uint32_t)(mus_len), &mid, &mid_len, 0);

    if (mus)
        free(mus);

    if (m2mret < 0)
    {
        midi_error_string_ = "Invalid MUS/DMX data format!";
        if (mid)
            free(mid);
        return false;
    }
    cvt_buf.set(mid);

    // Open converted MIDI file
    mfr = new epi::MemFile(mid, (size_t)(mid_len));

    return ParseSMF(mfr);
}