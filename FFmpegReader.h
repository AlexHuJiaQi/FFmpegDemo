#pragma once

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QReadWriteLock>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class FFmpegReader : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegReader( QReadWriteLock* locker,
						   AVFormatContext* fmt_ctx,
						   QList<AVPacket*>* av_packet_list,
						   QObject* parent = nullptr );
	virtual ~FFmpegReader();

	void stop();
	void trigger();

public slots:
	void doWork();

private:
	bool b_stop, b_trigger;
	QReadWriteLock* mLocker;
	AVFormatContext* i_fmt_ctx;
	QList<AVPacket*>* m_av_packet_list;
};