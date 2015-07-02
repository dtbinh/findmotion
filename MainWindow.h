#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QAction>
#include "QTOgreWindow.h"
#include <string>
#include <fstream>
#include <vector>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QFileDialog>
#include <QMenu>
#include <QToolBar>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QSlider>
#include <QIntValidator>
#include <QMessageBox>

#define SG_RAW_SEMG 1
#define NINA_PRO_EMG 2
#define NINA_PRO_GLOVE 4
#define NINA_PRO_MOV 3

using namespace std;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = 0);
signals:
	void sendPlayStat(bool);
private slots:
	void inGloveDataOP();
	void inCalibDataOP();
	void startWindowOP();
	void resetAnimaOP();
	void PlayPauseOp();
	void jumpToCertainFrame_slider(int count);
	void jumpToPreFrame();
	void jumpTOcertainFrame();
	void jumpToNextFrame();
	void updateCurFrame();
	void saveClipsOP();
	void saveCurOP();
private:
	void createActions();
	void createMenus();
	void createStatusBar();
	void createToolBar();
	void loadAnimeFromFile(string filename,string calib_name);
	void loadCalibDataFromFile(string calib_name);
	int read_data(const char* file_name, vector<vector<double>>* data_set, vector<double> gain, vector<double> offset);
	void SplitString(const string& str, const string& split, QStringList& q_str_list);
	void iniDockWidget();
	void initimeDockWidget();

private:
	QMdiArea* mainArea;
	
	QMenu *fileMenu;
	QAction *inputGloveData;
	QAction *inputCalibData;
	QAction *startWindow;
	QAction *resetAnima;

	QToolBar* fileToolBar;

	QLabel *locationLabel;
	QLabel *formulaLabel;
	QLabel *descriptionLabel;

	QDockWidget *dockWidget;
	QDockWidget *timeDockWidget;

	QPushButton* onOff;
	QLineEdit* fromFrame;
	QLineEdit* toFrame;
	QLineEdit* txtName;
	QPushButton *saveSection;
	QPushButton *saveCurrent;

	QLineEdit *firstFrame;
	QPushButton *preFrame;
	QLineEdit *curFrame;
	QPushButton *nextFrame;
	QLineEdit *lastFrame;
	QSlider *timeSlider;


	string mGloveData;
	string mCalibData;

	vector<double> gain;
	vector<double> offset;
	vector<vector<double>> anime_data_;

	QTOgreWindow *handWindow;
	QMdiSubWindow* subWindowModel;
	QWidget* handWidget;

	bool playStat;
	int FrameCount;
	int CurrentFrame;
	
};
#endif