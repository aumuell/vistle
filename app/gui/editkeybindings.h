#ifndef VISTLE_GUI_EDITKEYBINDINGS_H
#define VISTLE_GUI_EDITKEYBINDINGS_H

#include <QWidget>
#include <QLineEdit>
#include <QKeyEvent>
#include <QDebug>

template<class Widget>
class EditKeyBindings: public Widget {
public:
    using Widget::Widget;

    void keyPressEvent(QKeyEvent *event) override { overrideKeyEvent(event); }

    void keyReleaseEvent(QKeyEvent *event) override { overrideKeyEvent(event); }

private:
    void overrideKeyEvent(QKeyEvent *event)
    {
        auto makeKey = [event](int key, Qt::KeyboardModifiers mod = Qt::NoModifier) {
            return new QKeyEvent(event->type(), key, mod);
        };

        QKeyEvent *e = nullptr;
        auto mod = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
        if (mod == Qt::ControlModifier) {
            switch (event->key()) {
            case Qt::Key_W:
                e = makeKey(Qt::Key_Backspace, Qt::ControlModifier);
                break;
            case Qt::Key_H:
                qDebug() << "Ctrl+H";
                e = makeKey(Qt::Key_Backspace, Qt::NoModifier);
                break;
#if 0
        case Qt::Key_B:
        e = makeKey(Qt::Key_Left, Qt::NoModifier);
        break;
        case Qt::Key_F:
        event->setKey(Qt::Key_Right);
        event->setModifiers(Qt::NoModifier);
        break;
        case Qt::Key_A:
        event->setKey(Qt::Key_Home);
        event->setModifiers(Qt::NoModifier);
        break;
        case Qt::Key_E:
        event->setKey(Qt::Key_End);
        event->setModifiers(Qt::NoModifier);
        break;
#endif
            }
        } else if (mod == Qt::AltModifier) {
            switch (event->key()) {
            case Qt::Key_D:
                qDebug() << "Alt-D";
                e = makeKey(Qt::Key_Delete, Qt::ControlModifier);
                break;
            }
        }

        if (e)
            event = e;
        if (event->type() == QEvent::KeyPress) {
            return Widget::keyPressEvent(event);
        } else if (event->type() == QEvent::KeyRelease) {
            return Widget::keyReleaseEvent(event);
        }
        delete e;
    }
};

#if 0
class QLineEditExt: public EditKeyBindings<QLineEdit> {
    Q_OBJECT
    public:
    using EditKeyBindings<QLineEdit>::EditKeyBindings;
};
#endif

#define EDITKEYBINDINGS(Widget) \
    class Widget##Ext: public EditKeyBindings<Widget> { \
        Q_OBJECT \
    public: \
        using EditKeyBindings<Widget>::EditKeyBindings; \
    };

EDITKEYBINDINGS(QLineEdit)

#endif
