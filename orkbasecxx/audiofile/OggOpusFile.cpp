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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "OggOpusFile.h"
#include "ConfigManager.h"

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                           (buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
                                     buf[base+2]=((val)>>16)&0xff; \
                                     buf[base+1]=((val)>>8)&0xff; \
                                     buf[base]=(val)&0xff; \
                                 }while(0)

#define PACKAGE_NAME "opus-tools"

void comment_pad(char **comments, int* length, int amount)
{
    if(amount>0){
        int i;
        int newlen;
        char* p=*comments;
        /*Make sure there is at least amount worth of padding free, and
            round up to the maximum that fits in the current ogg segments.*/
        newlen=(*length+amount+255)/255*255-1;
        p=(char*)realloc(p,newlen);
        if(p==NULL){
            fprintf(stderr,"realloc failed in comment_pad()\n");
            exit(1);
        }
        for(i=*length;i<newlen;i++)p[i]=0;
        *comments=p;
        *length=newlen;
    }
}

void comment_add(char **comments, int* length, char *tag, char *val)
{
    char* p=*comments;
    int vendor_length=readint(p, 8);
    int user_comment_list_length=readint(p, 8+4+vendor_length);
    int tag_len=(tag?strlen(tag)+1:0);
    int val_len=strlen(val);
    int len=(*length)+4+tag_len+val_len;

    p=(char*)realloc(p, len);
    if(p==NULL){
        fprintf(stderr, "realloc failed in comment_add()\n");
        exit(1);
    }

    writeint(p, *length, tag_len+val_len);      /* length of comment */
    if(tag){
        memcpy(p+*length+4, tag, tag_len);        /* comment tag */
        (p+*length+4)[tag_len-1] = '=';           /* separator */
    }
    memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
    writeint(p, 8+4+vendor_length, user_comment_list_length+1);
    *comments=p;
    *length=len;
}

static void comment_init(char **comments, int* length, const char *vendor_string)
{
    /*The 'vendor' field should be the actual encoding library used.*/
    int vendor_length=strlen(vendor_string);
    int user_comment_list_length=0;
    int len=8+4+vendor_length+4;
    char *p=(char*)malloc(len);
    if(p==NULL){
        fprintf(stderr, "malloc failed in comment_init()\n");
        exit(1);
    }
    memcpy(p, "OpusTags", 8);
    writeint(p, 8, vendor_length);
    memcpy(p+12, vendor_string, vendor_length);
    writeint(p, 12+vendor_length, user_comment_list_length);
    *length=len;
    *comments=p;
}

OggOpusFile::OggOpusFile()
{
    nb_encoded=0;
    bytes_written=0;
    pages_out=0;
    total_bytes=0;
    total_samples=0;
    nbBytes;
    peak_bytes=0;
    min_bytes;
    start_time;
    stop_time;
    bitrate=-1;
    rate=8000;  //pcm rate
    coding_rate=8000;
    frame_size=960;
    chan=1;
    with_hard_cbr=0;
    with_cvbr=0;
    expect_loss=0;
    complexity=10;
    downmix=0;
    opt_ctls=0;
    max_ogg_delay=48000; /*48kHz samples*/
    seen_file_icons=0;
    comment_padding=512;
    lookahead=0;
    original_samples =0;
    opt_ctls_ctlval=NULL;
    inopt.channels=chan;
    inopt.rate=coding_rate=rate;
    last_granulepos=0;
    enc_granulepos=0;
    last_segments=0;
    /* 0 dB gain is recommended unless you know what you're doing */
    inopt.gain=0;
    inopt.samplesize=16;
    inopt.endianness=0;
    inopt.rawmode=0;
    inopt.ignorelength=0;
    inopt.copy_comments=1;
    inopt.copy_pictures=1;
    start_time = time(NULL);
    srand(((getpid()&65535)<<15)^start_time);
    serialno=rand();
    //For future use
    // if(rate>24000)coding_rate=48000;
    // else if(rate>16000)coding_rate=24000;
    // else if(rate>12000)coding_rate=16000;
    // else if(rate>8000)coding_rate=12000;
    // else coding_rate=8000;    //this will be the value

    memset(&m_pcmBuf, 0, 32000);
    memset(&m_extraPcmBuf, 0, 320);
    m_bytesOffsetFromLastFullFrame = 0;
    m_extraSamplesFromLastChunk = 0;
    m_extraPcmBufLen = 0;
    m_outBuf=NULL;
    st= NULL;
}

