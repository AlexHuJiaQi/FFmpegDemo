#pragma once

#include "AbstractReadWriter.h"

class FFmpegReader : public AbstractReadWriter
{
	Q_OBJECT
public:
	explicit FFmpegReader( QObject* parent = nullptr );
	virtual ~FFmpegReader();

	virtual void setParameter( FFmpegParameter* para )override;

	virtual bool isTrigger()  final { return b_trigger; }
	virtual void setTrigger() final { b_trigger = true; }
	virtual void clrTrigger() final { b_trigger = false; }

public slots:
	virtual void doWork()override;

private:
	double time_diff_a = 0;
	double time_diff_v = 0;
	double time_trig_v = 0;
	void parse_packet( AVPacket* p_packet );

private:
	bool b_trigger;
	FFmpegParameter* m_para;
};