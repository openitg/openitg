/* vim: set noet sw=4 ts=4: */
#ifndef FFMPEG_HELPER
#define FFMPEG_HELPER

#define OITG_AV_PIXFMT_NAME avcodec::AVPixelFormat
#include "FFMpeg_Helper_Common.h"
#include "RageFile.h"

#define FFCTX_BUFFER_SIZE 4096

class FFMpeg_Helper
{
public:
	avcodec::AVPacket pkt;
	avcodec::AVFrame *frame;

	float LastFrameDelay;
	int FrameNumber;
	int m_width;
	int m_height;

	float pts;

	int current_packet_offset;


	FFMpeg_Helper();
	~FFMpeg_Helper();
	int GetFrame();
	void Init();
	void RegisterProtocols();
	CString Open(CString what);
	void Close();
	void RenderFrame(RageSurface *img, int tex_fmt);
	float GetTimestamp() const;
	int GetRawFrameNumber();

private:
	/* 0 = no EOF
	 * 1 = EOF from ReadPacket
	 * 2 = EOF from ReadPacket and DecodePacket */
	int eof;

	bool GetNextTimestamp;
	float CurrentTimestamp, Last_IP_Timestamp;

	int ReadPacket();
	int DecodePacket();
	float TimestampOffset;

	RageFile *m_pFile;
	avcodec::AVIOContext *m_ioctx;
	avcodec::AVFormatContext *m_fctx;
	avcodec::AVCodecContext *m_cctx;
	avcodec::AVStream *m_stream;
	avcodec::SwsContext *m_swsctx;
};
#endif // FFMPEG_HELPER
