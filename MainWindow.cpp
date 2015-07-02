#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent /* = 0 */)
{
	playStat = false;
	setWindowTitle("motionEdit");
	setGeometry(100,100,1200,900);

	mainArea = new QMdiArea;
	setCentralWidget(mainArea);
	createActions();
	createMenus();
	createToolBar();
	createStatusBar();

}

void MainWindow::createActions()
{
	inputGloveData = new QAction(("Input glove"),this);
	connect(inputGloveData, SIGNAL(triggered()),this,SLOT(inGloveDataOP()));

	inputCalibData = new QAction(("Input Calib"),this);
	connect(inputCalibData,SIGNAL(triggered()),this,SLOT(inCalibDataOP()));

	startWindow = new QAction(("start Window"),this);
	connect(startWindow,SIGNAL(triggered()),this,SLOT(startWindowOP()));

	resetAnima = new QAction(("resetAnima"),this);
	connect(resetAnima,SIGNAL(triggered()),this,SLOT(resetAnimaOP()));
}

void MainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu(("File"));
	fileMenu->addAction(inputGloveData);
	fileMenu->addAction(inputCalibData);
	fileMenu->addAction(startWindow);
	fileMenu->addAction(resetAnima);
}

void MainWindow::createToolBar()
{
	fileToolBar = addToolBar("File");
	fileToolBar->addAction(inputGloveData);
	fileToolBar->addAction(inputCalibData);
	fileToolBar->addAction(startWindow);
	fileToolBar->addAction(resetAnima);
}

void MainWindow::createStatusBar()
{
	locationLabel = new QLabel("Status Bar");
	locationLabel->setAlignment(Qt::AlignHCenter);                         //这里就是一个对齐方式，是一个水平居中的设置
	locationLabel->setMinimumSize(locationLabel->sizeHint());              //设置标签的理想大小

	formulaLabel = new QLabel;                                             
	formulaLabel->setIndent(7);                                            //这里是设置标签缩进几个字符
	descriptionLabel = new QLabel;


	statusBar()->addWidget(locationLabel);
	statusBar()->addWidget(formulaLabel, 1);                            //这后面的参数是1表示伸展因子是可伸展，默认是0表示不可伸展
	statusBar()->addWidget(descriptionLabel, 1); 
}

void MainWindow::inGloveDataOP()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Input Glove"),".",tr("Glove files(*.txt)")); 
	if (fileName.isEmpty())
	{
		return ;
	}

	mGloveData = fileName.toStdString();

}

void MainWindow::inCalibDataOP()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Input Calib"),".",tr("Calib files(*.txt)")); 
	if (fileName.isEmpty())
	{
		return ;
	}

	mCalibData = fileName.toStdString();
	loadAnimeFromFile(mGloveData,mCalibData);
}

void MainWindow::startWindowOP()
{
	handWindow = new QTOgreWindow(anime_data_);
	handWidget = QWidget::createWindowContainer(handWindow);
	setFocusPolicy(Qt::WheelFocus);
	setFocusProxy(handWidget);
	subWindowModel = mainArea->addSubWindow(handWidget);
	subWindowModel->setGeometry(0,0,800,600);

	subWindowModel->show();
	iniDockWidget();
	initimeDockWidget();
}

void MainWindow::iniDockWidget()
{
	dockWidget = new QDockWidget(("Operation Panel"),this);
	dockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
	dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
	dockWidget->setFloating(false);

	QWidget *boxvector = new QWidget(dockWidget);
	QFormLayout *formLayout = new QFormLayout(dockWidget);
	onOff = new QPushButton("play",dockWidget);
	connect(onOff,SIGNAL(clicked()),this,SLOT(PlayPauseOp()));
	connect(this,SIGNAL(sendPlayStat(bool)),handWindow,SLOT(PlayPauseOp(bool)));
	formLayout->addRow(new QLabel("on/off:"),onOff);

	fromFrame = new QLineEdit(dockWidget);
	toFrame = new QLineEdit(dockWidget);
	txtName = new QLineEdit(dockWidget);
	saveSection = new QPushButton("saveSection",dockWidget);
	connect(saveSection,SIGNAL(clicked()),this,SLOT(saveClipsOP()));

	saveCurrent = new QPushButton("saveCurrent",dockWidget);
	connect(saveCurrent,SIGNAL(clicked()),this,SLOT(saveCurOP()));

	formLayout->addRow(new QLabel("from:"),fromFrame);
	formLayout->addRow(new QLabel("to:"),toFrame);
	formLayout->addRow(new QLabel("txtName"),txtName);
	formLayout->addRow(saveSection,saveCurrent);



	QVBoxLayout *Mainwidgetlayout = new QVBoxLayout(dockWidget);
	Mainwidgetlayout->addLayout(formLayout);
	Mainwidgetlayout->addStretch();
	boxvector->setLayout(Mainwidgetlayout);

	dockWidget->setWidget(boxvector);
	//dockWidget->setMaximumWidth(200);
	dockWidget->setVisible(true);

	addDockWidget(Qt::RightDockWidgetArea,dockWidget);
}

