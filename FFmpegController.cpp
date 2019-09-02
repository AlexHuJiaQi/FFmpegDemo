#include "FFmpegController.h"

/////////////////////////////////////////////////////////////
FFmpegController::FFmpegController( uint32_t interval_1,
									uint32_t interval_2,
									QByteArray url,
									QObject* parent )
	: QObject( parent )
	, pReader( NULL )
	, pWriter( NULL )
{
	mPara.i_filename       = url;
	mPara.Cache_Interval_1 = interval_1;
	mPara.Cache_Interval_2 = interval_2;

	mPara.i_fmt_ctx      = NULL;
	mPara.o_fmt_ctx      = NULL;
	mPara.b_start        = false;
	mPara.b_read_finish  = false;
	mPara.b_write_finish = false;
	mPara.mutex          = new QMutex;
	mPara.bufferEmpty    = new QWaitCondition;
	mPara.m_write_finish = new QWaitCondition;
	mPara.thread__read   = new QThread;
	mPara.thread_write   = new QThread;

	//////////////////////////////////////
	pReader = new FFmpegReader();
	pReader->setParameter( &mPara );
	pReader->moveToThread( mPara.thread__read );
	mPara.thread__read->start( QThread::HighPriority );

	//////////////////////////////////////
	pWriter = new FFmpegWriter();
	pWriter->setParameter( &mPara );
	pWriter->moveToThread( mPara.thread_write );
	mPara.thread_write->start( QThread::HighestPriority );
}

FFmpegController::~FFmpegController()
{
	pReader->termination();
	pWriter->termination();

	mPara.i_fmt_ctx = NULL;
	mPara.o_fmt_ctx = NULL;

	delete mPara.mutex;
	delete mPara.m_write_finish;

	mPara.thread__read->deleteLater();
	mPara.thread_write->deleteLater();
}

bool FFmpegController::start()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();
	if ( pReader->isRunning() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	mPara.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << mPara.o_filename;

	mPara.b_read_finish  = false;
	mPara.b_write_finish = false;
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

	mPara.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << mPara.o_filename;

	mPara.b_read_finish  = false;
	mPara.b_write_finish = false;
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

	mPara.b_read_finish  = false;
	mPara.b_write_finish = false;
	pReader->termination();

	mPara.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << mPara.o_filename;
	pWriter->termination();
	pWriter->doRecord();

	return true;
}