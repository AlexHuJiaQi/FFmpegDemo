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
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#if 0
class FFmpegWorker :public QObject
{
	Q_OBJECT
public:
	explicit FFmpegWorker( QObject* parent = nullptr );
	virtual ~FFmpegWorker();

	bool start();
	bool stop();

	void pause();

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
#endif
///////////////////////////////////////////////////////////////////////////

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

class FFmpegWriter : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegWriter( QReadWriteLock* locker,
						   AVFormatContext* i_fmt_ctx_,
						   AVFormatContext* o_fmt_ctx_,
						   QList<AVPacket*>* av_packet_list,
						   QObject* parent = nullptr );
	virtual ~FFmpegWriter();

	void stop();
	void trigger();

public slots:
	void doWork();

private:
	bool b_stop, b_trigger;
	QReadWriteLock* mLocker;
	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;
	QList<AVPacket*>* m_av_packet_list;
};

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