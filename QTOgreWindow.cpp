#include "QTOgreWindow.h"
#include <QMessageBox>
#include "dot_sceneloader.h"
#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
#include <Compositor/OgreCompositorManager2.h>
#endif

/*
Note that we pass any supplied QWindow parent to the base QWindow class. This is necessary should we need to use our class within a container.
More on that later.
*/
QTOgreWindow::QTOgreWindow(vector<vector<double>> animData,QWindow *parent)
	: QWindow(parent)
	, m_update_pending(false)
	, m_animating(false)
	, m_ogreRoot(NULL)
	, m_ogreWindow(NULL)
	, m_ogreCamera(NULL)
    , m_cameraMan(NULL)
	, play_flag(false)
	, frame(0)
	, anime_data_(animData)
	, little_last(0.0)
	, delayTime(38)
{
	setAnimating(true);
	installEventFilter(this);
    m_ogreBackground = Ogre::ColourValue(0.0f, 0.5f, 1.0f);
	//setGeometry(0,0,600,400);
	timerId = startTimer(delayTime,Qt::PreciseTimer);
}

/*
Upon destruction of the QWindow object we destroy the Ogre3D scene.
*/
QTOgreWindow::~QTOgreWindow()
{
    if (m_cameraMan) delete m_cameraMan;
	delete m_ogreRoot;
}

/*
In case any drawing surface backing stores (QRasterWindow or QOpenGLWindow) of Qt are supplied to this class in any way we inform Qt that they will be unused.
*/
void QTOgreWindow::render(QPainter *painter)
{
	Q_UNUSED(painter);
}

