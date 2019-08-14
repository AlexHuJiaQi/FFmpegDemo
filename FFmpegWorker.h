#pragma once

#include <QObject>
#include <QDebug>
#include <QThread>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

class FFmpegWorker :public QObject
{
	Q_OBJECT
public:
	explicit FFmpegWorker( QObject* parent = nullptr );
	virtual ~FFmpegWorker();

	bool bStop;

	bool init();
	bool deinit();
	bool openInput();
	bool initOutput();
	void do_muxing();

public slots:
	void readStream();

private:
	// AVPacket pkt;
	QList<AVPacket*> m_av_packet_list;
	AVFormatContext* i_fmt_ctx = NULL;
	AVFormatContext* o_fmt_ctx = NULL;
	const char* i_filename;
	const char* o_filename;
};
