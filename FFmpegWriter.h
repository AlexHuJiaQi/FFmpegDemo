#pragma once

#include "AbstractReadWriter.h"

class FFmpegWriter : public AbstractReadWriter
{
	Q_OBJECT
public:
	explicit FFmpegWriter( QObject* parent = nullptr );
	virtual ~FFmpegWriter();

	virtual void setParameter( FFmpegParameter* para )override;

public slots:
	virtual void doWork()override;

private:
	FFmpegParameter* m_para;
};