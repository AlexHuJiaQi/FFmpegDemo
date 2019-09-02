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
	explicit FFmpegController( uint32_t interval_1,
							   uint32_t interval_2,
							   QByteArray url,
							   QObject* parent = nullptr );
	virtual ~FFmpegController();

	bool start();
	bool termination();
	bool trigger();

signals:
	void toDoWorker();

public:
	FFmpegReader* pReader;
	FFmpegWriter* pWriter;
	FFmpegParameter mPara;
};

#endif