/*
Our initialization function. Called by our renderNow() function once when the window is first exposed.
*/
void QTOgreWindow::initialize()
{
	
	/*
	As shown Ogre3D is initialized normally; just like in other documentation.
	*/
#ifdef _MSC_VER
	
	m_ogreRoot = new Ogre::Root(Ogre::String("plugins" OGRE_BUILD_SUFFIX ".cfg"));
//	m_ogreRoot = new Ogre::Root(Ogre::String("plugins.cfg"));
	
#else
	m_ogreRoot = new Ogre::Root(Ogre::String("plugins.cfg"));
#endif
	//Ogre::ConfigFile ogreConfig;
	
#ifdef _DEBUG
	m_ogreRoot->loadPlugin("RenderSystem_GL_d");
	m_ogreRoot->loadPlugin("Plugin_OctreeSceneManager_d");
#else
	m_ogreRoot->loadPlugin("RenderSystem_GL");
	m_ogreRoot->loadPlugin("Plugin_OctreeSceneManager");
#endif 
	
	/*

	Commended out for simplicity but should you need to initialize resources you can do so normally.

	ogreConfig.load("resources/resource_configs/resources.cfg");

	Ogre::ConfigFile::SectionIterator seci = ogreConfig.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements()) {
	secName = seci.peekNextKey();
	Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
	Ogre::ConfigFile::SettingsMultiMap::iterator i;
	for (i = settings->begin(); i != settings->end(); ++i) {
	typeName = i->first;
	archName = i->second;
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
	archName, typeName, secName);
	}
	}

	*/

	/*
	I found that only OpenGL seems to work with Qt; event when on Microsoft Windows. The application crashes when I tried DirectX rendering. Thus, I set the render system to OpenGL in accordance with the supplied "plugins.cfg" file.
	*/
	const Ogre::RenderSystemList& rsList = m_ogreRoot->getAvailableRenderers();
	Ogre::RenderSystem* rs = rsList[0];
	std::vector<Ogre::String> renderOrder;
#if defined(Q_OS_WIN)
	renderOrder.push_back("Direct3D9");
	renderOrder.push_back("Direct3D11");
#endif
	renderOrder.push_back("OpenGL");
	renderOrder.push_back("OpenGL 3+");
	for (std::vector<Ogre::String>::iterator iter = renderOrder.begin(); iter != renderOrder.end(); iter++)
	{
		for (Ogre::RenderSystemList::const_iterator it = rsList.begin(); it != rsList.end(); it++)
		{
			if ((*it)->getName().find(*iter) != Ogre::String::npos)
			{
				rs = *it;
				break;
			}
		}
		if (rs != NULL) break;
	}
	if (rs == NULL)
	{
		if (!m_ogreRoot->restoreConfig())
		{
			if (!m_ogreRoot->showConfigDialog())
				OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Abort render system configuration", "QTOgreWindow::initialize");
		}
	}
	QString dimensions = QString("%1 x %2").arg(this->width()).arg(this->height());
	rs->setConfigOption("Video Mode", dimensions.toStdString());
	rs->setConfigOption("Full Screen", "No");
	rs->setConfigOption("VSync", "Yes");
	m_ogreRoot->setRenderSystem(rs);
	m_ogreRoot->initialise(false);
	
	Ogre::NameValuePairList parameters;
	/*
	Flag within the parameters set so that Ogre3D initializes an OpenGL context on it's own.
	*/
	if (rs->getName().find("GL") <= rs->getName().size())
		parameters["currentGLContext"] = Ogre::String("false");

	/*
	We need to supply the low level OS window handle to this QWindow so that Ogre3D knows where to draw the scene. Below is a cross-platform method on how to do this.
	*/
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
	parameters["externalWindowHandle"] = Ogre::StringConverter::toString((size_t)(this->winId()));
	parameters["parentWindowHandle"] = Ogre::StringConverter::toString((size_t)(this->winId()));
	parameters["FSAA"] = "2";
#else
	parameters["externalWindowHandle"] = Ogre::StringConverter::toString((unsigned long)(this->winId()));
	parameters["parentWindowHandle"] = Ogre::StringConverter::toString((unsigned long)(this->winId()));
#endif

#if defined(Q_OS_MAC)
	parameters["macAPI"] = "cocoa";
	parameters["macAPICocoaUseNSView"] = "true";
#endif
	
	/*
	Note below that we supply the creation function for the Ogre3D window the width and height from the current QWindow object using the "this" pointer.
	*/
	m_ogreWindow = m_ogreRoot->createRenderWindow("Hand Render", this->width(), this->height(), false, &parameters);
	m_ogreWindow->setVisible(true);
//	QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
	/*
	The rest of the code in the initialization function is standard Ogre3D scene code. Consult other tutorials for specifics.
	*/
#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
	const size_t numThreads = std::max<int>(1, Ogre::PlatformInformation::getNumLogicalCores());
	Ogre::InstancingTheadedCullingMethod threadedCullingMethod = (numThreads > 1) ? Ogre::INSTANCING_CULLING_THREADED : Ogre::INSTANCING_CULLING_SINGLETHREAD;
	m_ogreSceneMgr = m_ogreRoot->createSceneManager(Ogre::ST_GENERIC, numThreads, threadedCullingMethod);
#else
	m_ogreSceneMgr = m_ogreRoot->createSceneManager(Ogre::ST_GENERIC);
#endif
	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	InitialFromDotScene();
	
/*
	m_ogreCamera = m_ogreSceneMgr->createCamera("MainCamera");
	m_ogreCamera->setPosition(Ogre::Vector3(0.0f, 0.0f, 10.0f));
	m_ogreCamera->lookAt(Ogre::Vector3(0.0f, 0.0f, -300.0f));
	m_ogreCamera->setNearClipDistance(0.1f);
	m_ogreCamera->setFarClipDistance(200.0f);
    m_cameraMan = new OgreQtBites::SdkQtCameraMan(m_ogreCamera);   // create a default camera controller


#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
	createCompositor();
#else
	Ogre::Viewport* pViewPort = m_ogreWindow->addViewport(m_ogreCamera);
	pViewPort->setBackgroundColour(m_ogreBackground);
#endif

	m_ogreCamera->setAspectRatio(Ogre::Real(m_ogreWindow->getWidth()) / Ogre::Real(m_ogreWindow->getHeight()));
	m_ogreCamera->setAutoAspectRatio(true);

	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	createScene();
*/
    m_ogreRoot->addFrameListener(this);
}

void QTOgreWindow::createScene()
{
	m_ogreSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));

#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
	Ogre::Entity* sphereMesh = m_ogreSceneMgr->createEntity(Ogre::SceneManager::PT_SPHERE);
#else
	Ogre::Entity* sphereMesh = m_ogreSceneMgr->createEntity("mySphere", Ogre::SceneManager::PT_SPHERE);
#endif

	Ogre::SceneNode* childSceneNode = m_ogreSceneMgr->getRootSceneNode()->createChildSceneNode();

	childSceneNode->attachObject(sphereMesh);

	Ogre::MaterialPtr sphereMaterial = Ogre::MaterialManager::getSingleton().create("SphereMaterial",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);

	sphereMaterial->getTechnique(0)->getPass(0)->setAmbient(0.1f, 0.1f, 0.1f);
	sphereMaterial->getTechnique(0)->getPass(0)->setDiffuse(0.2f, 0.2f, 0.2f, 1.0f);
	sphereMaterial->getTechnique(0)->getPass(0)->setSpecular(0.9f, 0.9f, 0.9f, 1.0f);
	//sphereMaterial->setAmbient(0.2f, 0.2f, 0.5f);
	//sphereMaterial->setSelfIllumination(0.2f, 0.2f, 0.1f);

	sphereMesh->setMaterialName("SphereMaterial");
	childSceneNode->setPosition(Ogre::Vector3(0.0f, 0.0f, 0.0f));
	childSceneNode->setScale(Ogre::Vector3(0.01f, 0.01f, 0.01f)); // Radius, in theory.

