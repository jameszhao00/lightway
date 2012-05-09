#pragma once
#include <vector>
#include <map>
#include <QString>
#include "lwmath.h"
using namespace std;
struct SampleHistoryRecord
{
	int depth;
	string name;
	float3 f3Value;	
};
class SampleHistoryRecorder : public QObject
{
	Q_OBJECT
signals:
	void sampleRecorded(const vector<vector<vector<SampleHistoryRecord>>>*, int);
	void finishedRecording();
public:
	SampleHistoryRecorder(int maxDepth) : sampleIdx_(0), maxDepth_(maxDepth), recording_(false) { }
	void newSample(int2 xy);
	void record(int2 xy, int depth, string name, float3 value);
	void beginRecording(int2 pos, int count) 
	{ 
#ifndef LWFAST
		assert(count > 0);
		targetPosition_ = pos; 
		data_.clear();
		sampleIdx_ = -1;
		recording_ = true;
		recordCount_ = count;
#endif
	}
	void endRecording()
	{
#ifndef LWFAST
		recording_ = false;
		emit finishedRecording();
#endif
	}
private:
	vector<vector<vector<SampleHistoryRecord>>> data_;
	int sampleIdx_;
	int recordCount_;
	int maxDepth_;
	int2 targetPosition_;
	bool recording_;
};
const int MAX_DEPTH = 8;
struct SampleDebugger
{
	SampleDebugger() : shr(MAX_DEPTH)
	{
	}
	SampleHistoryRecorder shr;
};