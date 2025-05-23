// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// SPDX-License-Identifier: BSD-3-Clause


#include "vistlebuttonpropertybrowser.h"
#include "../parameterconnectionwidgets.h"
#include <QSet>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QMap>
#include <QToolButton>
#include <QStyle>

#include <iostream>

class VistleButtonPropertyBrowserPrivate {
    VistleButtonPropertyBrowser *q_ptr;
    Q_DECLARE_PUBLIC(VistleButtonPropertyBrowser)
public:
    struct WidgetItem {
        WidgetItem()
        : widget(0)
        , label(0)
        , widgetLabel(0)
        , button(0)
        , container(0)
        , layout(0)
        , /*line(0), */ parent(0)
        , expanded(false)
        {}
        QWidget *widget; // can be null
        QLabel *label; // main label with property name
        QString paramName;
        QLabel *widgetLabel; // label substitute showing the current value if there is no widget
        QToolButton *button; // expandable button for items with children
        QWidget *container; // container which is expanded when the button is clicked
        QGridLayout *layout; // layout in container
        WidgetItem *parent;
        QList<WidgetItem *> children;
        bool expanded;
    };

    void init(QWidget *parent);

    gui::ParameterConnectionLabel *propertyInserted(int moduleId, QtBrowserItem *index, QtBrowserItem *afterIndex);
    void propertyRemoved(QtBrowserItem *index);
    void propertyChanged(QtBrowserItem *index);
    QWidget *createEditor(QtProperty *property, QWidget *parent) const { return q_ptr->createEditor(property, parent); }

    void slotEditorDestroyed();
    void slotUpdate();
    void slotToggled(bool checked);
    void parametersConnected(int fromId, QString fromName, int toId, QString toName, bool direct);
    void parametersDisconnected(int fromId, QString fromName, int toId, QString toName);
    void clearConnections();

private:
    void updateLater();
    //this is the changed function to enable parameter connections via the WidgetItem::label
    void updateItem(WidgetItem *item);
    void insertRow(QGridLayout *layout, int row) const;
    void removeRow(QGridLayout *layout, int row) const;
    int gridRow(WidgetItem *item) const;
    int gridSpan(WidgetItem *item) const;
    void setExpanded(WidgetItem *item, bool expanded);
    QToolButton *createButton(QWidget *panret = 0) const;

    QMap<QtBrowserItem *, WidgetItem *> m_indexToItem;
    QMap<WidgetItem *, QtBrowserItem *> m_itemToIndex;
    QMap<QWidget *, WidgetItem *> m_widgetToItem;
    QMap<QObject *, WidgetItem *> m_buttonToItem;
    QGridLayout *m_mainLayout;
    QList<WidgetItem *> m_children;
    QList<WidgetItem *> m_recreateQueue;
};

QToolButton *VistleButtonPropertyBrowserPrivate::createButton(QWidget *parent) const
{
    QToolButton *button = new QToolButton(parent);
    button->setCheckable(true);
    button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setArrowType(Qt::DownArrow);
    button->setIconSize(QSize(3, 16));
    /*
    QIcon icon;
    icon.addPixmap(q_ptr->style()->standardPixmap(QStyle::SP_ArrowDown), QIcon::Normal, QIcon::Off);
    icon.addPixmap(q_ptr->style()->standardPixmap(QStyle::SP_ArrowUp), QIcon::Normal, QIcon::On);
    button->setIcon(icon);
    */
    return button;
}

int VistleButtonPropertyBrowserPrivate::gridRow(WidgetItem *item) const
{
    QList<WidgetItem *> siblings;
    if (item->parent)
        siblings = item->parent->children;
    else
        siblings = m_children;

    int row = 0;
    QListIterator<WidgetItem *> it(siblings);
    while (it.hasNext()) {
        WidgetItem *sibling = it.next();
        if (sibling == item)
            return row;
        row += gridSpan(sibling);
    }
    return -1;
}

int VistleButtonPropertyBrowserPrivate::gridSpan(WidgetItem *item) const
{
    if (item->container && item->expanded)
        return 2;
    return 1;
}

void VistleButtonPropertyBrowserPrivate::init(QWidget *parent)
{
    m_mainLayout = new QGridLayout();
    parent->setLayout(m_mainLayout);
    QLayoutItem *item = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_mainLayout->addItem(item, 0, 0);
}

void VistleButtonPropertyBrowserPrivate::slotEditorDestroyed()
{
    QWidget *editor = qobject_cast<QWidget *>(q_ptr->sender());
    if (!editor)
        return;
    if (!m_widgetToItem.contains(editor))
        return;
    m_widgetToItem[editor]->widget = 0;
    m_widgetToItem.remove(editor);
}

