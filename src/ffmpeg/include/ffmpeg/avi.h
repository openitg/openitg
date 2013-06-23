#ifndef FFMPEG_AVI_H
#define FFMPEG_AVI_H

#include "avcodec.h"

#define AVIF_HASINDEX		0x00000010	// Index at end of file?
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_TRUSTCKTYPE	0x00000800	// Use CKType to find key frames?
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

#define AVI_MAX_RIFF_SIZE       0x40000000LL
#define AVI_MASTER_INDEX_SIZE   256

/* index flags */
#define AVIIF_INDEX             0x10

offset_t start_tag(ByteIOContext *pb, const char *tag);
void end_tag(ByteIOContext *pb, offset_t start);

typedef struct CodecTag {
    int id;
    unsigned int tag;
    unsigned int invalid_asf : 1;
} CodecTag;

void put_bmp_header(ByteIOContext *pb, AVCodecContext *enc, const CodecTag *tags, int for_asf);
int put_wav_header(ByteIOContext *pb, AVCodecContext *enc);
int wav_codec_get_id(unsigned int tag, int bps);
void get_wav_header(ByteIOContext *pb, AVCodecContext *codec, int size); 

extern const CodecTag codec_bmp_tags[];
extern const CodecTag codec_wav_tags[];

unsigned int codec_get_tag(const CodecTag *tags, int id);
enum CodecID codec_get_id(const CodecTag *tags, unsigned int tag);
unsigned int codec_get_bmp_tag(int id);
unsigned int codec_get_wav_tag(int id);
enum CodecID codec_get_bmp_id(unsigned int tag);
enum CodecID codec_get_wav_id(unsigned int tag);

#endif /* FFMPEG_AVI_H */
