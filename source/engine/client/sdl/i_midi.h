/*
 * BW_Midi_Sequencer - MIDI Sequencer for C++
 *
 * Copyright (c) 2015-2022 Vitaly Novichkov <admin@wohlnet.ru>
 * Copyright (c) 2024 The MUD Team.
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

#pragma once

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm> // std::copy
#include <iterator>  // std::back_inserter
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Poco/Buffer.h"
#include "m_memio.h"
#include "mus2midi.hpp"

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
    uint64_t m_num1, m_num2;
    void     Optim();

  public:
    MidiFraction() : m_num1(0), m_num2(1)
    {
    }
    MidiFraction(uint64_t value) : m_num1(value), m_num2(1)
    {
    }
    MidiFraction(uint64_t n, uint64_t d) : m_num1(n), m_num2(d)
    {
    }
    inline double Value() const
    {
        return Nom() / (double)Denom();
    }
    MidiFraction &operator*=(const MidiFraction &b)
    {
        m_num1 *= b.Nom();
        m_num2 *= b.Denom();
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
        return m_num1;
    }
    const uint64_t &Denom() const
    {
        return m_num2;
    }
};

void MidiFraction::Optim()
{
    /* Euclidean algorithm */
    uint64_t n1, n2, nn1, nn2;

    nn1 = m_num1;
    nn2 = m_num2;

    if (nn1 < nn2)
        n1 = m_num1, n2 = m_num2;
    else
        n1 = m_num2, n2 = m_num1;

    if (!m_num1)
    {
        m_num2 = 1;
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
    m_num1 /= n1;
    m_num2 /= n1;
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
            EVENT_UNKNOWN = 0x00,
            //! Note-Off event
            EVENT_NOTE_OFF = 0x08,           // size == 2
                                             //! Note-On event
            EVENT_NOTE_ON = 0x09,            // size == 2
                                             //! Note After-Touch event
            EVENT_NOTE_TOUCH = 0x0A,         // size == 2
                                             //! Controller change event
            EVENT_CONTROL_CHANGE = 0x0B,     // size == 2
                                             //! Patch change event
            EVENT_PATCH_CHANGE = 0x0C,       // size == 1
                                             //! Channel After-Touch event
            EVENT_CHANNEL_AFTERTOUCH = 0x0D, // size == 1
                                             //! Pitch-bend change event
            EVENT_PITCH_WHEEL = 0x0E,        // size == 2

            //! System Exclusive message, type 1
            EVENT_SYSEX = 0xF0,           // size == len
                                          //! Sys Com Song Position Pntr [LSB, MSB]
            EVENT_SYSCOM_POSITION = 0xF2, // size == 2
                                          //! Sys Com Song Select(Song #) [0-127]
            EVENT_SYSCOM_SELECT = 0xF3,   // size == 1
                                          //! System Exclusive message, type 2
            EVENT_SYSEX2 = 0xF7,          // size == len
                                          //! Special event
            EVENT_SPECIAL = 0xFF
        };
        /**
         * @brief Special MIDI event sub-types
         */
        enum SubTypes
        {
            //! Sequension number
            EVENT_SUB_SEQUENSION = 0x00,       // size == 2
                                               //! Text label
            EVENT_SUB_TEXT = 0x01,             // size == len
                                               //! Copyright notice
            EVENT_SUB_COPYRIGHT = 0x02,        // size == len
                                               //! Sequence track title
            EVENT_SUB_TRACK_TITLE = 0x03,      // size == len
                                               //! Instrument title
            EVENT_SUB_INSTRUMENT_TITLE = 0x04, // size == len
                                               //! Lyrics text fragment
            EVENT_SUB_LYRICS = 0x05,           // size == len
                                               //! MIDI Marker
            EVENT_SUB_MARKER = 0x06,           // size == len
                                               //! Cue Point
            EVENT_SUB_CUE_POINT = 0x07,        // size == len
                                               //! [Non-Standard] Device Switch
            EVENT_SUB_DEVICE_SWITCH = 0x09,    // size == len <CUSTOM>
                                               //! MIDI Channel prefix
            EVENT_SUB_CHANNEL_PREFIX = 0x20,   // size == 1

            //! End of Track event
            EVENT_SUB_END_TRACK = 0x2F,       // size == 0
                                              //! Tempo change event
            EVENT_SUB_TEMPO_CHANGE = 0x51,    // size == 3
                                              //! SMPTE offset
            EVENT_SUB_SMPTE_OFFSET = 0x54,    // size == 5
                                              //! Time signature
            EVENT_SUB_TIME_SIGNATURE = 0x55,  // size == 4
                                              //! Key signature
            EVENT_SUB_KEY_SIGNATURE = 0x59,   // size == 2
                                              //! Sequencer specs
            EVENT_SUB_SEQUENCER_SPECS = 0x7F, // size == len

            /* Non-standard, internal ADLMIDI usage only */
            //! [Non-Standard] Loop Start point
            EVENT_SUB_LOOP_START = 0xE1, // size == 0 <CUSTOM>
                                         //! [Non-Standard] Loop End point
            EVENT_SUB_LOOP_END = 0xE2,   // size == 0 <CUSTOM>

            //! [Non-Standard] Loop Start point with support of multi-loops
            EVENT_SUB_LOOP_STACK_BEGIN = 0xE4, // size == 1 <CUSTOM>
                                               //! [Non-Standard] Loop End point with
                                               //! support of multi-loops
            EVENT_SUB_LOOP_STACK_END = 0xE5,   // size == 0 <CUSTOM>
                                               //! [Non-Standard] Loop End point with
                                               //! support of multi-loops
            EVENT_SUB_LOOP_STACK_BREAK = 0xE6, // size == 0 <CUSTOM>
                                               //! [Non-Standard] Callback Trigger
            EVENT_SUB_CALLBACK_TRIGGER = 0xE7, // size == 1 <CUSTOM>

            // Built-in hooks
            EVENT_SUB_SONG_BEGIN_HOOK = 0x101
        };
        //! Main type of event
        uint_fast16_t type = EVENT_UNKNOWN;
        //! Sub-type of the event
        uint_fast16_t sub_type = EVENT_UNKNOWN;
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
        double m_time;
        //! Delay to next event in ticks
        uint64_t m_delay;
        //! Absolute position in ticks
        uint64_t m_absolute_position;
        //! Delay to next event in seconds
        double m_time_delay;
        //! List of MIDI events in the current row
        std::vector<MidiEvent> m_events;
        /**
         * @brief Sort events in this position
         * @param note_states Buffer of currently pressed/released note keys in
         * the track
         */
        void sortEvents(bool *note_states = nullptr);
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
    const MidiRealTimeInterface *m_midi_output_interface;

    /**
     * @brief Prepare internal events storage for track data building
     * @param track_count Count of tracks
     */
    void buildSMFSetupReset(size_t track_count);

    /**
     * @brief Build MIDI track data from the raw track data storage
     * @return true if everything successfully processed, or false on any error
     */
    bool buildSMFTrackData(const std::vector<std::vector<uint8_t>> &track_data);

    /**
     * @brief Build the time line from off loaded events
     * @param tempos Pre-collected list of tempo events
     * @param loop_start_ticks Global loop start tick (give zero if no global
     * loop presented)
     * @param loop_end_ticks Global loop end tick (give zero if no global loop
     * presented)
     */
    void buildTimeline(const std::vector<MidiEvent> &tempos, uint64_t loop_start_ticks = 0,
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
    MidiEvent parseEvent(const uint8_t **ptr, const uint8_t *end, int32_t &status);

    /**
     * @brief Process MIDI events on the current tick moment
     * @param is_seek is a seeking process
     * @return returns false on reaching end of the song
     */
    bool processEvents(bool is_seek = false);

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
        LOOP_DEFAULT,
        LOOP_RPG_MAKER = 1,
        LOOP_EMIDI,
        LOOP_HMI
    };

  private:
    //! SMF format identifier.
    uint32_t m_midi_smf_format;
    //! Loop points format
    LoopFormat m_midi_loop_format;

    //! Current position
    Position m_midi_current_position;
    //! Track begin position
    Position m_midi_track_begin_position;
    //! Loop start point
    Position m_midi_loop_begin_position;

    //! Is looping enabled or not
    bool m_midi_loop_enabled;
    //! Don't process loop: trigger hooks only if they are set
    bool m_midi_loop_hooks_only;

    //! Full song length in seconds
    double m_midi_full_song_time_length;
    //! Delay after song playd before rejecting the output stream requests
    double m_midi_post_song_wait_delay;

    //! Global loop start time
    double m_midi_loop_start_time;
    //! Global loop end time
    double m_midi_loop_end_time;

    //! Pre-processed track data storage
    std::vector<std::list<MidiTrackRow>> m_midi_track_data;

    //! Title of music
    std::string m_midi_music_title;
    //! Copyright notice of music
    std::string m_midi_music_copyright;
    //! List of track titles
    std::vector<std::string> m_midi_music_track_titles;
    //! List of MIDI markers
    std::vector<MidiMarkerEntry> m_midi_music_markers;

    //! Time of one tick
    MidiFraction m_midi_individual_tick_delta;
    //! Current tempo
    MidiFraction m_midi_tempo;

    //! Tempo multiplier factor
    double m_midi_tempo_multiplier;
    //! Is song at end
    bool m_midi_at_end;

    //! Set the number of loops limit. Lesser than 0 - loop infinite
    int32_t m_midi_loop_count;

    /**
     * @brief Loop stack entry
     */
    struct LoopStackEntry
    {
        //! is infinite loop
        bool infinity = false;
        //! Count of loops left to break. <0 - infinite loop
        int32_t loops = 0;
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
        bool m_caught_start;
        //! Loop end has reached, reset on handling
        bool m_caught_end;

        //! Loop start has reached
        bool m_caught_m_stackstart;
        //! Loop next has reached, reset on handling
        bool m_caught_m_stackend;
        //! Loop break has reached, reset on handling
        bool m_caught_m_stackbreak;
        //! Skip next stack loop start event handling
        bool m_skip_m_stackstart;

        //! Are loop points invalid?
        bool m_invalid_loop; /*Loop points are invalid (loopStart after loopEnd
                             or loopStart and loopEnd are on same place)*/

        //! Is look got temporarily broken because of post-end seek?
        bool m_temporary_broken;

        //! How much times the loop should start repeat? For example, if you
        //! want to loop song twice, set value 1
        int32_t m_loops_count;

        //! how many loops left until finish the song
        int32_t m_loops_left;

        //! Stack of nested loops
        std::vector<LoopStackEntry> m_stack;
        //! Current level on the loop stack (<0 - out of loop, 0++ - the index
        //! in the loop stack)
        int32_t m_stacklevel;

        /**
         * @brief Reset loop state to initial
         */
        void Reset()
        {
            m_caught_start        = false;
            m_caught_end          = false;
            m_caught_m_stackstart = false;
            m_caught_m_stackend   = false;
            m_caught_m_stackbreak = false;
            m_skip_m_stackstart   = false;
            m_loops_left          = m_loops_count;
        }

        void fullReset()
        {
            m_loops_count = -1;
            Reset();
            m_invalid_loop     = false;
            m_temporary_broken = false;
            m_stack.clear();
            m_stacklevel = -1;
        }

        bool isStackEnd()
        {
            if (m_caught_m_stackend && (m_stacklevel >= 0) && (m_stacklevel < (int32_t)(m_stack.size())))
            {
                const LoopStackEntry &e = m_stack[(size_t)(m_stacklevel)];
                if (e.infinity || (!e.infinity && e.loops > 0))
                    return true;
            }
            return false;
        }

        void stackUp(int32_t count = 1)
        {
            m_stacklevel += count;
        }

        void stackDown(int32_t count = 1)
        {
            m_stacklevel -= count;
        }

        LoopStackEntry &getCurrentStack()
        {
            if ((m_stacklevel >= 0) && (m_stacklevel < (int32_t)(m_stack.size())))
                return m_stack[(size_t)(m_stacklevel)];
            if (m_stack.empty())
            {
                LoopStackEntry d;
                d.loops    = 0;
                d.infinity = 0;
                d.start    = 0;
                d.end      = 0;
                m_stack.push_back(d);
            }
            return m_stack[0];
        }
    };

    LoopState m_midi_loop;

    //! Whether the nth track has playback disabled
    std::vector<bool> m_midi_track_disabled;
    //! Index of solo track, or max for disabled
    size_t m_midi_track_solo;
    //! MIDI channel disable (exception for extra port-prefix-based channels)
    bool m_channeldisable[16];

    /**
     * @brief Handler of callback trigger events
     * @param userdata Pointer to user data (usually, context of something)
     * @param trigger Value of the event which triggered this callback.
     * @param track Identifier of the track which triggered this callback.
     */
    typedef void (*TriggerHandler)(void *userdata, uint32_t trigger, size_t track);

    //! Handler of callback trigger events
    TriggerHandler m_midi_trigger_handler;
    //! User data of callback trigger events
    void *m_midi_trigger_userdata;

    //! File parsing errors string (adding into m_midi_error_string on aborting
    //! of the process)
    std::string m_midi_parsing_errors_string;
    //! Common error string
    std::string m_midi_error_string;

    class SequencerTime
    {
      public:
        //! Time buffer
        double m_time_rest;
        //! Sample rate
        uint32_t m_sample_rate;
        //! Size of one frame in bytes
        uint32_t m_frame_size;
        //! Minimum possible delay, granuality
        double m_minimum_delay;
        //! Last delay
        double m_delay;

        void Init()
        {
            m_sample_rate = 44100;
            m_frame_size  = 2;
            Reset();
        }

        void Reset()
        {
            m_time_rest     = 0.0;
            m_minimum_delay = 1.0 / (double)(m_sample_rate);
            m_delay         = 0.0;
        }
    };

    SequencerTime m_midi_time;

  public:
    MidiSequencer();
    virtual ~MidiSequencer();

    /**
     * @brief Sets the RT interface
     * @param intrf Pre-Initialized interface structure (pointer will be taken)
     */
    void setInterface(const MidiRealTimeInterface *intrf);

    /**
     * @brief Runs ticking in a sync with audio streaming. Use this together
     * with onPcmRender hook to easily play MIDI.
     * @param stream pointer to the output PCM stream
     * @param length length of the buffer in bytes
     * @return Count of recorded data in bytes
     */
    int32_t playStream(uint8_t *stream, size_t length);

    /**
     * @brief Returns the number of tracks
     * @return Track count
     */
    size_t getTrackCount() const;

    /**
     * @brief Sets whether a track is playing
     * @param track Track identifier
     * @param enable Whether to enable track playback
     * @return true on success, false if there was no such track
     */
    bool setTrackEnabled(size_t track, bool enable);

    /**
     * @brief Disable/enable a channel is sounding
     * @param channel Channel number from 0 to 15
     * @param enable Enable the channel playback
     * @return true on success, false if there was no such channel
     */
    bool setChannelEnabled(size_t channel, bool enable);

    /**
     * @brief Enables or disables solo on a track
     * @param track Identifier of solo track, or max to disable
     */
    void setSoloTrack(size_t track);

    /**
     * @brief Defines a handler for callback trigger events
     * @param handler Handler to invoke from the sequencer when triggered, or
     * nullptr.
     * @param userdata Instance of the library
     */
    void setTriggerHandler(TriggerHandler handler, void *userdata);

    /**
     * @brief Get string that describes reason of error
     * @return Error string
     */
    const std::string &getErrorString();

    /**
     * @brief Check is loop enabled
     * @return true if loop enabled
     */
    bool getLoopEnabled();

    /**
     * @brief Switch loop on/off
     * @param enabled Enable loop
     */
    void setLoopEnabled(bool enabled);

    /**
     * @brief Get the number of loops set
     * @return number of loops or -1 if loop infinite
     */
    int32_t getLoopsCount();

    /**
     * @brief How many times song should loop
     * @param loops count or -1 to loop infinite
     */
    void setLoopsCount(int32_t loops);

    /**
     * @brief Switch loop hooks-only mode on/off
     * @param enabled Don't loop: trigger hooks only without loop
     */
    void setLoopHooksOnly(bool enabled);

    /**
     * @brief Get music title
     * @return music title string
     */
    const std::string &getMusicTitle();

    /**
     * @brief Get music copyright notice
     * @return music copyright notice string
     */
    const std::string &getMusicCopyright();

    /**
     * @brief Get list of track titles
     * @return array of track title strings
     */
    const std::vector<std::string> &getTrackTitles();

    /**
     * @brief Get list of MIDI markers
     * @return Array of MIDI marker structures
     */
    const std::vector<MidiMarkerEntry> &getMarkers();

    /**
     * @brief Is position of song at end
     * @return true if end of song was reached
     */
    bool positionAtEnd();

    /**
     * @brief Load MIDI file from a memory block
     * @param data Pointer to memory block with MIDI data
     * @param size Size of source memory block
     * @return true if file successfully opened, false on any error
     */
    bool loadMidi(const uint8_t *data, size_t size);

    /**
     * @brief Load MIDI file by using FileAndMemReader interface
     * @param mfr mem_file_c with opened source file
     * @return true if file successfully opened, false on any error
     */
    bool loadMidi(MEMFILE *mfr);

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
    double timeLength();

    /**
     * @brief Gives loop start time position in seconds
     * @return Loop start time position in seconds or -1 if song has no loop
     * points
     */
    double getLoopStart();

    /**
     * @brief Gives loop end time position in seconds
     * @return Loop end time position in seconds or -1 if song has no loop
     * points
     */
    double getLoopEnd();

    /**
     * @brief Return to begin of current song
     */
    void Rewind();

    /**
     * @brief Get current tempor multiplier value
     * @return
     */
    double getTempoMultiplier();

    /**
     * @brief Set tempo multiplier
     * @param tempo Tempo multiplier: 1.0 - original tempo. >1 - faster, <1 -
     * slower
     */
    void setTempo(double tempo);

  private:
    /**
     * @brief Load file as Standard MIDI file
     * @param mfr mem_file_c with opened source file
     * @return true on successful load
     */
    bool _ParseSmf(MEMFILE *mfr);

    /**
     * @brief Load file as DMX MUS file (Doom)
     * @param mfr mem_file_c with opened source file
     * @return true on successful load
     */
    bool _ParseMus(MEMFILE *mfr);
};

