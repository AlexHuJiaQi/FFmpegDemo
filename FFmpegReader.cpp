#include "FFmpegReader.h"

FFmpegReader::FFmpegReader( QObject* parent )
	: AbstractReadWriter( parent )
	, b_trigger( false )
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

	/******************************************************************************************/
	avformat_network_init();

	if ( avformat_open_input( &m_para->i_fmt_ctx, m_para->i_filename, 0, 0 ) < 0 ) {
		qDebug() << QString( "Could not open input file: %1" ).arg( m_para->i_filename.data() );
		return;
	}

	if ( avformat_find_stream_info( m_para->i_fmt_ctx, 0 ) < 0 ) {
		qDebug() << QString( "Failed to retrieve input stream information" );
		return;
	}

	av_dump_format( m_para->i_fmt_ctx, 0, m_para->i_filename, 0 );
	/******************************************************************************************/

	while ( true ) {
		// 停止读数据
		if ( !isStart() ) {
			m_para->mutex->lock();
			m_para->m_write_finish->wait( m_para->mutex );
			m_para->mutex->unlock();
			break;
		}
#if 1
		p_packet = av_packet_alloc();

		if ( av_read_frame( m_para->i_fmt_ctx, p_packet ) < 0 ) { continue; }

		// 计算一桢在整个视频中的时间位置;
		// 根据 pts 来计算一桢在整个视频中的时间位置：
		// timestamp (秒) = pts * av_q2d (st->time_base)
		const AVRational t_timebase = m_para->i_fmt_ctx->streams[p_packet->stream_index]->time_base;

		// 未触发记录信号的状态
		if ( !isTrigger() ) {
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

			if ( time_trig == 0 ) { time_trig = p_packet->pts * av_q2d( t_timebase ); }

			time_last = ( p_packet->pts ) * av_q2d( t_timebase );
			time_diff = time_last - time_trig;

			if ( time_diff >= Cache_Interval_2 ) {
				qDebug() << "#############################################################" << "Cache 2 Finish";
				m_para->b_read_finish = true;

				m_para->mutex->lock();
				m_para->bufferEmpty->wakeAll();
				m_para->mutex->unlock();

				m_para->mutex->lock();
				m_para->m_write_finish->wait( m_para->mutex );
				m_para->mutex->unlock();

				clrTrigger();

				time_first  = 0;
				time_last   = 0;
				time_trig   = 0;
				time_diff   = 0;
			}
		}

		if ( time_diff >= 0 ) {
			qDebug() << QString( "%1, %2, %3, time:%4" )
				.arg( __FUNCTION__ )
				.arg( __LINE__, 3 )
				.arg( (quint64 )QThread::currentThread() )
				.arg( time_diff );
		}
#endif
	}

	/******************************************************************************************/
	avformat_close_input( &m_para->i_fmt_ctx );
	avformat_network_deinit();
	qDebug() << "#############################################################" << "Read Finished";
}