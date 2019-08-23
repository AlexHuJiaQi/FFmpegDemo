#pragma once

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

	bool start();
	bool stop();
	bool trigger();

signals:
	void toDoWorker();

private:
	FFmpegReader* pReader;
	FFmpegWriter* pWriter;

	FFmpegParameter m_para;
};