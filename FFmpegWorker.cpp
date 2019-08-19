#include "FFmpegWorker.h"

FFmpegWorker::FFmpegWorker( QObject* parent )
	: QObject( parent )
	, b_stop( false ), b_trigger( false )
	, i_filename( "rtsp://admin:admin12345@192.168.11.2:554/h264/ch1/main/av_stream" )
	, o_filename( "test.avi" )
	, i_fmt_ctx( NULL )
	, o_fmt_ctx( NULL )
{}

FFmpegWorker::~FFmpegWorker()
{}

bool FFmpegWorker::start()
{
	avformat_network_init();
	return true;
}

bool FFmpegWorker::stop()
{
	if ( i_fmt_ctx ) {
		avformat_close_input( &i_fmt_ctx );
	}

	if ( o_fmt_ctx && !( o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
		avio_closep( &o_fmt_ctx->pb );
	}

	avformat_free_context( o_fmt_ctx );
	avformat_network_deinit();

	return true;
}

bool FFmpegWorker::open_input()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	if ( avformat_open_input( &i_fmt_ctx, i_filename, 0, 0 ) < 0 ) {
		fprintf( stderr, "Could not open input file '%s'", i_filename );
		return false;
	}

	if ( avformat_find_stream_info( i_fmt_ctx, 0 ) < 0 ) {
		fprintf( stderr, "Failed to retrieve input stream information" );
		return false;
	}

	av_dump_format( i_fmt_ctx, 0, i_filename, 0 );
	return true;
}

void FFmpegWorker::read_packet()
{
	AVPacket* p_packet = nullptr;
	double time_first = 0;
	double time_last  = 0;
	double time_trig  = 0;
	double time_diff  = 0;

	while ( !b_stop ) {
		p_packet = av_packet_alloc();

		if ( av_read_frame( i_fmt_ctx, p_packet ) < 0 ) {
			continue;
		}

		m_av_packet_list.append( p_packet );

		// 计算一桢在整个视频中的时间位置
		// 根据 pts 来计算一桢在整个视频中的时间位置：
		// timestamp (秒) = pts * av_q2d (st->time_base)
		AVRational t_timebase = i_fmt_ctx->streams[p_packet->stream_index]->time_base;

		if ( b_trigger == false ) {
			time_first = ( m_av_packet_list.first()->pts ) * av_q2d( t_timebase );
			time_last  = ( m_av_packet_list.last()->pts ) * av_q2d( t_timebase );
			time_diff  = time_last - time_first;

			if ( time_diff >= 10 ) {
				av_packet_free( &m_av_packet_list.first() );
				m_av_packet_list.removeFirst();
			}
		}
		else {
			if ( time_trig == 0 ) {
				time_trig = p_packet->pts * av_q2d( t_timebase );
			}
			time_last  = ( m_av_packet_list.last()->pts ) * av_q2d( t_timebase );
			time_diff  = time_last - time_trig;

			if ( time_diff >= 5 ) {
				break;
			}
		}

		qDebug() << QThread::currentThreadId() << ", time interval: " << time_diff;
	}
}

bool FFmpegWorker::open_output()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	avformat_alloc_output_context2( &o_fmt_ctx, NULL, NULL, o_filename );

	if ( !o_fmt_ctx ) {
		qDebug( "Could not create output context" );
		return false;
	}

	for ( auto i = 0; i < i_fmt_ctx->nb_streams; i++ ) {
		AVStream* out_stream = avformat_new_stream( o_fmt_ctx, NULL );
		if ( !out_stream ) {
			qDebug( "Failed allocating output stream" );
			return false;
		}

		if ( avcodec_parameters_copy( out_stream->codecpar, i_fmt_ctx->streams[i]->codecpar ) < 0 ) {
			qDebug( "Failed to copy codec parameters" );
			return false;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format( o_fmt_ctx, 0, o_filename, 1 );

	if ( !( o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
		if ( avio_open( &o_fmt_ctx->pb, o_filename, AVIO_FLAG_WRITE ) < 0 ) {
			qDebug( "Could not open output file '%s'", o_filename );
			return false;
		}
	}

	if ( avformat_write_header( o_fmt_ctx, NULL ) < 0 ) {
		qDebug( "Error occurred when opening output file" );
		return false;
	}

	return true;
}

void FFmpegWorker::write_packet()
{
	int64_t v_first_pts = 0;
	int64_t v_first_dts = 0;
	int64_t a_first_pts = 0;
	int64_t a_first_dts = 0;

	for ( AVPacket* p_packet : m_av_packet_list ) {
		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

		AVStream* i_stream = i_fmt_ctx->streams[p_packet->stream_index];
		AVStream* o_stream = o_fmt_ctx->streams[p_packet->stream_index];

		if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
			if ( v_first_pts == 0 || v_first_dts == 0 ) {
				v_first_pts = av_rescale_q_rnd( p_packet->pts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				v_first_dts = av_rescale_q_rnd( p_packet->dts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
		}
		else if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
			if ( a_first_pts == 0 || a_first_dts == 0 ) {
				a_first_pts = av_rescale_q_rnd( p_packet->pts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
				a_first_dts = av_rescale_q_rnd( p_packet->dts,
												i_stream->time_base,
												o_stream->time_base,
												AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
			}
		}

		p_packet->pts      = av_rescale_q_rnd( p_packet->pts,
											   i_stream->time_base,
											   o_stream->time_base,
											   AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
		p_packet->dts      = av_rescale_q_rnd( p_packet->dts,
											   i_stream->time_base,
											   o_stream->time_base,
											   AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
		p_packet->duration = av_rescale_q( p_packet->duration,
										   i_stream->time_base,
										   o_stream->time_base );

		if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
			p_packet->pts -= v_first_pts;
			p_packet->dts -= v_first_dts;
		}
		else if ( i_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
			p_packet->pts -= a_first_pts;
			p_packet->dts -= a_first_dts;
		}

		if ( av_interleaved_write_frame( o_fmt_ctx, p_packet ) < 0 ) {
			qDebug( "Error muxing packet" );
			av_packet_free( &p_packet );
			continue;
		}

		av_packet_free( &p_packet );
	}

	av_write_trailer( o_fmt_ctx );
	m_av_packet_list.clear();
}