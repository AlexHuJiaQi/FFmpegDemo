#include "FFmpegController.h"

/////////////////////////////////////////////////////////////
FFmpegController::FFmpegController( QObject* parent )
	: QObject( parent )
	, i_filename( "rtsp://admin:admin12345@192.168.11.2:554/h264/ch1/main/av_stream" )
	, o_filename( "test.avi" )
	, pReader( NULL )
	, pWriter( NULL )
{
	m_para.b_start       = false;
	m_para.b_stop        = false;
	m_para.b_trigger     = false;
	m_para.b_read_finish = false;

	m_para.i_fmt_ctx = NULL;
	m_para.o_fmt_ctx = NULL;

	m_para.mutex          = new QMutex;

	m_para.bufferEmpty    = new QWaitCondition;
	m_para.m_write_finish = new QWaitCondition;

	m_para.thread__read = new QThread;
	m_para.thread_write = new QThread;
}

FFmpegController::~FFmpegController()
{
	m_para.b_start   = false;
	m_para.b_stop    = false;
	m_para.b_trigger = false;

	m_para.i_fmt_ctx = NULL;
	m_para.o_fmt_ctx = NULL;

	delete m_para.mutex;
	delete m_para.m_write_finish;

	m_para.thread__read->deleteLater();
	m_para.thread_write->deleteLater();
}

bool FFmpegController::start()
{
	if ( m_para.b_start ) { return true; }

	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	avformat_network_init();

	if ( avformat_open_input( &m_para.i_fmt_ctx, i_filename, 0, 0 ) < 0 ) {
		qDebug() << QString( "Could not open input file: %1" ).arg( i_filename );
		return false;
	}

	if ( avformat_find_stream_info( m_para.i_fmt_ctx, 0 ) < 0 ) {
		qDebug() << QString( "Failed to retrieve input stream information" );
		return false;
	}

	av_dump_format( m_para.i_fmt_ctx, 0, i_filename, 0 );
	////////////////////////////////////////////
	pReader = new FFmpegReader();
	pReader->setParameter( &m_para );
	connect( m_para.thread__read, SIGNAL( started() ), pReader, SLOT( doWork() ), Qt::QueuedConnection );
	pReader->moveToThread( m_para.thread__read );
	m_para.thread__read->start();

	m_para.b_start = true;
	m_para.b_stop  = false;

	return true;
}

bool FFmpegController::stop()
{
	if ( m_para.b_stop ) { return true; }

	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	m_para.thread__read->quit();
	m_para.thread__read->wait();

	if ( !m_para.thread_write->isRunning() ) {
		m_para.b_read_finish = true;
		m_para.thread_write->start();
	}

	m_para.thread_write->wait();

	if ( m_para.i_fmt_ctx ) {
		avformat_close_input( &m_para.i_fmt_ctx );
	}

	// if ( m_para.o_fmt_ctx && !( m_para.o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
	// 	avio_closep( &m_para.o_fmt_ctx->pb );
	// }
	//
	// avformat_free_context( m_para.o_fmt_ctx );
	avformat_network_deinit();

	m_para.b_start = false;
	m_para.b_stop  = true;

	return true;
}

bool FFmpegController::trigger()
{
	qDebug() << __FUNCTION__ << __LINE__ << QThread::currentThreadId();

	avformat_alloc_output_context2( &m_para.o_fmt_ctx, NULL, NULL, o_filename );

	if ( !m_para.o_fmt_ctx ) {
		qDebug( "Could not create output context" );
		return false;
	}

	for ( auto i = 0; i < m_para.i_fmt_ctx->nb_streams; i++ ) {
		AVStream* out_stream = avformat_new_stream( m_para.o_fmt_ctx, NULL );
		if ( !out_stream ) {
			qDebug( "Failed allocating output stream" );
			return false;
		}

		if ( avcodec_parameters_copy( out_stream->codecpar, m_para.i_fmt_ctx->streams[i]->codecpar ) < 0 ) {
			qDebug( "Failed to copy codec parameters" );
			return false;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format( m_para.o_fmt_ctx, 0, o_filename, 1 );

	if ( !( m_para.o_fmt_ctx->oformat->flags & AVFMT_NOFILE ) ) {
		if ( avio_open( &m_para.o_fmt_ctx->pb, o_filename, AVIO_FLAG_WRITE ) < 0 ) {
			qDebug( "Could not open output file '%s'", o_filename );
			return false;
		}
	}

	if ( avformat_write_header( m_para.o_fmt_ctx, NULL ) < 0 ) {
		qDebug( "Error occurred when opening output file" );
		return false;
	}

	pWriter = new FFmpegWriter();
	pWriter->setParameter( &m_para );
	connect( m_para.thread_write, SIGNAL( started() ), pWriter, SLOT( doWork() ), Qt::QueuedConnection );
	pWriter->moveToThread( m_para.thread_write );
	m_para.thread_write->start();

	m_para.b_trigger = true;

	return true;
}