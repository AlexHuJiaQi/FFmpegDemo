#include "FFmpegController.h"

/////////////////////////////////////////////////////////////
FFmpegController::FFmpegController( QObject* parent )
	: QObject( parent )
	, pReader( NULL )
	, pWriter( NULL )
{
	m_para.b_read_finish = false;

	m_para.i_filename = "rtsp://admin:admin12345@192.168.11.2:554/h264/ch1/main/av_stream";

	m_para.i_fmt_ctx = NULL;
	m_para.o_fmt_ctx = NULL;

	m_para.mutex          = new QMutex;

	m_para.bufferEmpty    = new QWaitCondition;
	m_para.m_write_finish = new QWaitCondition;

	m_para.thread__read = new QThread;
	m_para.thread_write = new QThread;

	//////////////////////////////////////
	pReader = new FFmpegReader();
	pReader->setParameter( &m_para );
	pReader->moveToThread( m_para.thread__read );
	m_para.thread__read->start();

	//////////////////////////////////////
	pWriter = new FFmpegWriter();
	pWriter->setParameter( &m_para );
	pWriter->moveToThread( m_para.thread_write );
	m_para.thread_write->start();
}

FFmpegController::~FFmpegController()
{
	pReader->termination();
	pWriter->termination();

	m_para.i_fmt_ctx = NULL;
	m_para.o_fmt_ctx = NULL;

	delete m_para.mutex;
	delete m_para.m_write_finish;

	m_para.thread__read->deleteLater();
	m_para.thread_write->deleteLater();
}

bool FFmpegController::start()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();
	if ( pReader->isRunning() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	m_para.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << m_para.o_filename;

	m_para.b_read_finish  = false;
	m_para.b_write_finish = false;
	pReader->clrTrigger();
	pReader->start();
	pReader->doRecord();

	return true;
}

bool FFmpegController::trigger()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	if ( !pReader->isRunning() || pReader->isTrigger() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	m_para.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << m_para.o_filename;

	m_para.b_read_finish  = false;
	m_para.b_write_finish = false;
	pReader->setTrigger();
	pWriter->start();
	pWriter->doRecord();

	return true;
}

bool FFmpegController::termination()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	if ( !pReader->isRunning() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	m_para.b_read_finish  = false;
	m_para.b_write_finish = false;
	pReader->termination();

	m_para.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << m_para.o_filename;
	pWriter->termination();
	pWriter->doRecord();

	return true;
}