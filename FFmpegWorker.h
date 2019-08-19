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

	bool start();
	bool stop();

	void pause()
	{
		bStop = bStop ? false : true;
	}

	bool open_input();
	bool open_output();

	void write_packet();

public slots:
	void read_packet();

private:
	bool bStop;
	const char* i_filename;
	const char* o_filename;

	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;

	QList<AVPacket*> m_av_packet_list;
};