void MainWindow::initimeDockWidget()
{
	timeDockWidget = new QDockWidget(("Frame Panel"),this);
	timeDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
	timeDockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
	timeDockWidget->setFloating(false);

	QWidget *boxvector = new QWidget(timeDockWidget);
	timeSlider = new QSlider(Qt::Horizontal, timeDockWidget);
	FrameCount = anime_data_.size();
	timeSlider->setRange(1,FrameCount);
	connect(timeSlider,SIGNAL(sliderMoved(int)),this,SLOT(jumpToCertainFrame_slider(int)));

	QHBoxLayout *planelayout = new QHBoxLayout(timeDockWidget);
	firstFrame = new QLineEdit(timeDockWidget);
	firstFrame->setText(QString("1"));
	firstFrame->setReadOnly(true);

	preFrame = new QPushButton(("Back"),timeDockWidget);
	connect(preFrame,SIGNAL(clicked()),this,SLOT(jumpToPreFrame()));

	curFrame = new QLineEdit(timeDockWidget);
	CurrentFrame = handWindow->getCurFrame();
	curFrame->setText(QString::number(CurrentFrame+1,10));
	curFrame->setValidator(new QIntValidator(1,FrameCount,curFrame));
	//curFrame->setReadOnly(true);
	connect(curFrame,SIGNAL(returnPressed()),this,SLOT(jumpTOcertainFrame()));
	connect(handWindow,SIGNAL(sendFrameFinish()),this,SLOT(updateCurFrame()));
	connect(preFrame,SIGNAL(clicked()),this,SLOT(updateCurFrame()));

	nextFrame = new QPushButton(("Next"),timeDockWidget);
	connect(nextFrame,SIGNAL(clicked()),this,SLOT(jumpToNextFrame()));
	connect(nextFrame,SIGNAL(clicked()),this,SLOT(updateCurFrame()));

	lastFrame = new QLineEdit(timeDockWidget);
	lastFrame->setText(QString::number(FrameCount,10));
	lastFrame->setReadOnly(true);

	planelayout->addWidget(firstFrame);
	planelayout->addStretch();
	planelayout->addStretch();
	planelayout->addStretch();

	planelayout->addWidget(preFrame);
	planelayout->addStretch();
	planelayout->addStretch();
	planelayout->addWidget(curFrame);
	planelayout->addStretch();
	planelayout->addStretch();
	planelayout->addWidget(nextFrame);
	planelayout->addStretch();
	planelayout->addStretch();
	planelayout->addStretch();

	planelayout->addWidget(lastFrame);

	QVBoxLayout *allLayout = new QVBoxLayout(timeDockWidget);
	allLayout->addWidget(timeSlider);
	allLayout->addLayout(planelayout);
	boxvector->setLayout(allLayout);
	timeDockWidget->setWidget(boxvector);
	timeDockWidget->setVisible(true);

	addDockWidget(Qt::BottomDockWidgetArea,timeDockWidget);

}

void MainWindow::loadAnimeFromFile(string filename,string calib_name)
{
	vector<double> gain_o;
	vector<double> offset_o;
	gain_o.resize(0);
	offset_o.resize(0);

	loadCalibDataFromFile(calib_name);

	for (int i = 0;i<22;i++)
	{
		gain_o.push_back(-1.0);
		offset_o.push_back(0.0);
	}

	read_data(filename.data(),&anime_data_,gain,offset);
}