/**
 * @brief Utility function to read Big-Endian integer from raw binary data
 * @param buffer Pointer to raw binary buffer
 * @param nbytes Count of bytes to parse integer
 * @return Extracted uint32_teger
 */
static inline uint64_t readIntBigEndian(const void *buffer, size_t nbytes)
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
 * @return Extracted uint32_teger
 */
static inline uint64_t readIntLittleEndian(const void *buffer, size_t nbytes)
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
static inline uint64_t readVariableLengthValue(const uint8_t **ptr, const uint8_t *end, bool &ok)
{
    uint64_t result = 0;
    ok              = false;

    for (;;)
    {
        if (*ptr >= end)
            return 2;
        uint8_t byte = *((*ptr)++);
        result             = (result << 7) + (byte & 0x7F);
        if (!(byte & 0x80))
            break;
    }

    ok = true;
    return result;
}

MidiSequencer::MidiTrackRow::MidiTrackRow() : m_time(0.0), m_delay(0), m_absolute_position(0), m_time_delay(0.0)
{
}

void MidiSequencer::MidiTrackRow::Clear()
{
    m_time              = 0.0;
    m_delay             = 0;
    m_absolute_position = 0;
    m_time_delay        = 0.0;
    m_events.clear();
}

void MidiSequencer::MidiTrackRow::sortEvents(bool *note_states)
{
    typedef std::vector<MidiEvent> EvtArr;
    EvtArr                         sysEx;
    EvtArr                         metas;
    EvtArr                         noteOffs;
    EvtArr                         controllers;
    EvtArr                         anyOther;

    for (size_t i = 0; i < m_events.size(); i++)
    {
        if (m_events[i].type == MidiEvent::EVENT_NOTE_OFF)
        {
            if (noteOffs.capacity() == 0)
                noteOffs.reserve(m_events.size());
            noteOffs.push_back(m_events[i]);
        }
        else if (m_events[i].type == MidiEvent::EVENT_SYSEX || m_events[i].type == MidiEvent::EVENT_SYSEX2)
        {
            if (sysEx.capacity() == 0)
                sysEx.reserve(m_events.size());
            sysEx.push_back(m_events[i]);
        }
        else if ((m_events[i].type == MidiEvent::EVENT_CONTROL_CHANGE) ||
                 (m_events[i].type == MidiEvent::EVENT_PATCH_CHANGE) ||
                 (m_events[i].type == MidiEvent::EVENT_PITCH_WHEEL) ||
                 (m_events[i].type == MidiEvent::EVENT_CHANNEL_AFTERTOUCH))
        {
            if (controllers.capacity() == 0)
                controllers.reserve(m_events.size());
            controllers.push_back(m_events[i]);
        }
        else if ((m_events[i].type == MidiEvent::EVENT_SPECIAL) &&
                 ((m_events[i].sub_type == MidiEvent::EVENT_SUB_MARKER) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_DEVICE_SWITCH) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_SONG_BEGIN_HOOK) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_LOOP_START) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_LOOP_END) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_BEGIN) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_END) ||
                  (m_events[i].sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_BREAK)))
        {
            if (metas.capacity() == 0)
                metas.reserve(m_events.size());
            metas.push_back(m_events[i]);
        }
        else
        {
            if (anyOther.capacity() == 0)
                anyOther.reserve(m_events.size());
            anyOther.push_back(m_events[i]);
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
            if (e.type == MidiEvent::EVENT_NOTE_ON)
            {
                const size_t note_i = (size_t)(e.channel * 255) + (e.data[0] & 0x7F);
                // Check, was previously note is on or off
                bool wasOn = note_states[note_i];
                markAsOn.insert(note_i);
                // Detect zero-length notes are following previously pressed
                // note
                int32_t noteOffsOnSameNote = 0;
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

    m_events.clear();
    if (!sysEx.empty())
        m_events.insert(m_events.end(), sysEx.begin(), sysEx.end());
    if (!noteOffs.empty())
        m_events.insert(m_events.end(), noteOffs.begin(), noteOffs.end());
    if (!metas.empty())
        m_events.insert(m_events.end(), metas.begin(), metas.end());
    if (!controllers.empty())
        m_events.insert(m_events.end(), controllers.begin(), controllers.end());
    if (!anyOther.empty())
        m_events.insert(m_events.end(), anyOther.begin(), anyOther.end());
}

MidiSequencer::MidiSequencer()
    : m_midi_output_interface(nullptr), m_midi_smf_format(0), m_midi_loop_format(LOOP_DEFAULT),
      m_midi_loop_enabled(false), m_midi_loop_hooks_only(false), m_midi_full_song_time_length(0.0),
      m_midi_post_song_wait_delay(1.0), m_midi_loop_start_time(-1.0), m_midi_loop_end_time(-1.0),
      m_midi_tempo_multiplier(1.0), m_midi_at_end(false), m_midi_loop_count(-1), m_midi_track_solo(~(size_t)(0)),
      m_midi_trigger_handler(nullptr), m_midi_trigger_userdata(nullptr)
{
    m_midi_loop.Reset();
    m_midi_loop.m_invalid_loop = false;
    m_midi_time.Init();
}

MidiSequencer::~MidiSequencer()
{
}

void MidiSequencer::setInterface(const MidiRealTimeInterface *intrf)
{
    // Interface must NOT be nullptr
    assert(intrf);

    // Note ON hook is REQUIRED
    assert(intrf->rt_noteOn);
    // Note OFF hook is REQUIRED
    assert(intrf->rt_noteOff || intrf->rt_noteOffVel);
    // Note Aftertouch hook is REQUIRED
    assert(intrf->rt_noteAfterTouch);
    // Channel Aftertouch hook is REQUIRED
    assert(intrf->rt_channelAfterTouch);
    // Controller change hook is REQUIRED
    assert(intrf->rt_controllerChange);
    // Patch change hook is REQUIRED
    assert(intrf->rt_patchChange);
    // Pitch bend hook is REQUIRED
    assert(intrf->rt_pitchBend);
    // System Exclusive hook is REQUIRED
    assert(intrf->rt_systemExclusive);

    if (intrf->pcmSampleRate != 0 && intrf->pcmFrameSize != 0)
    {
        m_midi_time.m_sample_rate = intrf->pcmSampleRate;
        m_midi_time.m_frame_size  = intrf->pcmFrameSize;
        m_midi_time.Reset();
    }

    m_midi_output_interface = intrf;
}

int32_t MidiSequencer::playStream(uint8_t *stream, size_t length)
{
    int32_t      count      = 0;
    size_t   samples    = (size_t)(length / (size_t)(m_midi_time.m_frame_size));
    size_t   left       = samples;
    size_t   periodSize = 0;
    uint8_t *stream_pos = stream;

    assert(m_midi_output_interface->onPcmRender);

    while (left > 0)
    {
        const double leftDelay = left / double(m_midi_time.m_sample_rate);
        const double maxDelay  = m_midi_time.m_time_rest < leftDelay ? m_midi_time.m_time_rest : leftDelay;
        if ((positionAtEnd()) && (m_midi_time.m_delay <= 0.0))
            break; // Stop to fetch samples at reaching the song end with
                   // disabled loop

        m_midi_time.m_time_rest -= maxDelay;
        periodSize = (size_t)((double)(m_midi_time.m_sample_rate) * maxDelay);

        if (stream)
        {
            size_t generateSize = periodSize > left ? (size_t)(left) : (size_t)(periodSize);
            m_midi_output_interface->onPcmRender(m_midi_output_interface->onPcmRender_userdata, stream_pos,
                                                 generateSize * m_midi_time.m_frame_size);
            stream_pos += generateSize * m_midi_time.m_frame_size;
            count += generateSize;
            left -= generateSize;
            assert(left <= samples);
        }

        if (m_midi_time.m_time_rest <= 0.0)
        {
            m_midi_time.m_delay = Tick(m_midi_time.m_delay, m_midi_time.m_minimum_delay);
            m_midi_time.m_time_rest += m_midi_time.m_delay;
        }
    }

    return count * (int32_t)(m_midi_time.m_frame_size);
}

size_t MidiSequencer::getTrackCount() const
{
    return m_midi_track_data.size();
}

bool MidiSequencer::setTrackEnabled(size_t track, bool enable)
{
    size_t track_count = m_midi_track_data.size();
    if (track >= track_count)
        return false;
    m_midi_track_disabled[track] = !enable;
    return true;
}

bool MidiSequencer::setChannelEnabled(size_t channel, bool enable)
{
    if (channel >= 16)
        return false;

    if (!enable && m_channeldisable[channel] != !enable)
    {
        uint8_t ch = (uint8_t)(channel);

        // Releae all pedals
        m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, ch, 64, 0);
        m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, ch, 66, 0);

        // Release all notes on the channel now
        for (int32_t i = 0; i < 127; ++i)
        {
            if (m_midi_output_interface->rt_noteOff)
                m_midi_output_interface->rt_noteOff(m_midi_output_interface->rtUserData, ch, i);
            if (m_midi_output_interface->rt_noteOffVel)
                m_midi_output_interface->rt_noteOffVel(m_midi_output_interface->rtUserData, ch, i, 0);
        }
    }

    m_channeldisable[channel] = !enable;
    return true;
}

