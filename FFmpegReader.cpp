#include "FFmpegReader.h"
#include <math.h>

FFmpegReader::FFmpegReader( QObject* parent )
	: AbstractReadWriter( parent )
	, b_trigger( false )
{}

FFmpegReader::~FFmpegReader()
{}

void FFmpegReader::doWork()
{
	qDebug() << QString( "%1, %2, diff time:" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	/******************************************************************************************/
	avformat_network_init();
	if ( avformat_open_input( &getParameter()->i_fmt_ctx, getParameter()->i_filename, 0, 0 ) < 0 ) {
		qDebug() << QString( "Could not open input file: %1" ).arg( getParameter()->i_filename.data() );
		return;
	}

	if ( avformat_find_stream_info( getParameter()->i_fmt_ctx, 0 ) < 0 ) {
		qDebug() << QString( "Failed to retrieve input stream information" );
		return;
	}

	av_dump_format( getParameter()->i_fmt_ctx, 0, getParameter()->i_filename, 0 );
	/******************************************************************************************/

	while ( true ) {
		if ( isRunning() ) {
			p_packet = av_packet_alloc();
			if ( p_packet == NULL ) {
				continue;
			}

			if ( av_read_frame( getParameter()->i_fmt_ctx, p_packet ) < 0 ) {
				continue;
			}

			if ( p_packet->pts == AV_NOPTS_VALUE ) {
				qDebug() << "################################### AV_NOPTS_VALUE";
				av_packet_free( &p_packet );
				continue;
			}

			parse_packet( p_packet );
		}
		else {
			// 停止读数据
			getParameter()->b_read_finish = true;
			/////////////////////////////////////
			getParameter()->mutex->lock();
			getParameter()->bufferEmpty->wakeAll();
			getParameter()->mutex->unlock();

			/////////////////////////////////////
			if ( !getParameter()->b_write_finish ) {
				getParameter()->mutex->lock();
				getParameter()->m_write_finish->wait( getParameter()->mutex );
				getParameter()->mutex->unlock();
			}

			break;
		}
	}

	/******************************************************************************************/
	avformat_close_input( &getParameter()->i_fmt_ctx );
	avformat_network_deinit();
	qDebug() << "#############################################################" << "Read  Finished";
}

void FFmpegReader::parse_packet( AVPacket* p_packet )
{
	// 计算一桢在整个视频中的时间位置;
	// 根据 pts 来计算一桢在整个视频中的时间位置：
	// timestamp (秒) = pts * av_q2d (st->time_base)
	const AVRational  t_timebase = getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->time_base;
	const AVMediaType t_type     = getParameter()->i_fmt_ctx->streams[p_packet->stream_index]->codecpar->codec_type;

	if ( !isTrigger() ) { // 未收到记录触发信号
		if ( t_type == AVMEDIA_TYPE_VIDEO ) {
			getParameter()->v_packet_list.append( p_packet );
			time_diff_v = ( av_q2d( t_timebase ) * p_packet->pts ) - ( av_q2d( t_timebase ) * getParameter()->v_packet_list.first()->pts );
			if ( time_diff_v >= getParameter()->Cache_Interval_1 ) {
				AVPacket* pkt = getParameter()->v_packet_list.takeFirst();
				av_packet_free( &pkt );
			}
		}
		else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
			getParameter()->a_packet_list.append( p_packet );
			time_diff_a = ( av_q2d( t_timebase ) * p_packet->pts ) - ( av_q2d( t_timebase ) * getParameter()->a_packet_list.first()->pts );
			if ( time_diff_a >= getParameter()->Cache_Interval_1 ) {
				AVPacket* pkt = getParameter()->a_packet_list.takeFirst();
				av_packet_free( &pkt );
			}
		}
	}
	else { // 收到记录触发信号
		if ( t_type == AVMEDIA_TYPE_VIDEO ) {
			getParameter()->mutex->lock();
			if ( 0 == time_trig_v ) { time_trig_v = p_packet->pts * av_q2d( t_timebase ); }
			time_diff_v = ( av_q2d( t_timebase ) * p_packet->pts ) - time_trig_v;
			getParameter()->v_packet_list.append( p_packet );
			getParameter()->bufferEmpty->wakeAll();
			getParameter()->mutex->unlock();
		}
		else if ( t_type == AVMEDIA_TYPE_AUDIO ) {
			getParameter()->mutex->lock();
			if ( 0 == time_trig_a ) { time_trig_a = p_packet->pts * av_q2d( t_timebase ); }
			time_diff_a = ( av_q2d( t_timebase ) * p_packet->pts ) - time_trig_a;
			getParameter()->a_packet_list.append( p_packet );
			getParameter()->bufferEmpty->wakeAll();
			getParameter()->mutex->unlock();
		}

		if ( time_diff_v >= getParameter()->Cache_Interval_2 &&
			 time_diff_a >= getParameter()->Cache_Interval_2 ) {
			qDebug() << "#############################################################" << "Cache 2 Finish";
			/////////////////////////////////////
			getParameter()->mutex->lock();
			getParameter()->b_read_finish = true;
			getParameter()->bufferEmpty->wakeAll();
			getParameter()->mutex->unlock();
			/////////////////////////////////////
			getParameter()->mutex->lock();
			getParameter()->m_write_finish->wait( getParameter()->mutex );
			getParameter()->mutex->unlock();

			clrTrigger();
			time_diff_v = 0;
			time_diff_a = 0;
			time_trig_v = 0;
		}
	}

	qDebug() << QString( "%1, %2, (vedio, size:%3, time:%4), (audio, size:%5, time:%6)" )
		.arg( __FUNCTION__, -30 )
		.arg( __LINE__, 3 )
		.arg( getParameter()->v_packet_list.size(), -7 )
		.arg( time_diff_v, -7 )
		.arg( getParameter()->a_packet_list.size(), -7 )
		.arg( time_diff_a, -7 )
		<< QThread::currentThread();

	emit sig_read( QString( "v:%1, a:%2" ).arg( time_diff_v ).arg( time_diff_a ) );
}