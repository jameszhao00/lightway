#include "stdafx.h"
#include "SampleHistoryRecorder.h"

void SampleHistoryRecorder::newSample(int2 xy)
{
#ifndef LWFAST
	if(!recording_) return;
	if(xy != targetPosition_) return;

	if(sampleIdx_ > -1)
	{
		emit sampleRecorded(&data_, sampleIdx_);
	}
	sampleIdx_++;
	if(sampleIdx_ > (recordCount_ - 1))
	{
		endRecording();
	}
	else
	{
		data_.push_back(vector<vector<SampleHistoryRecord>>());
		data_[sampleIdx_].resize(maxDepth_, vector<SampleHistoryRecord>());
	}
#endif
}
void SampleHistoryRecorder::record(int2 xy, int depth, string name, float3 value)
{
#ifndef LWFAST
	if(xy != targetPosition_ || !recording_) return;
	//new sample hasn't be called yet
	if(data_.size() == 0) return;
	SampleHistoryRecord record;
	record.depth = depth;
	record.name = name;
	record.f3Value = value;
	data_[sampleIdx_][depth].push_back(record);
#endif
}