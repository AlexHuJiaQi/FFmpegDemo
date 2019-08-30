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
	explicit FFmpegController( QByteArray url, QObject* parent = nullptr );
	virtual ~FFmpegController();

	bool start();
	bool termination();
	bool trigger();

signals:
	void toDoWorker();

public:
	FFmpegReader* pReader;
	FFmpegWriter* pWriter;
	FFmpegParameter m_para;
};