void MidiSequencer::setSoloTrack(size_t track)
{
    m_midi_track_solo = track;
}

void MidiSequencer::setTriggerHandler(TriggerHandler handler, void *userdata)
{
    m_midi_trigger_handler  = handler;
    m_midi_trigger_userdata = userdata;
}

const std::string &MidiSequencer::getErrorString()
{
    return m_midi_error_string;
}

bool MidiSequencer::getLoopEnabled()
{
    return m_midi_loop_enabled;
}

void MidiSequencer::setLoopEnabled(bool enabled)
{
    m_midi_loop_enabled = enabled;
}

int32_t MidiSequencer::getLoopsCount()
{
    return m_midi_loop_count >= 0 ? (m_midi_loop_count + 1) : m_midi_loop_count;
}

void MidiSequencer::setLoopsCount(int32_t loops)
{
    if (loops >= 1)
        loops -= 1; // Internally, loops count has the 0 base
    m_midi_loop_count = loops;
}

void MidiSequencer::setLoopHooksOnly(bool enabled)
{
    m_midi_loop_hooks_only = enabled;
}

const std::string &MidiSequencer::getMusicTitle()
{
    return m_midi_music_title;
}

const std::string &MidiSequencer::getMusicCopyright()
{
    return m_midi_music_copyright;
}

