#include "FFmpegReader.h"
#include <math.h>

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
	qDebug() << QString( "%1, %2, diff time:" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread;
	AVPacket* p_packet = nullptr;

	double time_trig_v = 0;
	double time_diff   = 0;
	double time_diff_a = 0;
	double time_diff_v = 0;

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
		if ( isRunning() ) {
			p_packet = av_packet_alloc();

			if ( av_read_frame( m_para->i_fmt_ctx, p_packet ) < 0 ) { continue; }

			// 计算一桢在整个视频中的时间位置;
			// 根据 pts 来计算一桢在整个视频中的时间位置：
			// timestamp (秒) = pts * av_q2d (st->time_base)
			const AVStream* i_stream     = m_para->i_fmt_ctx->streams[p_packet->stream_index];
			const AVRational  t_timebase = i_stream->time_base;
			const AVMediaType t_type     = i_stream->codecpar->codec_type;

			// 未触发记录信号的状态
			if ( isTrigger() ) { // 触发记录信号的状态
				m_para->mutex->lock();
				if ( t_type == AVMEDIA_TYPE_VIDEO ) {
					m_para->v_packet_list.append( p_packet );
					if ( 0 == time_trig_v ) { time_trig_v = p_packet->pts * av_q2d( t_timebase ); }
					time_diff = ( av_q2d( t_timebase ) * p_packet->pts ) - time_trig_v;
				}
				else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
					m_para->a_packet_list.append( p_packet );
				}
				m_para->bufferEmpty->wakeAll();
				m_para->mutex->unlock();

				if ( time_diff >= Cache_Interval_2 ) {
					qDebug() << "#############################################################" << "Cache 2 Finish";
					m_para->b_read_finish = true;
					/////////////////////////////////////
					m_para->mutex->lock();
					m_para->bufferEmpty->wakeAll();
					m_para->mutex->unlock();
					/////////////////////////////////////
					m_para->mutex->lock();
					m_para->m_write_finish->wait( m_para->mutex );
					m_para->mutex->unlock();

					clrTrigger();
					time_diff_a = 0;
					time_diff_v = 0;
					time_trig_v = 0;
				}
			}
			else {
				if ( t_type == AVMEDIA_TYPE_VIDEO ) {
					m_para->v_packet_list.append( p_packet );
					time_diff_a = ( av_q2d( t_timebase ) * p_packet->pts ) - ( av_q2d( t_timebase ) * m_para->v_packet_list.first()->pts );
					if ( time_diff_a >= Cache_Interval_1 ) {
						av_packet_free( &m_para->v_packet_list.first() );
						m_para->v_packet_list.removeFirst();
					}
				}
				else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
					m_para->a_packet_list.append( p_packet );
					time_diff_v = ( av_q2d( t_timebase ) * p_packet->pts ) - ( av_q2d( t_timebase ) * m_para->a_packet_list.first()->pts );
					if ( time_diff_v >= Cache_Interval_1 ) {
						av_packet_free( &m_para->a_packet_list.first() );
						m_para->a_packet_list.removeFirst();
					}
				}

				m_para->b_read_finish = true;
				m_para->mutex->lock();
				m_para->bufferEmpty->wakeAll();
				m_para->mutex->unlock();
			}

			qDebug() << QString( "%1, %2, (%3, time:%4, size:%5), (%6, time:%7, size:%8)" )
				.arg( __FUNCTION__ )
				.arg( __LINE__, 3 )
				.arg( ( t_type == AVMEDIA_TYPE_VIDEO ) ? "vedio" : "audio" )
				.arg( time_diff_v, -7 )
				.arg( m_para->v_packet_list.size(), -7 )
				.arg( ( t_type == AVMEDIA_TYPE_VIDEO ) ? "vedio" : "audio" )
				.arg( time_diff_a, -8 )
				.arg( m_para->a_packet_list.size(), -7 )
				<< QThread::currentThread();
		}
		else {
			// 停止读数据
			qDebug() << __FUNCTION__ << __LINE__;

			m_para->b_read_finish = true;
			/////////////////////////////////////
			m_para->mutex->lock();
			m_para->bufferEmpty->wakeAll();
			m_para->mutex->unlock();
			qDebug() << __FUNCTION__ << __LINE__;

			/////////////////////////////////////
			if ( !m_para->b_write_finish ) {
				qDebug() << __FUNCTION__ << __LINE__;

				m_para->mutex->lock();
				qDebug() << __FUNCTION__ << __LINE__;
				m_para->m_write_finish->wait( m_para->mutex );
				qDebug() << __FUNCTION__ << __LINE__;
				m_para->mutex->unlock();
			}

			break;
		}
	}

	/******************************************************************************************/
	avformat_close_input( &m_para->i_fmt_ctx );
	avformat_network_deinit();
	qDebug() << "#############################################################" << "Read Finished";
}
