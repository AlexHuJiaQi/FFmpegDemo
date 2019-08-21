#pragma once

#include "GlobleDefine.h"

class FFmpegReader : public QObject
{
	Q_OBJECT
public:
	explicit FFmpegReader( QObject* parent = nullptr );
	virtual ~FFmpegReader();

	void setParameter( FFmpegParameter* para );

public slots:
	void doWork();

private:
	FFmpegParameter* m_para;
};