#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
	Ogre::SceneNode* pLightNode = m_ogreSceneMgr->getRootSceneNode()->createChildSceneNode();
	Ogre::Light* light = m_ogreSceneMgr->createLight();
	pLightNode->attachObject(light);
	pLightNode->setPosition(20.0f, 80.0f, 50.0f);
#else
	Ogre::Light* light = m_ogreSceneMgr->createLight("MainLight");
	light->setPosition(20.0f, 80.0f, 50.0f);
#endif
}

#if OGRE_VERSION >= ((2 << 16) | (0 << 8) | 0)
void QTOgreWindow::createCompositor()
{
	Ogre::CompositorManager2* compMan = m_ogreRoot->getCompositorManager2();
	const Ogre::String workspaceName = "default scene workspace";
	const Ogre::IdString workspaceNameHash = workspaceName;
	compMan->createBasicWorkspaceDef(workspaceName, m_ogreBackground);
	compMan->addWorkspace(m_ogreSceneMgr, m_ogreWindow, m_ogreCamera, workspaceNameHash, true);
}
#endif

void QTOgreWindow::render()
{
	/*
	How we tied in the render function for OGre3D with QWindow's render function. This is what gets call repeatedly. Note that we don't call this function directly; rather we use the renderNow() function to call this method as we don't want to render the Ogre3D scene unless everything is set up first.
	That is what renderNow() does.

	Theoretically you can have one function that does this check but from my experience it seems better to keep things separate and keep the render function as simple as possible.
	*/
    Ogre::WindowEventUtilities::messagePump();
    m_ogreRoot->renderOneFrame();

}

void QTOgreWindow::renderLater()
{
	/*
	This function forces QWindow to keep rendering. Omitting this causes the renderNow() function to only get called when the window is resized, moved, etc. as opposed to all of the time; which is generally what we need.
	*/
	/*
	if (!m_update_pending)
	{
		m_update_pending = true;
		QApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));                     //这是让QApplication给这个类发送一个强制更新的事件来保持更新
	}*/
}

bool QTOgreWindow::event(QEvent *event)
{
	/*
	QWindow's "message pump". The base method that handles all QWindow events. As you will see there are other methods that actually process the keyboard/other events of Qt and the underlying OS.

	Note that we call the renderNow() function which checks to see if everything is initialized, etc. before calling the render() function.
	*/
	/*
	switch (event->type())
	{
	case QEvent::UpdateRequest:
		m_update_pending = false;
		renderNow();
		return true;

	default:
		return QWindow::event(event);
	}*/
	return QWindow::event(event);
}

/*
Called after the QWindow is reopened or when the QWindow is first opened.
*/
void QTOgreWindow::exposeEvent(QExposeEvent *event)
{
	Q_UNUSED(event);
	
	if (isExposed())
		renderNow();
}

/*
The renderNow() function calls the initialize() function when needed and if the QWindow is already initialized and prepped calls the render() method.
*/
void QTOgreWindow::renderNow()
{
	if (!isExposed())
		return;
	
	if (m_ogreRoot == NULL)
	{
		initialize();
	}

	render();
/*
	if (m_animating)
        renderLater();*/
}

/*
Our event filter; handles the resizing of the QWindow. When the size of the QWindow changes note the call to the Ogre3D window and camera. This keeps the Ogre3D scene looking correct.
*/

