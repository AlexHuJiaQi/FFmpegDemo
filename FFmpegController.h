#pragma once

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QReadWriteLock>

#include "FFmpegReader.h"
#include "FFmpegWriter.h"

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

class FFmpegController : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegController( QObject* parent = nullptr );
	virtual ~FFmpegController();

	bool start();
	bool trigger();
	bool stop();

signals:
	void toDoWorker();

private:
	bool b_stop, b_trigger;
	char* i_filename;
	char* o_filename;

	FFmpegReader* pReader;
	FFmpegWriter* pWriter;

	QThread m_thread_read;
	QThread m_thread_write;

	QReadWriteLock mLock;
	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;
	QList<AVPacket*> m_av_packet_list;
};