const std::vector<std::string> &MidiSequencer::getTrackTitles()
{
    return m_midi_music_track_titles;
}

const std::vector<MidiSequencer::MidiMarkerEntry> &MidiSequencer::getMarkers()
{
    return m_midi_music_markers;
}

bool MidiSequencer::positionAtEnd()
{
    return m_midi_at_end;
}

double MidiSequencer::getTempoMultiplier()
{
    return m_midi_tempo_multiplier;
}

void MidiSequencer::buildSMFSetupReset(size_t track_count)
{
    m_midi_full_song_time_length = 0.0;
    m_midi_loop_start_time       = -1.0;
    m_midi_loop_end_time         = -1.0;
    m_midi_loop_format           = LOOP_DEFAULT;
    m_midi_track_disabled.clear();
    memset(m_channeldisable, 0, sizeof(m_channeldisable));
    m_midi_track_solo = ~(size_t)0;
    m_midi_music_title.clear();
    m_midi_music_copyright.clear();
    m_midi_music_track_titles.clear();
    m_midi_music_markers.clear();
    m_midi_track_data.clear();
    m_midi_track_data.resize(track_count, std::list<MidiTrackRow>());
    m_midi_track_disabled.resize(track_count);

    m_midi_loop.Reset();
    m_midi_loop.m_invalid_loop = false;
    m_midi_time.Reset();

    m_midi_current_position.began                  = false;
    m_midi_current_position.absolute_time_position = 0.0;
    m_midi_current_position.wait                   = 0.0;
    m_midi_current_position.track.clear();
    m_midi_current_position.track.resize(track_count);
}