OggOpusFile::~OggOpusFile()
{
    //Don't trigger cleanup if Init() was not called.
    if(m_outBuf) free(m_outBuf);
    if(st) opus_multistream_encoder_destroy(st);
    ogg_stream_clear(&os);
    if(opt_ctls) {
        if(opt_ctls_ctlval) free(opt_ctls_ctlval);
    }
} 

void OggOpusFile::Open(CStdString& filename, fileOpenModeEnum mode, bool stereo, int sampleRate)
{
	if(m_sampleRate == 0)
	{
		m_sampleRate = sampleRate;
	}

	if(!m_filename.Equals(filename))
	{
		m_filename = filename + GetExtension();
	}
	m_mode = mode;
	if (mode == READ)
	{
        fout.open(m_filename, std::fstream::in | std::fstream::binary);
	}
	else
	{
		FileRecursiveMkdir(m_filename, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPathMcf);
        apr_status_t ret;
        fout.open(m_filename, std::fstream::out | std::fstream::binary);
		if(CONFIG.m_audioFilePermissions)
		{
			FileSetPermissions(m_filename, CONFIG.m_audioFilePermissions);
		}

		if(CONFIG.m_audioFileGroup.size() && CONFIG.m_audioFileOwner.size())
		{
			FileSetOwnership(m_filename, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup);
		}
	}
	if(!fout.is_open())
	{
		throw(CStdString("Could not open file: ") + m_filename);
	}

    Init();
    WriteHeader();
    //Close();

}