bool QTOgreWindow::eventFilter(QObject *target, QEvent *event)
{
	if (target == this)
	{
		if (event->type() == QEvent::Resize)
		{
			if (isExposed() && m_ogreWindow != NULL)
			{
			//	QMessageBox::information(NULL, QStringLiteral("提示"), "zai", QMessageBox::Ok);
				QResizeEvent *e  = (QResizeEvent *)event;
				const QSize &newSize = e->size();
		//		m_ogreWindow->resize(this->width(), this->height());
			//	m_ogreWindow->reposition(this->position().x(),this->position().y());
				m_ogreWindow->resize(newSize.width(),newSize.height());
				m_ogreWindow->windowMovedOrResized();
				m_ogreCamera->setAspectRatio(Ogre::Real(m_ogreWindow->getWidth()) / Ogre::Real(m_ogreWindow->getHeight()));
				m_ogreWindow->update();
			}
		}
		/*
        if(event->type() == QEvent::WindowStateChange)
		{
			
				QMessageBox::information(NULL, QStringLiteral("提示"), "2", QMessageBox::Ok);
				m_ogreWindow->resize(this->width(), this->height());
				//	m_ogreWindow->reposition(this->position().x(),this->position().y());
				//m_ogreWindow->resize(newSize.width(),newSize.height());
				m_ogreWindow->windowMovedOrResized();
				m_ogreCamera->setAspectRatio(Ogre::Real(m_ogreWindow->getWidth()) / Ogre::Real(m_ogreWindow->getHeight()));
				m_ogreWindow->update();
		}*/
	}

	return false;
}
/*
void QTOgreWindow::showMaximized()
{
	QWindow::showMaximized();
	QApplication::postEvent(this, new QEvent(QEvent::Resize)); 
	QMessageBox::information(NULL, QStringLiteral("提示"), "3", QMessageBox::Ok);
}*/
/*
void QTOgreWindow::resizeEvent(QResizeEvent *e)
{
	
	
		const QSize &newSize = e->size();
		//		m_ogreWindow->resize(this->width(), this->height());
		//	m_ogreWindow->reposition(this->position().x(),this->position().y());
		m_ogreWindow->resize(newSize.width(),newSize.height());
		m_ogreWindow->windowMovedOrResized();
		m_ogreCamera->setAspectRatio(Ogre::Real(m_ogreWindow->getWidth()) / Ogre::Real(m_ogreWindow->getHeight()));
		m_ogreWindow->update();
	
}*/

/*
How we handle keyboard and mouse events.
*/
void QTOgreWindow::keyPressEvent(QKeyEvent * ev)
{
    if(m_cameraMan)
        m_cameraMan->injectKeyDown(*ev);

	if(ev->key() == Qt::Key_P)
	{
		play_flag = true;
	}

}

void QTOgreWindow::keyReleaseEvent(QKeyEvent * ev)
{
    if(m_cameraMan)
        m_cameraMan->injectKeyUp(*ev);
}

void QTOgreWindow::mouseMoveEvent( QMouseEvent* e )
{
    static int lastX = e->x();
    static int lastY = e->y();
    int relX = e->x() - lastX;
    int relY = e->y() - lastY;
    lastX = e->x();
    lastY = e->y();

    if(m_cameraMan && (e->buttons() & Qt::LeftButton))
        m_cameraMan->injectMouseMove(relX, relY);
}

void QTOgreWindow::mouseWheelEvent(QWheelEvent *e)
{
    if(m_cameraMan)
        m_cameraMan->injectWheelMove(*e);
}

void QTOgreWindow::mousePressEvent( QMouseEvent* e )
{
    if(m_cameraMan)
        m_cameraMan->injectMouseDown(*e);
}

void QTOgreWindow::mouseReleaseEvent( QMouseEvent* e )
{
    if(m_cameraMan)
        m_cameraMan->injectMouseUp(*e);
}

void QTOgreWindow::timerEvent(QTimerEvent *tEvent)
{
	if(m_animating)
	{
		renderNow();
	}
	emit sendFrameFinish();
}

/*
Function to keep track of when we should and shouldn't redraw the window; we wouldn't want to do rendering when the QWindow is minimized. This takes care of those scenarios.
*/
void QTOgreWindow::setAnimating(bool animating)
{
	m_animating = animating;
/*
	if (animating)
		renderLater();*/
}

bool QTOgreWindow::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	if (m_ogreWindow->isClosed())
	{
		return false;
	}
    m_cameraMan->frameRenderingQueued(evt);

	bindingData2(anime_data_[frame]);
	if (play_flag)
	{
		if(frame < anime_data_.size())
		{
			frame = frame+1;//每15帧渲染一次
		}
		else
		{
			play_flag = false;
			frame = 0;
		}
	}

    return true;
}

void QTOgreWindow::log(Ogre::String msg)
{
	if (Ogre::LogManager::getSingletonPtr() != NULL) Ogre::LogManager::getSingletonPtr()->logMessage(msg);
}

void QTOgreWindow::log(QString msg)
{
	log(Ogre::String(msg.toStdString().c_str()));
}