void VistleButtonPropertyBrowserPrivate::slotUpdate()
{
    QListIterator<WidgetItem *> itItem(m_recreateQueue);
    while (itItem.hasNext()) {
        WidgetItem *item = itItem.next();

        WidgetItem *parent = item->parent;
        QWidget *w = 0;
        QGridLayout *l = 0;
        const int oldRow = gridRow(item);
        if (parent) {
            w = parent->container;
            l = parent->layout;
        } else {
            w = q_ptr;
            l = m_mainLayout;
        }

        int span = 1;
        if (!item->widget && !item->widgetLabel)
            span = 2;
        item->label = new QLabel(w);
        item->label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        l->addWidget(item->label, oldRow, 0, 1, span);

        updateItem(item);
    }
    m_recreateQueue.clear();
}

void VistleButtonPropertyBrowserPrivate::setExpanded(WidgetItem *item, bool expanded)
{
    if (item->expanded == expanded)
        return;

    if (!item->container)
        return;

    item->expanded = expanded;
    const int row = gridRow(item);
    WidgetItem *parent = item->parent;
    QGridLayout *l = 0;
    if (parent)
        l = parent->layout;
    else
        l = m_mainLayout;

    if (expanded) {
        insertRow(l, row + 1);
        l->addWidget(item->container, row + 1, 0, 1, 2);
        item->container->show();
    } else {
        l->removeWidget(item->container);
        item->container->hide();
        removeRow(l, row + 1);
    }

    item->button->setChecked(expanded);
    item->button->setArrowType(expanded ? Qt::UpArrow : Qt::DownArrow);
}

void VistleButtonPropertyBrowserPrivate::slotToggled(bool checked)
{
    WidgetItem *item = m_buttonToItem.value(q_ptr->sender());
    if (!item)
        return;

    setExpanded(item, checked);

    if (checked)
        emit q_ptr->expanded(m_itemToIndex.value(item));
    else
        emit q_ptr->collapsed(m_itemToIndex.value(item));
}

void VistleButtonPropertyBrowserPrivate::parametersConnected(int fromId, QString fromName, int toId, QString toName,
                                                             bool direct)
{
    for (auto item: m_indexToItem) {
        if (item->paramName == fromName) {
            if (auto paramLabel = dynamic_cast<gui::ParameterConnectionLabel *>(item->label))
                paramLabel->connectParam(toId, toName, direct);
        }
    }
}

void VistleButtonPropertyBrowserPrivate::parametersDisconnected(int fromId, QString fromName, int toId, QString toName)
{
    for (auto item: m_indexToItem) {
        if (item->paramName == fromName) {
            if (auto paramLabel = dynamic_cast<gui::ParameterConnectionLabel *>(item->label))
                paramLabel->disconnectParam(toId, toName);
        }
    }
}

void VistleButtonPropertyBrowserPrivate::clearConnections()
{
    for (auto item: m_indexToItem) {
        if (item->label) {
            if (auto paramLabel = dynamic_cast<gui::ParameterConnectionLabel *>(item->label))
                paramLabel->clearConnections();
        }
    }
}

void VistleButtonPropertyBrowserPrivate::updateLater()
{
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdate()));
}

bool isVector(QtProperty *property)
{
    if (property->subProperties().size() == 0)
        return false;
    constexpr std::array<const char *, 4> names = {"x", "y", "z", "w"};
    size_t index = 0;
    for (const auto &sub: property->subProperties()) {
        if (sub->propertyName() != names[index])
            return false;
        ++index;
        if (index >= names.size())
            break;
    }
    return true;
}

