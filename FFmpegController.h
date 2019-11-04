#ifndef FFMPEGCONTROLLER_H
#define FFMPEGCONTROLLER_H

#include <QObject>
#include <QWaitCondition>

#include "FFmpegReader.h"
#include "FFmpegWriter.h"
#include "AbstractReadWriter.h"

class FFmpegController : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegController( QObject* parent = nullptr );
	virtual ~FFmpegController();

	void setCacheInterval( uint32_t interval );
	void setStoreInterval( uint32_t interval );
	void setURL( const QString& url );

	bool start();
	bool terminate();
	bool trigger();
	void directStore();

signals:
	void toDoWorker();

public:
	FFmpegReader* pReader;
	FFmpegWriter* pWriter;
	FFmpegParameter mPara;
};

#endif
