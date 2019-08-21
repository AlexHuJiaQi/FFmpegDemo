#include "FFmpegReader.h"

FFmpegReader::FFmpegReader( QObject* parent )
	: QObject( parent )
{}

FFmpegReader::~FFmpegReader()
{}

void FFmpegReader::setParameter( FFmpegParameter* para )
{
	m_para = para;
}

void FFmpegReader::doWork()
{
	AVPacket* p_packet = nullptr;
	double time_first  = 0;
	double time_last   = 0;
	double time_trig   = 0;
	double time_diff   = 0;

	while ( true ) {
		if ( m_para->b_stop ) {
			break;
		}

		p_packet = av_packet_alloc();

		if ( av_read_frame( m_para->i_fmt_ctx, p_packet ) < 0 ) {
			continue;
		}

		// 计算一桢在整个视频中的时间位置;
		// 根据 pts 来计算一桢在整个视频中的时间位置：
		// timestamp (秒) = pts * av_q2d (st->time_base)
		const AVRational t_timebase = m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base;

		// 未触发记录信号的状态
		if ( !m_para->b_trigger ) {
			m_para->av_packet_list.append( p_packet );

			AVPacket* t_first_packet = m_para->av_packet_list.first();

			time_first = ( t_first_packet->pts ) * av_q2d( t_timebase );
			time_last  = ( p_packet->pts ) * av_q2d( t_timebase );
			time_diff  = time_last - time_first;

			if ( time_diff >= Cache_Interval_1 ) {
				av_packet_free( &t_first_packet );
				m_para->av_packet_list.removeFirst();
			}
		}
		else { // 触发记录信号的状态
			m_para->mutex->lock();
			m_para->av_packet_list.append( p_packet );
			m_para->bufferEmpty->wakeAll();
			m_para->mutex->unlock();

			if ( time_trig == 0 ) {
				time_trig = p_packet->pts * av_q2d( t_timebase );
			}

			time_last = ( p_packet->pts ) * av_q2d( t_timebase );
			time_diff = time_last - time_trig;

			if ( time_diff >= Cache_Interval_2 ) {
				m_para->b_read_finish = true;

				m_para->mutex->lock();
				m_para->bufferEmpty->wakeAll();
				m_para->mutex->unlock();

				m_para->mutex->lock();
				m_para->m_write_finish->wait( m_para->mutex );
				m_para->mutex->unlock();

				m_para->b_trigger  = false;
				double time_first  = 0;
				double time_last   = 0;
				double time_trig   = 0;
				double time_diff   = 0;
				// break;
			}
		}

		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId() << time_diff;
	}
}