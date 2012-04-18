/*=========================================================================

   Program: ParaView
   Module:    pqObjectInspectorWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqObjectInspectorWidget.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabWidget>
#include <QVBoxLayout>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqApplyPropertiesManager.h"
#include "pqAutoGeneratedObjectPanel.h"
#include "pqCalculatorPanel.h"
#include "pqClipPanel.h"
#include "pqContourPanel.h"
#include "pqCutPanel.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqExodusIIPanel.h"
#include "pqExtractCTHPartsPanel.h"
#include "pqGlyphPanel.h"
#include "pqInterfaceTracker.h"
#include "pqIsoVolumePanel.h"
#include "pqLoadedFormObjectPanel.h"
#include "pqNetCDFPanel.h"
#include "pqObjectBuilder.h"
#include "pqObjectPanelInterface.h"
#include "pqOutputPort.h"
#include "pqParticleTracerPanel.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqSelectThroughPanel.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSettings.h"
#include "pqStreamTracerPanel.h"
#include "pqThresholdPanel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqYoungsMaterialInterfacePanel.h"
#include "vtkPVConfig.h" // To get PARAVIEW_USE_*
#include "vtkSMProxy.h"

bool pqObjectInspectorWidget::AutoAccept = false;

class pqStandardCustomPanels : public QObject, public pqObjectPanelInterface
{
public:
  pqStandardCustomPanels(QObject* p) : QObject(p) {}

  QString name() const
    {
    return "StandardPanels";
    }
  pqObjectPanel* createPanel(pqProxy* proxy, QWidget* p)
    {
    if(QString("filters") == proxy->getProxy()->GetXMLGroup())
      {
      if(QString("Cut") == proxy->getProxy()->GetXMLName() ||
         QString("GenericCut") == proxy->getProxy()->GetXMLName())
        {
        return new pqCutPanel(proxy, p);
        }
      if(QString("Clip") == proxy->getProxy()->GetXMLName() ||
         QString("GenericClip") == proxy->getProxy()->GetXMLName())
        {
        return new pqClipPanel(proxy, p);
        }
      if(QString("Calculator") == proxy->getProxy()->GetXMLName())
        {
        return new pqCalculatorPanel(proxy, p);
        }
      if (QString("ArbitrarySourceGlyph") == proxy->getProxy()->GetXMLName() ||
        QString("Glyph") == proxy->getProxy()->GetXMLName())
        {
        return new pqGlyphPanel(proxy, p);
        }
      if(QString("StreamTracer") == proxy->getProxy()->GetXMLName() ||
         QString("GenericStreamTracer") == proxy->getProxy()->GetXMLName())
        {
        return new pqStreamTracerPanel(proxy, p);
        }
//      if(QString("ParticleTracer") == proxy->getProxy()->GetXMLName())
//        {
//        return new pqParticleTracerPanel(proxy, p);
//        }
      if(QString("Threshold") == proxy->getProxy()->GetXMLName())
        {
        return new pqThresholdPanel(proxy, p);
        }
      if(QString("IsoVolume") == proxy->getProxy()->GetXMLName())
        {
        return new pqIsoVolumePanel(proxy, p);
        }
      if(QString("Contour") == proxy->getProxy()->GetXMLName() ||
         QString("GenericContour") == proxy->getProxy()->GetXMLName())
        {
        return new pqContourPanel(proxy, p);
        }
      if(QString("CTHPart") == proxy->getProxy()->GetXMLName())
        {
        return new pqExtractCTHPartsPanel(proxy, p);
        }
      if(QString("RectilinearGridConnectivity") == proxy->getProxy()->GetXMLName())
        {
        // allow RectilinearGridConnectivity to reuse the panel of CTHPart
        return new pqExtractCTHPartsPanel(proxy, p);
        }
      if (QString("YoungsMaterialInterface") == proxy->getProxy()->GetXMLName())
        {
        return new pqYoungsMaterialInterfacePanel(proxy, p);
        }
      }
    if(QString("sources") == proxy->getProxy()->GetXMLGroup())
      {
      if(QString("ExodusIIReader") == proxy->getProxy()->GetXMLName())
        {
        return new pqExodusIIPanel(proxy, p);
        }
      if(QString("ExodusRestartReader") == proxy->getProxy()->GetXMLName())
        {
        return new pqExodusIIPanel(proxy, p);
        }
      if(QString("netCDFReader") == proxy->getProxy()->GetXMLName())
        {
        return new pqNetCDFPanel(proxy, p);
        }
      }
    return NULL;
    }
  bool canCreatePanel(pqProxy* proxy) const
    {
    if(QString("filters") == proxy->getProxy()->GetXMLGroup())
      {
      if(QString("Cut") == proxy->getProxy()->GetXMLName() ||
         QString("GenericCut") == proxy->getProxy()->GetXMLName() ||
         QString("Clip") == proxy->getProxy()->GetXMLName() ||
         QString("GenericClip") == proxy->getProxy()->GetXMLName() ||
         QString("Calculator") == proxy->getProxy()->GetXMLName() ||
         QString("ArbitrarySourceGlyph") == proxy->getProxy()->GetXMLName() ||
         QString("Glyph") == proxy->getProxy()->GetXMLName() ||
         QString("StreamTracer") == proxy->getProxy()->GetXMLName() ||
         QString("GenericStreamTracer") == proxy->getProxy()->GetXMLName() ||
//         QString("ExtractDataSets") == proxy->getProxy()->GetXMLName() ||
//         QString("ParticleTracer") == proxy->getProxy()->GetXMLName() ||
         QString("Threshold") == proxy->getProxy()->GetXMLName() ||
         QString("IsoVolume") == proxy->getProxy()->GetXMLName() ||
         QString("ExtractSelection") == proxy->getProxy()->GetXMLName() ||
         QString("ExtractSelectionOverTime") == proxy->getProxy()->GetXMLName() ||
         QString("Contour") == proxy->getProxy()->GetXMLName() ||
         QString("GenericContour") == proxy->getProxy()->GetXMLName() ||
         QString("CTHPart") == proxy->getProxy()->GetXMLName() ||
         QString("RectilinearGridConnectivity") == proxy->getProxy()->GetXMLName() ||
         QString("YoungsMaterialInterface") == proxy->getProxy()->GetXMLName())
        {
        return true;
        }
      }
    if(QString("sources") == proxy->getProxy()->GetXMLGroup())
      {
      if (QString("ExodusIIReader") == proxy->getProxy()->GetXMLName() ||
        QString("ExodusRestartReader") == proxy->getProxy()->GetXMLName() ||
        QString("netCDFReader") == proxy->getProxy()->GetXMLName()
         )
        {
        return true;
        }
      }
    return false;
    }
};

//-----------------------------------------------------------------------------
pqObjectInspectorWidget::pqObjectInspectorWidget(QWidget *p)
  : QWidget(p)
{
  this->setObjectName("objectInspector");
  this->setProperty("PV_MUST_BE_MASTER", true);

  this->CurrentPanel = 0;
  this->ShowOnAccept = true;
  this->OutputPort = 0;

  // get custom panels
  this->StandardCustomPanels = new pqStandardCustomPanels(this);

  // main layout
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);

  QScrollArea*s = new QScrollArea();
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("ScrollArea");
  s->setFrameShape(QFrame::NoFrame);

  this->PanelArea = new QWidget;
  this->PanelArea->setSizePolicy(QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding);
  QVBoxLayout *panelLayout = new QVBoxLayout(this->PanelArea);
  panelLayout->setMargin(0);
  s->setWidget(this->PanelArea);
  this->PanelArea->setObjectName("PanelArea");

  QBoxLayout* buttonlayout = new QHBoxLayout();
  this->AcceptButton = new QPushButton(this);
  this->AcceptButton->setObjectName("Accept");
  this->AcceptButton->setText(tr("&Apply"));
  this->AcceptButton->setIcon(QIcon(QPixmap(":/pqWidgets/Icons/pqUpdate16.png")));
#ifdef Q_WS_MAC
  this->AcceptButton->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_A));
#endif
  this->ResetButton = new QPushButton(this);
  this->ResetButton->setObjectName("Reset");
  this->ResetButton->setText(tr("&Reset"));
  this->ResetButton->setIcon(QIcon(QPixmap(":/pqWidgets/Icons/pqCancel16.png")));
#ifdef Q_WS_MAC
  this->ResetButton->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_R));
#endif
  this->DeleteButton = new QPushButton(this);
  this->DeleteButton->setObjectName("Delete");
  this->DeleteButton->setText(tr("Delete"));
  this->DeleteButton->setIcon(QIcon(QPixmap(":/QtWidgets/Icons/pqDelete16.png")));

  this->HelpButton = new QPushButton(this);
  this->HelpButton->setObjectName("Help");
  this->HelpButton->setIcon(QIcon(":/pqWidgets/Icons/pqHelp16.png"));
  this->HelpButton->setEnabled(false);

  buttonlayout->addStretch();
  buttonlayout->addWidget(this->AcceptButton);
  buttonlayout->addWidget(this->ResetButton);
  buttonlayout->addWidget(this->DeleteButton);
  buttonlayout->addWidget(this->HelpButton);
  buttonlayout->addStretch();

  mainLayout->addLayout(buttonlayout);
  mainLayout->addWidget(s);

  // connect the apply button to the apply properties manager
  pqApplyPropertiesManager *applyPropertiesManager =
      qobject_cast<pqApplyPropertiesManager *>(
        pqApplicationCore::instance()->manager("APPLY_PROPERTIES"));

  if(applyPropertiesManager)
    {
    this->connect(this->AcceptButton,
                  SIGNAL(clicked()),
                  applyPropertiesManager,
                  SLOT(applyProperties()));

    this->connect(applyPropertiesManager,
                  SIGNAL(apply()),
                  this,
                  SLOT(accept()));
    }

  this->connect(this->ResetButton, SIGNAL(clicked()), SLOT(reset()));
  this->connect(this->DeleteButton, SIGNAL(clicked()), SLOT(deleteProxy()));
  this->connect(this->HelpButton, SIGNAL(clicked()), SLOT(showHelp()));

  this->AcceptButton->setEnabled(false);
  this->ResetButton->setEnabled(false);
  this->DeleteButton->setEnabled(false);

  // if XP Style is being used
  // swap it out for cleanlooks which looks almost the same
  // so we can have a green accept button
  // make all the buttons the same
  QString styleName = this->AcceptButton->style()->metaObject()->className();
  if(styleName == "QWindowsXPStyle")
     {
     QStyle* st = QStyleFactory::create("cleanlooks");
     st->setParent(this);
     this->AcceptButton->setStyle(st);
     this->ResetButton->setStyle(st);
     this->DeleteButton->setStyle(st);
     QPalette buttonPalette = this->AcceptButton->palette();
     buttonPalette.setColor(QPalette::Button, QColor(244,246,244));
     this->AcceptButton->setPalette(buttonPalette);
     this->ResetButton->setPalette(buttonPalette);
     this->DeleteButton->setPalette(buttonPalette);
     }

  // Change the accept button palette so it is green when it is active.
  QPalette acceptPalette = this->AcceptButton->palette();
  acceptPalette.setColor(QPalette::Active, QPalette::Button, QColor(161, 213, 135));
  this->AcceptButton->setPalette(acceptPalette);
  this->AcceptButton->setDefault(true);

  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
                SIGNAL(sourceRemoved(pqPipelineSource*)),
                SLOT(removeProxy(pqPipelineSource*)));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
      SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
      SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));
  this->connect(pqApplicationCore::instance()->getServerManagerModel(),
      SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
      SLOT(handleConnectionChanged(pqPipelineSource*, pqPipelineSource*)));

  this->AutoAccept = pqApplicationCore::instance()->settings()->
    value("autoAccept", false).toBool();

  this->AutoAcceptTimer.setSingleShot(true);
  this->AutoAcceptTimer.setInterval(1000);  // 1 sec
  if (applyPropertiesManager)
    {
    QObject::connect(&this->AutoAcceptTimer, SIGNAL(timeout()),
      applyPropertiesManager, SLOT(applyProperties()));
    }
  else
    {
    QObject::connect(&this->AutoAcceptTimer, SIGNAL(timeout()),
      this, SLOT(accept()));
    }

  this->connect(&pqActiveObjects::instance(),
                SIGNAL(portChanged(pqOutputPort*)),
                this,
                SLOT(setOutputPort(pqOutputPort*)));
  this->connect(&pqActiveObjects::instance(),
                SIGNAL(viewChanged(pqView*)),
                this,
                SLOT(setView(pqView*)));
}

//-----------------------------------------------------------------------------
pqObjectInspectorWidget::~pqObjectInspectorWidget()
{
  // delete all queued panels
  foreach(pqObjectPanel* p, this->PanelStore)
    {
    delete p;
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::setAutoAccept(bool status)
{
  pqObjectInspectorWidget::AutoAccept = status;
}

//-----------------------------------------------------------------------------
bool pqObjectInspectorWidget::autoAccept()
{
  return pqObjectInspectorWidget::AutoAccept;
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::canAccept(bool status)
{
  if(pqObjectInspectorWidget::AutoAccept && status)
    {
    // if its going already, stop it and restart it
    this->AutoAcceptTimer.stop();
    this->AutoAcceptTimer.start();
    return;
    }
  else if(pqObjectInspectorWidget::AutoAccept)
    {
    this->AutoAcceptTimer.stop();
    }

  this->AcceptButton->setEnabled(status);

  bool resetStatus = status;
  if(resetStatus && this->CurrentPanel &&
     this->CurrentPanel->referenceProxy()->modifiedState() ==
     pqProxy::UNINITIALIZED)
    {
    resetStatus = false;
    }
  this->ResetButton->setEnabled(resetStatus);
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::setProxy(pqProxy *proxy)
{
  // do nothing if this proxy is already current
  if(this->CurrentPanel && (this->CurrentPanel->referenceProxy() == proxy))
    {
    return;
    }

  if (this->CurrentPanel)
    {
    this->PanelArea->layout()->takeAt(0);
    this->CurrentPanel->deselect();
    this->CurrentPanel->hide();
    this->CurrentPanel->setObjectName("");
    // We don't delete the panel, it's managed by this->PanelStore.
    // Any deletion of any panel must ensure that call pointers
    // to the panel ie. this->CurrentPanel and this->PanelStore
    // are updated so that we don't have any dangling pointers.
    }

  this->CurrentPanel = NULL;
  bool reusedPanel = false;

  if(!proxy)
    {
    this->DeleteButton->setEnabled(false);
    this->HelpButton->setEnabled(false);
    return;
    }
  this->HelpButton->setEnabled(true);

  // search for a custom form for this proxy with pending changes
  QMap<pqProxy*, QPointer<pqObjectPanel> >::iterator iter;
  iter = this->PanelStore.find(proxy);
  if(iter != this->PanelStore.end())
    {
    this->CurrentPanel = iter.value();
    reusedPanel = true;
    }

  if(proxy && !this->CurrentPanel)
    {
    const QString xml_name = proxy->getProxy()->GetXMLName();

    // search custom panels
    pqInterfaceTracker* pm = pqApplicationCore::instance()->interfaceTracker();
    QObjectList ifaces = pm->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqObjectPanelInterface* piface =
        qobject_cast<pqObjectPanelInterface*>(iface);
      if (piface && piface->canCreatePanel(proxy))
        {
        this->CurrentPanel = piface->createPanel(proxy, NULL);
        break;
        }
      }

    // search standard custom panels
    if(!this->CurrentPanel)
      {
      if (this->StandardCustomPanels->canCreatePanel(proxy))
        {
        this->CurrentPanel =
          this->StandardCustomPanels->createPanel(proxy, NULL);
        }
      }

    // if there's no panel from a plugin, check the ui resources
    if(!this->CurrentPanel)
      {
      QString proxyui = QString(":/pqWidgets/UI/") + QString(proxy->getProxy()->GetXMLName()) + QString(".ui");
      pqLoadedFormObjectPanel* panel = new pqLoadedFormObjectPanel(proxyui, proxy, NULL);
      if(!panel->isValid())
        {
        delete panel;
        panel = NULL;
        }
      this->CurrentPanel = panel;
      }
    }

  if(this->CurrentPanel == NULL)
    {
    this->CurrentPanel = new pqAutoGeneratedObjectPanel(proxy);
    }

  // the current auto panel always has the name "Editor"
  this->CurrentPanel->setObjectName("Editor");

  if(!reusedPanel)
    {
    QObject::connect(this, SIGNAL(viewChanged(pqView*)),
                     this->CurrentPanel, SLOT(setView(pqView*)));

    QObject::connect(this->CurrentPanel, SIGNAL(modified()),
      this, SLOT(updateAcceptState()));

    QObject::connect(this->CurrentPanel->referenceProxy(),
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(updateAcceptState()));
    }

  this->PanelArea->layout()->addWidget(this->CurrentPanel);
  this->CurrentPanel->setView(this->View);
  this->CurrentPanel->select();
  this->CurrentPanel->show();
  this->updateDeleteButtonState();

  this->PanelStore[proxy] = this->CurrentPanel;

  this->updateAcceptState();
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::accept()
{
  QSet<pqProxy*> proxies_to_show;

  // accept all panels that are dirty.
  foreach(pqObjectPanel* panel, this->PanelStore)
    {
    pqProxy* refProxy = panel->referenceProxy();
    int modified_state = refProxy->modifiedState();
    if (this->ShowOnAccept && modified_state == pqProxy::UNINITIALIZED)
      {
      proxies_to_show.insert(refProxy);
      }
    if (modified_state != pqProxy::UNMODIFIED)
      {
      panel->accept();
      }
    }

  if (this->CurrentPanel)
    {
    pqProxy* refProxy = this->CurrentPanel->referenceProxy();
    int modified_state = refProxy->modifiedState();
    if (this->ShowOnAccept && modified_state == pqProxy::UNINITIALIZED)
      {
      proxies_to_show.insert(refProxy);
      }
    this->CurrentPanel->accept();
    }

  foreach (pqProxy* proxy_to_show, proxies_to_show)
    {
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(proxy_to_show);
    if (source)
      {
      this->show(source);
      pqProxyModifiedStateUndoElement* elem =
        pqProxyModifiedStateUndoElement::New();
      elem->SetSession(source->getServer()->session());
      elem->MadeUnmodified(source);
      ADD_UNDO_ELEM(elem);
      elem->Delete();
      }
    }

  emit this->accepted();
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::reset()
{
  emit this->prereject();

  // reset all panels that are dirty
  foreach(pqObjectPanel* p, this->PanelStore)
    {
    if (p->referenceProxy()->modifiedState() != pqProxy::UNMODIFIED)
      {
      p->reset();
      }
    }

  if(this->CurrentPanel)
    {
    this->CurrentPanel->reset();
    }

  emit this->postreject();
}

//-----------------------------------------------------------------------------
QSize pqObjectInspectorWidget::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), this));
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::removeProxy(pqPipelineSource* proxy)
{
  QObject::disconnect(proxy,
    SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
    this, SLOT(updateAcceptState()));

  if (this->CurrentPanel && (this->CurrentPanel->referenceProxy() == proxy))
    {
    this->CurrentPanel = NULL;
    }

  QMap<pqProxy*, QPointer<pqObjectPanel> >::iterator iter;
  iter = this->PanelStore.find(proxy);
  if (iter != this->PanelStore.end())
    {
    QObject::disconnect(iter.value(), SIGNAL(modified()),
      this, SLOT(updateAcceptState()));

    delete iter.value();
    this->PanelStore.erase(iter);
    }

  this->updateAcceptState();
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::deleteProxy()
{
  if (this->CurrentPanel && this->CurrentPanel->referenceProxy())
    {
    pqPipelineSource* source =
      qobject_cast<pqPipelineSource*>(this->CurrentPanel->referenceProxy());

    pqApplicationCore* core = pqApplicationCore::instance();
    BEGIN_UNDO_SET(
      QString("Delete %1").arg(source->getSMName()));
    core->getObjectBuilder()->destroy(source);
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::setDeleteButtonVisibility(bool visible)
{
  this->DeleteButton->setVisible(visible);
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::handleConnectionChanged(pqPipelineSource* in,
    pqPipelineSource*)
{
  if(this->CurrentPanel && this->CurrentPanel->referenceProxy() == in)
    {
    this->updateDeleteButtonState();
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::updateDeleteButtonState()
{
  pqPipelineSource *source = 0;
  if(this->CurrentPanel)
    {
    source = dynamic_cast<pqPipelineSource *>(this->CurrentPanel->referenceProxy());
    }

  this->DeleteButton->setEnabled(source && source->getNumberOfConsumers() == 0);
}

void pqObjectInspectorWidget::setOutputPort(pqOutputPort *port)
{
  if (this->OutputPort == port)
    {
    return;
    }
  if (this->OutputPort)
    {
    QObject::disconnect(this->OutputPort, 0, this, 0);
    }

  this->OutputPort = port;

  if(port)
    {
    this->setProxy(port->getSource());
    }
  else
    {
    this->setProxy(0);
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::updateAcceptState()
{
  // watch for modified state changes
  bool acceptable = false;
  foreach(pqObjectPanel* p, this->PanelStore)
    {
    if(p->referenceProxy()->modifiedState() != pqProxy::UNMODIFIED)
      {
      acceptable = true;
      }
    }
  this->canAccept(acceptable);
  if (acceptable)
    {
    emit this->canAccept();
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::setView(pqView* _view)
{
  this->View = _view;
  emit this->viewChanged(_view);
}

//-----------------------------------------------------------------------------
pqView* pqObjectInspectorWidget::view()
{
  return this->View;
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::showHelp()
{
  if(this->CurrentPanel && this->CurrentPanel->referenceProxy())
    {
    this->helpRequested(this->CurrentPanel->referenceProxy()->getProxy()->GetXMLName());
    this->helpRequested(
      this->CurrentPanel->referenceProxy()->getProxy()->GetXMLGroup(),
      this->CurrentPanel->referenceProxy()->getProxy()->GetXMLName());
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::show(pqPipelineSource* source)
{
  pqDisplayPolicy* displayPolicy =
    pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  // Create representations for all output ports.
  for (int cc=0; cc < source->getNumberOfOutputPorts(); cc++)
    {
    pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
      source->getOutputPort(cc), this->view(), false);
    if (!repr || !repr->getView())
      {
      continue;
      }

    pqView* cur_view = repr->getView();
    pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
    if (filter)
      {
      filter->hideInputIfRequired(cur_view);
      }
    cur_view->render(); // these renders are collapsed.
    }
}
