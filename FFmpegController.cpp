#include "FFmpegController.h"

/////////////////////////////////////////////////////////////
FFmpegController::FFmpegController( QObject* parent )
	: QObject( parent )
	, pReader( NULL )
	, pWriter( NULL )
{
	mPara.i_fmt_ctx     = NULL;
	mPara.o_fmt_ctx     = NULL;
	mPara.b_start       = false;
	mPara.b_r_finish    = false;
	mPara.b_w_finish    = false;
	mPara.mutex         = new QMutex;
	mPara._buffer_empty = new QWaitCondition;
	mPara._write_finish = new QWaitCondition;
	mPara.r_Thread      = new QThread;
	mPara.w_Thread      = new QThread;

	//////////////////////////////////////
	pReader = new FFmpegReader();
	pReader->setParameter( &mPara );
	pReader->moveToThread( mPara.r_Thread );
	mPara.r_Thread->start( QThread::HighPriority );

	//////////////////////////////////////
	pWriter = new FFmpegWriter();
	pWriter->setParameter( &mPara );
	pWriter->moveToThread( mPara.w_Thread );
	mPara.w_Thread->start( QThread::HighestPriority );
}

FFmpegController::~FFmpegController()
{
	pReader->doTerminate();
	pWriter->doTerminate();

	mPara.i_fmt_ctx = NULL;
	mPara.o_fmt_ctx = NULL;

	delete mPara.mutex;
	delete mPara._write_finish;

	mPara.r_Thread->deleteLater();
	mPara.w_Thread->deleteLater();
}

void FFmpegController::setCacheInterval( uint32_t interval )
{
	mPara._interval_before_trig = interval;
}

void FFmpegController::setStoreInterval( uint32_t interval )
{
	mPara._interval_after_trig = interval;
}

void FFmpegController::setURL( const QString& url )
{
	mPara.i_filename = url.toLatin1();
}

bool FFmpegController::start()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();
	if ( pReader->isStared() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	mPara.b_r_finish = false;
	mPara.b_w_finish = false;

	pReader->clrTrigger();
	pReader->doStart();
	pReader->doRecord();

	return true;
}

bool FFmpegController::trigger()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	if ( !pReader->isStared() || pReader->isTrigger() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	mPara.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << mPara.o_filename;

	mPara.b_r_finish = false;
	mPara.b_w_finish = false;

	pReader->setTrigger();
	pWriter->doStart();
	pWriter->doRecord();

	return true;
}

void FFmpegController::directStore()
{
	mPara.b_r_finish = false;
	mPara.b_w_finish = false;

	pReader->setTrigger();
	pReader->doStart();
	pReader->doRecord();
}

bool FFmpegController::terminate()
{
	qDebug() << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 ) << QThread::currentThread();

	if ( !pReader->isStared() ) {
		qDebug() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << QString( "%1, %2" ).arg( __FUNCTION__ ).arg( __LINE__, 3 );
		return false;
	}

	mPara.b_r_finish  = false;
	mPara.b_w_finish = false;
	pReader->doTerminate();

	mPara.o_filename = QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh_mm_ss" ).toLatin1().append( ".avi" );
	qDebug() << "#############################################################" << mPara.o_filename;
	pWriter->doTerminate();
	pWriter->doRecord();

	return true;
}