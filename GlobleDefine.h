#pragma once

#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#define Cache_Interval_1 5
#define Cache_Interval_2 5

struct FFmpegParameter
{
	bool b_start;
	bool b_stop;
	bool b_trigger;
	bool b_read_finish;

	// 线程控制
	QMutex* mutex;

	QWaitCondition* bufferEmpty;
	QWaitCondition* m_write_finish;

	QThread* thread__read;
	QThread* thread_write;

	AVFormatContext* i_fmt_ctx;
	AVFormatContext* o_fmt_ctx;

	QList<AVPacket*> av_packet_list;
};