bool MidiSequencer::buildSMFTrackData(const std::vector<std::vector<uint8_t>> &track_data)
{
    const size_t track_count = track_data.size();
    buildSMFSetupReset(track_count);

    bool gotGlobalLoopStart = false, gotGlobalLoopEnd = false, gotStacEVENT_SUB_LOOP_START = false,
         gotLoopEventInThisRow = false;

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
        int32_t            status       = 0;
        MidiEvent      event;
        bool           ok       = false;
        const uint8_t *end      = track_data[tk].data() + track_data[tk].size();
        const uint8_t *trackPtr = track_data[tk].data();
        memset(note_states, 0, sizeof(note_states));

        // Time delay that follows the first event in the track
        {
            MidiTrackRow evtPos;
            evtPos.m_delay = readVariableLengthValue(&trackPtr, end, ok);
            if (!ok)
            {
                int32_t len = snprintf(error, 150,
                                   "buildTrackData: Can't read variable-length "
                                   "value at begin of track %d.\n",
                                   (int32_t)tk);
                if ((len > 0) && (len < 150))
                    m_midi_parsing_errors_string += std::string(error, (size_t)len);
                return false;
            }

            // HACK: Begin every track with "Reset all controllers" event to
            // avoid controllers state break came from end of song
            if (tk == 0)
            {
                MidiEvent resetEvent;
                resetEvent.type     = MidiEvent::EVENT_SPECIAL;
                resetEvent.sub_type = MidiEvent::EVENT_SUB_SONG_BEGIN_HOOK;
                evtPos.m_events.push_back(resetEvent);
            }

            evtPos.m_absolute_position = abs_position;
            abs_position += evtPos.m_delay;
            m_midi_track_data[tk].push_back(evtPos);
        }

        MidiTrackRow evtPos;
        do
        {
            event = parseEvent(&trackPtr, end, status);
            if (!event.is_valid)
            {
                int32_t len = snprintf(error, 150, "buildTrackData: Fail to parse event in the track %d.\n", (int32_t)tk);
                if ((len > 0) && (len < 150))
                    m_midi_parsing_errors_string += std::string(error, (size_t)len);
                return false;
            }

            evtPos.m_events.push_back(event);
            if (event.type == MidiEvent::EVENT_SPECIAL)
            {
                if (event.sub_type == MidiEvent::EVENT_SUB_TEMPO_CHANGE)
                {
                    event.absolute_tick_position = abs_position;
                    temposList.push_back(event);
                }
                else if (!m_midi_loop.m_invalid_loop && (event.sub_type == MidiEvent::EVENT_SUB_LOOP_START))
                {
                    /*
                     * loopStart is invalid when:
                     * - starts together with loopEnd
                     * - appears more than one time in same MIDI file
                     */
                    if (gotGlobalLoopStart || gotLoopEventInThisRow)
                        m_midi_loop.m_invalid_loop = true;
                    else
                    {
                        gotGlobalLoopStart = true;
                        loop_start_ticks   = abs_position;
                    }
                    // In this row we got loop event, register this!
                    gotLoopEventInThisRow = true;
                }
                else if (!m_midi_loop.m_invalid_loop && (event.sub_type == MidiEvent::EVENT_SUB_LOOP_END))
                {
                    /*
                     * loopEnd is invalid when:
                     * - starts before loopStart
                     * - starts together with loopStart
                     * - appars more than one time in same MIDI file
                     */
                    if (gotGlobalLoopEnd || gotLoopEventInThisRow)
                    {
                        m_midi_loop.m_invalid_loop = true;
                        if (m_midi_output_interface->onDebugMessage)
                        {
                            m_midi_output_interface->onDebugMessage(
                                m_midi_output_interface->onDebugMessage_userdata, "== Invalid loop detected! %s %s ==",
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
                else if (!m_midi_loop.m_invalid_loop && (event.sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_BEGIN))
                {
                    if (!gotStacEVENT_SUB_LOOP_START)
                    {
                        if (!gotGlobalLoopStart)
                            loop_start_ticks = abs_position;
                        gotStacEVENT_SUB_LOOP_START = true;
                    }

                    m_midi_loop.stackUp();
                    if (m_midi_loop.m_stacklevel >= (int32_t)(m_midi_loop.m_stack.size()))
                    {
                        LoopStackEntry e;
                        e.loops    = event.data[0];
                        e.infinity = (event.data[0] == 0);
                        e.start    = abs_position;
                        e.end      = abs_position;
                        m_midi_loop.m_stack.push_back(e);
                    }
                }
                else if (!m_midi_loop.m_invalid_loop && ((event.sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_END) ||
                                                         (event.sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_BREAK)))
                {
                    if (m_midi_loop.m_stacklevel <= -1)
                    {
                        m_midi_loop.m_invalid_loop = true; // Caught loop end without of loop start!
                        if (m_midi_output_interface->onDebugMessage)
                        {
                            m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                                    "== Invalid loop detected! [Caught loop end "
                                                                    "without of loop start] ==");
                        }
                    }
                    else
                    {
                        if (loop_end_ticks < abs_position)
                            loop_end_ticks = abs_position;
                        m_midi_loop.getCurrentStack().end = abs_position;
                        m_midi_loop.stackDown();
                    }
                }
            }

            if (event.sub_type != MidiEvent::EVENT_SUB_END_TRACK) // Don't try to read delta after
                                                                  // EndOfTrack event!
            {
                evtPos.m_delay = readVariableLengthValue(&trackPtr, end, ok);
                if (!ok)
                {
                    /* End of track has been reached! However, there is no EOT
                     * event presented */
                    event.type     = MidiEvent::EVENT_SPECIAL;
                    event.sub_type = MidiEvent::EVENT_SUB_END_TRACK;
                }
            }

            if ((evtPos.m_delay > 0) || (event.sub_type == MidiEvent::EVENT_SUB_END_TRACK))
            {
                evtPos.m_absolute_position = abs_position;
                abs_position += evtPos.m_delay;
                evtPos.sortEvents(note_states);
                m_midi_track_data[tk].push_back(evtPos);
                evtPos.Clear();
                gotLoopEventInThisRow = false;
            }
        } while ((trackPtr <= end) && (event.sub_type != MidiEvent::EVENT_SUB_END_TRACK));

        if (ticksSongLength < abs_position)
            ticksSongLength = abs_position;
        // Set the chain of events begin
        if (m_midi_track_data[tk].size() > 0)
            m_midi_current_position.track[tk].pos = m_midi_track_data[tk].begin();
    }

    if (gotGlobalLoopStart && !gotGlobalLoopEnd)
    {
        gotGlobalLoopEnd = true;
        loop_end_ticks   = ticksSongLength;
    }

    // loopStart must be located before loopEnd!
    if (loop_start_ticks >= loop_end_ticks)
    {
        m_midi_loop.m_invalid_loop = true;
        if (m_midi_output_interface->onDebugMessage && (gotGlobalLoopStart || gotGlobalLoopEnd))
        {
            m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                    "== Invalid loop detected! [loopEnd is "
                                                    "going before loopStart] ==");
        }
    }

    buildTimeline(temposList, loop_start_ticks, loop_end_ticks);

    return true;
}

void MidiSequencer::buildTimeline(const std::vector<MidiEvent> &tempos, uint64_t loop_start_ticks,
                                  uint64_t loop_end_ticks)
{
    const size_t track_count = m_midi_track_data.size();
    /********************************************************************************/
    // Calculate time basing on collected tempo events
    /********************************************************************************/
    for (size_t tk = 0; tk < track_count; ++tk)
    {
        MidiFraction             currentTempo       = m_midi_tempo;
        double                   time               = 0.0;
        size_t                   tempo_change_index = 0;
        std::list<MidiTrackRow> &track              = m_midi_track_data[tk];
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
                if (tempos[tempo_change_index].absolute_tick_position <= pos.m_absolute_position)
                {
                    // Stop points: begin point and tempo change points are
                    // before end point
                    std::vector<TempoChangePoint> points;
                    MidiFraction                  t;
                    TempoChangePoint              firstPoint = {posPrev->m_absolute_position, currentTempo};
                    points.push_back(firstPoint);

                    // Collect tempo change points between previous and current
                    // events
                    do
                    {
                        TempoChangePoint tempoMarker;
                        const MidiEvent &tempoPoint   = tempos[tempo_change_index];
                        tempoMarker.absolute_position = tempoPoint.absolute_tick_position;
                        tempoMarker.tempo =
                            m_midi_individual_tick_delta *
                            MidiFraction(readIntBigEndian(tempoPoint.data.data(), tempoPoint.data.size()));
                        points.push_back(tempoMarker);
                        tempo_change_index++;
                    } while ((tempo_change_index < tempos.size()) &&
                             (tempos[tempo_change_index].absolute_tick_position <= pos.m_absolute_position));

                    // Re-calculate time delay of previous event
                    time -= posPrev->m_time_delay;
                    posPrev->m_time_delay = 0.0;

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
                        posPrev->m_time_delay += t.Value();

                        // Apply next tempo
                        currentTempo = points[j].tempo;
                    }
                    // Then calculate time between last tempo change point and
                    // end point
                    TempoChangePoint tailTempo = points.back();
                    uint64_t         postDelay = pos.m_absolute_position - tailTempo.absolute_position;
                    t                          = postDelay * currentTempo;
                    posPrev->m_time_delay += t.Value();

                    // Store Common time delay
                    posPrev->m_time = time;
                    time += posPrev->m_time_delay;
                }
            }

            MidiFraction t   = pos.m_delay * currentTempo;
            pos.m_time_delay = t.Value();
            pos.m_time       = time;
            time += pos.m_time_delay;

            // Capture markers after time value calculation
            for (size_t i = 0; i < pos.m_events.size(); i++)
            {
                MidiEvent &e = pos.m_events[i];
                if ((e.type == MidiEvent::EVENT_SPECIAL) && (e.sub_type == MidiEvent::EVENT_SUB_MARKER))
                {
                    MidiMarkerEntry marker;
                    marker.label          = std::string((char *)e.data.data(), e.data.size());
                    marker.position_ticks = pos.m_absolute_position;
                    marker.position_time  = pos.m_time;
                    m_midi_music_markers.push_back(marker);
                }
            }

            // Capture loop points time positions
            if (!m_midi_loop.m_invalid_loop)
            {
                // Set loop points times
                if (loop_start_ticks == pos.m_absolute_position)
                    m_midi_loop_start_time = pos.m_time;
                else if (loop_end_ticks == pos.m_absolute_position)
                    m_midi_loop_end_time = pos.m_time;
            }
            posPrev = &pos;
        }

        if (time > m_midi_full_song_time_length)
            m_midi_full_song_time_length = time;
    }

    m_midi_full_song_time_length += m_midi_post_song_wait_delay;
    // Set begin of the music
    m_midi_track_begin_position = m_midi_current_position;
    // Initial loop position will begin at begin of track until passing of the
    // loop point
    m_midi_loop_begin_position = m_midi_current_position;
    // Set lowest level of the loop stack
    m_midi_loop.m_stacklevel = -1;

    // Set the count of loops
    m_midi_loop.m_loops_count = m_midi_loop_count;
    m_midi_loop.m_loops_left  = m_midi_loop_count;

    /********************************************************************************/
    // Find and set proper loop points
    /********************************************************************************/
    if (!m_midi_loop.m_invalid_loop && !m_midi_current_position.track.empty())
    {
        uint32_t     caughLoopStart = 0;
        bool         scanDone       = false;
        const size_t ctrack_count   = m_midi_current_position.track.size();
        Position     rowPosition(m_midi_current_position);

        while (!scanDone)
        {
            const Position rowBeginPosition(rowPosition);

            for (size_t tk = 0; tk < ctrack_count; ++tk)
            {
                Position::TrackInfo &track = rowPosition.track[tk];
                if ((track.lastHandledEvent >= 0) && (track.delay <= 0))
                {
                    // Check is an end of track has been reached
                    if (track.pos == m_midi_track_data[tk].end())
                    {
                        track.lastHandledEvent = -1;
                        continue;
                    }

                    for (size_t i = 0; i < track.pos->m_events.size(); i++)
                    {
                        const MidiEvent &evt = track.pos->m_events[i];
                        if (evt.type == MidiEvent::EVENT_SPECIAL && evt.sub_type == MidiEvent::EVENT_SUB_LOOP_START)
                        {
                            caughLoopStart++;
                            scanDone = true;
                            break;
                        }
                    }

                    if (track.lastHandledEvent >= 0)
                    {
                        track.delay += track.pos->m_delay;
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
                m_midi_loop_begin_position                        = rowBeginPosition;
                m_midi_loop_begin_position.absolute_time_position = m_midi_loop_start_time;
                scanDone                                          = true;
            }

            if (shortestDelayNotFound)
                break;
        }
    }
}

bool MidiSequencer::processEvents(bool is_seek)
{
    if (m_midi_current_position.track.size() == 0)
        m_midi_at_end = true; // No MIDI track data to play
    if (m_midi_at_end)
        return false;         // No more events in the queue

    m_midi_loop.m_caught_end   = false;
    const size_t   track_count = m_midi_current_position.track.size();
    const Position rowBeginPosition(m_midi_current_position);
    bool           doLoopJump             = false;
    uint32_t       caughLoopStart         = 0;
    uint32_t       caughLoopStackStart    = 0;
    uint32_t       caughLoopStackEnds     = 0;
    double         caughLoopStackEndsTime = 0.0;
    uint32_t       caughLoopStackBreaks   = 0;

    for (size_t tk = 0; tk < track_count; ++tk)
    {
        Position::TrackInfo &track = m_midi_current_position.track[tk];
        if ((track.lastHandledEvent >= 0) && (track.delay <= 0))
        {
            // Check is an end of track has been reached
            if (track.pos == m_midi_track_data[tk].end())
            {
                track.lastHandledEvent = -1;
                break;
            }

            // Handle event
            for (size_t i = 0; i < track.pos->m_events.size(); i++)
            {
                const MidiEvent &evt = track.pos->m_events[i];

                if (is_seek && (evt.type == MidiEvent::EVENT_NOTE_ON))
                    continue;
                handleEvent(tk, evt, track.lastHandledEvent);

                if (m_midi_loop.m_caught_start)
                {
                    if (m_midi_output_interface->onloopStart) // Loop Start hook
                        m_midi_output_interface->onloopStart(m_midi_output_interface->onloopStart_userdata);

                    caughLoopStart++;
                    m_midi_loop.m_caught_start = false;
                }

                if (m_midi_loop.m_caught_m_stackstart)
                {
                    if (m_midi_output_interface->onloopStart &&
                        (m_midi_loop_start_time >= track.pos->m_time)) // Loop Start hook
                        m_midi_output_interface->onloopStart(m_midi_output_interface->onloopStart_userdata);

                    caughLoopStackStart++;
                    m_midi_loop.m_caught_m_stackstart = false;
                }

                if (m_midi_loop.m_caught_m_stackbreak)
                {
                    caughLoopStackBreaks++;
                    m_midi_loop.m_caught_m_stackbreak = false;
                }

                if (m_midi_loop.m_caught_end || m_midi_loop.isStackEnd())
                {
                    if (m_midi_loop.m_caught_m_stackend)
                    {
                        m_midi_loop.m_caught_m_stackend = false;
                        caughLoopStackEnds++;
                        caughLoopStackEndsTime = track.pos->m_time;
                    }
                    doLoopJump = true;
                    break; // Stop event handling on catching loopEnd event!
                }
            }

            // Read next event time (unless the track just ended)
            if (track.lastHandledEvent >= 0)
            {
                track.delay += track.pos->m_delay;
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
        Position::TrackInfo &track = m_midi_current_position.track[tk];
        if ((track.lastHandledEvent >= 0) && (shortestDelayNotFound || track.delay < shortestDelay))
        {
            shortestDelay         = track.delay;
            shortestDelayNotFound = false;
        }
    }

    // Schedule the next playevent to be processed after that delay
    for (size_t tk = 0; tk < track_count; ++tk)
        m_midi_current_position.track[tk].delay -= shortestDelay;

    MidiFraction t = shortestDelay * m_midi_tempo;

    m_midi_current_position.wait += t.Value();

    if (caughLoopStart > 0 && m_midi_loop_begin_position.absolute_time_position <= 0.0)
        m_midi_loop_begin_position = rowBeginPosition;

    if (caughLoopStackStart > 0)
    {
        while (caughLoopStackStart > 0)
        {
            m_midi_loop.stackUp();
            LoopStackEntry &s = m_midi_loop.getCurrentStack();
            s.start_position  = rowBeginPosition;
            caughLoopStackStart--;
        }
        return true;
    }

    if (caughLoopStackBreaks > 0)
    {
        while (caughLoopStackBreaks > 0)
        {
            LoopStackEntry &s = m_midi_loop.getCurrentStack();
            s.loops           = 0;
            s.infinity        = false;
            // Quit the loop
            m_midi_loop.stackDown();
            caughLoopStackBreaks--;
        }
    }

    if (caughLoopStackEnds > 0)
    {
        while (caughLoopStackEnds > 0)
        {
            LoopStackEntry &s = m_midi_loop.getCurrentStack();
            if (s.infinity)
            {
                if (m_midi_output_interface->onloopEnd &&
                    (m_midi_loop_end_time >= caughLoopStackEndsTime))                // Loop End hook
                {
                    m_midi_output_interface->onloopEnd(m_midi_output_interface->onloopEnd_userdata);
                    if (m_midi_loop_hooks_only)                                      // Stop song on reaching loop
                                                                                     // end
                    {
                        m_midi_at_end = true;                                        // Don't handle events anymore
                        m_midi_current_position.wait += m_midi_post_song_wait_delay; // One second delay
                                                                                     // until stop playing
                    }
                }

                m_midi_current_position         = s.start_position;
                m_midi_loop.m_skip_m_stackstart = true;

                for (uint8_t i = 0; i < 16; i++)
                    m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, i, 123, 0);

                return true;
            }
            else if (s.loops >= 0)
            {
                s.loops--;
                if (s.loops > 0)
                {
                    m_midi_current_position         = s.start_position;
                    m_midi_loop.m_skip_m_stackstart = true;

                    for (uint8_t i = 0; i < 16; i++)
                        m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, i, 123, 0);

                    return true;
                }
                else
                {
                    // Quit the loop
                    m_midi_loop.stackDown();
                }
            }
            else
            {
                // Quit the loop
                m_midi_loop.stackDown();
            }
            caughLoopStackEnds--;
        }

        return true;
    }

    if (shortestDelayNotFound || m_midi_loop.m_caught_end)
    {
        if (m_midi_output_interface->onloopEnd) // Loop End hook
            m_midi_output_interface->onloopEnd(m_midi_output_interface->onloopEnd_userdata);

        for (uint8_t i = 0; i < 16; i++)
            m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, i, 123, 0);

        // Loop if song end or loop end point has reached
        m_midi_loop.m_caught_end = false;
        shortestDelay            = 0;

        if (!m_midi_loop_enabled ||
            (shortestDelayNotFound && m_midi_loop.m_loops_count >= 0 && m_midi_loop.m_loops_left < 1) ||
            m_midi_loop_hooks_only)
        {
            m_midi_at_end = true;                                        // Don't handle events anymore
            m_midi_current_position.wait += m_midi_post_song_wait_delay; // One second delay until stop
                                                                         // playing
            return true;                                                 // We have caugh end here!
        }

        if (m_midi_loop.m_temporary_broken)
        {
            m_midi_current_position        = m_midi_track_begin_position;
            m_midi_loop.m_temporary_broken = false;
        }
        else if (m_midi_loop.m_loops_count < 0 || m_midi_loop.m_loops_left >= 1)
        {
            m_midi_current_position = m_midi_loop_begin_position;
            if (m_midi_loop.m_loops_count >= 1)
                m_midi_loop.m_loops_left--;
        }
    }

    return true; // Has events in queue
}

