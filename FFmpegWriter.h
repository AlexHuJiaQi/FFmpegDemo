#pragma once

#include "GlobleDefine.h"

class FFmpegWriter : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegWriter( QObject* parent = nullptr );
	virtual ~FFmpegWriter();

	void setParameter( FFmpegParameter* para );

public slots:
	void doWork();

private:
	FFmpegParameter* m_para;
};