void QTOgreWindow::InitialFromDotScene(void)
{
	string resource_name = "HandRenderer Scene Resources";
	std::string scene_name = "hand.scene";
	Ogre::ResourceGroupManager* mResource_mgr;
	mResource_mgr = Ogre::ResourceGroupManager::getSingletonPtr();
	mResource_mgr->createResourceGroup(resource_name);

	try
	{
		mResource_mgr->addResourceLocation("../ogre",
			"FileSystem",
			resource_name,false);
		mResource_mgr->initialiseResourceGroup(resource_name);
		mResource_mgr->loadResourceGroup(resource_name);
		if (!mResource_mgr->resourceExists(resource_name,scene_name))
		{
			QMessageBox::warning(NULL, QStringLiteral("提示"), QStringLiteral("文件无法打开"), QMessageBox::Ok);
			return ;
			//printf("No scene file\n");
		}
		Ogre::DotSceneLoader scene_loader;

		//load .scene file
		scene_loader.parseDotScene(scene_name,resource_name,m_ogreSceneMgr);

		//set camera and viewport
		Ogre::SceneManager::CameraIterator cameras = m_ogreSceneMgr->getCameraIterator();
		if(!cameras.hasMoreElements()){
			throw runtime_error("The scene does not have a camera!");
		}
		m_ogreCamera = cameras.getNext();

		Ogre::Node* camera_node_ = m_ogreCamera->getParentNode();
		if(!camera_node_){
			throw runtime_error("The camera is not attached to a node!");
		}
		camera_node_->setPosition(0,0,0);
		camera_node_->resetOrientation();
		camera_node_->setInheritOrientation(false);
		m_ogreCamera->setPosition(-1,0,25);
		m_cameraMan = new OgreQtBites::SdkQtCameraMan(m_ogreCamera);   // create a default camera controller

		Ogre::Viewport* viewport_ = m_ogreWindow->addViewport(m_ogreCamera);
		viewport_->setBackgroundColour(Ogre::ColourValue(0,0,0));

		m_ogreCamera->setAspectRatio(Ogre::Real(m_ogreWindow->getWidth()) / Ogre::Real(m_ogreWindow->getHeight()));
		m_ogreCamera->setAutoAspectRatio(true);

		//hand node,skeleton and bones
		if(!m_ogreSceneMgr->hasEntity("hand"))
		{
			throw runtime_error("The scene does not have the object!");
		}
		Ogre::Entity* hand_entity_ = m_ogreSceneMgr->getEntity("hand");

		if (!hand_entity_->hasSkeleton())
		{
			throw runtime_error("The hand object does not have a skeleton!");
		}
		hand_node_ = hand_entity_->getParentSceneNode();
		m_cameraMan->setTarget(hand_node_);
		//	 m_ogreCamera->setAutoTracking(true, hand_node_);
		m_cameraMan->setStyle(OgreQtBites::CS_ORBIT);
		hand_node_->roll(Ogre::Degree(-90));
		hand_node_->pitch(Ogre::Degree(-180));
		if (!hand_node_)
		{
			throw runtime_error("The hand entity does not attached to a node!");
		}

		Ogre::Skeleton* hand_skeleton_ = hand_entity_->getSkeleton();
		vector<string> bone_name;
		bone_name.push_back("finger1joint1");	
		bone_name.push_back("finger1joint2");
		bone_name.push_back("finger1joint3");
		bone_name.push_back("finger2joint1");
		bone_name.push_back("finger2joint2");
		bone_name.push_back("finger2joint3");
		bone_name.push_back("finger3joint1");
		bone_name.push_back("finger3joint2");
		bone_name.push_back("finger3joint3");
		bone_name.push_back("finger4joint1");
		bone_name.push_back("finger4joint2");
		bone_name.push_back("finger4joint3");
		bone_name.push_back("finger5joint1");
		bone_name.push_back("finger5joint2");
		bone_name.push_back("finger5joint3");
		bone_name.push_back("metacarpals");
		bone_name.push_back("carpals");
		bone_name.push_back("root");
		bone_name.push_back("Bone");
		bone_name.push_back("Bone.001");
		bone_name.push_back("Bone.002");
		bone_name.push_back("Bone.003");

		for (int i = 0;i<bone_name.size();i++)
		{
			string name = bone_name[i];
			if (!hand_skeleton_->hasBone(name))
			{
				throw runtime_error("The hand object does not attached to a node!");
			}
			Ogre::Bone* bone = hand_skeleton_->getBone(name);
			bone->setManuallyControlled(true);
			bone_by_index.push_back(bone);
		}

		setBonesToStandard();

	}
	catch(...){}
	
}

