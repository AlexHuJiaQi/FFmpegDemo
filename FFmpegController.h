#pragma once

#include <QObject>
#include <QWaitCondition>

#include "FFmpegReader.h"
#include "FFmpegWriter.h"
#include "GlobleDefine.h"

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
	char* i_filename;
	char* o_filename;

	FFmpegReader* pReader;
	FFmpegWriter* pWriter;

	FFmpegParameter m_para;
};