void OggOpusFile::Init()
{
    inopt.skip=0;
    //In our case, its 160 samples/frame. Changing rate to anything rather than 8000Hz may require significant change in the way of handle number of samples to fix the encoder
    frame_size=frame_size/(48000/coding_rate);  
    /*OggOpus headers*/ /*FIXME: broke forcemono*/
    header.channels=chan;
    header.channel_mapping=255;//header.channels>8?255:chan>2;  //=0 for wav //255 for separate channels.
    header.input_sample_rate=rate;
    header.gain=inopt.gain;   //=0 here

    st=opus_multistream_surround_encoder_create(coding_rate, chan, header.channel_mapping, &header.nb_streams, &header.nb_coupled,
        header.stream_map, frame_size<480/(48000/coding_rate)?OPUS_APPLICATION_RESTRICTED_LOWDELAY:OPUS_APPLICATION_AUDIO, &ret);
    if(ret!=OPUS_OK){
        fprintf(stderr, "Error cannot create encoder: %s\n", opus_strerror(ret));
        exit(1);
    }
    
    max_frame_bytes=(1275*3+7)*header.nb_streams;
    min_bytes=max_frame_bytes;
    m_outBuf = (unsigned char*)malloc(max_frame_bytes);
    if(m_outBuf==NULL){
        fprintf(stderr,"Error allocating m_outBuf buffer.\n");
        exit(1);
    }

    //We would set bitrate configurable as low as 6k
    //Or use the provided formulae to calculate a optimized bitrate
    if(bitrate<0){
    /*Lower default rate for sampling rates [8000-44100) by a factor of (rate+16k)/(64k)*/
    // bitrate=((64000*header.nb_streams+32000*header.nb_coupled)*
    //             (IMIN(48,IMAX(8,((rate<44100?rate:48000)+1000)/1000))+16)+32)>>6;
        bitrate=6000;
        bitrate = CONFIG.m_audioFileBitRate;
    }

    if(bitrate>(1024000*chan)||bitrate<500){
        fprintf(stderr,"Error: Bitrate %d bits/sec is insane.\nDid you mistake bits for kilobits?\n",bitrate);
        fprintf(stderr,"--bitrate values from 6-256 kbit/sec per channel are meaningful.\n");
        exit(1);
    }
    bitrate=IMIN(chan*256000,bitrate);

    ret=opus_multistream_encoder_ctl(st, OPUS_SET_BITRATE(bitrate));
    if(ret!=OPUS_OK){
        fprintf(stderr,"Error OPUS_SET_BITRATE returned: %s\n",opus_strerror(ret));
        exit(1);
    }

    ret=opus_multistream_encoder_ctl(st, OPUS_SET_VBR(!with_hard_cbr));
    if(ret!=OPUS_OK){
        fprintf(stderr,"Error OPUS_SET_VBR returned: %s\n",opus_strerror(ret));
        exit(1);
    }

    if(!with_hard_cbr){
        ret=opus_multistream_encoder_ctl(st, OPUS_SET_VBR_CONSTRAINT(with_cvbr));
        if(ret!=OPUS_OK){
            fprintf(stderr,"Error OPUS_SET_VBR_CONSTRAINT returned: %s\n",opus_strerror(ret));
            exit(1);
        }
    }
    ret=opus_multistream_encoder_ctl(st, OPUS_SET_COMPLEXITY(complexity));
    if(ret!=OPUS_OK){
        fprintf(stderr,"Error OPUS_SET_COMPLEXITY returned: %s\n",opus_strerror(ret));
        exit(1);
    }

    ret=opus_multistream_encoder_ctl(st, OPUS_SET_PACKET_LOSS_PERC(expect_loss));
    if(ret!=OPUS_OK){
        fprintf(stderr,"Error OPUS_SET_PACKET_LOSS_PERC returned: %s\n",opus_strerror(ret));
        exit(1);
    }

#ifdef OPUS_SET_LSB_DEPTH
    ret=opus_multistream_encoder_ctl(st, OPUS_SET_LSB_DEPTH(IMAX(8,IMIN(24,inopt.samplesize))));
    if(ret!=OPUS_OK){
        fprintf(stderr,"Warning OPUS_SET_LSB_DEPTH returned: %s\n",opus_strerror(ret));
    }
#endif

    for(i=0;i<opt_ctls;i++){
    int target=opt_ctls_ctlval[i*3];
    if(target==-1){
        ret=opus_multistream_encoder_ctl(st,opt_ctls_ctlval[i*3+1],opt_ctls_ctlval[i*3+2]);
        if(ret!=OPUS_OK){
        fprintf(stderr,"Error opus_multistream_encoder_ctl(st,%d,%d) returned: %s\n",opt_ctls_ctlval[i*3+1],opt_ctls_ctlval[i*3+2],opus_strerror(ret));
        exit(1);
        }
    }else if(target<header.nb_streams){
        OpusEncoder *oe;
        opus_multistream_encoder_ctl(st,OPUS_MULTISTREAM_GET_ENCODER_STATE(target,&oe));
        ret=opus_encoder_ctl(oe, opt_ctls_ctlval[i*3+1],opt_ctls_ctlval[i*3+2]);
        if(ret!=OPUS_OK){
        fprintf(stderr,"Error opus_encoder_ctl(st[%d],%d,%d) returned: %s\n",target,opt_ctls_ctlval[i*3+1],opt_ctls_ctlval[i*3+2],opus_strerror(ret));
        exit(1);
        }
    }else{
        fprintf(stderr,"Error --set-ctl-int target stream %d is higher than the maximum stream number %d.\n",target,header.nb_streams-1);
        exit(1);
    }
    }


    /*We do the lookahead check late so user CTLs can change it*/
    ret=opus_multistream_encoder_ctl(st, OPUS_GET_LOOKAHEAD(&lookahead));
    if(ret!=OPUS_OK){
    fprintf(stderr,"Error OPUS_GET_LOOKAHEAD returned: %s\n",opus_strerror(ret));
    exit(1);
    }
    inopt.skip+=lookahead;
    /*Regardless of the rate we're coding at the ogg timestamping/skip is
    always timed at 48000.*/
    header.preskip=inopt.skip*(48000./coding_rate);
    /* Extra samples that need to be read to compensate for the pre-skip */
    inopt.extraout=(int)header.preskip*(rate/48000.);

    /*Initialize Ogg stream struct*/
    if(ogg_stream_init(&os, serialno)==-1){
    fprintf(stderr,"Error: stream init failed\n");
    exit(1);
    }



    int opus_app;
    fprintf(stderr,"Encoding using 1.1");
    opus_multistream_encoder_ctl(st,OPUS_GET_APPLICATION(&opus_app));
    if(opus_app==OPUS_APPLICATION_VOIP)fprintf(stderr," (VoIP)\n");
    else if(opus_app==OPUS_APPLICATION_AUDIO)fprintf(stderr," (audio)\n");
    else if(opus_app==OPUS_APPLICATION_RESTRICTED_LOWDELAY)fprintf(stderr," (low-delay)\n");
    else fprintf(stderr," (unknown)\n");
    fprintf(stderr,"-----------------------------------------------------\n");
    fprintf(stderr,"   Input: %0.6gkHz %d channel%s\n",
            header.input_sample_rate/1000.,chan,chan<2?"":"s");
    fprintf(stderr,"  Output: %d channel%s (",header.channels,header.channels<2?"":"s");
    if(header.nb_coupled>0)fprintf(stderr,"%d coupled",header.nb_coupled*2);
    if(header.nb_streams-header.nb_coupled>0)fprintf(stderr,
       "%s%d uncoupled",header.nb_coupled>0?", ":"",
       header.nb_streams-header.nb_coupled);
    fprintf(stderr,")\n          %0.2gms packets, %0.6gkbit/sec%s\n",
       frame_size/(coding_rate/1000.), bitrate/1000.,
       with_hard_cbr?" CBR":with_cvbr?" CVBR":" VBR");
    fprintf(stderr," Preskip: %d\n",header.preskip);
  

}

