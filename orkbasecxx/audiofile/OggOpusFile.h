/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __OGGOPUSFILE_H__
#define __OGGOPUSFILE_H__

#include <opus_types.h>
//#include <ogg/ogg.h>
#include <opus.h>
#include <opus_multistream.h>
#include "opus_header.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(X) gettext(X)
#else
#define _(X) (X)
#define textdomain(X)
#define bindtextdomain(X, Y)
#endif
#ifdef gettext_noop
#define N_(X) gettext_noop(X)
#else
#define N_(X) (X)
#endif

#include "StdString.h"
#include "AudioCapture.h"
#include "sndfile.h"
#include "AudioFile.h"
#include "Utils.h"
#include <fstream>

#define MAX_FRAME_BYTES 61295
#define IMIN(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum int value.   */
#define IMAX(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum int value.   */


#define PCM_SAMPLES_IN_FRAME 160

typedef struct
{
    void *readdata;
    opus_int64 total_samples_per_channel;
    int rawmode;
    int channels;
    long rate;
    int gain;
    int samplesize;
    int endianness;
    char *infilename;
    int ignorelength;
    int skip;
    int extraout;
    char *comments;
    int comments_length;
    int copy_comments;
    int copy_pictures;
} oe_enc_opt;


class DLL_IMPORT_EXPORT_ORKBASE OggOpusFile : public AudioFile
{
public:
	OggOpusFile();
	~OggOpusFile();

    ogg_page *page;
	void Open(CStdString& filename, fileOpenModeEnum mode, bool stereo = false, int sampleRate = 8000);
	void Close();

	void WriteChunk(AudioChunkRef chunkRef);
	int ReadChunkMono(AudioChunkRef& chunk);

	CStdString GetExtension();
	void SetNumOutputChannels(int numChan);
private:
    int oe_write_page(ogg_page *page);
    void Init();
    void WriteHeader();
    void EncodeChunks(void* pcmBuf, int numSamples, bool lastChunk);

    int i, ret;
    OpusMSEncoder      *st;
    const char         *opus_version;
    unsigned char      *m_outBuf;
    float              *input;
    /*I/O*/
    oe_enc_opt         inopt;
    //const input_format *in_format;
    char               *inFile;
    char               *outFile;
    char               *range_file;
    std::fstream               fout;
    ogg_stream_state   os;
    ogg_page           og;
    ogg_packet         op;
    ogg_int64_t        last_granulepos;
    ogg_int64_t        enc_granulepos;
    ogg_int64_t        original_samples;
    ogg_int32_t        id;
    int                last_segments;
    OpusHeader         header;
    char               ENCODER_string[1024];
    /*Counters*/
    opus_int64         nb_encoded;
    opus_int64         bytes_written;
    opus_int64         pages_out;
    opus_int64         total_bytes;
    opus_int64         total_samples;
    opus_int32         nbBytes;
    opus_int32         nb_samples;
    opus_int32         peak_bytes;
    opus_int32         min_bytes;
    time_t             start_time;
    time_t             stop_time;
    time_t             last_spin;
    int                last_spin_len;
    /*Settings*/
    int                quiet;
    int                max_frame_bytes;
    opus_int32         bitrate;
    opus_int32         rate;
    opus_int32         coding_rate;
    opus_int32         frame_size;
    int                chan;
    int                with_hard_cbr;
    int                with_cvbr;
    int                expect_loss;
    int                complexity;
    int                downmix;
    int                *opt_ctls_ctlval;
    int                opt_ctls;
    int                max_ogg_delay; /*48kHz samples*/
    int                seen_file_icons;
    int                comment_padding;
    int                serialno;
    opus_int32         lookahead;

    unsigned char m_pcmBuf[64000];
    unsigned char m_extraPcmBuf[PCM_SAMPLES_IN_FRAME*2]; //samples from previous chunks do not fix into frames(multiple of 160*2) 
    unsigned char m_extraStreoPcmBuf[PCM_SAMPLES_IN_FRAME*4];
    int m_bytesOffsetFromLastFullFrame;
    int m_extraSamplesFromLastChunk;
    int m_extraPcmBufLen;
};

#endif

 
