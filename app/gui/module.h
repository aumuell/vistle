#ifndef GUI_MODULE_H
#define GUI_MODULE_H

#include <QAction>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QList>
#include <QPoint>
#include <QPolygon>
#include <QRect>
#include <QSize>
#include <QString>

#include <vistle/core/uuid.h>

#include <vistle/userinterface/vistleconnection.h>

#include "port.h"

namespace gui {

class Connection;
class DataFlowNetwork;

class Module: public QObject, public QGraphicsRectItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    typedef QGraphicsRectItem Base;

    static const double portDistance;
    static const double borderWidth;
    static bool s_snapToGrid;

signals:
    void mouseClickEvent();

public:
    enum Status { SPAWNING, INITIALIZED, KILLED, BUSY, EXECUTING, ERROR_STATUS };

    struct Message {
        int type;
        QString text;
    };

    Module(QGraphicsItem *parent = nullptr, QString name = QString());
    virtual ~Module();

    static QPointF gridSpacing();
    static float gridSpacingX();
    static float gridSpacingY();
    static float snapX(float x);
    static float snapY(float y);

    QRectF boundingRect() const; // re-implemented
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget); // re-implemented
    ///\todo this functionality is unnecessary, push functionality to port
    QPointF portPos(const Port *port) const;
    void setStatus(Module::Status status);
    void setStatusText(QString text, int prio);
    void setInfo(QString text);
    void clearMessages();
    void moduleMessage(int type, QString message);
    QList<Message> &messages();
    void setMessagesVisibility(bool visible);
    bool messagesVisible() const;

    void addPort(const vistle::Port &port);
    void removePort(const vistle::Port &port);
    QList<Port *> inputPorts() const;
    QList<Port *> outputPorts() const;

    // vistle methods
    QString name() const;
    void setName(QString name);

    int id() const;
    void setId(int id);
    int hub() const;
    void setHub(int hub);

    boost::uuids::uuid spawnUuid() const;
    void setSpawnUuid(const boost::uuids::uuid &uuid);

    template<class T>
    void setParameter(QString name, const T &value) const;
    template<class T>
    std::shared_ptr<vistle::ParameterBase<T>> getParameter(QString name) const;
    void sendPosition() const;
    bool isPositionValid() const;
    void setPositionValid();

    Port *getGuiPort(const vistle::Port *port) const;
    const vistle::Port *getVistlePort(Port *port) const;

    DataFlowNetwork *scene() const;

    static QColor hubColor(int hub);
signals:
    void createModuleCompound();
    void selectConnected(int direction, int id, QString port = QString());

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    void updatePosition(QPointF newPos) const;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public slots:
    void restartModule();
    void moveToHub(int hub);
    void replaceWith(QString moduleName);
    void execModule();
    void cancelExecModule();
    void deleteModule();
    void attachDebugger();
    void projectToGrid();
    void setParameterDefaults();

private:
    void createGeometry();
    void createActions();
    void createMenus();
    void doLayout();
    void updateText();

    QMenu *m_moduleMenu = nullptr;
    QAction *m_selectUpstreamAct = nullptr, *m_selectDownstreamAct = nullptr, *m_selectConnectedAct = nullptr;
    QAction *m_deleteThisAct = nullptr, *m_deleteSelAct = nullptr;
    QAction *m_attachDebugger = nullptr;
    QAction *m_execAct = nullptr;
    QAction *m_cancelExecAct = nullptr;
    QAction *m_restartAct = nullptr;
    QAction *m_moveToAct = nullptr;
    QMenu *m_moveToMenu = nullptr;
    QMenu *m_replaceWithMenu = nullptr;
    QAction *m_createModuleGroup = nullptr;


    int m_hub;
    int m_id;
    boost::uuids::uuid m_spawnUuid;

    ///\todo add data structure for the module information
    QString m_name;
    QString m_displayName;
    Module::Status m_Status;
    QString m_statusText;
    QString m_info;
    bool m_errorState = false;
    QList<Message> m_messages;
    bool m_messagesVisible = true;
    bool m_validPosition = false;

    QList<Port *> m_inPorts, m_outPorts, m_paramPorts;
    QColor m_color;
    qreal m_fontHeight;
    std::map<vistle::Port, Port *> m_vistleToGui;

    QColor m_borderColor;
};

template<class T>
void Module::setParameter(QString name, const T &value) const
{
    vistle::VistleConnection::the().setParameter(id(), name.toStdString(), value);
}

template<class T>
std::shared_ptr<vistle::ParameterBase<T>> Module::getParameter(QString name) const
{
    return std::dynamic_pointer_cast<vistle::ParameterBase<T>>(
        vistle::VistleConnection::the().getParameter(id(), name.toStdString()));
}

} //namespace gui

#endif // VMODULE_H