void OggOpusFile::WriteHeader()
{  
    /*The Identification Header is 19 bytes, plus a Channel Mapping Table for
    mapping families other than 0. The Channel Mapping Table is 2 bytes +
    1 byte per channel. Because the maximum number of channels is 255, the
    maximum size of this header is 19 + 2 + 255 = 276 bytes.*/
    unsigned char header_data[276];
    const char         *opus_version;
    opus_version=opus_get_version_string();
    comment_init(&inopt.comments, &inopt.comments_length, opus_version);
//    snprintf(ENCODER_string, sizeof(ENCODER_string), "opusenc from %s %s",PACKAGE_NAME,"1.5");
    CStdString encode_string;
    encode_string.Format("opusenc from %s 1.5", PACKAGE_NAME);
    strcpy(ENCODER_string, encode_string.c_str());
    comment_add(&inopt.comments, &inopt.comments_length, "ENCODER", ENCODER_string);

    int packet_size=opus_header_to_packet(&header, header_data, sizeof(header_data));
    op.packet=header_data;
    op.bytes=packet_size;
    op.b_o_s=1;
    op.e_o_s=0;
    op.granulepos=0;
    op.packetno=0;
    ogg_stream_packetin(&os, &op);

    while((ret=ogg_stream_flush(&os, &og))){
        if(!ret)break;
        
        ret=oe_write_page(&og);
        if(ret!=og.header_len+og.body_len){
            fprintf(stderr,"Error: failed writing header to output stream\n");
            exit(1);
        }
        bytes_written+=ret;
        pages_out++;
    }

    comment_pad(&inopt.comments, &inopt.comments_length, comment_padding);
    op.packet=(unsigned char *)inopt.comments;
    op.bytes=inopt.comments_length;
    op.b_o_s=0;
    op.e_o_s=0;
    op.granulepos=0;
    op.packetno=1;
    ogg_stream_packetin(&os, &op);
    
    /* writing the rest of the Opus header packets */
    while((ret=ogg_stream_flush(&os, &og))){
        if(!ret)break;
        ret=oe_write_page(&og);
        if(ret!=og.header_len + og.body_len){
            fprintf(stderr,"Error: failed writing header to output stream\n");
            exit(1);
        }
            bytes_written+=ret;
            pages_out++;
    }    
    free(inopt.comments);
}

