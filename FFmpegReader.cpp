#include "FFmpegReader.h"

FFmpegReader::FFmpegReader( QReadWriteLock* locker,
							AVFormatContext* fmt_ctx,
							QList<AVPacket*>* av_packet_list,
							QObject* parent )
	: QObject( parent )
	, b_stop( false )
	, b_trigger( false )
	, mLocker( locker )
	, i_fmt_ctx( fmt_ctx )
	, m_av_packet_list( av_packet_list )
{}

FFmpegReader::~FFmpegReader()
{}

void FFmpegReader::stop()
{
	b_stop = true;
}

void FFmpegReader::trigger()
{
	b_trigger = true;
}

void FFmpegReader::doWork()
{
	AVPacket* p_packet = nullptr;
	double time_first = 0;
	double time_last = 0;
	double time_trig = 0;
	double time_diff = 0;

	while ( true ) {
		if ( b_stop ) {
			break;
		}

		p_packet = av_packet_alloc();

		if ( av_read_frame( i_fmt_ctx, p_packet ) < 0 ) {
			continue;
		}

		mLocker->lockForWrite();
		m_av_packet_list->append( p_packet );
		AVPacket* t_first_packet = m_av_packet_list->first();
		AVPacket* t_last__packet = m_av_packet_list->last();
		mLocker->unlock();

		// 计算一桢在整个视频中的时间位置;
		// 根据 pts 来计算一桢在整个视频中的时间位置：
		// timestamp (秒) = pts * av_q2d (st->time_base)
		AVRational t_timebase = i_fmt_ctx->streams[p_packet->stream_index]->time_base;

		if ( !b_trigger ) {
			time_first = ( t_first_packet->pts ) * av_q2d( t_timebase );
			time_last = ( t_last__packet->pts ) * av_q2d( t_timebase );
			time_diff = time_last - time_first;

			if ( time_diff >= 10 ) {
				av_packet_free( &t_first_packet );
				mLocker->lockForWrite();
				m_av_packet_list->removeFirst();
				mLocker->unlock();
			}
		}
		else {
			if ( time_trig == 0 ) {
				time_trig = p_packet->pts * av_q2d( t_timebase );
			}
			time_last = ( t_last__packet->pts ) * av_q2d( t_timebase );
			time_diff = time_last - time_trig;

			if ( time_diff >= 5 ) {
				break;
			}
		}

		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId() << time_diff;
	}
}