void QTOgreWindow::setBonesToStandard()
{
	bone_by_index[0]->rotate((bone_by_index[0]->getOrientation().Inverse())*(bone_by_index[18]->getOrientation().Inverse()));
	bone_by_index[1]->rotate(bone_by_index[1]->getOrientation().Inverse());
	bone_by_index[2]->rotate(bone_by_index[2]->getOrientation().Inverse());

	bone_by_index[3]->rotate((bone_by_index[3]->getOrientation().Inverse())*(bone_by_index[19]->getOrientation().Inverse()));
	bone_by_index[4]->rotate(bone_by_index[4]->getOrientation().Inverse());
	bone_by_index[5]->rotate(bone_by_index[5]->getOrientation().Inverse());

	bone_by_index[6]->rotate((bone_by_index[6]->getOrientation().Inverse())*(bone_by_index[20]->getOrientation().Inverse()));
	bone_by_index[7]->rotate(bone_by_index[7]->getOrientation().Inverse());
	bone_by_index[8]->rotate(bone_by_index[8]->getOrientation().Inverse());

	bone_by_index[9]->rotate((bone_by_index[9]->getOrientation().Inverse())*(bone_by_index[21]->getOrientation().Inverse()));
	bone_by_index[10]->rotate(bone_by_index[10]->getOrientation().Inverse());
	bone_by_index[11]->rotate(bone_by_index[11]->getOrientation().Inverse());	


	bone_by_index[14]->resetOrientation();
	bone_by_index[13]->resetOrientation();
	bone_by_index[12]->resetOrientation();
	bone_by_index[12]->setPosition(-0.362,0.628,0);

	//bone_by_index[12]->roll(Ogre::Degree(20));
	//bone_by_index[13]->roll(Ogre::Degree(-20));
	//bone_by_index[13]->yaw(Ogre::Degree(-45));
	//bone_by_index[13]->pitch(Ogre::Degree(10));
	//bone_by_index[14]->pitch(Ogre::Degree(30));

	//形态修正
	bone_by_index[3]->pitch(Ogre::Degree(-15));
	bone_by_index[4]->pitch(Ogre::Degree(15));   

	//bone_by_index[17]->yaw(Ogre::Degree(90));
	//bone_by_index[17]->roll(Ogre::Degree(-50));


	for (int i = 0;i<=17;i++)
	{
		bone_by_index[i]->setInitialState();
	}
}

