#ifndef FFMPEGPARAMETER_H
#define FFMPEGPARAMETER_H

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

struct FFmpegParameter
{
	bool b_start;
	bool b_r_finish;
	bool b_w_finish;

	QByteArray i_filename;
	QByteArray o_filename;

	// 缓存间隔
	uint32_t _interval_before_trig;
	uint32_t _interval_after_trig;

	// 线程控制
	QMutex* mutex;

	QWaitCondition* _buffer_empty;
	QWaitCondition* _write_finish;

	QThread* r_Thread;
	QThread* w_Thread;

	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;

	QList<AVPacket*> a_packet_list;
	QList<AVPacket*> v_packet_list;
};

class AbstractReadWriter : public QObject
{
	Q_OBJECT
public:
	explicit AbstractReadWriter( QObject* parent = nullptr )
		: QObject( parent )
	{
		connect( this, SIGNAL( execute() ), this, SLOT( doWork() ), Qt::QueuedConnection );
	}
	virtual ~AbstractReadWriter() {}

	virtual void setParameter( FFmpegParameter* para )final { m_para = para; }
	virtual FFmpegParameter* getParameter()final { return m_para; }

	virtual void doRecord()    final { emit execute(); }
	virtual bool isStared()    final { return m_para->b_start; }
	virtual void doStart()     final { m_para->b_start = true; }
	virtual void doTerminate() final { m_para->b_start = false; }

signals:
	void execute();

public slots:
	virtual void doWork()=0;

private:
	FFmpegParameter* m_para;
};

#endif