gui::ParameterConnectionLabel *VistleButtonPropertyBrowserPrivate::propertyInserted(int moduleId, QtBrowserItem *index,
                                                                                    QtBrowserItem *afterIndex)
{
    WidgetItem *afterItem = m_indexToItem.value(afterIndex);
    WidgetItem *parentItem = m_indexToItem.value(index->parent());
    gui::ParameterConnectionLabel *retval = nullptr;
    WidgetItem *newItem = new WidgetItem();
    newItem->parent = parentItem;
    newItem->paramName = index->property()->propertyName();

    QGridLayout *layout = 0;
    QWidget *parentWidget = 0;
    int row = -1;
    if (!afterItem) {
        row = 0;
        if (parentItem)
            parentItem->children.insert(0, newItem);
        else
            m_children.insert(0, newItem);
    } else {
        row = gridRow(afterItem) + gridSpan(afterItem);
        if (parentItem)
            parentItem->children.insert(parentItem->children.indexOf(afterItem) + 1, newItem);
        else
            m_children.insert(m_children.indexOf(afterItem) + 1, newItem);
    }
    if (!parentItem) {
        layout = m_mainLayout;
        parentWidget = q_ptr;
    } else {
        if (!parentItem->container) {
            m_recreateQueue.removeAll(parentItem);
            WidgetItem *grandParent = parentItem->parent;
            QGridLayout *l = 0;
            const int oldRow = gridRow(parentItem);
            if (grandParent) {
                l = grandParent->layout;
            } else {
                l = m_mainLayout;
            }
            QFrame *container = new QFrame();
            container->setFrameShape(QFrame::Panel);
            container->setFrameShadow(QFrame::Raised);
            parentItem->container = container;
            parentItem->button = createButton();
            m_buttonToItem[parentItem->button] = parentItem;
            q_ptr->connect(parentItem->button, SIGNAL(toggled(bool)), q_ptr, SLOT(slotToggled(bool)));
            parentItem->layout = new QGridLayout();
            container->setLayout(parentItem->layout);
            if (parentItem->label) {
                l->removeWidget(parentItem->label);
                delete parentItem->label;
                parentItem->label = 0;
            }
            int span = 1;
            if (!parentItem->widget && !parentItem->widgetLabel)
                span = 2;
            auto column = 0;
            if (isVector(m_itemToIndex[parentItem]->property())) {
                QWidget *container = new QWidget(parentWidget);
                auto hLayout = new QHBoxLayout(container);
                hLayout->setContentsMargins(0, 0, 0, 0);
                auto label = new gui::ParameterConnectionLabel(
                    moduleId, m_itemToIndex[parentItem]->property()->propertyName(), container);
                label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
                label->hideText();
                parentItem->label = label;
                retval = label;
                hLayout->addWidget(label);
                hLayout->addWidget(parentItem->button);
                l->addWidget(container, oldRow, column, 1, span);
            } else
                l->addWidget(parentItem->button, oldRow, column, 1, span);
            updateItem(parentItem);
        }
        layout = parentItem->layout;
        parentWidget = parentItem->container;
    }

    if (!parentItem || !isVector(m_itemToIndex[parentItem]->property())) {
        retval = new gui::ParameterConnectionLabel(moduleId, index->property()->propertyName(), parentWidget);
        newItem->label = retval;
    } else {
        newItem->label = new QLabel(parentWidget);
        newItem->label->setText(index->property()->propertyName());
    }
    newItem->label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    newItem->widget = createEditor(index->property(), parentWidget);
    if (newItem->widget) {
        QObject::connect(newItem->widget, SIGNAL(destroyed()), q_ptr, SLOT(slotEditorDestroyed()));
        m_widgetToItem[newItem->widget] = newItem;
    } else if (index->property()->hasValue()) {
        newItem->widgetLabel = new QLabel(parentWidget);
        newItem->widgetLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed));
    }

    insertRow(layout, row);
    int span = 1;
    if (newItem->widget)
        layout->addWidget(newItem->widget, row, 1);
    else if (newItem->widgetLabel)
        layout->addWidget(newItem->widgetLabel, row, 1);
    else
        span = 2;
    layout->addWidget(newItem->label, row, 0, span, 1);

    m_itemToIndex[newItem] = index;
    m_indexToItem[index] = newItem;

    updateItem(newItem);
    return retval;
}

void VistleButtonPropertyBrowserPrivate::propertyRemoved(QtBrowserItem *index)
{
    WidgetItem *item = m_indexToItem.value(index);

    m_indexToItem.remove(index);
    m_itemToIndex.remove(item);

    WidgetItem *parentItem = item->parent;

    const int row = gridRow(item);

    if (parentItem)
        parentItem->children.removeAt(parentItem->children.indexOf(item));
    else
        m_children.removeAt(m_children.indexOf(item));

    const int colSpan = gridSpan(item);

    m_buttonToItem.remove(item->button);

    if (item->widget)
        delete item->widget;
    if (item->label)
        delete item->label;
    if (item->widgetLabel)
        delete item->widgetLabel;
    if (item->button)
        delete item->button;
    if (item->container)
        delete item->container;

    if (!parentItem) {
        removeRow(m_mainLayout, row);
        if (colSpan > 1)
            removeRow(m_mainLayout, row);
    } else if (parentItem->children.count() != 0) {
        removeRow(parentItem->layout, row);
        if (colSpan > 1)
            removeRow(parentItem->layout, row);
    } else {
        const WidgetItem *grandParent = parentItem->parent;
        QGridLayout *l = 0;
        if (grandParent) {
            l = grandParent->layout;
        } else {
            l = m_mainLayout;
        }

        const int parentRow = gridRow(parentItem);
        const int parentSpan = gridSpan(parentItem);

        l->removeWidget(parentItem->button);
        l->removeWidget(parentItem->container);
        delete parentItem->button;
        delete parentItem->container;
        parentItem->button = 0;
        parentItem->container = 0;
        parentItem->layout = 0;
        if (!m_recreateQueue.contains(parentItem))
            m_recreateQueue.append(parentItem);
        if (parentSpan > 1)
            removeRow(l, parentRow + 1);

        updateLater();
    }
    m_recreateQueue.removeAll(item);

    delete item;
}