void QTOgreWindow::bindingData(vector<double> data)
{
	Handreset();
	//1 DOF
	//little
	if (data.at(18)>0){
		bone_by_index[2]->pitch(Ogre::Radian(0));}
	else{bone_by_index[2]->pitch(Ogre::Radian(data.at(18)));}
	if (data.at(17)>0){
		bone_by_index[1]->pitch(Ogre::Radian(0));}
	else{bone_by_index[1]->pitch(Ogre::Radian(data.at(17)));}
	//ring
	if (data.at(14)>0){
		bone_by_index[5]->pitch(Ogre::Radian(0));}
	else{bone_by_index[5]->pitch(Ogre::Radian(data.at(14)));}
	if (data.at(13)>0){
		bone_by_index[4]->pitch(Ogre::Radian(0));}
	else{bone_by_index[4]->pitch(Ogre::Radian(data.at(13)));}
	//middle
	if (data.at(10)>0){
		bone_by_index[8]->pitch(Ogre::Radian(0));}
	else{bone_by_index[8]->pitch(Ogre::Radian(data.at(10)));}
	if (data.at(9)>0){
		bone_by_index[7]->pitch(Ogre::Radian(0));}
	else{bone_by_index[7]->pitch(Ogre::Radian(data.at(9)));}
	bone_by_index[6]->pitch(Ogre::Radian(data.at(8)));
	//index
	if (data.at(7)>0){
		bone_by_index[11]->pitch(Ogre::Radian(0));}
	else{bone_by_index[11]->pitch(Ogre::Radian(data.at(7)));}
	if (data.at(6)>0){
		bone_by_index[10]->pitch(Ogre::Radian(0));}
	else{bone_by_index[10]->pitch(Ogre::Radian(data.at(6)));}


	//2 DOF
	//index,食指roll正方向
	double i1,i2;
	calculateAngle(data.at(11),data.at(8),data.at(5),0,i1,i2);
	              //w               p1        p2     b1 b  b22
	Ogre::Matrix3 mi;
	mi.FromEulerAnglesZXY(Ogre::Radian(abs(i1)),Ogre::Radian(data.at(5)),Ogre::Radian(0));
	bone_by_index[9]->rotate(Ogre::Quaternion(mi));

	//ring，无名指roll负方向
	double r1,r2;
	calculateAngle(data.at(15),data.at(8),data.at(12),0,r1,r2);
	mi.FromEulerAnglesZXY(Ogre::Radian(-abs(r1)),Ogre::Radian(data.at(12)),Ogre::Radian(0));
	bone_by_index[3]->rotate(Ogre::Quaternion(mi));
	//savefile<<r1<<" "<<r2<<" | ";

	//little
	double l1,l2;
	calculateAngle(data.at(19),data.at(12),data.at(16),-abs(r1),l1,l2);
	if (l1>=0 && l2<=0)
	{
		mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
		bone_by_index[0]->rotate(Ogre::Quaternion(mi));
		little_last = l2;
	}
	else if (l1<=0 && l2>=0)
	{
		mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
		bone_by_index[0]->rotate(Ogre::Quaternion(mi));
		little_last = l1;
	}
	else if(l1<=0 && l2<=0)
	{
		if(l1<-abs(r1) && l2<-abs(r1))
		{
			if (abs(little_last-l1) < abs(little_last-l2))
			{
				mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
				bone_by_index[0]->rotate(Ogre::Quaternion(mi));
				little_last = l1;
			}
			else
			{
				mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
				bone_by_index[0]->rotate(Ogre::Quaternion(mi));
				little_last = l2;
			}
		}
		else if (l1<-abs(r1) && l2>-abs(r1))
		{
			mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
			bone_by_index[0]->rotate(Ogre::Quaternion(mi));
			little_last = l1;
		}
		else if (l1>-abs(r1) && l2<-abs(r1))
		{
			mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
			bone_by_index[0]->rotate(Ogre::Quaternion(mi));
			little_last = l2;
		}
	}
	//savefile<<l1<<" "<<l2<<endl;


	//thumb

	bone_by_index[12]->yaw(Ogre::Radian(data.at(1)));
	if (data.at(4)>0.785)
	{
		bone_by_index[12]->roll(Ogre::Degree(45));
	}
	else
	{
		bone_by_index[12]->roll(Ogre::Radian(data.at(4)));
		//bone_by_index[13]->roll(Ogre::Radian(0.7*data.at(4)-0.349));
	}
	bone_by_index[13]->yaw(Ogre::Degree(-45));
	bone_by_index[13]->pitch(Ogre::Radian(data.at(2)));
	bone_by_index[14]->pitch(Ogre::Radian(data.at(3)));
}

