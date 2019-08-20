#include "FFmpegController.h"

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