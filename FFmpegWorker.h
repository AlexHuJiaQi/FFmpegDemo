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
		b_stop = b_stop ? false : true;
	}

	void trigger() { b_trigger = true; }

	bool open_input();
	bool open_output();

public slots:
	void read_packet();
	void write_packet();

private:
	bool b_stop, b_trigger;
	const char* i_filename;
	const char* o_filename;

	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;

	QList<AVPacket*> m_av_packet_list;
};