MidiSequencer::MidiEvent MidiSequencer::parseEvent(const uint8_t **pptr, const uint8_t *end, int32_t &status)
{
    const uint8_t          *&ptr = *pptr;
    MidiSequencer::MidiEvent evt;

    if (ptr + 1 > end)
    {
        // When track doesn't ends on the middle of event data, it's must be
        // fine
        evt.type     = MidiEvent::EVENT_SPECIAL;
        evt.sub_type = MidiEvent::EVENT_SUB_END_TRACK;
        return evt;
    }

    uint8_t byte = *(ptr++);
    bool          ok   = false;

    if (byte == MidiEvent::EVENT_SYSEX || byte == MidiEvent::EVENT_SYSEX2) // Ignore SysEx
    {
        uint64_t length = readVariableLengthValue(pptr, end, ok);
        if (!ok || (ptr + length > end))
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read SysEx event - Unexpected end of track "
                                            "data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.type = MidiEvent::EVENT_SYSEX;
        evt.data.clear();
        evt.data.push_back(byte);
        std::copy(ptr, ptr + length, std::back_inserter(evt.data));
        ptr += (size_t)length;
        return evt;
    }

    if (byte == MidiEvent::EVENT_SPECIAL)
    {
        // Special event FF
        uint8_t  evtype = *(ptr++);
        uint64_t length = readVariableLengthValue(pptr, end, ok);
        if (!ok || (ptr + length > end))
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read Special event - Unexpected end of "
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
        if (evt.sub_type == MidiEvent::EVENT_SUB_COPYRIGHT)
        {
            if (m_midi_music_copyright.empty())
            {
                m_midi_music_copyright = std::string((const char *)evt.data.data(), evt.data.size());
                m_midi_music_copyright.push_back('\0'); /* ending fix for UTF16 strings */
                if (m_midi_output_interface->onDebugMessage)
                    m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                            "Music copyright: %s", m_midi_music_copyright.c_str());
            }
            else if (m_midi_output_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                        "Extra copyright event: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::EVENT_SUB_TRACK_TITLE)
        {
            if (m_midi_music_title.empty())
            {
                m_midi_music_title = std::string((const char *)evt.data.data(), evt.data.size());
                m_midi_music_title.push_back('\0'); /* ending fix for UTF16 strings */
                if (m_midi_output_interface->onDebugMessage)
                    m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                            "Music title: %s", m_midi_music_title.c_str());
            }
            else
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_midi_music_track_titles.push_back(str);
                if (m_midi_output_interface->onDebugMessage)
                    m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                            "Track title: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::EVENT_SUB_INSTRUMENT_TITLE)
        {
            if (m_midi_output_interface->onDebugMessage)
            {
                std::string str((const char *)evt.data.data(), evt.data.size());
                str.push_back('\0'); /* ending fix for UTF16 strings */
                m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                        "Instrument: %s", str.c_str());
            }
        }
        else if (evt.sub_type == MidiEvent::EVENT_SUB_MARKER)
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
                evt.sub_type = MidiEvent::EVENT_SUB_LOOP_START;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if (data == "loopend")
            {
                // Return a custom Loop End event instead of Marker
                evt.sub_type = MidiEvent::EVENT_SUB_LOOP_END;
                evt.data.clear(); // Data is not needed
                return evt;
            }

            if (data.substr(0, 10) == "loopstart=")
            {
                evt.type      = MidiEvent::EVENT_SPECIAL;
                evt.sub_type  = MidiEvent::EVENT_SUB_LOOP_STACK_BEGIN;
                uint8_t loops = (uint8_t)(atoi(data.substr(10).c_str()));
                evt.data.clear();
                evt.data.push_back(loops);

                if (m_midi_output_interface->onDebugMessage)
                {
                    m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                            "Stack Marker Loop Start at %d to %d level with %d "
                                                            "loops",
                                                            m_midi_loop.m_stacklevel, m_midi_loop.m_stacklevel + 1,
                                                            loops);
                }
                return evt;
            }

            if (data.substr(0, 8) == "loopend=")
            {
                evt.type     = MidiEvent::EVENT_SPECIAL;
                evt.sub_type = MidiEvent::EVENT_SUB_LOOP_STACK_END;
                evt.data.clear();

                if (m_midi_output_interface->onDebugMessage)
                {
                    m_midi_output_interface->onDebugMessage(
                        m_midi_output_interface->onDebugMessage_userdata, "Stack Marker Loop %s at %d to %d level",
                        (evt.sub_type == MidiEvent::EVENT_SUB_LOOP_STACK_END ? "End" : "Break"),
                        m_midi_loop.m_stacklevel, m_midi_loop.m_stacklevel - 1);
                }
                return evt;
            }
        }

        if (evtype == MidiEvent::EVENT_SUB_END_TRACK)
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
    if (byte == MidiEvent::EVENT_SYSCOM_SELECT)
    {
        if (ptr + 1 > end)
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read System Command Song Select event - "
                                            "Unexpected end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }
        evt.type = byte;
        evt.data.push_back(*(ptr++));
        return evt;
    }

    // Sys Com Song Position Pntr [LSB, MSB]
    if (byte == MidiEvent::EVENT_SYSCOM_POSITION)
    {
        if (ptr + 2 > end)
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read System Command Position Pointer event "
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
    case MidiEvent::EVENT_NOTE_OFF: // 2 byte length
    case MidiEvent::EVENT_NOTE_ON:
    case MidiEvent::EVENT_NOTE_TOUCH:
    case MidiEvent::EVENT_CONTROL_CHANGE:
    case MidiEvent::EVENT_PITCH_WHEEL:
        if (ptr + 2 > end)
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read regular 2-byte event - Unexpected "
                                            "end of track data.\n";
            evt.is_valid = 0;
            return evt;
        }

        evt.data.push_back(*(ptr++));
        evt.data.push_back(*(ptr++));

        if ((evType == MidiEvent::EVENT_NOTE_ON) && (evt.data[1] == 0))
        {
            evt.type = MidiEvent::EVENT_NOTE_OFF; // Note ON with zero velocity
                                                  // is Note OFF!
        }
        else if (evType == MidiEvent::EVENT_CONTROL_CHANGE)
        {
            // 111'th loopStart controller (RPG Maker and others)
            switch (evt.data[0])
            {
            case 110:
                if (m_midi_loop_format == LOOP_DEFAULT)
                {
                    // Change event type to custom Loop Start event
                    // and clear data
                    evt.type     = MidiEvent::EVENT_SPECIAL;
                    evt.sub_type = MidiEvent::EVENT_SUB_LOOP_START;
                    evt.data.clear();
                    m_midi_loop_format = LOOP_HMI;
                }
                else if (m_midi_loop_format == LOOP_HMI)
                {
                    // Repeating of 110'th point is BAD practice,
                    // treat as EMIDI
                    m_midi_loop_format = LOOP_EMIDI;
                }
                break;

            case 111:
                if (m_midi_loop_format == LOOP_HMI)
                {
                    // Change event type to custom Loop End event
                    // and clear data
                    evt.type     = MidiEvent::EVENT_SPECIAL;
                    evt.sub_type = MidiEvent::EVENT_SUB_LOOP_END;
                    evt.data.clear();
                }
                else if (m_midi_loop_format != LOOP_EMIDI)
                {
                    // Change event type to custom Loop Start event
                    // and clear data
                    evt.type     = MidiEvent::EVENT_SPECIAL;
                    evt.sub_type = MidiEvent::EVENT_SUB_LOOP_START;
                    evt.data.clear();
                }
                break;

            case 113:
                if (m_midi_loop_format == LOOP_EMIDI)
                {
                    // EMIDI does using of CC113 with same purpose
                    // as CC7
                    evt.data[0] = 7;
                }
                break;
            }
        }

        return evt;
    case MidiEvent::EVENT_PATCH_CHANGE: // 1 byte length
    case MidiEvent::EVENT_CHANNEL_AFTERTOUCH:
        if (ptr + 1 > end)
        {
            m_midi_parsing_errors_string += "parseEvent: Can't read regular 1-byte event - Unexpected "
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
    if (track == 0 && m_midi_smf_format < 2 && evt.type == MidiEvent::EVENT_SPECIAL &&
        (evt.sub_type == MidiEvent::EVENT_SUB_TEMPO_CHANGE || evt.sub_type == MidiEvent::EVENT_SUB_TIME_SIGNATURE))
    {
        /* never reject track 0 timing events on SMF format != 2 */
    }
    else
    {
        if (m_midi_track_solo != ~(size_t)(0) && track != m_midi_track_solo)
            return;
        if (m_midi_track_disabled[track])
            return;
    }

    if (m_midi_output_interface->onEvent)
    {
        m_midi_output_interface->onEvent(m_midi_output_interface->onEvent_userdata, evt.type, evt.sub_type, evt.channel,
                                         evt.data.data(), evt.data.size());
    }

    if (evt.type == MidiEvent::EVENT_SYSEX || evt.type == MidiEvent::EVENT_SYSEX2) // Ignore SysEx
    {
        m_midi_output_interface->rt_systemExclusive(m_midi_output_interface->rtUserData, evt.data.data(),
                                                    evt.data.size());
        return;
    }

    if (evt.type == MidiEvent::EVENT_SPECIAL)
    {
        // Special event FF
        uint_fast16_t evtype = evt.sub_type;
        uint64_t      length = (uint64_t)(evt.data.size());
        const char   *data(length ? (const char *)(evt.data.data()) : "\0\0\0\0\0\0\0\0");

        if (m_midi_output_interface->rt_metaEvent) // Meta event hook
            m_midi_output_interface->rt_metaEvent(m_midi_output_interface->rtUserData, evtype, (const uint8_t *)(data),
                                                  size_t(length));

        if (evtype == MidiEvent::EVENT_SUB_END_TRACK) // End Of Track
        {
            status = -1;
            return;
        }

        if (evtype == MidiEvent::EVENT_SUB_TEMPO_CHANGE) // Tempo change
        {
            m_midi_tempo =
                m_midi_individual_tick_delta * MidiFraction(readIntBigEndian(evt.data.data(), evt.data.size()));
            return;
        }

        if (evtype == MidiEvent::EVENT_SUB_MARKER) // Meta event
        {
            // Do nothing! :-P
            return;
        }

        if (evtype == MidiEvent::EVENT_SUB_DEVICE_SWITCH)
        {
            if (m_midi_output_interface->onDebugMessage)
                m_midi_output_interface->onDebugMessage(m_midi_output_interface->onDebugMessage_userdata,
                                                        "Switching another device: %s", data);
            if (m_midi_output_interface->rt_deviceSwitch)
                m_midi_output_interface->rt_deviceSwitch(m_midi_output_interface->rtUserData, track, data,
                                                         size_t(length));
            return;
        }

        // Turn on Loop handling when loop is enabled
        if (m_midi_loop_enabled && !m_midi_loop.m_invalid_loop)
        {
            if (evtype == MidiEvent::EVENT_SUB_LOOP_START) // Special non-spec MIDI
                                                           // loop Start point
            {
                m_midi_loop.m_caught_start = true;
                return;
            }

            if (evtype == MidiEvent::EVENT_SUB_LOOP_END) // Special non-spec MIDI loop End point
            {
                m_midi_loop.m_caught_end = true;
                return;
            }

            if (evtype == MidiEvent::EVENT_SUB_LOOP_STACK_BEGIN)
            {
                if (m_midi_loop.m_skip_m_stackstart)
                {
                    m_midi_loop.m_skip_m_stackstart = false;
                    return;
                }

                char   x      = data[0];
                size_t slevel = (size_t)(m_midi_loop.m_stacklevel + 1);
                while (slevel >= m_midi_loop.m_stack.size())
                {
                    LoopStackEntry e;
                    e.loops    = x;
                    e.infinity = (x == 0);
                    e.start    = 0;
                    e.end      = 0;
                    m_midi_loop.m_stack.push_back(e);
                }

                LoopStackEntry &s                 = m_midi_loop.m_stack[slevel];
                s.loops                           = (int32_t)(x);
                s.infinity                        = (x == 0);
                m_midi_loop.m_caught_m_stackstart = true;
                return;
            }

            if (evtype == MidiEvent::EVENT_SUB_LOOP_STACK_END)
            {
                m_midi_loop.m_caught_m_stackend = true;
                return;
            }

            if (evtype == MidiEvent::EVENT_SUB_LOOP_STACK_BREAK)
            {
                m_midi_loop.m_caught_m_stackbreak = true;
                return;
            }
        }

        if (evtype == MidiEvent::EVENT_SUB_CALLBACK_TRIGGER)
        {
            if (m_midi_trigger_handler)
                m_midi_trigger_handler(m_midi_trigger_userdata, (uint32_t)(data[0]), track);
            return;
        }

        if (evtype == MidiEvent::EVENT_SUB_SONG_BEGIN_HOOK)
        {
            if (m_midi_output_interface->onSongStart)
                m_midi_output_interface->onSongStart(m_midi_output_interface->onSongStart_userdata);
            return;
        }

        return;
    }

    if (evt.type == MidiEvent::EVENT_SYSCOM_SELECT || evt.type == MidiEvent::EVENT_SYSCOM_POSITION)
        return;

    size_t midCh = evt.channel;
    if (m_midi_output_interface->rt_currentDevice)
        midCh += m_midi_output_interface->rt_currentDevice(m_midi_output_interface->rtUserData, track);
    status = evt.type;

    switch (evt.type)
    {
    case MidiEvent::EVENT_NOTE_OFF: // Note off
    {
        if (midCh < 16 && m_channeldisable[midCh])
            break;                  // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        if (m_midi_output_interface->rt_noteOff)
            m_midi_output_interface->rt_noteOff(m_midi_output_interface->rtUserData, (uint8_t)(midCh), note);
        if (m_midi_output_interface->rt_noteOffVel)
            m_midi_output_interface->rt_noteOffVel(m_midi_output_interface->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::EVENT_NOTE_ON: // Note on
    {
        if (midCh < 16 && m_channeldisable[midCh])
            break;                 // Disabled channel
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        m_midi_output_interface->rt_noteOn(m_midi_output_interface->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::EVENT_NOTE_TOUCH: // Note touch
    {
        uint8_t note = evt.data[0];
        uint8_t vol  = evt.data[1];
        m_midi_output_interface->rt_noteAfterTouch(m_midi_output_interface->rtUserData, (uint8_t)(midCh), note, vol);
        break;
    }

    case MidiEvent::EVENT_CONTROL_CHANGE: // Controller change
    {
        uint8_t ctrlno = evt.data[0];
        uint8_t value  = evt.data[1];
        m_midi_output_interface->rt_controllerChange(m_midi_output_interface->rtUserData, (uint8_t)(midCh), ctrlno,
                                                     value);
        break;
    }

    case MidiEvent::EVENT_PATCH_CHANGE: // Patch change
    {
        m_midi_output_interface->rt_patchChange(m_midi_output_interface->rtUserData, (uint8_t)(midCh), evt.data[0]);
        break;
    }

    case MidiEvent::EVENT_CHANNEL_AFTERTOUCH: // Channel after-touch
    {
        uint8_t chanat = evt.data[0];
        m_midi_output_interface->rt_channelAfterTouch(m_midi_output_interface->rtUserData, (uint8_t)(midCh), chanat);
        break;
    }

    case MidiEvent::EVENT_PITCH_WHEEL: // Wheel/pitch bend
    {
        uint8_t a = evt.data[0];
        uint8_t b = evt.data[1];
        m_midi_output_interface->rt_pitchBend(m_midi_output_interface->rtUserData, (uint8_t)(midCh), b, a);
        break;
    }

    default:
        break;
    } // switch
}

double MidiSequencer::Tick(double s, double granularity)
{
    assert(m_midi_output_interface); // MIDI output interface must be defined!

    s *= m_midi_tempo_multiplier;
    m_midi_current_position.wait -= s;
    m_midi_current_position.absolute_time_position += s;

    int32_t antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
    while ((m_midi_current_position.wait <= granularity * 0.5) && (antiFreezeCounter > 0))
    {
        if (!processEvents())
            break;
        if (m_midi_current_position.wait <= 0.0)
            antiFreezeCounter--;
    }

    if (antiFreezeCounter <= 0)
        m_midi_current_position.wait += 1.0; /* Add extra 1 second when over 10000 events
                                               with zero delay are been detected */

    if (m_midi_current_position.wait < 0.0)  // Avoid negative delay value!
        return 0.0;

    return m_midi_current_position.wait;
}

double MidiSequencer::Seek(double seconds, const double granularity)
{
    if (seconds < 0.0)
        return 0.0;                        // Seeking negative position is forbidden! :-P
    const double granualityHalf = granularity * 0.5,
                 s              = seconds; // m_setup.m_delay < m_setup.maxdelay ?
                                           // m_setup.m_delay : m_setup.maxdelay;

    /* Attempt to go away out of song end must rewind position to begin */
    if (seconds > m_midi_full_song_time_length)
    {
        this->Rewind();
        return 0.0;
    }

    bool loopFlagState = m_midi_loop_enabled;
    // Turn loop pooints off because it causes wrong position rememberin on a
    // quick seek
    m_midi_loop_enabled = false;

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
    m_midi_loop.m_caught_start = false;

    m_midi_loop.m_temporary_broken = (seconds >= m_midi_loop_end_time);

    while ((m_midi_current_position.absolute_time_position < seconds) &&
           (m_midi_current_position.absolute_time_position < m_midi_full_song_time_length))
    {
        m_midi_current_position.wait -= s;
        m_midi_current_position.absolute_time_position += s;
        int32_t    antiFreezeCounter = 10000; // Limit 10000 loops to avoid freezing
        double dstWait           = m_midi_current_position.wait + granualityHalf;
        while ((m_midi_current_position.wait <= granualityHalf) /*&& (antiFreezeCounter > 0)*/)
        {
            // std::fprintf(stderr, "wait = %g...\n", CurrentPosition.wait);
            if (!processEvents(true))
                break;
            // Avoid freeze because of no waiting increasing in more than 10000
            // cycles
            if (m_midi_current_position.wait <= dstWait)
                antiFreezeCounter--;
            else
            {
                dstWait           = m_midi_current_position.wait + granualityHalf;
                antiFreezeCounter = 10000;
            }
        }
        if (antiFreezeCounter <= 0)
            m_midi_current_position.wait += 1.0; /* Add extra 1 second when over 10000 events
                                                   with zero delay are been detected */
    }

    if (m_midi_current_position.wait < 0.0)
        m_midi_current_position.wait = 0.0;

    if (m_midi_at_end)
    {
        this->Rewind();
        m_midi_loop_enabled = loopFlagState;
        return 0.0;
    }

    m_midi_time.Reset();
    m_midi_time.m_delay = m_midi_current_position.wait;

    m_midi_loop_enabled = loopFlagState;
    return m_midi_current_position.wait;
}

double MidiSequencer::Tell()
{
    return m_midi_current_position.absolute_time_position;
}

double MidiSequencer::timeLength()
{
    return m_midi_full_song_time_length;
}

double MidiSequencer::getLoopStart()
{
    return m_midi_loop_start_time;
}

double MidiSequencer::getLoopEnd()
{
    return m_midi_loop_end_time;
}

void MidiSequencer::Rewind()
{
    m_midi_current_position = m_midi_track_begin_position;
    m_midi_at_end           = false;

    m_midi_loop.m_loops_count = m_midi_loop_count;
    m_midi_loop.Reset();
    m_midi_loop.m_caught_start     = true;
    m_midi_loop.m_temporary_broken = false;
    m_midi_time.Reset();
}

void MidiSequencer::setTempo(double tempo)
{
    m_midi_tempo_multiplier = tempo;
}

bool MidiSequencer::loadMidi(const uint8_t *data, size_t size)
{
    MEMFILE *mfr = mem_fopen_read((void *)data, size);
    return loadMidi(mfr);
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

bool MidiSequencer::loadMidi(MEMFILE *mfr)
{
    size_t fsize = 0;
    (void)(fsize);
    m_midi_parsing_errors_string.clear();

    assert(m_midi_output_interface); // MIDI output interface must be defined!

    m_midi_at_end = false;
    m_midi_loop.fullReset();
    m_midi_loop.m_caught_start = true;

    m_midi_smf_format = 0;

    const size_t headerSize            = 4 + 4 + 2 + 2 + 2; // 14
    char         headerBuf[headerSize] = "";

    fsize = mem_fread(headerBuf, headerSize, 1, mfr) * headerSize;
    if (fsize < headerSize)
    {
        m_midi_error_string = "Unexpected end of file at header!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    if (memcmp(headerBuf, "MThd\0\0\0\6", 8) == 0)
    {
        mem_fseek(mfr, 0, MEM_SEEK_SET);
        return _ParseSmf(mfr);
    }

    if (memcmp(headerBuf, "MUS\x1A", 4) == 0)
    {
        mem_fseek(mfr, 0, MEM_SEEK_SET);
        return _ParseMus(mfr);
    }

    m_midi_error_string = "Unknown or unsupported file format";
    mem_fclose(mfr);
    mfr = NULL;
    return false;
}

bool MidiSequencer::_ParseSmf(MEMFILE *mfr)
{
    const size_t                      headerSize            = 14; // 4 + 4 + 2 + 2 + 2
    char                              headerBuf[headerSize] = "";
    size_t                            fsize                 = 0;
    size_t                            deltaTicks = 192, TrackCount = 1;
    uint32_t                          smfFormat = 0;
    std::vector<std::vector<uint8_t>> rawTrackData;

    fsize = mem_fread(headerBuf, headerSize, 1, mfr) * headerSize;
    if (fsize < headerSize)
    {
        m_midi_error_string = "Unexpected end of file at header!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    if (memcmp(headerBuf, "MThd\0\0\0\6", 8) != 0)
    {
        m_midi_error_string = "MIDI Loader: Invalid format, MThd signature is not found!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    smfFormat  = (uint32_t)(readIntBigEndian(headerBuf + 8, 2));
    TrackCount = (size_t)(readIntBigEndian(headerBuf + 10, 2));
    deltaTicks = (size_t)(readIntBigEndian(headerBuf + 12, 2));

    if (smfFormat > 2)
        smfFormat = 1;

    rawTrackData.clear();
    rawTrackData.resize(TrackCount, std::vector<uint8_t>());
    m_midi_individual_tick_delta = MidiFraction(1, 1000000l * (uint64_t)(deltaTicks));
    m_midi_tempo                 = MidiFraction(1, (uint64_t)(deltaTicks) * 2);

    size_t totalGotten = 0;

    for (size_t tk = 0; tk < TrackCount; ++tk)
    {
        // Read track header
        size_t trackLength;

        fsize = mem_fread(headerBuf, 8, 1, mfr) * 8;
        if ((fsize < 8) || (memcmp(headerBuf, "MTrk", 4) != 0))
        {
            m_midi_error_string = "MIDI Loader: Invalid format, MTrk signature is not found!\n";
            mem_fclose(mfr);
            mfr = NULL;
            return false;
        }
        trackLength = (size_t)readIntBigEndian(headerBuf + 4, 4);

        // read track data
        rawTrackData[tk].resize(trackLength);
        fsize = mem_fread(&rawTrackData[tk][0], trackLength, 1, mfr) * trackLength;
        if (fsize < trackLength)
        {
            m_midi_error_string = "MIDI Loader: Unexpected file ending while getting raw track "
                                  "data!\n";
            mem_fclose(mfr);
            mfr = NULL;
            return false;
        }

        totalGotten += fsize;
    }

    for (size_t tk = 0; tk < TrackCount; ++tk)
        totalGotten += rawTrackData[tk].size();

    if (totalGotten == 0)
    {
        m_midi_error_string = "MIDI Loader: Empty track data";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    // Build new MIDI events table
    if (!buildSMFTrackData(rawTrackData))
    {
        m_midi_error_string = "MIDI Loader: MIDI data parsing error has occouped!\n" + m_midi_parsing_errors_string;
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    m_midi_smf_format        = smfFormat;
    m_midi_loop.m_stacklevel = -1;

    mem_fclose(mfr);
    mfr = NULL;

    return true;
}

bool MidiSequencer::_ParseMus(MEMFILE *mfr)
{
    const size_t         headerSize            = 14;
    char                 headerBuf[headerSize] = "";
    size_t               fsize                 = 0;
    BufferGuard<uint8_t> cvt_buf;

    fsize = mem_fread(headerBuf, headerSize, 1, mfr) * headerSize;
    if (fsize < headerSize)
    {
        m_midi_error_string = "Unexpected end of file at header!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    if (memcmp(headerBuf, "MUS\x1A", 4) != 0)
    {
        m_midi_error_string = "MIDI Loader: Invalid format, MUS\\x1A signature is not found!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    size_t mus_len = mem_fsize(mfr);

    mem_fseek(mfr, 0, MEM_SEEK_SET);
    Poco::Buffer<uint8_t> mus(mus_len);

    fsize = mem_fread(mus.begin(), mus_len, 1, mfr) * mus_len;
    if (fsize < mus_len)
    {
        m_midi_error_string = "Failed to read MUS file data!\n";
        mem_fclose(mfr);
        mfr = NULL;
        return false;
    }

    // Close source stream
    mem_fclose(mfr);
    mfr = NULL;
    mfr = nullptr;

    uint8_t *mid     = nullptr;
    uint32_t mid_len = 0;
    int32_t      m2mret  = ConvertMusToMidi(mus.begin(), (uint32_t)(mus_len), &mid, &mid_len, 0);

    if (m2mret < 0)
    {
        m_midi_error_string = "Invalid MUS/DMX data format!";
        if (mid)
            free(mid);
        return false;
    }
    cvt_buf.set(mid);

    // Open converted MIDI file
    mfr = mem_fopen_read(mid, (size_t)(mid_len));

    return _ParseSmf(mfr);
}