void VistleButtonPropertyBrowserPrivate::insertRow(QGridLayout *layout, int row) const
{
    QMap<QLayoutItem *, QRect> itemToPos;
    int idx = 0;
    while (idx < layout->count()) {
        int r, c, rs, cs;
        layout->getItemPosition(idx, &r, &c, &rs, &cs);
        if (r >= row) {
            itemToPos[layout->takeAt(idx)] = QRect(r + 1, c, rs, cs);
        } else {
            idx++;
        }
    }

    const QMap<QLayoutItem *, QRect>::ConstIterator icend = itemToPos.constEnd();
    for (QMap<QLayoutItem *, QRect>::ConstIterator it = itemToPos.constBegin(); it != icend; ++it) {
        const QRect r = it.value();
        layout->addItem(it.key(), r.x(), r.y(), r.width(), r.height());
    }
}

void VistleButtonPropertyBrowserPrivate::removeRow(QGridLayout *layout, int row) const
{
    QMap<QLayoutItem *, QRect> itemToPos;
    int idx = 0;
    while (idx < layout->count()) {
        int r, c, rs, cs;
        layout->getItemPosition(idx, &r, &c, &rs, &cs);
        if (r > row) {
            itemToPos[layout->takeAt(idx)] = QRect(r - 1, c, rs, cs);
        } else {
            idx++;
        }
    }

    const QMap<QLayoutItem *, QRect>::ConstIterator icend = itemToPos.constEnd();
    for (QMap<QLayoutItem *, QRect>::ConstIterator it = itemToPos.constBegin(); it != icend; ++it) {
        const QRect r = it.value();
        layout->addItem(it.key(), r.x(), r.y(), r.width(), r.height());
    }
}

void VistleButtonPropertyBrowserPrivate::propertyChanged(QtBrowserItem *index)
{
    WidgetItem *item = m_indexToItem.value(index);

    updateItem(item);
}

void VistleButtonPropertyBrowserPrivate::updateItem(WidgetItem *item)
{
    QtProperty *property = m_itemToIndex[item]->property();
    if (item->button) {
        QFont font = item->button->font();
        font.setUnderline(property->isModified());
        item->button->setFont(font);
        item->button->setText(property->propertyName());
        item->button->setToolTip(property->toolTip());
        item->button->setStatusTip(property->statusTip());
        item->button->setWhatsThis(property->whatsThis());
        item->button->setEnabled(property->isEnabled());
    }
    if (item->label) {
        QFont font = item->label->font();
        font.setUnderline(property->isModified());
        item->label->setFont(font);
        // item->label->setText(property->propertyName());
        item->label->setToolTip(property->toolTip());
        item->label->setStatusTip(property->statusTip());
        item->label->setWhatsThis(property->whatsThis());
        item->label->setEnabled(property->isEnabled());
    }
    if (item->widgetLabel) {
        QFont font = item->widgetLabel->font();
        font.setUnderline(false);
        item->widgetLabel->setFont(font);
        item->widgetLabel->setText(property->valueText());
        item->widgetLabel->setToolTip(property->valueText());
        item->widgetLabel->setEnabled(property->isEnabled());
    }
    if (item->widget) {
        QFont font = item->widget->font();
        font.setUnderline(false);
        item->widget->setFont(font);
        item->widget->setEnabled(property->isEnabled());
        item->widget->setToolTip(property->valueText());
    }
}