void QTOgreWindow::bindingData2(vector<double> data)
{
	Handreset();
	Ogre::Quaternion ro;
	Ogre::Matrix3 mo;
	//1 DOF
	//little
	if (data.at(18)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[2]->rotate(Ogre::Quaternion(mo));
	}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(18)),Ogre::Radian(0));
		bone_by_index[2]->rotate(Ogre::Quaternion(mo));
		}
	if (data.at(17)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[1]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(17)),Ogre::Radian(0));
		bone_by_index[1]->rotate(Ogre::Quaternion(mo));
	}
	//ring
	if (data.at(14)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[5]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(14)),Ogre::Radian(0));
		bone_by_index[5]->rotate(Ogre::Quaternion(mo));
	}
	if (data.at(13)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[4]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(13)),Ogre::Radian(0));
		bone_by_index[4]->rotate(Ogre::Quaternion(mo));
		}
	//middle
	if (data.at(10)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[8]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(10)),Ogre::Radian(0));
		bone_by_index[8]->rotate(Ogre::Quaternion(mo));
	}
	if (data.at(9)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[7]->rotate(Ogre::Quaternion(mo));
	}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(9)),Ogre::Radian(0));
		bone_by_index[7]->rotate(Ogre::Quaternion(mo));
		}
	mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(8)),Ogre::Radian(0));
	bone_by_index[6]->rotate(Ogre::Quaternion(mo));
	//index
	if (data.at(7)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[11]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(7)),Ogre::Radian(0));
		bone_by_index[11]->rotate(Ogre::Quaternion(mo));
		}
	if (data.at(6)>0){
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[10]->rotate(Ogre::Quaternion(mo));
		}
	else{
		mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(6)),Ogre::Radian(0));
		bone_by_index[10]->rotate(Ogre::Quaternion(mo));
		}


	//2 DOF
	//index,食指roll正方向
	double i1,i2;
	calculateAngle(data.at(11),data.at(8),data.at(5),0,i1,i2);
	//w               p1        p2     b1 b  b22
	Ogre::Matrix3 mi;
	mi.FromEulerAnglesZXY(Ogre::Radian(abs(i1)),Ogre::Radian(data.at(5)),Ogre::Radian(0));
	bone_by_index[9]->rotate(Ogre::Quaternion(mi));

	//ring，无名指roll负方向
	double r1,r2;
	calculateAngle(data.at(15),data.at(8),data.at(12),0,r1,r2);
	mi.FromEulerAnglesZXY(Ogre::Radian(-abs(r1)),Ogre::Radian(data.at(12)),Ogre::Radian(0));
	bone_by_index[3]->rotate(Ogre::Quaternion(mi));
	//savefile<<r1<<" "<<r2<<" | ";

	//little
	double l1,l2;
	calculateAngle(data.at(19),data.at(12),data.at(16),-abs(r1),l1,l2);
	if (l1>=0 && l2<=0)
	{
		mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
		bone_by_index[0]->rotate(Ogre::Quaternion(mi));
		little_last = l2;
	}
	else if (l1<=0 && l2>=0)
	{
		mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
		bone_by_index[0]->rotate(Ogre::Quaternion(mi));
		little_last = l1;
	}
	else if(l1<=0 && l2<=0)
	{
		if(l1<-abs(r1) && l2<-abs(r1))
		{
			if (abs(little_last-l1) < abs(little_last-l2))
			{
				mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
				bone_by_index[0]->rotate(Ogre::Quaternion(mi));
				little_last = l1;
			}
			else
			{
				mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
				bone_by_index[0]->rotate(Ogre::Quaternion(mi));
				little_last = l2;
			}
		}
		else if (l1<-abs(r1) && l2>-abs(r1))
		{
			mi.FromEulerAnglesZXY(Ogre::Radian(l1),Ogre::Radian(data.at(16)),Ogre::Radian(0));
			bone_by_index[0]->rotate(Ogre::Quaternion(mi));
			little_last = l1;
		}
		else if (l1>-abs(r1) && l2<-abs(r1))
		{
			mi.FromEulerAnglesZXY(Ogre::Radian(l2),Ogre::Radian(data.at(16)),Ogre::Radian(0));
			bone_by_index[0]->rotate(Ogre::Quaternion(mi));
			little_last = l2;
		}
	}
	//savefile<<l1<<" "<<l2<<endl;


	//thumb
	mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Radian(data.at(1)));
	bone_by_index[12]->rotate(Ogre::Quaternion(mo));
	if (data.at(4)>0.785)
	{
		mo.FromEulerAnglesZXY(Ogre::Degree(45),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[12]->rotate(Ogre::Quaternion(mo));
	}
	else
	{
		mo.FromEulerAnglesZXY(Ogre::Radian(data.at(4)),Ogre::Radian(0),Ogre::Radian(0));
		bone_by_index[12]->rotate(Ogre::Quaternion(mo));
		//bone_by_index[13]->roll(Ogre::Radian(0.7*data.at(4)-0.349));
	}
	mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(0),Ogre::Degree(-45));
//	bone_by_index[13]->rotate(Ogre::Quaternion(mo));
	mi.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(2)),Ogre::Radian(0));
	bone_by_index[13]->rotate(Ogre::Quaternion(mo)*Ogre::Quaternion(mi));
	mo.FromEulerAnglesZXY(Ogre::Radian(0),Ogre::Radian(data.at(3)),Ogre::Radian(0));
	bone_by_index[14]->rotate(Ogre::Quaternion(mo));
}

void QTOgreWindow::Handreset()
{
	for (int i =0;i<=17;i++)
	{
		bone_by_index[i]->reset();
	}
}

void QTOgreWindow::calculateAngle(double w,double p1,double p2,double b1,double &b21,double &b22)
{
	double A,B,C;
	A = sin(b1);
	B = cos(b1)*(cos(p1)*cos(p2)+sin(p1)*sin(p2));
	C = cos(w);
	double result;
	result = A*A+B*B-C*C;
	if(result<0)
		result = 0;
	result = sqrt(result);
	b21 = (A+result)/(B+C);
	b22 = (A-result)/(B+C);
	b21 = atan(b21)*2;
	b22 = atan(b22)*2;
}

void QTOgreWindow::PlayPauseOp(bool on)
{
	play_flag = on;
}

void QTOgreWindow::saveDepthTrain()
{

}

int QTOgreWindow::getCurFrame()
{
	return frame;
}

void QTOgreWindow::setCurFrame(int curFrame)
{
	if (curFrame<anime_data_.size())
	{
		frame = curFrame;
	}
	else
	{
		frame = anime_data_.size()-1;
	}
	
}

void QTOgreWindow::setFrame(vector<vector<double>> animData)
{
	anime_data_.clear();
	anime_data_ = animData;
}
