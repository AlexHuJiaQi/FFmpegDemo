#pragma once

#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QWaitCondition>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#define Cache_Interval_1 180
#define Cache_Interval_2 420

struct FFmpegParameter
{
	bool b_read_finish, b_write_finish;

	QByteArray i_filename;
	QByteArray o_filename;

	// 线程控制
	QMutex* mutex;

	QWaitCondition* bufferEmpty;
	QWaitCondition* m_write_finish;

	QThread* thread__read;
	QThread* thread_write;

	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;

	// QList<AVPacket*> av_packet_list;

	QList<AVPacket*> a_packet_list;
	QList<AVPacket*> v_packet_list;
};

class AbstractReadWriter : public QObject
{
	Q_OBJECT
public:
	explicit AbstractReadWriter( QObject* parent = nullptr )
		: QObject( parent )
		, b_start( false )
	{
		connect( this, SIGNAL( execute() ), this, SLOT( doWork() ), Qt::QueuedConnection );
	}
	virtual ~AbstractReadWriter() {}

	virtual void setParameter( FFmpegParameter* para )=0;

	virtual void doRecord() final { emit execute(); }

	virtual bool isRunning()   final { return b_start; }
	virtual void start()       final { b_start = true; }
	virtual void termination() final { b_start = false; }

signals:
	void execute();

public slots:
	virtual void doWork()=0;

private:
	bool b_start;
};