/*!
    \class VistleButtonPropertyBrowser

    \brief The VistleButtonPropertyBrowser class provides a drop down QToolButton
    based property browser.

    A property browser is a widget that enables the user to edit a
    given set of properties. Each property is represented by a label
    specifying the property's name, and an editing widget (e.g. a line
    edit or a combobox) holding its value. A property can have zero or
    more subproperties.

    VistleButtonPropertyBrowser provides drop down button for all nested
    properties, i.e. subproperties are enclosed by a container associated with
    the drop down button. The parent property's name is displayed as button text. For example:

    \image qtbuttonpropertybrowser.png

    Use the QtAbstractPropertyBrowser API to add, insert and remove
    properties from an instance of the VistleButtonPropertyBrowser
    class. The properties themselves are created and managed by
    implementations of the QtAbstractPropertyManager class.

    \sa QtTreePropertyBrowser, QtAbstractPropertyBrowser
*/

/*!
    \fn void VistleButtonPropertyBrowser::collapsed(QtBrowserItem *item)

    This signal is emitted when the \a item is collapsed.

    \sa expanded(), setExpanded()
*/

/*!
    \fn void VistleButtonPropertyBrowser::expanded(QtBrowserItem *item)

    This signal is emitted when the \a item is expanded.

    \sa collapsed(), setExpanded()
*/

/*!
    Creates a property browser with the given \a parent.
*/
VistleButtonPropertyBrowser::VistleButtonPropertyBrowser(QWidget *parent): QtAbstractPropertyBrowser(parent)
{
    d_ptr = new VistleButtonPropertyBrowserPrivate;
    d_ptr->q_ptr = this;

    d_ptr->init(this);
}

/*!
    Destroys this property browser.

    Note that the properties that were inserted into this browser are
    \e not destroyed since they may still be used in other
    browsers. The properties are owned by the manager that created
    them.

    \sa QtProperty, QtAbstractPropertyManager
*/
VistleButtonPropertyBrowser::~VistleButtonPropertyBrowser()
{
    const QMap<VistleButtonPropertyBrowserPrivate::WidgetItem *, QtBrowserItem *>::ConstIterator icend =
        d_ptr->m_itemToIndex.constEnd();
    for (QMap<VistleButtonPropertyBrowserPrivate::WidgetItem *, QtBrowserItem *>::ConstIterator it =
             d_ptr->m_itemToIndex.constBegin();
         it != icend; ++it)
        delete it.key();
    delete d_ptr;
}

/*!
    \reimp
*/
void VistleButtonPropertyBrowser::itemInserted(QtBrowserItem *item, QtBrowserItem *afterItem)
{
    auto label = d_ptr->propertyInserted(m_moduleId, item, afterItem);
    if (label) {
        connect(label, &gui::ParameterConnectionLabel::highlightModule, this,
                &VistleButtonPropertyBrowser::highlightModule);
        connect(label, &gui::ParameterConnectionLabel::disconnectParameters, this,
                [this](int fromId, const QString &fromName, int toId, const QString &toName) {
                    emit disconnectParameters(fromId, fromName, toId, toName);
                });
    }
}

/*!
    \reimp
*/
void VistleButtonPropertyBrowser::itemRemoved(QtBrowserItem *item)
{
    d_ptr->propertyRemoved(item);
}

/*!
    \reimp
*/
void VistleButtonPropertyBrowser::itemChanged(QtBrowserItem *item)
{
    d_ptr->propertyChanged(item);
}

/*!
    Sets the \a item to either collapse or expanded, depending on the value of \a expanded.

    \sa isExpanded(), expanded(), collapsed()
*/

void VistleButtonPropertyBrowser::setExpanded(QtBrowserItem *item, bool expanded)
{
    VistleButtonPropertyBrowserPrivate::WidgetItem *itm = d_ptr->m_indexToItem.value(item);
    if (itm)
        d_ptr->setExpanded(itm, expanded);
}

/*!
    Returns true if the \a item is expanded; otherwise returns false.

    \sa setExpanded()
*/

bool VistleButtonPropertyBrowser::isExpanded(QtBrowserItem *item) const
{
    VistleButtonPropertyBrowserPrivate::WidgetItem *itm = d_ptr->m_indexToItem.value(item);
    if (itm)
        return itm->expanded;
    return false;
}

void VistleButtonPropertyBrowser::parametersConnected(const std::vector<Connection> &connections)
{
    d_ptr->clearConnections();
    for (auto &c: connections) {
        if (m_moduleId == c.toId)
            d_ptr->parametersConnected(c.toId, c.toName, c.fromId, c.fromName, c.direct);
        else
            d_ptr->parametersConnected(c.fromId, c.fromName, c.toId, c.toName, c.direct);
    }
}


#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#include "moc_vistlebuttonpropertybrowser.cpp"