/*Write an Ogg page to a file pointer*/
int OggOpusFile::oe_write_page(ogg_page *page)
{
    int writtenHdr =page->header_len;
    fout.write((char*)page->header, writtenHdr);
    int writtenBody = page->body_len;
    fout.write((char*)page->body, writtenBody);
    return (writtenHdr+writtenBody);
}

//Description: 
//A chunk has X samples
//A sample is a short(2 bytes)
//Stereo: L/R is interlaced. A sample will become 2 short(4 bytes) in stereo buffer. Therefore stereo buffer will be double in size
//Opus encoder is configured to take multiple of 160(x160) samples

void OggOpusFile::WriteChunk(AudioChunkRef chunkRef)
{
	if(chunkRef.get() == NULL) return;

	if(chunkRef->GetDetails()->m_numBytes == 0) return;

    if(!fout.is_open()) return;

    int numSamples = chunkRef->GetNumSamples() + m_extraSamplesFromLastChunk;   // total number of samples, this chunk + bytes from previous chunk which was cut out because it wont make into frames (multiple of 160 samples)
    if(numSamples < PCM_SAMPLES_IN_FRAME)
    {
        //Too less samples, store them in extraPcmBuf
        if(chunkRef->m_numChannels > 0 && chan == 2)
        {
            short *wrPtr = NULL;
            wrPtr = (short*)&m_extraStreoPcmBuf[m_extraPcmBufLen];
            for(int x = 0; x < chunkRef->GetNumSamples(); x++)
            {
                for(int i = 0; i < chunkRef->m_numChannels; i++)
                {
                    *wrPtr++ = (short)*((short*)(chunkRef->m_pChannelAudio[i])+x);
                }
            }

            m_extraPcmBufLen += chunkRef->GetNumSamples()*4;
        }
        else
        {
            memcpy(&m_extraPcmBuf[m_extraPcmBufLen], chunkRef->m_pBuffer, chunkRef->GetNumBytes());
            m_extraPcmBufLen += chunkRef->GetNumBytes();
        }
        m_extraSamplesFromLastChunk = numSamples + m_extraSamplesFromLastChunk;
        return;
    }

     int extraBytesPos = 0;  //The index of the chunkRef->m_pBuffer that will be put into the extra buffer. The bytes before this index will combine with bytes already in extra buffer into main buffer
    //Opus encoder take 160 samples at a time only. Frame=160samples
    int numFullFrames = numSamples/PCM_SAMPLES_IN_FRAME;     //number of complete frame(multiple of 160) from this chunk + previous chunks(if any)
    int numSamplesInFullFrames = numFullFrames*PCM_SAMPLES_IN_FRAME; //number of samples in all compelete frames, can be differ from total number of samples
    int numSamplesExtra = numSamples - numSamplesInFullFrames;  //number of samples did not make into complete frame, will combine with next chunk's samples
    int numBytesInFullFrames = numSamplesInFullFrames*2;
    int numBytesExtra = numSamplesExtra*2;
   
    extraBytesPos = chunkRef->GetNumBytes() - numBytesExtra;    //Index in chunk pcm payload bytes no longer fix to pcmBuf(multiple of 160 samples), from here samples will be put into extraBuf
    int extraSamplesPos = extraBytesPos/2;  //index in chunk pcm payload samples no longer fix to pcmBuf, from here samples will be put into extraBuf

    if(m_extraSamplesFromLastChunk > 0)
    {
        //This chunk has/accumulated more than x160 samples
        //If the ExtraPcmBuf has samples from previous chunk, put them into the first available space of main pcmBuf
        //Put the extra samples/bytes into extraBuf
        if(chunkRef->m_numChannels > 0 && chan == 2)
        {
            memcpy(m_pcmBuf, m_extraStreoPcmBuf, m_extraPcmBufLen);
            short *wrPtr = NULL;
            wrPtr = (short*)&m_pcmBuf[m_extraPcmBufLen];
            for(int x = 0; x < extraSamplesPos; x++)
            {
                for(int i = 0; i < chunkRef->m_numChannels; i++)
                {
                    *wrPtr++ = (short)*((short*)(chunkRef->m_pChannelAudio[i])+x);
                }
            }
            memset(m_extraStreoPcmBuf, 0, m_extraPcmBufLen);
        }
        else
        {
            memcpy(m_pcmBuf, m_extraPcmBuf, m_extraPcmBufLen);
            memcpy(&m_pcmBuf[m_extraPcmBufLen], chunkRef->m_pBuffer, extraBytesPos); 
            memset(m_extraPcmBuf, 0, m_extraPcmBufLen); 
        }
       
        m_extraPcmBufLen = 0;
        
    }
    else
    {
        //Put chunk pcm payload x160 samples to the main buffer, the rest will be put in extraPcmBuf in next step
        if(chunkRef->m_numChannels > 0 && chan == 2)
        {
            short *wrPtr = NULL;
            wrPtr = (short*)&m_pcmBuf;
            for(int x = 0; x < numBytesInFullFrames/2; x++)  //chunkRef->GetNumSamples()
            {
                for(int i = 0; i < chunkRef->m_numChannels; i++)
                {
                    *wrPtr++ = (short)*((short*)(chunkRef->m_pChannelAudio[i])+x);
                }
            }
        }
        else
        {
            memcpy(m_pcmBuf, chunkRef->m_pBuffer, numBytesInFullFrames); //would be chunkRef->GetNumBytes
        }       
    }
    if(numSamplesExtra > 0)
    {
        //Extra bytes/samples more than x160 samples will be put in extraPcmBuf
        if(chunkRef->m_numChannels > 0 && chan == 2)
        {
            short *wrPtr = NULL;
            wrPtr = (short*)&m_extraStreoPcmBuf;
            for(int x = extraSamplesPos; x < (extraSamplesPos+numBytesExtra/2); x++)
            {
                for(int i = 0; i < chunkRef->m_numChannels; i++)
                {
                    *wrPtr++ = (short)*((short*)(chunkRef->m_pChannelAudio[i])+x);
                }
            }
            m_extraPcmBufLen += numBytesExtra*2;
        }
        else
        {
            memcpy(m_extraPcmBuf, (char*)chunkRef->m_pBuffer + extraBytesPos, numBytesExtra);     //(char*) for VC++ compilation
            m_extraPcmBufLen += numBytesExtra;
        }      
    }
    //update "last" number for next chunk to signal extraPcmBuf has chunk to move to main buffer before the arrival chunk payload
    m_extraSamplesFromLastChunk = numSamplesExtra;

    bool lastChunk = false;
    if(chunkRef->GetDetails()->m_marker == MEDIA_CHUNK_EOS_MARKER)
    {
        lastChunk = true;
    } 
    
    EncodeChunks(&m_pcmBuf, numSamplesInFullFrames, lastChunk); 
}

