#ifndef QTOGREWINDOW_H
#define QTOGREWINDOW_H

/*
Source: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Integrating+Ogre+into+QT5

For the purposes of creating a bare-bones Qt implementation I found that I only need to include these three (3) Qt headers.
*/
#include <QtWidgets/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QWindow>
#include <QResizeEvent>
#include <iostream>
#include <string>

using namespace std;

/*
As expected; the Ogre3D header.
*/
#include <Ogre.h>

/*
Changed SdkCameraMan implementation to work with QKeyEvent, QMouseEvent, QWheelEvent
*/
#include "SdkQtCameraMan.h"

/*
With the headers included we now need to inherit from QWindow.
*/
class QTOgreWindow : public QWindow, public Ogre::FrameListener
{
	/*
	A QWindow still inherits from QObject and can have signals/slots; we need to add the appropriate Q_OBJECT keyword so that Qt's intermediate compiler can do the necessary wireup between our class and the rest of Qt.
	*/

	Q_OBJECT

public:
	explicit QTOgreWindow(vector<vector<double>> animData,QWindow *parent = NULL);
	~QTOgreWindow();

	/*
	We declare these methods virtual to allow for further inheritance.
	*/
	virtual void render(QPainter *painter);
	virtual void render();
	virtual void initialize();
	virtual void createScene();
	int getCurFrame();
	void setCurFrame(int curFrame);
	void setFrame(vector<vector<double>> animData);
#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
	virtual void createCompositor();
#endif

	void setAnimating(bool animating);
signals:
	void sendFrameFinish();
public slots:

	virtual void renderLater();
	virtual void renderNow();
	void PlayPauseOp(bool on);

	/*
	We use an event filter to be able to capture keyboard/mouse events. More on this later.
	*/
	virtual bool eventFilter(QObject *target, QEvent *event);
	//void resizeEvent(QResizeEvent *e);
	//void showMaximized();

protected:
	/*
	Ogre3D pointers added here. Useful to have the pointers here for use by the window later.
	*/
	Ogre::Root* m_ogreRoot;
	Ogre::RenderWindow* m_ogreWindow;
	Ogre::SceneManager* m_ogreSceneMgr;
	Ogre::Camera* m_ogreCamera;
	Ogre::ColourValue m_ogreBackground;
    OgreQtBites::SdkQtCameraMan* m_cameraMan;
	Ogre::SceneNode* hand_node_;
	vector<Ogre::Bone*> bone_by_index;

	bool m_update_pending;
	bool m_animating;
	bool play_flag;
	int frame;
	vector<vector<double>> anime_data_;
	double little_last ;
	int timerId;
	int delayTime;

	/*
	The below methods are what is actually fired when they keys on the keyboard are hit.
	Similar events are fired when the mouse is pressed or other events occur.
	*/
	virtual void keyPressEvent(QKeyEvent * ev);
	virtual void keyReleaseEvent(QKeyEvent * ev);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseWheelEvent(QWheelEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
	virtual void exposeEvent(QExposeEvent *event);
	virtual bool event(QEvent *event);
	void timerEvent(QTimerEvent *tEvent);
	
	void InitialFromDotScene(void);
	void setBonesToStandard();
	void bindingData(vector<double> data);
	void bindingData2(vector<double> data);
	void Handreset();
	void calculateAngle(double w,double p1,double p2,double b1,double &b21,double &b22);
	void saveDepthTrain();

    /*
    FrameListener method
    */
    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

	void log(Ogre::String msg);
	void log(QString msg);
};

#endif // QTOGREWINDOW_H
