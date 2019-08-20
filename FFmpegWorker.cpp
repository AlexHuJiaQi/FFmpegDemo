#include "FFmpegWorker.h"

#if 0
FFmpegWorker::FFmpegWorker( QObject * parent )
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

void FFmpegWorker::pause()
{
	b_stop = b_stop ? false : true;
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

#endif

////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////
FFmpegWriter::FFmpegWriter( QReadWriteLock* locker,
							AVFormatContext* i_fmt_ctx_,
							AVFormatContext* o_fmt_ctx_,
							QList<AVPacket*>* av_packet_list,
							QObject* parent )
	: QObject( parent )
	, b_stop( false )
	, b_trigger( false )
	, mLocker( locker )
	, i_fmt_ctx( i_fmt_ctx_ )
	, o_fmt_ctx( o_fmt_ctx_ )
	, m_av_packet_list( av_packet_list )
{}

FFmpegWriter::~FFmpegWriter()
{}

void FFmpegWriter::stop()
{
	b_stop = true;
}

void FFmpegWriter::trigger()
{
	b_trigger = true;
}

void FFmpegWriter::doWork()
{
	int64_t v_first_pts = 0;
	int64_t v_first_dts = 0;
	int64_t a_first_pts = 0;
	int64_t a_first_dts = 0;

	while ( true ) {
		qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

		if ( b_stop && m_av_packet_list->isEmpty() ) {
			break;
		}

		if ( m_av_packet_list->isEmpty() ) {
			QThread::usleep( 500 );
			continue;
		}

		mLocker->lockForWrite();
		AVPacket* p_packet = m_av_packet_list->takeFirst();
		mLocker->unlock();

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

		p_packet->pts = av_rescale_q_rnd( p_packet->pts,
										  i_stream->time_base,
										  o_stream->time_base,
										  AVRounding( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX ) );
		p_packet->dts = av_rescale_q_rnd( p_packet->dts,
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
}

/////////////////////////////////////////////////////////////
FFmpegController::FFmpegController( QObject* parent )
	: QObject( parent )
	, b_stop( false )
	, b_trigger( false )
	, i_filename( "rtsp://admin:admin12345@192.168.11.2:554/h264/ch1/main/av_stream" )
	, o_filename( "test.avi" )
	, i_fmt_ctx( NULL )
	, o_fmt_ctx( NULL )
{}

FFmpegController::~FFmpegController()
{}

bool FFmpegController::start()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();
	avformat_network_init();

	if ( avformat_open_input( &i_fmt_ctx, i_filename, 0, 0 ) < 0 ) {
		fprintf( stderr, "Could not open input file '%s'", i_filename );
		return false;
	}

	if ( avformat_find_stream_info( i_fmt_ctx, 0 ) < 0 ) {
		fprintf( stderr, "Failed to retrieve input stream information" );
		return false;
	}

	av_dump_format( i_fmt_ctx, 0, i_filename, 0 );
	////////////////////////////////////////////
	pReader = new FFmpegReader( &mLock, i_fmt_ctx, &m_av_packet_list );
	connect( &m_thread_read, SIGNAL( started() ), pReader, SLOT( doWork() ), Qt::QueuedConnection );
	pReader->moveToThread( &m_thread_read );
	m_thread_read.start();
	return true;
}

bool FFmpegController::trigger()
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

	pWriter = new FFmpegWriter( &mLock, i_fmt_ctx, o_fmt_ctx, &m_av_packet_list );
	connect( &m_thread_write, SIGNAL( started() ), pWriter, SLOT( doWork() ), Qt::QueuedConnection );
	pWriter->moveToThread( &m_thread_write );
	m_thread_write.start();

	pReader->trigger();
	pWriter->trigger();

	return true;
}

bool FFmpegController::stop()
{
	pReader->stop();
	m_thread_read.quit();
	m_thread_read.wait();

	pWriter->stop();
	m_thread_write.quit();
	m_thread_write.wait();

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