int OggOpusFile::ReadChunkMono(AudioChunkRef& chunk)
{
	unsigned int numRead = 0;

	return numRead;
}

void OggOpusFile::Close()
{
    if(m_extraPcmBufLen > 0)
    {
        //Need to pad the buffer to full 160 samples(1 sample=short*chan)
        if(m_extraPcmBufLen < 320*chan)
        {
            for(int i = m_extraPcmBufLen; i < 320*chan; i++)
            {
                m_extraPcmBuf[i] = char(0);
            }
        }
        EncodeChunks(&m_extraPcmBuf, PCM_SAMPLES_IN_FRAME, true);  
    }
    if(fout.is_open())
    {
        fout.close();
    }
    
}

CStdString OggOpusFile::GetExtension()
{
	return ".opus";
}

void OggOpusFile::SetNumOutputChannels(int numChan)
{
    chan = numChan;
}

#ifndef CENTOS_6

void OggOpusFile::EncodeChunks(void* pcmBuf, int numSamples, bool lastChunk)
{
    int numFrames = numSamples/PCM_SAMPLES_IN_FRAME;
    bool flush = false;
    for(int j = 0; j < numFrames; j++)
    {        
        if(j == (numFrames -1) && (lastChunk == true))
        {
            flush =true;
        }
        nb_samples=PCM_SAMPLES_IN_FRAME;//make it 160
        id++;
        int size_segments,cur_frame_size;
        ////frame_size=160 for 8k
        cur_frame_size=frame_size;

        /*Encode current frame*/          
        //Stereo: each pcm samples will have 2 short samples(L/R interlaced) so the num of bytes for each frame will be double to mono
        nbBytes=opus_multistream_encode	(st, (short*)pcmBuf + (j*PCM_SAMPLES_IN_FRAME*chan), cur_frame_size, m_outBuf, max_frame_bytes);
        if(nbBytes<0){
            fprintf(stderr, "Encoding failed: %s. Aborting.\n", opus_strerror(nbBytes));
            return;
        }

        nb_encoded+=cur_frame_size;
        enc_granulepos+=cur_frame_size*48000/coding_rate;
        total_bytes+=nbBytes;
        size_segments=(nbBytes+255)/255;
        peak_bytes=IMAX(nbBytes,peak_bytes);
        min_bytes=IMIN(nbBytes,min_bytes);                      
        //printf("TotalByes:%d\n", total_bytes);

        /*Flush early if adding this packet would make us end up with a
            continued page which we wouldn't have otherwise.*/
        while((((size_segments<=255)&&(last_segments+size_segments>255))|| (enc_granulepos-last_granulepos>max_ogg_delay)) 
                && ogg_stream_flush_fill(&os, &og,255*255)){
            if(ogg_page_packets(&og)!=0)  last_granulepos=ogg_page_granulepos(&og);

            last_segments-=og.header[26];
            ret=oe_write_page(&og);
            if(ret!=og.header_len+og.body_len){
                fprintf(stderr,"Error: failed writing data to output stream\n");
                exit(1);
            }
            bytes_written+=ret;
            pages_out++;
        }

        /*The downside of early reading is if the input is an exact
        multiple of the frame_size you'll get an extra frame that needs
        to get cropped off. The downside of late reading is added delay.
        If your ogg_delay is 120ms or less we'll assume you want the
        low delay behavior.*/
        // if(max_ogg_delay>5760){
        //     nb_samples = inopt.read_samples(inopt.readdata,input,frame_size);
        //     total_samples+=nb_samples;
        //     if(nb_samples==0)op.e_o_s=1;
        // } else nb_samples=-1;

        op.packet=(unsigned char *)m_outBuf;
        op.bytes=nbBytes;
        op.b_o_s=0;
        op.granulepos=enc_granulepos;
        if(flush == true){
            /*We compute the final GP as ceil(len*48k/input_rate)+preskip. When a
            resampling decoder does the matching floor((len-preskip)*input_rate/48k)
            conversion, the resulting output length will exactly equal the original
            input length when 0<input_rate<=48000.*/
            op.granulepos=((original_samples*48000+rate-1)/rate)+header.preskip;
        }
        op.packetno=2+id;
        ogg_stream_packetin(&os, &op);
        last_segments+=size_segments;

        /*If the stream is over or we're sure that the delayed flush will fire,
            go ahead and flush now to avoid adding delay.*/
        while(((flush == true) || (enc_granulepos+(frame_size*48000/coding_rate)-last_granulepos>max_ogg_delay)||
                (last_segments>=255))?
                ogg_stream_flush_fill(&os, &og,255*255):
                ogg_stream_pageout_fill(&os, &og,255*255)){

            if(ogg_page_packets(&og)!=0)last_granulepos=ogg_page_granulepos(&og);
            last_segments-=og.header[26];
            ret=oe_write_page(&og);
            if(ret!=og.header_len+og.body_len){
                fprintf(stderr,"Error: failed writing data to output stream\n");
                exit(1);
            }
            bytes_written+=ret;
            pages_out++;
        }
    }
}

#else
void OggOpusFile::EncodeChunks(void* pcmBuf, int numSamples, bool lastChunk)
{
    ;
}
#endif
