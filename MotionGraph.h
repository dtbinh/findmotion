#ifndef MOTIONGRAPH_H
#define MOTIONGRAPH_H
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QMessageBox>
using namespace std;

#define GloveDataPath "../Data/GloveData"
#define CalibDataPath "../Data/CalibData"
#define GConfigPath "../Data/graphConfig.txt"
#define CConfigPath "../Data/CalibConfig.txt"

#define SG_RAW_SEMG 1
#define NINA_PRO_EMG 2
#define NINA_PRO_GLOVE 4
#define NINA_PRO_MOV 3

class MGnode
{
public:
	MGnode(){}
	MGnode(vector<vector<double>>data_set,int index):motionClips(data_set),mIndex(index){}
	void construction(vector<vector<double>>data_set,int index);
	vector<vector<double>> motionClips;
	int mIndex;
};
class MotionGraph
{
public:
	MotionGraph(){};
	bool LoadConfig(string GraphPath, string CalibPath);
	bool CreateGraph();
	void loadCalibDataFromFile(string calib_name);
	int read_data(const char* file_name, vector<vector<double>>* data_set, vector<double> gain, vector<double> offset);
	void SplitString(const string& str, const string& split, QStringList& q_str_list);
	vector<MGnode>mNode;

	vector<string> MgClipsName;
	vector<string> MgCalib;
	vector<int> ClipsIndex;
	vector<double> gain;
	vector<double> offset;
};
#endif