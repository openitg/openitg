#ifndef FFMPEG_HELPER
#define FFMPEG_HELPER

#define OITG_AV_PIXFMT_NAME avcodec::PixelFormat
#include "FFMpeg_Helper_Common.h"

class FFMpeg_Helper
{
public:
	avcodec::AVFormatContext *m_fctx;
    avcodec::AVCodecContext *m_cctx;
	avcodec::AVStream *m_stream;
	avcodec::AVPacket pkt;
	avcodec::AVFrame frame;

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
};
#endif // FFMPEG_HELPER
