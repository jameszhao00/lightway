#pragma once

#include <QtGui/QTableView>
#include <QStandardItemModel>
#include "SampleHistoryRecorder.h"
class SampleHistoryViewer : public QTableView
{
	Q_OBJECT
public slots:
	void sampleRecorded(const vector<vector<vector<SampleHistoryRecord>>>* allSamplesPtr, int sampleIdx)
	{		
		//this method is kinda expensive... but we gotta recompute everything
		//because each sample may have different entries
		const vector<vector<vector<SampleHistoryRecord>>>& allSamples = *allSamplesPtr;
		model.clear();
		//compute a entry name -> row (relative to its depth) mapping 
		for(int sample_i = 0; sample_i < sampleIdx+1; sample_i++)
		{
			for(int depth_i = 0; depth_i < allSamples[sample_i].size(); depth_i++)
			{
				const vector<SampleHistoryRecord>& recordsForDepth = allSamples[sample_i][depth_i];
				if(keyToRowMapping.size() < (depth_i + 1))
				{
					keyToRowMapping.push_back(QMap<string, int>());
				}
				for(int entry_i = 0; entry_i < recordsForDepth.size(); entry_i++)
				{
					if(!keyToRowMapping[depth_i].contains(recordsForDepth[entry_i].name))
					{
						keyToRowMapping[depth_i][recordsForDepth[entry_i].name] = keyToRowMapping[depth_i].size();
					}
				}
				
			}
		}
		//set the row names
		{
			int rowOffset = 0;
			QStringList rowLables;
			
			for(int depth_i = 0; depth_i < keyToRowMapping.size(); depth_i++)
			{
				const QMap<string, int>& rowMappingForDepth = keyToRowMapping[depth_i];
				for(auto it = rowMappingForDepth.begin(); it != rowMappingForDepth.end(); it++)
				{
					QStandardItem* rowHeader = new QStandardItem(QString::number(depth_i) + " - " + QString::fromStdString(it.key()));
					model.setVerticalHeaderItem(rowOffset + it.value(), rowHeader);
				}
				rowOffset += rowMappingForDepth.size();
			}
		}
		//row offset based on current depth
		{
			for(int sample_i = 0; sample_i < (sampleIdx + 1); sample_i++)
			{
				int rowOffset = 0;
				const vector<vector<SampleHistoryRecord>>& currentColumn = allSamples[sample_i];		
				for(int depth_i = 0; depth_i < keyToRowMapping.size(); depth_i++)
				{			
					for(int history_i = 0; history_i < allSamples[sample_i][depth_i].size(); history_i++)
					{				
						auto & entry = allSamples[sample_i][depth_i][history_i];
						QString text = 
							QString::number(entry.f3Value.x) + ", " + 
							QString::number(entry.f3Value.y) + ", " + 
							QString::number(entry.f3Value.z);
						QStandardItem* item = new QStandardItem(text);
						//the row idx relative to row 0
						int computedRowIdx = rowOffset + keyToRowMapping[depth_i][entry.name];
						model.setItem(computedRowIdx, sample_i, item);
					}
					rowOffset += keyToRowMapping[depth_i].size();
				}
			}
		}
	}
	void resetViewer()
	{
		model.clear();
		keyToRowMapping.clear();
	}
public:
	SampleHistoryViewer(QWidget* parent = nullptr) : QTableView(parent), historyRecorder_(nullptr) 
	{ 
		setModel(&model);
	}
	void syncTo(SampleHistoryRecorder* shr)
	{
		if(historyRecorder_ != nullptr)
		{
			disconnect(this, "sampleRecorded");
		}
		reset();

		historyRecorder_ = shr;

		connect(historyRecorder_, SIGNAL(sampleRecorded(const vector<vector<vector<SampleHistoryRecord>>>*, int)),
			this, SLOT(sampleRecorded(const vector<vector<vector<SampleHistoryRecord>>>*, int)));
	}
private:
	QVector<QMap<string, int>> keyToRowMapping;
	QStandardItemModel model;
	SampleHistoryRecorder* historyRecorder_;
};
