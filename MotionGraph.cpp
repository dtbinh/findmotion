#include "MotionGraph.h"

bool MotionGraph::LoadConfig(string GraphPath, string CalibPath)
{
	
	string CalibFile;
	string motionData;
	QString Cdata;
	string ClipsName;
	int index;
	QStringList dataList;
	ifstream InputFile(CalibPath);
	if (!InputFile.is_open())
	{
		return false;
	}

	while(getline(InputFile,CalibFile))
	{
		MgCalib.push_back(CalibFile);
	}
	InputFile.close();

	InputFile.open(GraphPath);

	if (!InputFile.is_open())
	{
		return false;
	}

	while(getline(InputFile,motionData))
	{
		Cdata.fromStdString(motionData);
		dataList = Cdata.split(" ",QString::SkipEmptyParts);
		ClipsName = dataList.at(0).toStdString();
		MgClipsName.push_back(ClipsName);
		index = dataList.at(1).toInt();
		ClipsIndex.push_back(index);
	}

	InputFile.close();

    return true;

}

bool MotionGraph::CreateGraph()
{
	string GlovePath(GloveDataPath);
	string CalibPath(CalibDataPath);
	MGnode newNode;
	vector<vector<double>> newDataSet;
	int newIndex;
	if(!(MgClipsName.size()==MgCalib.size()))
	{
		QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("配置文件错误"), QMessageBox::Ok);
		return false;
	}


	for (int i = 0;i<MgClipsName.size();i++)
	{
		GlovePath = GlovePath+"/"+ MgClipsName[i];
		CalibPath = CalibPath+"/"+MgCalib[i];
		loadCalibDataFromFile(CalibPath);
		read_data(GlovePath.data(),&newDataSet,gain,offset);
		newNode.construction(newDataSet,ClipsIndex[i]);
		mNode.push_back(newNode);
	}

	
}

void MotionGraph::loadCalibDataFromFile(string calib_name)
{
	fstream calibfile;
	string sub;
	calibfile.open(calib_name.data(),ios::in);
	int i;
	//获取gain，第一行
	gain.clear();
	offset.clear();
	for (i = 0;i < 21;i++)
	{
		getline(calibfile,sub,' ');
		gain.push_back(atof(sub.c_str()));
	}
	getline(calibfile,sub,'\n');
	gain.push_back(atof(sub.c_str()));

	//获取offset，第二行
	for (i = 0;i < 21;i++)
	{
		getline(calibfile,sub,' ');
		offset.push_back(atof(sub.c_str()));
	}
	getline(calibfile,sub,'\n');
	offset.push_back(atof(sub.c_str()));
}

int MotionGraph::read_data(const char* file_name, vector<vector<double>>* data_set, vector<double> gain, vector<double> offset)
{
	ifstream fs;
	string line;
	int dataset_idx;

	data_set->clear();
	//	locale::global(locale(""));
	fs.open(file_name);
	//	locale::global(locale("C"));
	if (!fs)
	{
		return -1;
	}

	getline(fs,line);
	int test_index = line.find(' ');
	if (test_index == -1)
		return -1;

	/*const char* header_line = line.c_str();*/
	QStringList header_row;
	SplitString(line, " ", header_row);

	double header_data_f = header_row.at(0).toDouble();
	if (header_data_f != 0)
	{
		if (header_data_f == 2011.0)
		{
			if (header_row.size() == 7)
			{
				dataset_idx = NINA_PRO_MOV;
			}
			else if (header_row.size() == 28)
			{
				dataset_idx = NINA_PRO_GLOVE;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else
	{
		return -1;		
	}
	switch (dataset_idx)
	{
	case NINA_PRO_GLOVE:
		{
			double basetime_yy = header_row.at(0).toDouble();
			double basetime_mm = header_row.at(1).toDouble();
			double basetime_dd = header_row.at(2).toDouble();
			double basetime_h = header_row.at(3).toDouble();
			double basetime_m = header_row.at(4).toDouble();
			double basetime_s = header_row.at(5).toDouble();
			vector<double> header_double;
			double data;
			header_double.push_back(0.0);
			for (int i = 6; i < header_row.count(); i++)
			{
				data = gain.at(i-6) * (offset.at(i-6) - header_row.at(i).toDouble());
				header_double.push_back(data);
			}
			/*cout<<endl;*/
			(*data_set).push_back(header_double);
			while (getline(fs, line))
			{
				/*const char* data_line = line.c_str();*/
				QStringList data_row;
				vector<double> data_double;
				SplitString(line, " ", data_row);
				double data_h = data_row.at(3).toDouble();
				double data_m = data_row.at(4).toDouble();
				double data_s = data_row.at(5).toDouble();
				data = (data_h - basetime_h)*3600 + (data_m - basetime_m)*60 + (data_s - basetime_s);
				/*cout << data << " ";*/
				data_double.push_back(data);

				for (int i = 6; i < data_row.count(); i++)
				{
					data = gain.at(i-6) * (offset.at(i-6) - data_row.at(i).toDouble());
					data_double.push_back(data);
				}
				/*cout<<endl;*/
				(*data_set).push_back(data_double);
			}
			return NINA_PRO_GLOVE;
		}
		break;
	case NINA_PRO_MOV:
		{
			vector<double> header_double;
			double data;
			for (int i = 0; i < header_row.count(); i++)
			{
				data = header_row.at(i).toDouble();
				/*cout << data << " ";*/
				header_double.push_back(data);
			}
			/*cout<<endl;*/
			(*data_set).push_back(header_double);
			while (getline(fs, line))
			{
				/*const char* data_line = line.c_str();*/
				QStringList data_row;
				vector<double> data_double;
				SplitString(line, " ", data_row);
				for (int i = 0; i < data_row.count(); i++)
				{
					data = data_row.at(i).toDouble();
					/*cout << data << " ";*/
					data_double.push_back(data);
				}
				/*cout<<endl;*/
				(*data_set).push_back(data_double);

			}
			return NINA_PRO_MOV;
		}
		break;
	default:
		return -1;
		break;
	}
}

void MotionGraph::SplitString(const string& str, const string& split, QStringList& q_str_list)
{
	QString q_str = QString::fromLocal8Bit(str.c_str());
	QString q_split = QString::fromLocal8Bit(split.c_str());
	q_str_list = q_str.split(q_split, QString::SkipEmptyParts);
}

void MGnode::construction(vector<vector<double>>data_set,int index)
{
	motionClips = data_set;
	mIndex = index;
}