void MainWindow::loadCalibDataFromFile(string calib_name)
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

int MainWindow::read_data(const char* file_name, vector<vector<double>>* data_set, vector<double> gain, vector<double> offset)
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

void MainWindow::SplitString(const string& str, const string& split, QStringList& q_str_list)
{
	QString q_str = QString::fromLocal8Bit(str.c_str());
	QString q_split = QString::fromLocal8Bit(split.c_str());
	q_str_list = q_str.split(q_split, QString::SkipEmptyParts);
}

void MainWindow::PlayPauseOp()
{
	playStat = !playStat;
	if (playStat)
	{
		onOff->setText(QString("Pause"));
		emit sendPlayStat(playStat);
	}
	else
	{
		onOff->setText(QString("Play"));
		emit sendPlayStat(playStat);
	}
	
}

void MainWindow::jumpToCertainFrame_slider(int count)
{
	handWindow->setCurFrame(count-1);
	curFrame->setText(QString::number(count,10));
}

void MainWindow::jumpToPreFrame()
{
	handWindow->setCurFrame(handWindow->getCurFrame()-1);
	if(handWindow->getCurFrame()<0)
	{
		handWindow->setCurFrame(0);
	}
}

void MainWindow::jumpTOcertainFrame()
{
	QString curframecount = curFrame->text();
	int count = curframecount.toInt();
	handWindow->setCurFrame(count-1);
	timeSlider->setValue(count);
}

void MainWindow::jumpToNextFrame()
{
	handWindow->setCurFrame(handWindow->getCurFrame()+1);
	if(handWindow->getCurFrame()>=anime_data_.size())
	{
		handWindow->setCurFrame(anime_data_.size()-1);
	}
}

void MainWindow::updateCurFrame()
{
	CurrentFrame = handWindow->getCurFrame();
	curFrame->setText(QString::number(CurrentFrame+1,10));
	timeSlider->setValue(CurrentFrame);
}

void MainWindow::saveClipsOP()
{
	QString fromText = fromFrame->text();
	QString toText = toFrame->text();
	QString NameText = txtName->text();
//	QString path("F:/Laboratory.CAPG.Project/GestureSystem/motionEdit/findMotion/GestureData");
	QString path("../toData");
	int fromLine = fromText.toInt();
	int toLine = toText.toInt();
	string lineText;

	path = path+"/"+ NameText+".txt";

	ifstream inputGlove(mGloveData);
	if(!inputGlove.is_open())
	{
		QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
		return;
	}

	for(int i = 0;i<fromLine-1;i++)getline(inputGlove,lineText);

	ofstream outGlove(path.toStdString());
	if (!outGlove.is_open())
	{
		QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
		return;
	}

	for (int i = 0;i<toLine-fromLine+1;i++)
	{
		getline(inputGlove,lineText);
		outGlove<<lineText<<endl;
	}

	inputGlove.close();
	outGlove.close();
}

void MainWindow::saveCurOP()
{
	QString NameText = txtName->text();
//	QString path("F:/Laboratory.CAPG.Project/GestureSystem/motionEdit/findMotion/GestureData");
	QString path("../toData");
	string lineText;
	path = path+"/"+ NameText+".txt";
	int curLine = handWindow->getCurFrame();

	ifstream inputGlove(mGloveData);
	if(!inputGlove.is_open())
	{
		QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
		return;
	}

	for(int i = 0;i<curLine;i++)getline(inputGlove,lineText);

	ofstream outGlove(path.toStdString());
	if (!outGlove.is_open())
	{
		QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
		return;
	}

	getline(inputGlove,lineText);
	outGlove<<lineText<<endl;

	inputGlove.close();
	outGlove.close();

}

void MainWindow::resetAnimaOP()
{
	handWindow->setFrame(anime_data_);
	lastFrame->setText(QString::number(anime_data_.size(),10));
	timeSlider->setRange(1,anime_data_.size());
	curFrame->setValidator(new QIntValidator(1, anime_data_.size(), this));
	curFrame->setText("1");
	timeSlider->setValue(1);
	handWindow->setCurFrame(0);
}